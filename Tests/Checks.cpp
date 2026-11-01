#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <pluginterfaces/vst/ivstcomponent.h>
#include <cmath>
#include <iostream>

namespace
{
int passed = 0, failed = 0;
void check(bool condition, const char* message)
{
    std::cout << (condition ? "PASS " : "FAIL ") << message << '\n';
    condition ? ++passed : ++failed;
}
rr::Audio makeSource(double rate = 44100)
{
    rr::Audio a;
    a.name = "Learning demo.wav"; a.sampleRate = rate;
    a.samples.setSize(1, static_cast<int>(rate * 1.6));
    for (int n = 0; n < a.samples.getNumSamples(); ++n)
    {
        const double t = n / rate;
        const double envelope = std::pow(std::sin(juce::MathConstants<double>::pi * t / 1.6), 2);
        const double signal = std::sin(t * 2 * juce::MathConstants<double>::pi * 170)
            + 0.35 * std::sin(t * 2 * juce::MathConstants<double>::pi * 430);
        a.samples.setSample(0, n, static_cast<float>(signal * envelope * 0.25));
    }
    return a;
}
double energy(const rr::Audio& audio, int first, int count)
{
    double sum = 0;
    for (int n = first; n < first + count; ++n)
        sum += std::pow(audio.samples.getSample(0, n), 2);
    return sum;
}
void setParam(ReverseReverbProcessor& p, const char* name, float actual)
{
    auto* parameter = p.parameters.getParameter(name);
    parameter->setValueNotifyingHost(parameter->convertTo0to1(actual));
}
void screenshot(ReverseReverbProcessor& p, const juce::File& file, bool wave = false)
{
    std::unique_ptr<juce::AudioProcessorEditor> editor(p.createEditor());
    if (wave)
        for (int n = 0; n < editor->getNumChildComponents(); ++n)
            if (auto* button = dynamic_cast<juce::TextButton*>(editor->getChildComponent(n));
                button && button->getButtonText() == "Wave" && button->onClick) button->onClick();
    auto image = editor->createComponentSnapshot(editor->getLocalBounds(), true, 1.0f);
    auto stream = file.createOutputStream();
    if (stream) { stream->setPosition(0); stream->truncate(); }
    check(stream != nullptr && juce::PNGImageFormat().writeImageToStream(image, *stream), "Editor snapshot written");
}
struct TestPlayHead : juce::AudioPlayHead
{
    double tempo = 100;
    bool playing = false;
    bool looping = false;
    double loopStart = 0, loopEnd = 0;
    juce::int64 position = 0;
    juce::Optional<PositionInfo> getPosition() const override
    {
        PositionInfo info; info.setBpm(tempo); info.setTimeSignature(TimeSignature { 6, 8 });
        info.setIsPlaying(playing); info.setTimeInSamples(position);
        info.setIsLooping(looping);
        if (looping) info.setLoopPoints(LoopPoints { loopStart, loopEnd });
        return info;
    }
};

void checkVst3Bundle()
{
    const auto executable = juce::File::getSpecialLocation(juce::File::currentExecutableFile);
    const auto config = executable.getParentDirectory().getFileName();
    const auto buildFolder = executable.getParentDirectory().getParentDirectory().getParentDirectory();
    const auto binary = buildFolder.getChildFile("ReverseReverb_artefacts/" + config
        + "/VST3/ReverseReverb.vst3/Contents/x86_64-win/ReverseReverb.vst3");
    juce::DynamicLibrary library;
    check(library.open(binary.getFullPathName()), "Built VST3 binary loads into a host process");
    const auto init = library.getFunction<bool()>("InitDll");
    const auto exit = library.getFunction<bool()>("ExitDll");
    const auto factoryFunction = reinterpret_cast<Steinberg::IPluginFactory* (PLUGIN_API*)()>(library.getFunction("GetPluginFactory"));
    check(init && exit && factoryFunction, "VST3 exports its module lifecycle and plugin factory");
    if (!init || !exit || !factoryFunction || !init()) return;
    auto* factory = factoryFunction();
    bool created = false, initialized = false, buses = false;
    if (factory)
    {
        for (Steinberg::int32 index = 0; index < factory->countClasses(); ++index)
        {
            Steinberg::PClassInfo info {};
            if (factory->getClassInfo(index, &info) != Steinberg::kResultOk
                || juce::String(info.category) != "Audio Module Class") continue;
            Steinberg::Vst::IComponent* component = nullptr;
            if (factory->createInstance(info.cid, Steinberg::Vst::IComponent_iid,
                reinterpret_cast<void**>(&component)) != Steinberg::kResultOk || !component) continue;
            created = true;
            initialized = component->initialize(nullptr) == Steinberg::kResultOk;
            if (initialized)
            {
                buses = component->getBusCount(Steinberg::Vst::kAudio, Steinberg::Vst::kInput) == 1
                    && component->getBusCount(Steinberg::Vst::kAudio, Steinberg::Vst::kOutput) == 1;
                component->terminate();
            }
            component->release();
        }
        factory->release();
    }
    check(created && initialized, "VST3 factory creates and initializes the audio component");
    check(buses, "VST3 exposes the expected mixer input and output buses");
    exit();
}

juce::AudioBuffer<float> streamImpulse(double rate, const rr::Settings& s, int blockSize, float amount = 1.0f)
{
    rr::TrackEngine engine(rate, s);
    juce::AudioBuffer<float> warmup(2, 2048); warmup.clear(); engine.process(warmup, amount, 0); engine.reset();
    const int total = engine.latencySamples() + 4096;
    juce::AudioBuffer<float> result(2, total); result.clear();
    for (int position = 0; position < total; position += blockSize)
    {
        const int count = juce::jmin(blockSize, total - position);
        juce::AudioBuffer<float> block(2, count); block.clear();
        if (position == 0) { block.setSample(0, 0, 1); block.setSample(1, 0, 1); }
        engine.process(block, amount, 0);
        for (int c = 0; c < 2; ++c) result.copyFrom(c, position, block, c, 0, count);
    }
    return result;
}

void featureChecks(const juce::File& folder)
{
    auto source = makeSource();
    rr::Settings s; s.durationMode = 0; s.seconds = 0.5;
    const auto natural = rr::render(source, 1000, 24000, s);
    check(rr::shapeGain(0.1, 0) == 1 && rr::shapeGain(1.0, 1.0f) == 1, "Neutral Shape is unity and all curves end at unity");
    s.shape = 0.7f;
    const auto shaped = rr::render(source, 1000, 24000, s);
    check(energy(*shaped, 0, 10000) < energy(*natural, 0, 10000), "Shape emphasizes the end by attenuating the early effect");
    s.gapSeconds = 0.125;
    const auto gapped = rr::render(source, 1000, 24000, s);
    const int lead = shaped->samples.getNumSamples(), gap = static_cast<int>(std::llround(source.sampleRate * s.gapSeconds));
    check(gapped->samples.getNumSamples() == lead + gap && gapped->samples.getMagnitude(lead, gap) == 0,
          "Gap adds exact trailing silence without shortening the audible lead-in");
    bool sameLead = true;
    for (int n = 0; n < lead; ++n) sameLead = sameLead && gapped->samples.getSample(0, n) == shaped->samples.getSample(0, n);
    check(sameLead, "Adding Gap preserves every sample of the shaped effect");
    const auto combined = rr::combinedPreview(*gapped, source, 1000, 24000);
    check(combined->samples.getNumSamples() == lead + gap + 23000, "Combined audition includes effect, Gap, then only the selected original");
    check(std::abs(combined->samples.getSample(0, lead + gap + 500) - source.samples.getSample(0, 1500)) < 0.000001f,
          "Original begins at the end of the effect-only export, with no extra offset");

    s = {}; s.durationMode = 0; s.seconds = 0.125; s.gapSeconds = 0.025; s.shape = 0.35f;
    rr::TrackEngine engine(8000, s);
    const int delay = engine.latencySamples();
    check(delay == 1200, "Track latency equals lead-in plus Gap at 8 kHz");
    const auto a = streamImpulse(8000, s, 64), b = streamImpulse(8000, s, 257);
    check(std::abs(a.getSample(0, delay) - 1.0f) < 0.00001f, "Track original lands exactly at the reported delay");
    check(a.getMagnitude(0, delay - 200) > 0.000001f, "Track produces a wet buildup before the delayed original");
    check(a.getMagnitude(delay - 200, 200) < 0.00001f, "Track preserves the requested quiet gap before an isolated hit");
    float difference = 0;
    for (int c = 0; c < 2; ++c) for (int n = 0; n < a.getNumSamples(); ++n)
        difference = juce::jmax(difference, std::abs(a.getSample(c, n) - b.getSample(c, n)));
    check(difference < 0.00001f, "Track timing and sound are independent of host block size");
    const auto dryOnly = streamImpulse(8000, s, 113, 0);
    check(dryOnly.getMagnitude(0, delay) < 0.000001f && std::abs(dryOnly.getSample(0, delay) - 1) < 0.000001f,
          "Zero Track Amount preserves a correctly delayed original without wet audio");
    juce::AudioBuffer<float> block(2, 256); block.clear(); block.setSample(0, 0, 1);
    engine.process(block, 1, 0); engine.reset();
    bool resetSilent = true;
    for (int n = 0; n < 32; ++n) { block.clear(); engine.process(block, 1, 0); resetSilent = resetSilent && block.getMagnitude(0, 256) < 0.000001f; }
    check(resetSilent, "Reset removes previous wet and dry history");
    rr::TrackEngine highRate(48000, s);
    check(highRate.latencySamples() == 7200, "Track latency scales correctly at 48 kHz");

    ReverseReverbProcessor p;
    setParam(p, "duration", 0); setParam(p, "seconds", 0.12f); setParam(p, "gap", 0.025f); setParam(p, "mode", 1);
    p.prepareToPlay(8000, 256);
    juce::MidiBuffer midi;
    block.clear(); p.processBlock(block, midi);
    check(p.isTrackActive() && p.getLatencySamples() == 1160, "Processor activates Track mode and reports measured latency");
    TestPlayHead head; head.playing = true; head.position = 256;
    p.setPlayHead(&head); block.clear(); p.processBlock(block, midi); head.position += 256;
    setParam(p, "gap", 0.04f); p.serviceTrack(); p.waitForWork(); p.serviceTrack();
    check(p.getLatencySamples() == 1160 && p.trackStatus().contains("Stop FL"), "Latency-changing edits wait while the host is playing");
    head.playing = false; block.clear(); p.processBlock(block, midi); p.serviceTrack();
    block.clear(); p.processBlock(block, midi);
    check(p.getLatencySamples() == 1280, "Pending Track settings apply after host playback stops");
    setParam(p, "amount", 0.2f); p.serviceTrack();
    check(p.getLatencySamples() == 1280, "Live Amount changes do not alter host latency");
    screenshot(p, folder.getChildFile("interface-track.png"));
    setParam(p, "mode", 0); p.serviceTrack(); block.clear(); p.processBlock(block, midi);
    check(!p.isTrackActive() && p.getLatencySamples() == 0, "Returning to File mode restores zero processing latency");
    p.setPlayHead(nullptr);

    const auto demo = folder.getChildFile("Features-" + juce::Uuid().toString() + ".wav");
    juce::String error; rr::writeWav(source, demo, error);
    p.loadFile(demo); p.waitForWork(); p.setSelection(1000, 24000);
    setParam(p, "seconds", 1.25f); setParam(p, "shape", 0.45f); setParam(p, "gap", 0.15f);
    p.generate(); p.waitForWork();
    screenshot(p, folder.getChildFile("interface-loaded.png"));
    screenshot(p, folder.getChildFile("interface-wave.png"), true);
    const auto v = p.view();
    p.playCombined(); p.waitForWork();
    int blocks = 0;
    do { block.clear(); p.processBlock(block, midi); ++blocks; } while (p.isPreviewPlaying() && blocks < 2000);
    const auto duration = (v.result->samples.getNumSamples() + v.end - v.start) / source.sampleRate;
    check(std::abs(blocks - std::ceil(duration * 8000 / 256)) <= 1, "Combined audition plays for the exact effect-plus-selection duration");
    p.playCombined(); p.stopPreview(); p.waitForWork(); block.clear(); p.processBlock(block, midi);
    check(!p.isPreviewPlaying(), "Stop also cancels an audition still being assembled");
    juce::MemoryBlock state; p.getStateInformation(state);
    ReverseReverbProcessor restored;
    restored.setStateInformation(state.getData(), static_cast<int>(state.getSize())); restored.waitForWork();
    check(std::abs(restored.settings().shape - 0.45f) < 0.00001f && std::abs(restored.settings().gapSeconds - 0.15) < 0.00001,
          "Project restore retains Shape and Gap");
    juce::MemoryInputStream oldIn(state, false); const auto magic = oldIn.readInt();
    auto oldTree = juce::ValueTree::fromXml(*juce::parseXML(oldIn.readString()));
    for (int n = oldTree.getNumChildren(); --n >= 0;)
        if (juce::StringArray { "shape", "gap", "mode", "amount" }.contains(oldTree.getChild(n)["id"].toString())) oldTree.removeChild(n, nullptr);
    juce::MemoryBlock legacy;
    { juce::MemoryOutputStream out(legacy, false); out.writeInt(magic); out.writeString(oldTree.toXmlString());
      out.write(static_cast<const char*>(state.getData()) + oldIn.getPosition(), static_cast<size_t>(oldIn.getNumBytesRemaining())); }
    setParam(restored, "mode", 1);
    restored.setStateInformation(legacy.getData(), static_cast<int>(legacy.getSize())); restored.waitForWork();
    check(!restored.wantsTrack() && restored.settings().shape == 0 && restored.settings().gapSeconds == 0,
          "Legacy projects restore File mode, neutral Shape, and zero Gap rather than retaining new session values");
    setParam(p, "mode", 1); p.prepareToPlay(8000, 256);
    p.getStateInformation(state);
    ReverseReverbProcessor trackRestored;
    trackRestored.setStateInformation(state.getData(), static_cast<int>(state.getSize())); trackRestored.waitForWork();
    trackRestored.prepareToPlay(8000, 256); block.clear(); trackRestored.processBlock(block, midi);
    check(trackRestored.wantsTrack() && trackRestored.isTrackActive() && trackRestored.view().source,
          "Track mode restores without discarding the loaded File source");

    ReverseReverbProcessor transportTest;
    setParam(transportTest, "duration", 0); setParam(transportTest, "seconds", 0.12f);
    setParam(transportTest, "mode", 1); setParam(transportTest, "amount", 0);
    transportTest.prepareToPlay(8000, 256);
    TestPlayHead transportHead; transportHead.playing = true; transportTest.setPlayHead(&transportHead);
    block.clear(); block.setSample(0, 0, 1); transportTest.processBlock(block, midi);
    transportHead.position = 10000;
    bool seekSilent = true;
    for (int n = 0; n < 20; ++n)
    {
        block.clear(); transportTest.processBlock(block, midi); transportHead.position += 256;
        seekSilent = seekSilent && block.getMagnitude(0, 256) < 0.000001f;
    }
    check(seekSilent, "Seeking clears delayed audio instead of leaking the previous passage");
    const int bypassDelay = transportTest.getLatencySamples();
    juce::AudioBuffer<float> bypassOutput(2, 2048); bypassOutput.clear();
    for (int position = 0; position < 2048; position += 256)
    {
        block.clear(); if (position == 0) block.setSample(0, 0, 1);
        transportTest.processBlockBypassed(block, midi); transportHead.position += 256;
        bypassOutput.copyFrom(0, position, block, 0, 0, 256);
    }
    check(bypassOutput.getMagnitude(0, bypassDelay) < 0.000001f && std::abs(bypassOutput.getSample(0, bypassDelay) - 1) < 0.000001f,
          "Host bypass retains Track dry-path latency and alignment");
    transportTest.setPlayHead(nullptr);
}

// Exercise host position reporting separately from ideal, sample-exact playback.
// Some wrappers report a position once for multiple processing sub-blocks.
void trackHostChecks()
{
    const auto run = [](bool repeated, bool jitter) {
        ReverseReverbProcessor processor;
        setParam(processor, "duration", 0); setParam(processor, "seconds", 0.12f);
        setParam(processor, "mode", 1); setParam(processor, "amount", 0);
        processor.prepareToPlay(48000, 256);
        TestPlayHead head; head.playing = true; processor.setPlayHead(&head);
        juce::AudioBuffer<float> block(2, 256); juce::MidiBuffer midi;
        float error = 0;
        for (int b = 0; b < 160; ++b)
        {
            head.position = (repeated ? b / 2 * 2 : b) * 256 + (jitter ? (b % 2) * 8 : 0);
            for (int c = 0; c < 2; ++c) juce::FloatVectorOperations::fill(block.getWritePointer(c), 0.125f, 256);
            processor.processBlock(block, midi);
            if (b > 80)
                for (int c = 0; c < 2; ++c) for (int n = 0; n < 256; ++n)
                    error = juce::jmax(error, std::abs(block.getSample(c, n) - 0.125f));
        }
        processor.setPlayHead(nullptr);
        return error;
    };
    check(run(false, false) < 0.000001f, "Track preserves original level with ordinary host positions");
    check(run(true, false) < 0.000001f, "Repeated host positions do not continually erase Track audio");
    check(run(false, true) < 0.000001f, "Small host position rounding does not continually erase Track audio");

    rr::Settings settings; settings.durationMode = 0; settings.seconds = 0.12;
    rr::TrackEngine a(48000, settings), b(48000, settings);
    juce::AudioBuffer<float> first(2, 256), second(2, 256);
    bool isolated = true;
    for (int block = 0; block < 80; ++block)
    {
        for (int c = 0; c < 2; ++c) juce::FloatVectorOperations::fill(first.getWritePointer(c), 0.125f, 256);
        second.clear(); a.process(first, 0.5f, 0); b.process(second, 0.5f, 0);
        isolated = isolated && second.getMagnitude(0, 256) == 0;
    }
    check(isolated, "Track instances do not share audio buffers or leak sound into another channel");

    ReverseReverbProcessor track, plain;
    setParam(track, "duration", 0); setParam(track, "seconds", 0.12f);
    setParam(track, "mode", 1); setParam(track, "amount", 0);
    track.prepareToPlay(8000, 256); plain.prepareToPlay(8000, 256);
    TestPlayHead loopHead; loopHead.playing = true; loopHead.looping = true;
    loopHead.loopEnd = 512.0 / 8000 * loopHead.tempo / 60;
    track.setPlayHead(&loopHead); plain.setPlayHead(&loopHead);
    bool plainUnchanged = true, loopAudible = true;
    juce::MidiBuffer midi;
    for (int blockIndex = 0; blockIndex < 80; ++blockIndex)
    {
        loopHead.position = blockIndex % 2 * 256;
        for (int c = 0; c < 2; ++c)
        {
            juce::FloatVectorOperations::fill(first.getWritePointer(c), 0.125f, 256);
            juce::FloatVectorOperations::fill(second.getWritePointer(c), -0.25f, 256);
        }
        track.processBlock(first, midi); plain.processBlock(second, midi);
        for (int c = 0; c < 2; ++c) for (int n = 0; n < 256; ++n)
        {
            plainUnchanged = plainUnchanged && second.getSample(c, n) == -0.25f;
            if (blockIndex > 10) loopAudible = loopAudible && first.getSample(c, n) == 0.125f;
        }
    }
    check(plainUnchanged && plain.getLatencySamples() == 0 && !plain.wantsTrack(),
          "A separate File-mode channel keeps its own sound, mode, and zero plugin latency");
    check(loopAudible, "Declared loops shorter than Track delay still produce the original after warmup");
    loopHead.position = 10000;
    bool loopSeekSilent = true;
    for (int blockIndex = 0; blockIndex < 20; ++blockIndex)
    {
        first.clear(); track.processBlock(first, midi); loopHead.position += 256;
        loopSeekSilent = loopSeekSilent && first.getMagnitude(0, 256) == 0;
    }
    check(loopSeekSilent, "An unrelated seek still clears history when looping is enabled");
    track.setPlayHead(nullptr); plain.setPlayHead(nullptr);

    // Measure sustained audio, not only isolated impulses. Zero Amount must
    // preserve both channels exactly after the advertised waiting time.
    for (const auto rate : { 44100.0, 48000.0 })
    {
        rr::Settings defaults;
        rr::TrackEngine engine(rate, defaults);
        const int total = engine.latencySamples() + static_cast<int>(rate);
        juce::AudioBuffer<float> signal(2, total); signal.clear();
        for (int c = 0; c < 2; ++c) for (int n = 0; n < total; ++n)
            signal.setSample(c, n, 0.1f * std::sin(static_cast<float>(n * 2 * juce::MathConstants<double>::pi * (c == 0 ? 170 : 430) / rate)));
        engine.process(signal, 0, 0);
        float error = 0;
        for (int c = 0; c < 2; ++c) for (int n = engine.latencySamples(); n < total; ++n)
        {
            const float expected = 0.1f * std::sin(static_cast<float>((n - engine.latencySamples()) * 2 * juce::MathConstants<double>::pi * (c == 0 ? 170 : 430) / rate));
            error = juce::jmax(error, std::abs(signal.getSample(c, n) - expected));
        }
        check(error < 0.000001f, rate == 44100 ? "Default Track settings preserve sustained stereo original at 44.1 kHz"
                                             : "Default Track settings preserve sustained stereo original at 48 kHz");
    }
}
}

int main()
{
    juce::ScopedJuceInitialiser_GUI gui;
    const auto folder = juce::File(RR_EXPORT_DIRECTORY).getParentDirectory().getChildFile("test-output");
    if (!folder.createDirectory()) { std::cerr << "Cannot create test output folder\n"; return 1; }
    try
    {
        rr::Settings settings;
        settings.durationMode = 0; settings.seconds = 1.0;
        auto source = makeSource();
        const auto output = rr::render(source, 0, source.samples.getNumSamples(), settings);
        check(output && output->samples.getNumChannels() == 2 && output->samples.getNumSamples() == 44100,
              "Mono input creates a stereo effect of the requested duration");
        check(rr::peak(*output) > 0.0001f && rr::peak(*output) < 1.0f, "Reverb creates finite useful audio at a sensible level");
        check(output->samples.getSample(0, 0) == 0 && output->samples.getSample(0, 44099) == 0,
              "Export boundaries fade to zero");
        check(energy(*output, 22050, 22050) > energy(*output, 0, 22050), "Reverse tail grows toward its endpoint");
        settings.outputDb = -6;
        const auto quieter = rr::render(source, 0, source.samples.getNumSamples(), settings);
        check(std::abs(rr::peak(*quieter) / rr::peak(*output) - juce::Decibels::decibelsToGain(-6.0f)) < 0.0001f,
              "Output control changes exported gain correctly");
        settings.outputDb = 0; settings.width = 0;
        const auto mono = rr::render(source, 0, source.samples.getNumSamples(), settings);
        bool identical = true;
        for (int n = 0; n < mono->samples.getNumSamples(); ++n)
            identical = identical && mono->samples.getSample(0, n) == mono->samples.getSample(1, n);
        check(identical, "Zero width produces identical left and right channels");
        settings.width = 1; settings.tremolo = 2; settings.depth = 0;
        const auto noPulse = rr::render(source, 0, source.samples.getNumSamples(), settings);
        check(std::abs(rr::peak(*noPulse) - rr::peak(*output)) < 0.00001f, "Zero pulse depth preserves the unpulsed sound");
        settings.depth = 1;
        const auto pulse = rr::render(source, 0, source.samples.getNumSamples(), settings);
        check(energy(*pulse, 0, 44100) < energy(*output, 0, 44100), "Tremolo reduces volume rhythmically");
        check(!rr::render(source, 0, source.samples.getNumSamples(), settings, [] { return true; }),
              "A cancelled render returns no stale result");
        settings = {}; settings.numerator = 6; settings.denominator = 8;
        check(std::abs(rr::durationSeconds(settings) - 1.5) < 0.000001, "Bar lengths respect 6/8 time");
        settings.durationMode = 0; settings.seconds = 0.1;
        rr::Audio silent; silent.sampleRate = 48000; silent.samples.setSize(2, 100); silent.samples.clear();
        const auto silentResult = rr::render(silent, 0, 100, settings);
        check(silentResult->samples.getNumSamples() == 4800 && rr::peak(*silentResult) == 0,
              "Silent stereo source stays silent at 48 kHz");
        bool rejected = false;
        try { rr::render(source, 100, 100, settings); } catch (const std::exception&) { rejected = true; }
        check(rejected, "Empty selection is rejected");

        juce::String error;
        const auto sourceFile = folder.getChildFile("Demo-" + juce::Uuid().toString() + ".wav");
        check(rr::writeWav(source, sourceFile, error), "Source WAV written");
        const auto readBack = rr::loadAudio(sourceFile, error);
        check(readBack && readBack->samples.getNumSamples() == source.samples.getNumSamples()
            && std::abs(rr::peak(*readBack) - rr::peak(source)) < 0.000001f, "32-bit WAV round trip retains the source");
        check(!rr::writeWav(source, sourceFile, error), "Saving does not silently overwrite an existing file");
        check(!rr::loadAudio(folder.getChildFile("missing.wav"), error), "Missing audio file gives a useful failure");

        ReverseReverbProcessor p;
        p.prepareToPlay(48000, 256);
        juce::AudioBuffer<float> block(2, 256); juce::MidiBuffer midi;
        for (int c = 0; c < 2; ++c) for (int n = 0; n < 256; ++n) block.setSample(c, n, 0.125f);
        p.processBlock(block, midi);
        check(block.getSample(0, 0) == 0.125f && block.getSample(1, 255) == 0.125f, "Normal mixer audio passes through unchanged");
        screenshot(p, folder.getChildFile("interface-empty.png"));
        p.loadFile(sourceFile); check(p.waitForWork(), "Background import completes");
        auto view = p.view();
        check(view.source && view.end == source.samples.getNumSamples(), "Imported source is selected in full");
        p.setSelection(1000, 24000);
        setParam(p, "duration", 0); setParam(p, "seconds", 1.25f);
        p.generate(); check(p.waitForWork(), "Background render completes");
        view = p.view();
        check(view.result && view.result->samples.getNumSamples() == 55125 && view.exportFile.existsAsFile(),
              "Plugin publishes an exact-length result and a persistent drag WAV");
        screenshot(p, folder.getChildFile("interface-loaded.png"));
        p.playOriginal();
        int blocksPlayed = 0;
        do { block.clear(); p.processBlock(block, midi); ++blocksPlayed; } while (p.isPreviewPlaying() && blocksPlayed < 1000);
        const double expectedBlocks = (24000 - 1000) * 48000.0 / 44100.0 / 256;
        check(std::abs(blocksPlayed - std::ceil(expectedBlocks)) <= 1, "Preview keeps duration across 44.1-to-48 kHz playback");
        p.collectRetiredPreviews();
        p.playEffect(); block.clear(); p.processBlock(block, midi);
        check(p.isPreviewPlaying(), "Processed preview starts");
        p.stopPreview(); block.clear(); p.processBlock(block, midi);
        check(!p.isPreviewPlaying() && block.getMagnitude(0, 256) == 0, "Stop ends preview without changing pass-through");
        p.collectRetiredPreviews();
        juce::MemoryBlock state; p.getStateInformation(state);
        ReverseReverbProcessor restored;
        restored.setStateInformation(state.getData(), static_cast<int>(state.getSize()));
        check(restored.waitForWork(), "Restored source regenerates in the background");
        check(restored.view().source && restored.view().start == 1000 && restored.view().end == 24000 && restored.view().result,
              "Project state embeds source audio, settings and selection");
        restored.setStateInformation(state.getData(), static_cast<int>(state.getSize() / 2));
        check(restored.view().source && restored.view().end == 24000, "Truncated project state leaves valid work intact");
        setParam(p, "tone", 0.2f);
        check(!p.view().result && p.view().exportFile == juce::File(), "Changed settings disable stale playback/export");
        p.generate(); setParam(p, "tone", 0.3f); p.waitForWork();
        check(!p.view().result && !p.view().busy, "A superseded render cannot publish over new settings");
        setParam(p, "duration", 3);
        TestPlayHead head; p.setPlayHead(&head); block.clear(); p.processBlock(block, midi);
        check(p.hasHostTempo() && std::abs(p.settings().bpm - 100) < 0.001 && p.settings().numerator == 6,
              "Host tempo and time signature are read correctly");
        p.generate(); p.waitForWork();
        check(p.view().result && p.view().result->samples.getNumSamples() == 79380, "Host tempo determines musical export length");
        head.tempo = 110; block.clear(); p.processBlock(block, midi);
        check(!p.view().result, "Host tempo changes invalidate a musical-duration result");
        p.setPlayHead(nullptr);
        p.loadFile(folder.getChildFile("missing.wav")); p.waitForWork();
        check(p.view().source && p.view().status.contains("Cannot open"), "Failed replacement preserves the source and reports its error");
        checkVst3Bundle();
        featureChecks(folder);
        trackHostChecks();
        std::cout << "\n" << passed << " passed; " << failed << " failed.\n";
        std::cout << "Screenshots and test recordings: " << folder.getFullPathName() << '\n';
    }
    catch (const std::exception& e) { std::cerr << e.what() << '\n'; ++failed; }
    return failed == 0 ? 0 : 1;
}
