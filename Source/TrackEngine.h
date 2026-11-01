#pragma once
#include "AudioEngine.h"
#include <juce_dsp/juce_dsp.h>

namespace rr
{
// Construct/prepare off the audio thread. After handoff only the callback owns this engine.
class TrackEngine
{
public:
    TrackEngine(double sampleRate, const Settings&, const std::function<bool()>& cancelled = {});
    void process(juce::AudioBuffer<float>&, float amount, float outputDb) noexcept;
    void reset() noexcept;
    int latencySamples() const { return latency; }
    double sampleRate() const { return rate; }
private:
    static constexpr int chunkSize = 256;
    juce::dsp::Convolution convolution;
    juce::AudioBuffer<float> wet { 2, chunkSize }, dry { 2, chunkSize }, delay;
    juce::SmoothedValue<float> wetGain, finalGain;
    double rate = 44100;
    int latency = 0, writePosition = 0, history = 0;
};
}
