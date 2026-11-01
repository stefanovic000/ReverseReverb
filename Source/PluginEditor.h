#pragma once
#include "PluginProcessor.h"
#include "Waveform.h"
#include "DarkTheme.h"
#include "ShapeGraph.h"

class ReverseReverbEditor final : public juce::AudioProcessorEditor,
                                  private juce::Timer,
                                  public juce::FileDragAndDropTarget,
                                  public juce::DragAndDropContainer
{
public:
    explicit ReverseReverbEditor(ReverseReverbProcessor&);
    ~ReverseReverbEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    bool isInterestedInFileDrag(const juce::StringArray&) override;
    void fileDragEnter(const juce::StringArray&, int, int) override;
    void fileDragExit(const juce::StringArray&) override;
    void filesDropped(const juce::StringArray&, int, int) override;
private:
    rr::ui::DarkTheme theme;
    class DragButton final : public juce::TextButton
    {
    public:
        DragButton() : TextButton("Drag WAV to FL") {}
        std::function<void()> onDrag;
        void mouseDown(const juce::MouseEvent& e) override { began = false; TextButton::mouseDown(e); }
        void mouseDrag(const juce::MouseEvent& e) override
        { if (!began && e.getDistanceFromDragStart() > 5 && isEnabled()) { began = true; if (onDrag) onDrag(); } }
    private:
        bool began = false;
    } drag;
    struct Scope final : juce::Component
    {
        std::array<float, 128> bins {};
        void paint(juce::Graphics&) override;
    } inputScope, outputScope;
    void timerCallback() override;
    void chooseAudio();
    void chooseSave();
    void sync();
    ReverseReverbProcessor& processor;
    Waveform waveform, effectWaveform { false };
    ShapeGraph graph;
    juce::TextButton open { "Open audio" }, selectAll { "Select all" }, zoomIn { "+" }, zoomOut { "-" }, fit { "Fit" };
    juce::TextButton original { "Original" }, generate { "Generate" }, preview { "Play" }, stop { "Stop" }, save { "Save WAV" };
    juce::TextButton curveTab { "Curve" }, waveTab { "Wave" };
    juce::Label sourceName, selectionInfo, statusLabel, timingLabel, bpmLabel, effectInfo, pulseLabel, amountLabel;
    juce::ComboBox duration, tremolo, mode, previewMode;
    juce::Slider manualBpm, amount;
    std::array<juce::Slider, 8> knobs;
    std::array<juce::Label, 8> knobLabels;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 8> knobAttachments;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> bpmAttachment, amountAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> durationAttachment, tremoloAttachment, modeAttachment;
    std::unique_ptr<juce::FileChooser> chooser;
    juce::TooltipWindow tooltips { this, 500 };
    bool curveView = true;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ReverseReverbEditor)
};

