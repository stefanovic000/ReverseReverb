#pragma once
#include "AudioEngine.h"
#include <juce_gui_basics/juce_gui_basics.h>

class ShapeGraph final : public juce::Component, public juce::SettableTooltipClient
{
public:
    void setSettings(const rr::Settings& next) { settings = next; repaint(); }
    void setPlayhead(double seconds) { playhead = seconds; repaint(); }
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    std::function<void(float)> onShape;
    std::function<void(bool)> onGesture;
private:
    rr::Settings settings;
    double playhead = -1;
    float dragStart = 0;
};
