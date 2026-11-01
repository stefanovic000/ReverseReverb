#include "DarkTheme.h"

namespace rr::ui
{
DarkTheme::DarkTheme()
{
    setColour(juce::ResizableWindow::backgroundColourId, background);
    setColour(juce::TextButton::buttonColourId, field);
    setColour(juce::TextButton::textColourOffId, text);
    setColour(juce::ComboBox::backgroundColourId, field);
    setColour(juce::ComboBox::outlineColourId, border);
    setColour(juce::ComboBox::textColourId, text);
    setColour(juce::PopupMenu::backgroundColourId, surface);
    setColour(juce::PopupMenu::textColourId, text);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, track);
    setColour(juce::PopupMenu::highlightedTextColourId, text);
    setColour(juce::Slider::textBoxTextColourId, text);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxHighlightColourId, accent.withAlpha(0.2f));
    setColour(juce::TextEditor::backgroundColourId, field);
    setColour(juce::TextEditor::textColourId, text);
    setColour(juce::TextEditor::highlightColourId, accent.withAlpha(0.2f));
    setColour(juce::TextEditor::outlineColourId, border);
    setColour(juce::TextEditor::focusedOutlineColourId, accent);
    setColour(juce::TooltipWindow::backgroundColourId, surface);
    setColour(juce::TooltipWindow::textColourId, text);
    setColour(juce::TooltipWindow::outlineColourId, border);
}

void DarkTheme::drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour&,
                                     bool hovered, bool pressed)
{
    const bool primary = button.getComponentID() == "generate";
    const bool exportButton = button.getComponentID() == "export";
    auto fill = primary && button.isEnabled() ? accent.darker(0.13f) : exportButton ? surface : field;
    if (button.isEnabled() && hovered) fill = fill.brighter(pressed ? 0.04f : 0.08f);
    const auto box = button.getLocalBounds().toFloat().reduced(0.5f);
    g.setColour(fill); g.fillRoundedRectangle(box, exportButton ? 9.0f : 5.0f);
    g.setColour(button.hasKeyboardFocus(true) || (exportButton && button.isEnabled() && hovered)
        ? accent : border.withAlpha(button.isEnabled() ? 1.0f : 0.55f));
    g.drawRoundedRectangle(box, 4.0f, 1.0f);
    if (button.getToggleState())
    { g.setColour(accent); g.fillRect(8, button.getHeight() - 2, button.getWidth() - 16, 2); }
}

void DarkTheme::drawButtonText(juce::Graphics& g, juce::TextButton& button, bool, bool)
{
    const bool primary = button.getComponentID() == "generate";
    const bool exportButton = button.getComponentID() == "export";
    const auto colour = text.withAlpha(button.isEnabled() ? 1.0f : 0.32f);
    g.setColour(colour);
    if (exportButton)
    {
        const float y = button.getHeight() * 0.5f;
        juce::Path icon;
        icon.startNewSubPath(13, y + 6); icon.lineTo(13, y - 6); icon.lineTo(25, y - 6);
        icon.startNewSubPath(16, y + 3); icon.lineTo(26, y - 7);
        icon.startNewSubPath(19, y - 7); icon.lineTo(26, y - 7); icon.lineTo(26, y);
        g.strokePath(icon, juce::PathStrokeType(1.5f));
        g.setFont(juce::FontOptions(12.0f));
        g.drawText("Drag WAV to FL", 38, 0, button.getWidth() - 45, button.getHeight(), juce::Justification::centredLeft);
        return;
    }
    g.setFont(juce::FontOptions(button.getHeight() < 30 ? 12.0f : 13.0f));
    g.drawText(button.getButtonText(), button.getLocalBounds().reduced(4, 0), juce::Justification::centred);
}

void DarkTheme::drawComboBox(juce::Graphics& g, int width, int height, bool, int, int, int, int, juce::ComboBox& combo)
{
    const auto box = juce::Rectangle<float>(0.5f, 0.5f, width - 1.0f, height - 1.0f);
    g.setColour(field); g.fillRoundedRectangle(box, 5);
    g.setColour(combo.hasKeyboardFocus(true) ? accent : border); g.drawRoundedRectangle(box, 5, 1);
    juce::Path arrow;
    arrow.startNewSubPath(width - 22.0f, height * 0.5f - 2);
    arrow.lineTo(width - 18.0f, height * 0.5f + 2); arrow.lineTo(width - 14.0f, height * 0.5f - 2);
    g.setColour(muted.withAlpha(combo.isEnabled() ? 1.0f : 0.3f)); g.strokePath(arrow, juce::PathStrokeType(1.2f));
}
juce::Font DarkTheme::getComboBoxFont(juce::ComboBox&) { return juce::Font(juce::FontOptions(13.0f)); }
void DarkTheme::positionComboBoxText(juce::ComboBox& combo, juce::Label& label)
{ label.setBounds(12, 1, combo.getWidth() - 42, combo.getHeight() - 2); label.setFont(getComboBoxFont(combo)); }

void DarkTheme::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                                 float position, float, float, const juce::Slider::SliderStyle, juce::Slider& slider)
{
    const float left = static_cast<float>(x), right = static_cast<float>(x + width), centre = y + height * 0.5f;
    g.setColour(track.withAlpha(slider.isEnabled() ? 1.0f : 0.4f));
    g.fillRoundedRectangle(left, centre - 1.5f, static_cast<float>(width), 3, 1.5f);
    const auto colour = accent.withAlpha(slider.isEnabled() ? 1.0f : 0.25f);
    g.setColour(colour);
    g.fillRoundedRectangle(left, centre - 1.5f, juce::jlimit(0.0f, static_cast<float>(width), position - left), 3, 1.5f);
    g.fillEllipse(juce::jlimit(left, right, position) - 4, centre - 4, 8, 8);
}

void DarkTheme::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                float position, float start, float end, juce::Slider& slider)
{
    const auto bounds = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y), static_cast<float>(width), static_cast<float>(height)).reduced(6);
    const auto centre = bounds.getCentre();
    const float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;
    juce::Path base, active;
    base.addCentredArc(centre.x, centre.y, radius, radius, 0, start, end, true);
    active.addCentredArc(centre.x, centre.y, radius, radius, 0, start, start + position * (end - start), true);
    g.setColour(track.withAlpha(slider.isEnabled() ? 1.0f : 0.35f)); g.strokePath(base, juce::PathStrokeType(2.5f));
    g.setColour(accent.withAlpha(slider.isEnabled() ? 1.0f : 0.2f)); g.strokePath(active, juce::PathStrokeType(2.5f));
    g.setColour(field); g.fillEllipse(centre.x - radius + 5, centre.y - radius + 5, (radius - 5) * 2, (radius - 5) * 2);
    const float angle = start + position * (end - start);
    g.setColour(text.withAlpha(slider.isEnabled() ? 1.0f : 0.3f));
    g.drawLine(centre.x + std::sin(angle) * radius * 0.35f, centre.y - std::cos(angle) * radius * 0.35f,
        centre.x + std::sin(angle) * radius * 0.7f, centre.y - std::cos(angle) * radius * 0.7f, 1.5f);
}
}
