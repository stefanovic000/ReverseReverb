#include "PluginEditor.h"

namespace
{
using namespace rr::ui;
void labelStyle(juce::Label& label, float size = 11.5f, juce::Colour colour = muted)
{
    label.setFont(juce::FontOptions(size));
    label.setColour(juce::Label::textColourId, colour);
    label.setInterceptsMouseClicks(false, false);
}
}

void ReverseReverbEditor::Scope::paint(juce::Graphics& g)
{
    using namespace rr::ui;
    g.setColour(field); g.fillRoundedRectangle(getLocalBounds().toFloat(), 4);
    const float middle = getHeight() * 0.5f;
    g.setColour(border); g.drawHorizontalLine(static_cast<int>(middle), 10, getWidth() - 10.0f);
    g.setColour(accent.withAlpha(0.8f));
    for (size_t n = 0; n < bins.size(); ++n)
    {
        const float h = juce::jlimit(0.0f, 1.0f, bins[n]) * (middle - 8);
        const float x = 10 + static_cast<float>(n) * (getWidth() - 20) / bins.size();
        g.drawVerticalLine(static_cast<int>(x), middle - h, middle + h + 1);
    }
}

ReverseReverbEditor::ReverseReverbEditor(ReverseReverbProcessor& p) : AudioProcessorEditor(p), processor(p)
{
    setLookAndFeel(&theme);
    setSize(900, 560);
    for (auto* c : std::initializer_list<juce::Component*> { &waveform, &effectWaveform, &graph, &inputScope, &outputScope,
        &open, &selectAll, &zoomIn, &zoomOut, &fit, &original, &generate, &preview, &stop, &save, &drag,
        &curveTab, &waveTab, &sourceName, &selectionInfo, &statusLabel, &timingLabel, &bpmLabel, &effectInfo,
        &pulseLabel, &amountLabel, &duration, &tremolo, &mode, &previewMode, &manualBpm, &amount }) addAndMakeVisible(c);
    for (auto* label : { &sourceName, &selectionInfo, &statusLabel, &timingLabel, &bpmLabel, &effectInfo, &pulseLabel, &amountLabel })
        labelStyle(*label);
    labelStyle(sourceName, 12, text);
    labelStyle(statusLabel, 11, muted);
    timingLabel.setJustificationType(juce::Justification::centredRight);
    bpmLabel.setText("Manual", juce::dontSendNotification);
    pulseLabel.setText("Pulse", juce::dontSendNotification);
    amountLabel.setText("Amount", juce::dontSendNotification);
    mode.addItemList({ "File", "Track" }, 1);
    modeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.parameters, "mode", mode);
    mode.setTooltip("File: load and export a selected sound. Track: buffered mixer processing with added delay.");
    duration.addItemList({ "Seconds", "1/4 bar", "1/2 bar", "1 bar", "2 bars" }, 1);
    tremolo.addItemList({ "Off", "1/8", "1/16", "1/32" }, 1);
    durationAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.parameters, "duration", duration);
    tremoloAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.parameters, "tremolo", tremolo);
    previewMode.addItemList({ "Effect only", "Effect + original" }, 1); previewMode.setSelectedId(2);
    manualBpm.setSliderStyle(juce::Slider::IncDecButtons);
    manualBpm.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 52, 24);
    bpmAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.parameters, "manualBpm", manualBpm);
    amount.setSliderStyle(juce::Slider::LinearHorizontal);
    amount.setTextBoxStyle(juce::Slider::TextBoxRight, false, 54, 24);
    amountAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.parameters, "amount", amount);
    amount.textFromValueFunction = [](double v) { return juce::String(juce::roundToInt(v * 100)) + "%"; };
    amount.valueFromTextFunction = [](const juce::String& v) { return v.getDoubleValue() / 100.0; };
    amount.updateText();
    const char* ids[] { "seconds", "size", "tone", "width", "shape", "gap", "depth", "output" };
    const char* names[] { "Length", "Size", "Tone", "Width", "Shape", "Gap", "Depth", "Output" };
    const char* hints[] {
        "Set Length to Seconds to edit this knob. Bar divisions use FL's tempo.",
        "Small or spacious reverb", "Dark and muffled to bright and crisp", "Mono to a wider stereo field",
        "Natural at zero; increase to emphasize the end of the buildup",
        "Silence before the original. File exports include this silence at their end.",
        "How deeply the pulse lowers the volume", "Final output level"
    };
    const double defaults[] { 2.0, 0.72, 0.55, 1.0, 0.0, 0.0, 0.5, 0.0 };
    for (size_t i = 0; i < knobs.size(); ++i)
    {
        auto& knob = knobs[i]; addAndMakeVisible(knob); addAndMakeVisible(knobLabels[i]);
        labelStyle(knobLabels[i], 11.5f, muted); knobLabels[i].setText(names[i], juce::dontSendNotification);
        knobLabels[i].setJustificationType(juce::Justification::centred);
        knob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        knob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 90, 20);
        knob.setTooltip(hints[i]); knob.setMouseDragSensitivity(160);
        knobAttachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.parameters, ids[i], knob);
        knob.setDoubleClickReturnValue(true, defaults[i]);
        if (i == 0)
        {
            knob.textFromValueFunction = [this](double v) {
                return juce::String(processor.settings().durationMode == 0 ? v : rr::durationSeconds(processor.settings()), 2) + " s";
            };
            knob.valueFromTextFunction = [](const juce::String& v) { return v.getDoubleValue(); };
        }
        else if (i == 5)
        {
            knob.textFromValueFunction = [](double v) { return juce::String(juce::roundToInt(v * 1000)) + " ms"; };
            knob.valueFromTextFunction = [](const juce::String& v) { return v.getDoubleValue() / 1000.0; };
        }
        else if (i == 7)
        {
            knob.textFromValueFunction = [](double v) { return juce::String(v, 1) + " dB"; };
            knob.valueFromTextFunction = [](const juce::String& v) { return v.getDoubleValue(); };
        }
        else
        {
            knob.textFromValueFunction = [i](double v) {
                return i == 4 && v == 0 ? juce::String("Natural") : juce::String(juce::roundToInt(v * 100)) + "%";
            };
            knob.valueFromTextFunction = [](const juce::String& v) { return v.getDoubleValue() / 100.0; };
        }
        knob.updateText();
    }
    generate.setComponentID("generate"); drag.setComponentID("export");
    open.onClick = [this] { chooseAudio(); };
    waveform.onSelection = [this](int a, int b) { processor.setSelection(a, b); sync(); };
    graph.onShape = [this](float v) { processor.parameters.getParameter("shape")->setValueNotifyingHost(v); };
    graph.onGesture = [this](bool start) {
        auto* p = processor.parameters.getParameter("shape");
        if (start) p->beginChangeGesture(); else p->endChangeGesture();
    };
    graph.setTooltip("Drag up/down to change Shape. The curve shows the added volume envelope; Wave shows actual audio.");
    selectAll.onClick = [this] { const auto v = processor.view(); if (v.source) processor.setSelection(0, v.source->samples.getNumSamples()); sync(); };
    zoomIn.onClick = [this] { waveform.zoom(0.5); }; zoomOut.onClick = [this] { waveform.zoom(2); }; fit.onClick = [this] { waveform.showAll(); };
    original.onClick = [this] { processor.playOriginal(); };
    generate.onClick = [this] { if (processor.wantsTrack()) processor.serviceTrack(); else processor.generate(); sync(); };
    preview.onClick = [this] { if (previewMode.getSelectedId() == 2) processor.playCombined(); else processor.playEffect(); };
    stop.onClick = [this] { processor.stopPreview(); };
    save.onClick = [this] { chooseSave(); };
    curveTab.onClick = [this] { curveView = true; sync(); }; waveTab.onClick = [this] { curveView = false; sync(); };
    drag.onDrag = [this] {
        const auto v = processor.view();
        if (!processor.wantsTrack() && !processor.isTrackActive() && v.result && v.exportFile.existsAsFile())
            performExternalDragDropOfFiles({ v.exportFile.getFullPathName() }, false, &drag);
    };
    drag.setTooltip("Effect-only WAV. Place its END at the start of the original word. Gap silence is already included.");
    waveform.setTooltip("Drag to select; drag edges to adjust. Scroll to zoom. Double-click to select all.");
    manualBpm.setTooltip("Fallback tempo when the host does not supply one.");
    amount.setTooltip("Track effect level. Automate this in FL to fade the effect in and out; original level stays unchanged.");
    sync(); startTimerHz(20);
}

ReverseReverbEditor::~ReverseReverbEditor()
{ stopTimer(); processor.stopPreview(); setLookAndFeel(nullptr); }

void ReverseReverbEditor::paint(juce::Graphics& g)
{
    g.fillAll(background);
    g.setColour(text); g.setFont(juce::FontOptions(20));
    g.drawText("ReverseReverb", 14, 12, 166, 30, juce::Justification::centredLeft);
    g.setColour(accent); g.fillRect(14, 44, 26, 2);
    for (const auto box : { juce::Rectangle<float>(14, 54, 872, 112), juce::Rectangle<float>(14, 176, 872, 142) })
    {
        g.setColour(surface); g.fillRoundedRectangle(box, 5);
        g.setColour(border); g.drawRoundedRectangle(box.reduced(0.5f), 5, 1);
    }
    g.setColour(muted); g.setFont(juce::FontOptions(11.5f));
    g.drawText("CLASSIC", 24, 328, 100, 28, juce::Justification::centredLeft);
    g.drawText("Length unit", 130, 328, 80, 28, juce::Justification::centredLeft);
    g.drawText(processor.wantsTrack() ? "Buffered Track / beta" : "Effect-only export", 338, 328, 230, 28, juce::Justification::centredLeft);
    g.setColour(border); g.drawHorizontalLine(364, 14, 886); g.drawHorizontalLine(468, 14, 886);
    g.setColour(faint); g.setFont(juce::FontOptions(10.5f));
    g.drawText(JucePlugin_VersionString, 836, 14, 50, 24, juce::Justification::centredRight);
    if (processor.wantsTrack())
    {
        g.setColour(muted); g.setFont(juce::FontOptions(11.5f));
        g.drawText("Playback follows FL. Printing uses File mode.", 456, 478, 420, 34, juce::Justification::centredRight);
    }
}

void ReverseReverbEditor::resized()
{
    mode.setBounds(192, 14, 92, 28);
    bpmLabel.setBounds(476, 15, 50, 26); manualBpm.setBounds(528, 14, 91, 28);
    timingLabel.setBounds(630, 15, 190, 26);
    sourceName.setBounds(24, 57, 610, 22); open.setBounds(766, 58, 110, 22);
    waveform.setBounds(24, 82, 852, 58); inputScope.setBounds(waveform.getBounds());
    original.setBounds(24, 143, 78, 20); selectionInfo.setBounds(110, 141, 530, 24);
    selectAll.setBounds(666, 143, 76, 20); zoomOut.setBounds(750, 143, 28, 20);
    zoomIn.setBounds(782, 143, 28, 20); fit.setBounds(818, 143, 58, 20);
    effectInfo.setBounds(24, 180, 654, 24); curveTab.setBounds(716, 181, 76, 24); waveTab.setBounds(798, 181, 78, 24);
    graph.setBounds(24, 210, 852, 98); effectWaveform.setBounds(graph.getBounds()); outputScope.setBounds(graph.getBounds());
    duration.setBounds(212, 329, 110, 26); pulseLabel.setBounds(684, 329, 56, 26); tremolo.setBounds(744, 329, 132, 26);
    for (size_t i = 0; i < knobs.size(); ++i)
    {
        const int x = 18 + static_cast<int>(i) * 109;
        knobLabels[i].setBounds(x, 369, 100, 18); knobs[i].setBounds(x, 386, 100, 76);
    }
    generate.setBounds(14, 478, 104, 34); previewMode.setBounds(128, 478, 158, 34);
    preview.setBounds(296, 478, 60, 34); stop.setBounds(366, 478, 58, 34);
    save.setBounds(590, 478, 84, 34); drag.setBounds(684, 478, 202, 34);
    amountLabel.setBounds(128, 478, 62, 34); amount.setBounds(196, 480, 228, 30);
    statusLabel.setBounds(14, 523, 872, 24);
}

void ReverseReverbEditor::sync()
{
    const auto v = processor.view(); const auto s = processor.settings();
    const bool track = processor.wantsTrack(), transition = processor.isTrackActive() != track;
    const bool fileReady = !track && !transition && !v.busy;
    waveform.setAudio(v.source); waveform.setSelection(v.start, v.end);
    effectWaveform.setAudio(v.result); graph.setSettings(s);
    waveform.setVisible(!track); inputScope.setVisible(track);
    for (auto* c : std::initializer_list<juce::Component*> { &open, &original, &selectAll, &zoomIn, &zoomOut, &fit })
        c->setVisible(!track);
    graph.setVisible(curveView);
    effectWaveform.setVisible(!curveView && !track); outputScope.setVisible(!curveView && track);
    curveTab.setToggleState(curveView, juce::dontSendNotification); waveTab.setToggleState(!curveView, juce::dontSendNotification);
    sourceName.setText(track ? "Incoming mixer audio" : v.source ? v.source->name : "Source / drop a recording", juce::dontSendNotification);
    selectionInfo.setText(track ? "Use on one source mixer channel. FL may delay other tracks to keep sync."
        : v.source ? juce::String(v.start / v.source->sampleRate, 2) + " - " + juce::String(v.end / v.source->sampleRate, 2)
            + " s  /  " + juce::String((v.end - v.start) / v.source->sampleRate, 2) + " s selected"
        : "Select a word or sound after loading", juce::dontSendNotification);
    effectInfo.setText(track ? "REVERSE / " + juce::String(1000.0 * processor.getLatencySamples() / processor.processingRate(), 0) + " ms reported delay"
        : "REVERSE / " + juce::String(rr::durationSeconds(s), 2) + " s swell + " + juce::String(s.gapSeconds * 1000, 0) + " ms gap",
        juce::dontSendNotification);
    timingLabel.setText(juce::String(s.bpm, 1) + " BPM / " + juce::String(s.numerator) + "/" + juce::String(s.denominator)
        + (processor.hasHostTempo() ? " / synced" : " / manual"), juce::dontSendNotification);
    statusLabel.setText(track || processor.isTrackActive() ? processor.trackStatus() : v.status, juce::dontSendNotification);
    statusLabel.setTooltip(statusLabel.getText());
    open.setButtonText(v.source ? "Replace audio" : "Open audio");
    open.setEnabled(fileReady); waveform.setEnabled(fileReady);
    original.setEnabled(fileReady && v.source != nullptr); selectAll.setEnabled(fileReady && v.source != nullptr);
    zoomIn.setEnabled(fileReady && v.source != nullptr); zoomOut.setEnabled(fileReady && v.source != nullptr); fit.setEnabled(fileReady && v.source != nullptr);
    generate.setVisible(!track);
    generate.setButtonText(v.busy ? "Working..." : v.result ? "Generate again" : "Generate");
    generate.setEnabled(fileReady && v.source != nullptr);
    for (auto* c : std::initializer_list<juce::Component*> { &previewMode, &preview, &stop, &save, &drag }) c->setVisible(!track);
    previewMode.setEnabled(fileReady && v.result != nullptr); preview.setEnabled(fileReady && v.result != nullptr);
    stop.setEnabled(processor.isPreviewPlaying()); save.setEnabled(fileReady && v.result != nullptr);
    drag.setEnabled(fileReady && v.result != nullptr && v.exportFile.existsAsFile());
    amount.setVisible(track); amountLabel.setVisible(track);
    manualBpm.setEnabled(!processor.hasHostTempo()); bpmLabel.setAlpha(processor.hasHostTempo() ? 0.45f : 1.0f);
    knobs[0].setEnabled(s.durationMode == 0); knobs[0].updateText();
    knobs[6].setEnabled(s.tremolo != 0); knobLabels[6].setAlpha(s.tremolo != 0 ? 1.0f : 0.4f);
    graph.setEnabled(!v.busy || track);
    const double time = processor.isPreviewPlaying() ? processor.previewSeconds() : -1;
    const int kind = processor.previewKind();
    graph.setPlayhead(kind >= 2 ? time : -1);
    effectWaveform.setPlayhead(v.result && kind >= 2 && time >= 0 ? time * v.result->sampleRate : -1);
    double originalPosition = -1;
    if (time >= 0 && v.source && kind == 1) originalPosition = v.start + time * v.source->sampleRate;
    if (time >= 0 && v.source && v.result && kind == 3)
    {
        const auto offset = v.result->samples.getNumSamples() / v.result->sampleRate;
        if (time >= offset) originalPosition = v.start + (time - offset) * v.source->sampleRate;
    }
    waveform.setPlayhead(originalPosition);
    if (track)
    {
        inputScope.bins = processor.scope(false); outputScope.bins = processor.scope(true);
        inputScope.repaint(); outputScope.repaint();
    }
    repaint();
}
void ReverseReverbEditor::timerCallback() { processor.collectRetiredPreviews(); sync(); }

bool ReverseReverbEditor::isInterestedInFileDrag(const juce::StringArray& files)
{
    return !processor.wantsTrack() && !processor.isTrackActive() && !processor.view().busy && files.size() == 1
        && juce::File(files[0]).hasFileExtension("wav;aif;aiff;flac;ogg;mp3");
}
void ReverseReverbEditor::fileDragEnter(const juce::StringArray& files, int, int)
{ waveform.setDropHighlight(isInterestedInFileDrag(files)); }
void ReverseReverbEditor::fileDragExit(const juce::StringArray&) { waveform.setDropHighlight(false); }
void ReverseReverbEditor::filesDropped(const juce::StringArray& files, int, int)
{
    waveform.setDropHighlight(false);
    if (isInterestedInFileDrag(files)) { processor.loadFile(juce::File(files[0])); sync(); }
}
void ReverseReverbEditor::chooseAudio()
{
    chooser = std::make_unique<juce::FileChooser>("Choose a recording", juce::File(), "*.wav;*.aif;*.aiff;*.flac;*.ogg;*.mp3");
    chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [safe = juce::Component::SafePointer<ReverseReverbEditor>(this)](const juce::FileChooser& c)
        { if (safe && c.getResult().existsAsFile()) { safe->processor.loadFile(c.getResult()); safe->sync(); } });
}
void ReverseReverbEditor::chooseSave()
{
    chooser = std::make_unique<juce::FileChooser>("Save effect as a new WAV file",
        juce::File(RR_EXPORT_DIRECTORY).getChildFile("My reverse reverb.wav"), "*.wav");
    chooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
        [safe = juce::Component::SafePointer<ReverseReverbEditor>(this)](const juce::FileChooser& c)
        {
            if (!safe || c.getResult() == juce::File()) return;
            safe->processor.saveCopy(c.getResult().withFileExtension("wav"));
        });
}

