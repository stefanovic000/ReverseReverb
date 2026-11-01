#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace rr::ui
{
inline const juce::Colour background(0xff141414), surface(0xff1d1d1d), field(0xff181818);
inline const juce::Colour text(0xffe8e8e8), muted(0xffa4a4a4), faint(0xff858585);
inline const juce::Colour accent(0xffd05252), border(0xff343434), track(0xff404040);

class DarkTheme final : public juce::LookAndFeel_V4
{
public:
    DarkTheme();
    void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
    void drawButtonText(juce::Graphics&, juce::TextButton&, bool, bool) override;
    void drawComboBox(juce::Graphics&, int, int, bool, int, int, int, int, juce::ComboBox&) override;
    juce::Font getComboBoxFont(juce::ComboBox&) override;
    void positionComboBoxText(juce::ComboBox&, juce::Label&) override;
    void drawLinearSlider(juce::Graphics&, int, int, int, int, float, float, float,
                          const juce::Slider::SliderStyle, juce::Slider&) override;
    void drawRotarySlider(juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override;
};
}
