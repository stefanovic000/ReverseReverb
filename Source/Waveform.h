#pragma once
#include "AudioEngine.h"
#include <juce_gui_basics/juce_gui_basics.h>

class Waveform final : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit Waveform(bool selectable = true) : editable(selectable) {}
    void setAudio(std::shared_ptr<const rr::Audio>);
    void setSelection(int start, int end);
    void zoom(double factor);
    void showAll();
    void setDropHighlight(bool highlighted) { dropHighlight = highlighted; repaint(); }
    void setPlayhead(double sample) { playhead = sample; repaint(); }
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    std::function<void(int, int)> onSelection;
private:
    double sampleAt(float x) const;
    float xAt(double sample) const;
    void updateDrag(float x);
    std::shared_ptr<const rr::Audio> audio;
    struct Peak { float low = 0, high = 0; };
    std::vector<Peak> peaks;
    static constexpr int groupSize = 256;
    double viewStart = 0, viewEnd = 1;
    int start = 0, end = 0, anchor = 0, dragMode = 0;
    bool editable = true;
    bool dropHighlight = false;
    float displayScale = 1.0f;
    double playhead = -1;
};
