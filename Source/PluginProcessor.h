#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "AudioEngine.h"
#include "TrackEngine.h"

class ReverseReverbProcessor final : public juce::AudioProcessor,
                                     private juce::AudioProcessorValueTreeState::Listener,
                                     private juce::Timer
{
public:
    ReverseReverbProcessor();
    ~ReverseReverbProcessor() override;
    void prepareToPlay(double, int) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlockBypassed(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "ReverseReverb"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return trackActive.load() ? getLatencySamples() / preparedRate.load() : 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    struct View
    {
        std::shared_ptr<const rr::Audio> source, result;
        int start = 0, end = 0;
        juce::String status;
        bool busy = false;
        juce::File exportFile;
    };
    View view() const;
    rr::Settings settings() const;
    void loadFile(const juce::File&);
    void setSelection(int, int);
    void generate();
    void playOriginal();
    void playEffect();
    void playCombined();
    void stopPreview();
    bool isPreviewPlaying() const { return playing.load(); }
    void collectRetiredPreviews();
    void saveCopy(const juce::File&);
    bool waitForWork(int milliseconds = 20000);
    double hostBpm() const { return bpm.load(); }
    bool hasHostTempo() const { return tempoAvailable.load(); }
    double processingRate() const { return preparedRate.load(); }
    void serviceTrack(); // message-thread service, also driven by processor timer without an editor
    bool wantsTrack() const { return value("mode") > 0.5f; }
    bool isTrackActive() const { return trackActive.load(); }
    bool isHostPlaying() const { return hostPlaying.load(); }
    juce::String trackStatus() const;
    std::array<float, 128> scope(bool output) const;
    double previewSeconds() const { return previewTime.load(); }
    int previewKind() const { return previewType.load(); }
    juce::AudioProcessorValueTreeState parameters;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout makeParameters();
    void parameterChanged(const juce::String&, float) override;
    void timerCallback() override { serviceTrack(); collectRetiredPreviews(); }
    float value(const char* id) const;
    void queuePreview(std::shared_ptr<const rr::Audio>, int, int, int kind);
    struct Preview
    {
        std::shared_ptr<const rr::Audio> audio;
        int start = 0, end = 0;
        double cursor = 0;
        unsigned epoch = 0;
        Preview* next = nullptr;
        int kind = 0;
    };
    void retire(Preview*) noexcept;
    mutable juce::CriticalSection stateLock; // message, host state, and worker only; never in processBlock
    std::shared_ptr<const rr::Audio> source, result;
    int selectionStart = 0, selectionEnd = 0;
    juce::String status = "Drop a recording to begin.";
    juce::File exportFile;
    bool busy = false;
    std::atomic<unsigned> revision { 1 }, request { 0 };
    unsigned resultRevision = 0;
    std::atomic<double> bpm { 120.0 };
    std::atomic<int> numerator { 4 }, denominator { 4 };
    std::atomic<bool> tempoAvailable { false }, playing { false };
    std::atomic<double> previewTime { 0 };
    std::atomic<int> previewType { 0 };
    double outputRate = 44100;
    std::atomic<unsigned> previewEpoch { 0 };
    std::atomic<Preview*> pendingPreview { nullptr }, retiredPreview { nullptr };
    Preview* activePreview = nullptr; // exclusively owned by the audio callback while running
    struct TrackRoute
    {
        std::unique_ptr<rr::TrackEngine> engine;
        unsigned version = 0;
        TrackRoute* next = nullptr;
    };
    void retireRoute(TrackRoute*) noexcept;
    void collectRoutes();
    std::atomic<TrackRoute*> pendingRoute { nullptr }, retiredRoutes { nullptr };
    TrackRoute* activeRoute = nullptr;
    std::unique_ptr<TrackRoute> readyRoute;
    std::atomic<unsigned> trackRevision { 1 };
    unsigned queuedTrackRevision = 0, publishedTrackRevision = 0;
    bool preparingTrack = false;
    juce::String trackError;
    std::atomic<bool> hostPlaying { false }, trackActive { false }, prepared { false };
    std::atomic<double> preparedRate { 44100 };
    std::array<std::atomic<float>, 128> inputScope {}, outputScope {};
    std::atomic<unsigned> scopePosition { 0 };
    bool previousTransportPlaying = false;
    bool bypassing = false;
    juce::int64 lastReportedPosition = -1, samplesSincePosition = 0;
    juce::ThreadPool workers { 1 };
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ReverseReverbProcessor)
};
