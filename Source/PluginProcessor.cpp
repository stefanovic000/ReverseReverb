#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

namespace
{
const juce::StringArray parameterIds { "duration", "seconds", "manualBpm", "size", "tone", "width", "tremolo", "depth", "output", "shape", "gap", "mode", "amount" };
constexpr int stateMagic = 0x52525631;
}

juce::AudioProcessorValueTreeState::ParameterLayout ReverseReverbProcessor::makeParameters()
{
    juce::AudioProcessorValueTreeState::ParameterLayout p;
    const auto id = [](const char* name) { return juce::ParameterID(name, 1); };
    p.add(std::make_unique<juce::AudioParameterChoice>(id("duration"), "Duration",
        juce::StringArray { "Seconds", "1/4 bar", "1/2 bar", "1 bar", "2 bars" }, 3));
    p.add(std::make_unique<juce::AudioParameterFloat>(id("seconds"), "Seconds", juce::NormalisableRange<float>(0.1f, 12.0f, 0.01f, 0.5f), 2.0f));
    p.add(std::make_unique<juce::AudioParameterFloat>(id("manualBpm"), "Manual tempo", juce::NormalisableRange<float>(20.0f, 400.0f, 0.1f), 120.0f));
    p.add(std::make_unique<juce::AudioParameterFloat>(id("size"), "Size", 0.0f, 1.0f, 0.72f));
    p.add(std::make_unique<juce::AudioParameterFloat>(id("tone"), "Tone", 0.0f, 1.0f, 0.55f));
    p.add(std::make_unique<juce::AudioParameterFloat>(id("width"), "Width", 0.0f, 2.0f, 1.0f));
    p.add(std::make_unique<juce::AudioParameterChoice>(id("tremolo"), "Tremolo",
        juce::StringArray { "Off", "1/8", "1/16", "1/32" }, 0));
    p.add(std::make_unique<juce::AudioParameterFloat>(id("depth"), "Depth", 0.0f, 1.0f, 0.5f));
    p.add(std::make_unique<juce::AudioParameterFloat>(id("output"), "Output", juce::NormalisableRange<float>(-36.0f, 12.0f, 0.1f), 0.0f));
    p.add(std::make_unique<juce::AudioParameterFloat>(id("shape"), "Shape", 0.0f, 1.0f, 0.0f));
    p.add(std::make_unique<juce::AudioParameterFloat>(id("gap"), "Gap", juce::NormalisableRange<float>(0.0f, 2.0f, 0.001f), 0.0f));
    p.add(std::make_unique<juce::AudioParameterChoice>(id("mode"), "Mode", juce::StringArray { "File", "Track" }, 0));
    p.add(std::make_unique<juce::AudioParameterFloat>(id("amount"), "Track amount", 0.0f, 1.0f, 0.5f));
    return p;
}

ReverseReverbProcessor::ReverseReverbProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                      .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "ReverseReverb", makeParameters())
{
    for (const auto& id : parameterIds) parameters.addParameterListener(id, this);
    startTimerHz(10);
}

ReverseReverbProcessor::~ReverseReverbProcessor()
{
    stopTimer();
    ++trackRevision;
    ++request;
    ++revision;
    workers.removeAllJobs(true, -1);
    for (const auto& id : parameterIds) parameters.removeParameterListener(id, this);
    delete pendingPreview.exchange(nullptr);
    delete activePreview;
    collectRetiredPreviews();
    delete pendingRoute.exchange(nullptr);
    delete activeRoute;
    collectRoutes();
}

void ReverseReverbProcessor::parameterChanged(const juce::String& id, float)
{
    if (id != "mode" && id != "amount") ++revision;
    if (id != "amount" && id != "output") ++trackRevision;
}
float ReverseReverbProcessor::value(const char* id) const { return parameters.getRawParameterValue(id)->load(); }

rr::Settings ReverseReverbProcessor::settings() const
{
    rr::Settings s;
    s.durationMode = static_cast<int>(value("duration")); s.seconds = value("seconds");
    s.bpm = tempoAvailable.load() ? bpm.load() : value("manualBpm");
    s.numerator = tempoAvailable.load() ? numerator.load() : 4;
    s.denominator = tempoAvailable.load() ? denominator.load() : 4;
    s.size = value("size"); s.tone = value("tone"); s.width = value("width");
    s.tremolo = static_cast<int>(value("tremolo")); s.depth = value("depth"); s.outputDb = value("output");
    s.shape = value("shape"); s.gapSeconds = value("gap");
    return s;
}

void ReverseReverbProcessor::prepareToPlay(double rate, int)
{
    prepared.store(false);
    outputRate = rate > 0 ? rate : 44100;
    preparedRate.store(outputRate);
    const auto version = ++trackRevision;
    auto route = std::make_unique<TrackRoute>(); route->version = version;
    juce::String error;
    if (wantsTrack())
    {
        try { route->engine = std::make_unique<rr::TrackEngine>(outputRate, settings()); }
        catch (const std::exception& e) { error = e.what(); }
    }
    setLatencySamples(route->engine ? route->engine->latencySamples() : 0);
    delete pendingRoute.exchange(route.release(), std::memory_order_release);
    {
        const juce::ScopedLock lock(stateLock);
        readyRoute.reset(); queuedTrackRevision = publishedTrackRevision = version;
        preparingTrack = false; trackError = error;
    }
    lastReportedPosition = -1; samplesSincePosition = 0; previousTransportPlaying = false;
    prepared.store(true);
}
void ReverseReverbProcessor::releaseResources() { prepared.store(false); stopPreview(); }
bool ReverseReverbProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto output = layouts.getMainOutputChannelSet();
    return (output == juce::AudioChannelSet::mono() || output == juce::AudioChannelSet::stereo())
        && layouts.getMainInputChannelSet() == output;
}

void ReverseReverbProcessor::retire(Preview* preview) noexcept
{
    if (!preview) return;
    auto* head = retiredPreview.load(std::memory_order_relaxed);
    do { preview->next = head; }
    while (!retiredPreview.compare_exchange_weak(head, preview, std::memory_order_release, std::memory_order_relaxed));
}

void ReverseReverbProcessor::collectRetiredPreviews()
{
    auto* head = retiredPreview.exchange(nullptr, std::memory_order_acquire);
    while (head) { auto* next = head->next; delete head; head = next; }
}

void ReverseReverbProcessor::retireRoute(TrackRoute* route) noexcept
{
    if (!route) return;
    auto* head = retiredRoutes.load(std::memory_order_relaxed);
    do { route->next = head; }
    while (!retiredRoutes.compare_exchange_weak(head, route, std::memory_order_release, std::memory_order_relaxed));
}
void ReverseReverbProcessor::collectRoutes()
{
    auto* head = retiredRoutes.exchange(nullptr, std::memory_order_acquire);
    while (head) { auto* next = head->next; delete head; head = next; }
}

void ReverseReverbProcessor::serviceTrack()
{
    collectRoutes();
    if (!prepared.load()) return;
    const auto version = trackRevision.load();
    const bool track = wantsTrack();
    auto s = settings();
    const auto rate = preparedRate.load();
    std::unique_ptr<TrackRoute> publish;
    {
        const juce::ScopedLock lock(stateLock);
        if (queuedTrackRevision != version)
        {
            queuedTrackRevision = version; readyRoute.reset(); trackError.clear();
            preparingTrack = track;
            if (!track)
            { readyRoute = std::make_unique<TrackRoute>(); readyRoute->version = version; }
            else workers.addJob(std::function<void()>([this, s, rate, version]
            {
                try
                {
                    auto route = std::make_unique<TrackRoute>(); route->version = version;
                    route->engine = std::make_unique<rr::TrackEngine>(rate, s, [this, version] { return version != trackRevision.load(); });
                    const juce::ScopedLock workerLock(stateLock);
                    if (version == trackRevision.load()) { readyRoute = std::move(route); preparingTrack = false; }
                }
                catch (const std::exception& e)
                {
                    const juce::ScopedLock workerLock(stateLock);
                    if (version == trackRevision.load()) { trackError = e.what(); preparingTrack = false; }
                }
            }));
        }
        if (readyRoute && readyRoute->version == trackRevision.load() && !hostPlaying.load() && prepared.load())
        { publishedTrackRevision = readyRoute->version; publish = std::move(readyRoute); }
    }
    if (publish)
    {
        stopPreview();
        setLatencySamples(publish->engine ? publish->engine->latencySamples() : 0);
        delete pendingRoute.exchange(publish.release(), std::memory_order_release);
    }
}

juce::String ReverseReverbProcessor::trackStatus() const
{
    const juce::ScopedLock lock(stateLock);
    if (trackError.isNotEmpty()) return "Track setup: " + trackError;
    if (publishedTrackRevision != trackRevision.load() && hostPlaying.load())
        return "Stop FL playback to apply Track settings. Amount and Output stay live.";
    if (preparingTrack) return "Preparing Track reverb...";
    if (publishedTrackRevision != trackRevision.load()) return "Applying Track settings...";
    if (!wantsTrack() && trackActive.load()) return "Returning to File mode...";
    return "Track ready - " + juce::String(1000.0 * getLatencySamples() / preparedRate.load(), 0)
        + " ms delay. Start playback before the sound to hear its full lead-in.";
}

std::array<float, 128> ReverseReverbProcessor::scope(bool output) const
{
    std::array<float, 128> values {};
    const auto position = scopePosition.load(std::memory_order_acquire);
    const auto& bins = output ? outputScope : inputScope;
    for (size_t n = 0; n < values.size(); ++n) values[n] = bins[(position + n) % bins.size()].load(std::memory_order_relaxed);
    return values;
}

void ReverseReverbProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    midi.clear();
    bool available = false;
    double nextBpm = bpm.load();
    int nextNumerator = numerator.load(), nextDenominator = denominator.load();
    bool transport = false;
    bool looping = false;
    juce::Optional<juce::AudioPlayHead::LoopPoints> loopPoints;
    juce::int64 positionSamples = -1;
    if (auto* playHead = getPlayHead())
        if (const auto position = playHead->getPosition())
        {
            transport = position->getIsPlaying();
            looping = position->getIsLooping();
            loopPoints = position->getLoopPoints();
            if (const auto time = position->getTimeInSamples()) positionSamples = *time;
            if (const auto tempo = position->getBpm(); tempo && std::isfinite(*tempo) && *tempo >= 20 && *tempo <= 400)
            {
                available = true; nextBpm = *tempo;
                if (const auto time = position->getTimeSignature())
                { nextNumerator = time->numerator; nextDenominator = time->denominator; }
            }
        }
    hostPlaying.store(transport);
    const bool timingChanged = available != tempoAvailable.load() || std::abs(nextBpm - bpm.load()) > 0.001
        || nextNumerator != numerator.load() || nextDenominator != denominator.load();
    bpm.store(nextBpm); numerator.store(nextNumerator); denominator.store(nextDenominator); tempoAvailable.store(available);
    if (timingChanged && (value("duration") > 0 || value("tremolo") > 0)) { ++revision; ++trackRevision; }

    for (int c = getTotalNumInputChannels(); c < buffer.getNumChannels(); ++c) buffer.clear(c, 0, buffer.getNumSamples());
    const auto scopeIndex = scopePosition.load(std::memory_order_relaxed) % inputScope.size();
    inputScope[scopeIndex].store(buffer.getMagnitude(0, buffer.getNumSamples()), std::memory_order_relaxed);
    if (auto* next = pendingRoute.exchange(nullptr, std::memory_order_acquire))
    { retireRoute(activeRoute); activeRoute = next; }
    const bool track = activeRoute && activeRoute->engine;
    trackActive.store(track);
    bool discontinuity = transport != previousTransportPlaying;
    if (!transport || positionSamples < 0)
    {
        lastReportedPosition = -1;
        samplesSincePosition = 0;
    }
    else
    {
        // A host may reuse one timestamp for several processing sub-blocks.
        // Repeated timestamps alone do not prove that playback has sought.
        // Compare advancing reports against the samples actually processed
        // since the previous report, allowing 1 ms of timestamp rounding.
        if (discontinuity || lastReportedPosition < 0)
        {
            lastReportedPosition = positionSamples;
            samplesSincePosition = 0;
        }
        else if (positionSamples != lastReportedPosition)
        {
            const auto tolerance = juce::jmax<juce::int64>(2, static_cast<juce::int64>(outputRate * 0.001));
            if (std::abs(positionSamples - lastReportedPosition - samplesSincePosition) > tolerance)
            {
                bool loopWrap = false;
                if (looping && loopPoints && available && positionSamples < lastReportedPosition
                    && std::isfinite(loopPoints->ppqStart) && std::isfinite(loopPoints->ppqEnd))
                {
                    const auto samplesPerBeat = outputRate * 60.0 / nextBpm;
                    const auto loopStart = loopPoints->ppqStart * samplesPerBeat;
                    const auto loopEnd = loopPoints->ppqEnd * samplesPerBeat;
                    const auto loopLength = loopEnd - loopStart;
                    const auto expected = static_cast<double>(lastReportedPosition) + static_cast<double>(samplesSincePosition);
                    if (std::isfinite(loopLength) && loopLength > tolerance
                        && lastReportedPosition >= loopStart - tolerance && lastReportedPosition < loopEnd
                        && expected >= loopEnd - tolerance
                        && positionSamples >= loopStart - tolerance && positionSamples < loopEnd)
                    {
                        const auto wraps = std::round((expected - positionSamples) / loopLength);
                        loopWrap = wraps >= 1 && std::abs(expected - positionSamples - wraps * loopLength) <= tolerance;
                    }
                }
                // Keep a repeating passage's history so even a short loop can
                // fill the delay. An unrelated seek still discards old audio.
                discontinuity = !loopWrap;
            }
            lastReportedPosition = positionSamples;
            samplesSincePosition = 0;
        }
        samplesSincePosition += buffer.getNumSamples();
    }
    if (track && discontinuity) activeRoute->engine->reset();
    previousTransportPlaying = transport;
    if (track)
    {
        activeRoute->engine->process(buffer, bypassing ? 0.0f : value("amount"), bypassing ? 0.0f : value("output"));
        playing.store(false);
        outputScope[scopeIndex].store(buffer.getMagnitude(0, buffer.getNumSamples()), std::memory_order_relaxed);
        scopePosition.fetch_add(1, std::memory_order_release);
        return;
    }
    if (bypassing)
    {
        retire(activePreview); activePreview = nullptr;
        retire(pendingPreview.exchange(nullptr, std::memory_order_acquire));
    }
    if (auto* next = pendingPreview.exchange(nullptr, std::memory_order_acquire))
    { retire(activePreview); activePreview = next; }
    if (activePreview && activePreview->epoch != previewEpoch.load())
    { retire(activePreview); activePreview = nullptr; }
    if (activePreview)
    {
        auto& p = *activePreview;
        previewType.store(p.kind);
        const auto& audio = p.audio->samples;
        const double step = p.audio->sampleRate / outputRate;
        for (int n = 0; n < buffer.getNumSamples() && p.cursor < p.end; ++n, p.cursor += step)
        {
            const int a = juce::jlimit(p.start, p.end - 1, static_cast<int>(p.cursor));
            const int b = juce::jmin(a + 1, p.end - 1);
            const float fraction = static_cast<float>(p.cursor - a);
            const double fade = juce::jmax(1.0, p.audio->sampleRate * 0.003);
            const float edge = static_cast<float>(juce::jlimit(0.0, 1.0,
                juce::jmin((p.cursor - p.start) / fade, (p.end - 1 - p.cursor) / fade)));
            for (int c = 0; c < buffer.getNumChannels(); ++c)
            {
                float sample = 0;
                if (buffer.getNumChannels() == 1 && audio.getNumChannels() == 2)
                    sample = 0.5f * (juce::jmap(fraction, audio.getSample(0, a), audio.getSample(0, b))
                                    + juce::jmap(fraction, audio.getSample(1, a), audio.getSample(1, b)));
                else
                {
                    const int sourceChannel = juce::jmin(c, audio.getNumChannels() - 1);
                    sample = juce::jmap(fraction, audio.getSample(sourceChannel, a), audio.getSample(sourceChannel, b));
                }
                buffer.addSample(c, n, sample * edge);
            }
        }
        previewTime.store((p.cursor - p.start) / p.audio->sampleRate);
        if (p.cursor >= p.end) { retire(activePreview); activePreview = nullptr; }
    }
    playing.store(activePreview != nullptr);
    outputScope[scopeIndex].store(buffer.getMagnitude(0, buffer.getNumSamples()), std::memory_order_relaxed);
    scopePosition.fetch_add(1, std::memory_order_release);
}

void ReverseReverbProcessor::processBlockBypassed(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    const juce::ScopedValueSetter<bool> guard(bypassing, true);
    processBlock(buffer, midi);
}

ReverseReverbProcessor::View ReverseReverbProcessor::view() const
{
    const juce::ScopedLock lock(stateLock);
    const bool fresh = result && resultRevision == revision.load();
    const bool showingResultStatus = status.startsWith("Ready.") || status.startsWith("Peak ")
        || status.startsWith("Very quiet") || status.startsWith("Saved ");
    return { source, fresh ? result : nullptr, selectionStart, selectionEnd,
        !busy && result && !fresh && showingResultStatus ? "Settings changed. Generate the effect again." : status,
        busy, fresh ? exportFile : juce::File() };
}

void ReverseReverbProcessor::loadFile(const juce::File& file)
{
    stopPreview();
    const auto token = ++request;
    ++revision;
    { const juce::ScopedLock lock(stateLock); busy = true; status = "Opening " + file.getFileName() + "..."; }
    workers.addJob(std::function<void()>([this, file, token]
    {
        juce::String error;
        auto loaded = rr::loadAudio(file, error);
        const juce::ScopedLock lock(stateLock);
        if (token != request.load()) return;
        busy = false;
        if (!loaded) { status = error; return; }
        source = std::move(loaded); result.reset(); exportFile = {};
        selectionStart = 0; selectionEnd = source->samples.getNumSamples();
        status = "Highlight a word or sound, then generate its effect.";
    }));
}

void ReverseReverbProcessor::setSelection(int start, int end)
{
    const juce::ScopedLock lock(stateLock);
    if (!source) return;
    start = juce::jlimit(0, source->samples.getNumSamples() - 1, start);
    end = juce::jlimit(start + 1, source->samples.getNumSamples(), end);
    if (start != selectionStart || end != selectionEnd)
    { selectionStart = start; selectionEnd = end; ++revision; stopPreview(); }
}

void ReverseReverbProcessor::generate()
{
    // Capture the revision before reading any render inputs. Concurrent host automation
    // then invalidates this request instead of stamping old settings with a new revision.
    const auto version = revision.load();
    const auto before = view();
    if (!before.source) return;
    stopPreview();
    const auto s = settings();
    const auto token = ++request;
    { const juce::ScopedLock lock(stateLock); busy = true; status = "Making your reverse reverb..."; }
    workers.addJob(std::function<void()>([this, before, s, version, token]
    {
        const auto cancelled = [this, version, token] { return version != revision.load() || token != request.load(); };
        try
        {
            auto rendered = rr::render(*before.source, before.start, before.end, s, cancelled);
            if (!rendered || cancelled())
            {
                const juce::ScopedLock lock(stateLock);
                if (token == request.load()) { busy = false; status = "Settings changed. Generate the effect again."; }
                return;
            }
            const auto folder = juce::File(RR_EXPORT_DIRECTORY);
            juce::String error;
            juce::File file;
            if (folder.createDirectory())
            {
                file = folder.getChildFile("ReverseReverb-" + juce::Uuid().toString() + ".wav");
                if (!rr::writeWav(*rendered, file, error)) file = {};
            }
            else error = "Cannot create the export folder. Use Save WAV to choose a writable folder.";
            const juce::ScopedLock lock(stateLock);
            if (cancelled())
            {
                if (file.existsAsFile()) file.deleteFile();
                if (token == request.load()) busy = false;
                return;
            }
            result = std::move(rendered); resultRevision = version; exportFile = file; busy = false;
            status = error.isNotEmpty() ? error : "Ready. Place the effect's END at the start of your original sound.";
            if (rr::peak(*result) > 1.0f) status = "Peak above 0 dB. Lower Output and generate again to avoid overload.";
            if (rr::peak(*result) < 0.000001f) status = "Very quiet result. Check your selection or try a larger Size.";
        }
        catch (const std::exception& e)
        {
            const juce::ScopedLock lock(stateLock);
            if (token == request.load()) { busy = false; status = "Could not generate: " + juce::String(e.what()); }
        }
    }));
}

void ReverseReverbProcessor::queuePreview(std::shared_ptr<const rr::Audio> audio, int start, int end, int kind)
{
    if (!audio || end <= start) return;
    collectRetiredPreviews();
    auto* preview = new Preview { std::move(audio), start, end, static_cast<double>(start), ++previewEpoch, nullptr };
    preview->kind = kind;
    delete pendingPreview.exchange(preview, std::memory_order_release);
}
void ReverseReverbProcessor::playOriginal()
{ const auto v = view(); if (v.source && !wantsTrack()) queuePreview(v.source, v.start, v.end, 1); }
void ReverseReverbProcessor::playEffect()
{ const auto v = view(); if (v.result && !wantsTrack()) queuePreview(v.result, 0, v.result->samples.getNumSamples(), 2); }
void ReverseReverbProcessor::playCombined()
{
    const auto v = view();
    if (!v.result || !v.source || wantsTrack()) return;
    stopPreview();
    const auto epoch = previewEpoch.load(), version = revision.load();
    workers.addJob(std::function<void()>([this, v, epoch, version]
    {
        auto joined = rr::combinedPreview(*v.result, *v.source, v.start, v.end);
        if (epoch != previewEpoch.load() || version != revision.load()) return;
        const int length = joined->samples.getNumSamples();
        auto* preview = new Preview { std::move(joined), 0, length, 0, epoch, nullptr };
        preview->kind = 3;
        delete pendingPreview.exchange(preview, std::memory_order_release);
    }));
}
void ReverseReverbProcessor::stopPreview()
{ ++previewEpoch; delete pendingPreview.exchange(nullptr); playing.store(false); previewType.store(0); }

void ReverseReverbProcessor::saveCopy(const juce::File& target)
{
    const auto v = view();
    if (!v.result) return;
    workers.addJob(std::function<void()>([this, audio = v.result, target]
    {
        juce::String error;
        const auto ok = rr::writeWav(*audio, target, error);
        const juce::ScopedLock lock(stateLock);
        status = ok ? "Saved " + target.getFileName() : error;
    }));
}

bool ReverseReverbProcessor::waitForWork(int milliseconds)
{
    const auto end = juce::Time::getMillisecondCounterHiRes() + milliseconds;
    while (workers.getNumJobs() != 0 && juce::Time::getMillisecondCounterHiRes() < end) juce::Thread::sleep(5);
    return workers.getNumJobs() == 0;
}

void ReverseReverbProcessor::getStateInformation(juce::MemoryBlock& data)
{
    const auto v = view();
    juce::MemoryOutputStream out(data, false);
    out.writeInt(stateMagic);
    out.writeString(parameters.copyState().toXmlString());
    out.writeBool(v.source != nullptr);
    if (!v.source) return;
    out.writeString(v.source->name); out.writeDouble(v.source->sampleRate);
    out.writeInt(v.source->samples.getNumChannels()); out.writeInt(v.source->samples.getNumSamples());
    out.writeInt(v.start); out.writeInt(v.end);
    for (int c = 0; c < v.source->samples.getNumChannels(); ++c)
        out.write(v.source->samples.getReadPointer(c), static_cast<size_t>(v.source->samples.getNumSamples()) * sizeof(float));
}

void ReverseReverbProcessor::setStateInformation(const void* data, int size)
{
    if (!data || size < 10 || size > rr::maxSourceBytes + 65536) return;
    juce::MemoryInputStream in(data, static_cast<size_t>(size), false);
    if (in.readInt() != stateMagic) return;
    const auto xml = juce::parseXML(in.readString());
    if (!xml || !xml->hasTagName("ReverseReverb")) return;
    auto tree = juce::ValueTree::fromXml(*xml);
    // APVTS must not retain values from a previous session when a legacy state
    // lacks newly introduced controls. Fill every missing parameter explicitly.
    for (const auto& id : parameterIds)
    {
        bool found = false;
        for (const auto child : tree) if (child["id"].toString() == id) found = true;
        if (!found)
        {
            auto* p = parameters.getParameter(id);
            juce::ValueTree child("PARAM"); child.setProperty("id", id, nullptr);
            child.setProperty("value", p->convertFrom0to1(p->getDefaultValue()), nullptr);
            tree.appendChild(child, nullptr);
        }
    }
    std::shared_ptr<rr::Audio> restored;
    int start = 0, end = 0;
    if (in.readBool())
    {
        restored = std::make_shared<rr::Audio>();
        restored->name = in.readString(); restored->sampleRate = in.readDouble();
        const auto channels = in.readInt(), samples = in.readInt();
        start = in.readInt(); end = in.readInt();
        const auto bytes = static_cast<juce::int64>(channels) * samples * sizeof(float);
        if (channels < 1 || channels > 2 || samples < 2 || bytes > rr::maxSourceBytes
            || bytes > in.getNumBytesRemaining() || !std::isfinite(restored->sampleRate)
            || restored->sampleRate < 8000 || restored->sampleRate > 192000) return;
        restored->samples.setSize(channels, samples);
        for (int c = 0; c < channels; ++c)
        {
            const int channelBytes = samples * static_cast<int>(sizeof(float));
            if (in.read(restored->samples.getWritePointer(c), channelBytes) != channelBytes) return;
            for (int n = 0; n < samples; ++n)
                if (!std::isfinite(restored->samples.getSample(c, n))) return;
        }
        start = juce::jlimit(0, samples - 1, start); end = juce::jlimit(start + 1, samples, end);
    }
    stopPreview(); ++revision; ++request;
    parameters.replaceState(tree);
    {
        const juce::ScopedLock lock(stateLock);
        source = std::move(restored); result.reset(); exportFile = {};
        selectionStart = start; selectionEnd = end; busy = false;
        status = source ? "Recording restored. Generating its effect..." : "Drop a recording to begin.";
    }
    if (view().source && !wantsTrack()) generate();
}

juce::AudioProcessorEditor* ReverseReverbProcessor::createEditor() { return new ReverseReverbEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new ReverseReverbProcessor(); }
