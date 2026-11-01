#include "TrackEngine.h"
#include <cmath>
#include <stdexcept>

namespace rr
{
TrackEngine::TrackEngine(double sampleRate, const Settings& settings, const std::function<bool()>& cancelled)
    : rate(sampleRate)
{
    Audio impulse;
    impulse.sampleRate = rate; impulse.samples.setSize(1, 2); impulse.samples.clear(); impulse.samples.setSample(0, 1, 1.0f);
    auto kernelSettings = settings;
    kernelSettings.gapSeconds = 0; kernelSettings.outputDb = 0;
    // A fixed response of Classic provides the first streaming character.
    auto response = render(impulse, 0, 2, kernelSettings, cancelled);
    if (!response) throw std::runtime_error("Track preparation cancelled.");
    const int leadSamples = response->samples.getNumSamples();
    convolution.loadImpulseResponse(std::move(response->samples), rate,
        juce::dsp::Convolution::Stereo::yes, juce::dsp::Convolution::Trim::no,
        juce::dsp::Convolution::Normalise::no);
    convolution.prepare({ rate, chunkSize, 2 });
    latency = leadSamples + static_cast<int>(std::llround(juce::jlimit(0.0, 2.0, settings.gapSeconds) * rate))
        + convolution.getLatency();
    delay.setSize(2, latency + 1); delay.clear();
    wetGain.reset(rate, 0.02); wetGain.setCurrentAndTargetValue(0.5f);
    finalGain.reset(rate, 0.02); finalGain.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(settings.outputDb));
}

void TrackEngine::reset() noexcept
{
    convolution.reset();
    // Count valid history instead of clearing a potentially long dry-delay buffer.
    writePosition = 0; history = 0;
}

void TrackEngine::process(juce::AudioBuffer<float>& buffer, float amount, float outputDb) noexcept
{
    wetGain.setTargetValue(juce::jlimit(0.0f, 1.0f, amount));
    finalGain.setTargetValue(juce::Decibels::decibelsToGain(juce::jlimit(-36.0f, 12.0f, outputDb)));
    for (int offset = 0; offset < buffer.getNumSamples(); offset += chunkSize)
    {
        const int count = juce::jmin(chunkSize, buffer.getNumSamples() - offset);
        for (int n = 0; n < count; ++n)
        {
            const int readPosition = (writePosition + 1) % delay.getNumSamples();
            for (int c = 0; c < 2; ++c)
            {
                const float input = buffer.getSample(juce::jmin(c, buffer.getNumChannels() - 1), offset + n);
                wet.setSample(c, n, input);
                dry.setSample(c, n, history >= latency ? delay.getSample(c, readPosition) : 0.0f);
                delay.setSample(c, writePosition, input);
            }
            writePosition = readPosition; history = juce::jmin(latency, history + 1);
        }
        auto block = juce::dsp::AudioBlock<float>(wet).getSubBlock(0, static_cast<size_t>(count));
        convolution.process(juce::dsp::ProcessContextReplacing<float>(block));
        for (int n = 0; n < count; ++n)
        {
            const float mix = wetGain.getNextValue(), gain = finalGain.getNextValue();
            for (int c = 0; c < buffer.getNumChannels(); ++c)
            {
                const float original = dry.getSample(c, n);
                const float effect = buffer.getNumChannels() == 1 ? 0.5f * (wet.getSample(0, n) + wet.getSample(1, n)) : wet.getSample(c, n);
                buffer.setSample(c, offset + n, (original + effect * mix) * gain);
            }
        }
    }
}
}
