#include "AudioEngine.h"
#include <cmath>
#include <stdexcept>

namespace rr
{
float shapeGain(double position, float shape)
{
    return shape <= 0.0f ? 1.0f : static_cast<float>(std::pow(juce::jlimit(0.0, 1.0, position), 4.0 * juce::jlimit(0.0f, 1.0f, shape)));
}

double durationSeconds(const Settings& s)
{
    if (s.durationMode == 0)
        return juce::jlimit(0.1, 30.0, s.seconds);
    const double fractions[] { 0.0, 0.25, 0.5, 1.0, 2.0 };
    const auto beatsPerBar = juce::jmax(1, s.numerator) * 4.0 / juce::jmax(1, s.denominator);
    return juce::jlimit(0.1, 30.0, fractions[juce::jlimit(1, 4, s.durationMode)]
        * beatsPerBar * 60.0 / juce::jlimit(20.0, 400.0, s.bpm));
}

std::shared_ptr<Audio> loadAudio(const juce::File& file, juce::String& error)
{
    juce::AudioFormatManager formats;
    formats.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader(formats.createReaderFor(file));
    if (!reader) { error = "Cannot open this audio file. Try a WAV or AIFF."; return {}; }
    if (reader->numChannels < 1 || reader->numChannels > 2)
    { error = "Please use a mono or stereo recording."; return {}; }
    if (reader->sampleRate < 8000.0 || reader->sampleRate > 192000.0)
    { error = "Supported sample rates are 8 kHz to 192 kHz."; return {}; }
    if (reader->lengthInSamples < 2 || reader->lengthInSamples > maxSourceBytes / (4 * reader->numChannels))
    { error = "Recording is empty or too large. Load a shorter file (maximum 64 MB of decoded audio)."; return {}; }
    auto audio = std::make_shared<Audio>();
    audio->sampleRate = reader->sampleRate;
    audio->name = file.getFileName();
    audio->samples.setSize(static_cast<int>(reader->numChannels), static_cast<int>(reader->lengthInSamples));
    if (!reader->read(&audio->samples, 0, audio->samples.getNumSamples(), 0, true, true))
    { error = "The recording could not be read completely."; return {}; }
    for (int c = 0; c < audio->samples.getNumChannels(); ++c)
        for (int n = 0; n < audio->samples.getNumSamples(); ++n)
            if (!std::isfinite(audio->samples.getSample(c, n)))
            { error = "The recording contains invalid audio values."; return {}; }
    return audio;
}

std::shared_ptr<Audio> render(const Audio& source, int start, int end, const Settings& s,
                            const std::function<bool()>& cancelled)
{
    if (source.sampleRate < 8000 || source.sampleRate > 192000 || source.samples.getNumChannels() < 1)
        throw std::runtime_error("Unsupported source audio.");
    start = juce::jlimit(0, source.samples.getNumSamples(), start);
    end = juce::jlimit(start, source.samples.getNumSamples(), end);
    if (end <= start) throw std::runtime_error("Select a piece of the recording first.");
    auto output = std::make_shared<Audio>();
    output->sampleRate = source.sampleRate;
    output->name = "Reverse reverb";
    const int length = juce::jmax(2, static_cast<int>(std::llround(durationSeconds(s) * source.sampleRate)));
    output->samples.setSize(2, length);
    output->samples.clear();

    juce::Reverb reverb;
    reverb.setSampleRate(source.sampleRate);
    juce::Reverb::Parameters parameters;
    parameters.roomSize = juce::jmap(juce::jlimit(0.0f, 1.0f, s.size), 0.35f, 0.95f);
    parameters.damping = 0.35f;
    parameters.wetLevel = 0.65f;
    parameters.dryLevel = 0.0f;
    parameters.width = 1.0f;
    reverb.setParameters(parameters);
    reverb.reset();
    constexpr int chunk = 512;
    juce::AudioBuffer<float> work(2, chunk);
    // Allow JUCE's smoothed dry/wet parameters to settle before feeding the source.
    for (int n = 0; n < static_cast<int>(source.sampleRate * 0.03) + chunk; n += chunk)
    {
        work.clear();
        reverb.processStereo(work.getWritePointer(0), work.getWritePointer(1), chunk);
    }
    const auto isCancelled = [&] { return cancelled && cancelled(); };
    const int selectionLength = end - start;
    for (int position = 0; position < selectionLength; position += chunk)
    {
        if (isCancelled()) return {};
        const int count = juce::jmin(chunk, selectionLength - position);
        for (int c = 0; c < 2; ++c)
            for (int n = 0; n < count; ++n)
                work.setSample(c, n, source.samples.getSample(juce::jmin(c, source.samples.getNumChannels() - 1), end - 1 - position - n));
        reverb.processStereo(work.getWritePointer(0), work.getWritePointer(1), count);
    }
    // Only retain the tail after the reversed source ends. Its reverse is the pre-source lead-in.
    for (int position = 0; position < length; position += chunk)
    {
        if (isCancelled()) return {};
        const int count = juce::jmin(chunk, length - position);
        work.clear();
        reverb.processStereo(work.getWritePointer(0), work.getWritePointer(1), count);
        for (int c = 0; c < 2; ++c)
            for (int n = 0; n < count; ++n)
                output->samples.setSample(c, length - 1 - position - n, work.getSample(c, n));
    }

    const double cutoff = juce::jmin(source.sampleRate * 0.45,
        500.0 * std::pow(36.0, juce::jlimit(0.0f, 1.0f, s.tone)));
    const float pole = static_cast<float>(std::exp(-juce::MathConstants<double>::twoPi * cutoff / source.sampleRate));
    float filterL = 0, filterR = 0;
    const float gain = juce::Decibels::decibelsToGain(juce::jlimit(-36.0f, 12.0f, s.outputDb));
    const float width = juce::jlimit(0.0f, 2.0f, s.width);
    const float depth = juce::jlimit(0.0f, 1.0f, s.depth);
    const double beatLengths[] { 1.0, 0.5, 0.25, 0.125 };
    const double hz = juce::jlimit(20.0, 400.0, s.bpm) / (60.0 * beatLengths[juce::jlimit(0, 3, s.tremolo)]);
    const int fade = juce::jmin(length / 4, juce::jmax(1, static_cast<int>(source.sampleRate * 0.004)));
    for (int n = 0; n < length; ++n)
    {
        if ((n % 4096 == 0) && isCancelled()) return {};
        filterL = (1.0f - pole) * output->samples.getSample(0, n) + pole * filterL;
        filterR = (1.0f - pole) * output->samples.getSample(1, n) + pole * filterR;
        const float mid = (filterL + filterR) * 0.5f;
        const float side = (filterL - filterR) * 0.5f * width;
        const float tremolo = s.tremolo == 0 ? 1.0f : 1.0f - depth * 0.5f *
            (1.0f - static_cast<float>(std::cos(juce::MathConstants<double>::twoPi * hz * (length - 1 - n) / source.sampleRate)));
        const float edge = juce::jmin(1.0f, static_cast<float>(n) / fade, static_cast<float>(length - 1 - n) / fade);
        const float envelope = shapeGain(n / static_cast<double>(length - 1), s.shape);
        const float left = (mid + side) * gain * tremolo * edge * envelope;
        const float right = (mid - side) * gain * tremolo * edge * envelope;
        if (!std::isfinite(left) || !std::isfinite(right)) throw std::runtime_error("Invalid audio in render.");
        output->samples.setSample(0, n, left);
        output->samples.setSample(1, n, right);
    }
    const int gap = static_cast<int>(std::llround(juce::jlimit(0.0, 2.0, s.gapSeconds) * source.sampleRate));
    if (gap > 0)
    {
        output->samples.setSize(2, length + gap, true, true);
        output->samples.clear(length, gap);
    }
    return output;
}

std::shared_ptr<Audio> combinedPreview(const Audio& effect, const Audio& source, int start, int end)
{
    if (std::abs(effect.sampleRate - source.sampleRate) > 0.01) throw std::runtime_error("Preview sample rates do not match.");
    start = juce::jlimit(0, source.samples.getNumSamples(), start);
    end = juce::jlimit(start, source.samples.getNumSamples(), end);
    auto joined = std::make_shared<Audio>();
    joined->sampleRate = effect.sampleRate;
    joined->name = "Effect + original";
    const int offset = effect.samples.getNumSamples(), count = end - start;
    joined->samples.setSize(2, offset + count);
    for (int c = 0; c < 2; ++c)
    {
        joined->samples.copyFrom(c, 0, effect.samples, juce::jmin(c, effect.samples.getNumChannels() - 1), 0, offset);
        joined->samples.copyFrom(c, offset, source.samples, juce::jmin(c, source.samples.getNumChannels() - 1), start, count);
        const int fade = juce::jmin(count / 2, static_cast<int>(source.sampleRate * 0.003));
        for (int n = 0; n < fade; ++n)
        {
            joined->samples.getWritePointer(c)[offset + n] *= n / static_cast<float>(fade);
            joined->samples.getWritePointer(c)[offset + count - 1 - n] *= n / static_cast<float>(fade);
        }
    }
    return joined;
}

bool writeWav(const Audio& audio, const juce::File& file, juce::String& error)
{
    if (file.existsAsFile()) { error = "Choose a new filename; this file already exists."; return false; }
    auto stream = std::unique_ptr<juce::OutputStream>(file.createOutputStream().release());
    if (!stream) { error = "Cannot write to this folder."; return false; }
    juce::WavAudioFormat format;
    const auto options = juce::AudioFormatWriterOptions().withSampleRate(audio.sampleRate)
        .withNumChannels(audio.samples.getNumChannels()).withBitsPerSample(32)
        .withSampleFormat(juce::AudioFormatWriterOptions::SampleFormat::floatingPoint);
    auto writer = format.createWriterFor(stream, options);
    if (!writer || !writer->writeFromAudioSampleBuffer(audio.samples, 0, audio.samples.getNumSamples()))
    { error = "Could not finish writing the WAV file."; return false; }
    writer.reset();
    return true;
}

float peak(const Audio& audio)
{
    return audio.samples.getMagnitude(0, audio.samples.getNumSamples());
}
}
