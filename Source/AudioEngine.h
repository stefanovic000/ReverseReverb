#pragma once
#include <juce_audio_formats/juce_audio_formats.h>
#include <functional>
#include <memory>

namespace rr
{
constexpr juce::int64 maxSourceBytes = 64 * 1024 * 1024;

struct Audio
{
    juce::AudioBuffer<float> samples;
    double sampleRate = 44100.0;
    juce::String name;
};

struct Settings
{
    int durationMode = 3; // seconds, 1/4 bar, 1/2 bar, 1 bar, 2 bars
    double seconds = 2.0;
    double bpm = 120.0;
    int numerator = 4, denominator = 4;
    float size = 0.72f, tone = 0.55f, width = 1.0f;
    int tremolo = 0; // off, 1/8, 1/16, 1/32
    float depth = 0.5f, outputDb = 0.0f;
    float shape = 0.0f;
    double gapSeconds = 0.0;
};

double durationSeconds(const Settings&);
float shapeGain(double position, float shape);
std::shared_ptr<Audio> combinedPreview(const Audio& effect, const Audio& source, int start, int end);
std::shared_ptr<Audio> loadAudio(const juce::File&, juce::String& error);
std::shared_ptr<Audio> render(const Audio&, int start, int end, const Settings&,
                            const std::function<bool()>& cancelled = {});
bool writeWav(const Audio&, const juce::File&, juce::String& error);
float peak(const Audio&);
}
