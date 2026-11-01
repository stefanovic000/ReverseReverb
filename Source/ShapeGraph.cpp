#include "ShapeGraph.h"
#include "DarkTheme.h"

void ShapeGraph::paint(juce::Graphics& g)
{
    using namespace rr::ui;
    g.setColour(field); g.fillRoundedRectangle(getLocalBounds().toFloat(), 4);
    const float left = 12, right = getWidth() - 18.0f, top = 14, bottom = getHeight() - 24.0f;
    const auto length = rr::durationSeconds(settings), gap = juce::jlimit(0.0, 2.0, settings.gapSeconds);
    const float swellEnd = left + static_cast<float>(length / (length + gap)) * (right - left);
    g.setColour(border); g.drawHorizontalLine(static_cast<int>(bottom), left, right);
    if (gap > 0)
    {
        g.setColour(track.withAlpha(0.25f)); g.fillRect(juce::Rectangle<float>(swellEnd, top, right - swellEnd, bottom - top));
        if (right - swellEnd > 32)
        { g.setColour(muted); g.setFont(juce::FontOptions(10)); g.drawText("GAP", static_cast<int>(swellEnd), 1, static_cast<int>(right - swellEnd), 14, juce::Justification::centred); }
    }
    juce::Path curve;
    for (int n = 0; n <= 200; ++n)
    {
        const double u = n / 200.0;
        const float x = left + (swellEnd - left) * static_cast<float>(u);
        const float y = bottom - rr::shapeGain(u, settings.shape) * (bottom - top);
        if (n == 0) curve.startNewSubPath(x, y); else curve.lineTo(x, y);
    }
    auto fill = curve;
    fill.lineTo(swellEnd, bottom); fill.lineTo(left, bottom); fill.closeSubPath();
    g.setColour(accent.withAlpha(0.07f)); g.fillPath(fill);
    g.setColour(accent); g.strokePath(curve, juce::PathStrokeType(1.5f));
    const float handleX = left + (swellEnd - left) * 0.5f;
    const float handleY = bottom - rr::shapeGain(0.5, settings.shape) * (bottom - top);
    g.setColour(field); g.fillEllipse(handleX - 4, handleY - 4, 8, 8);
    g.setColour(accent); g.drawEllipse(handleX - 4, handleY - 4, 8, 8, 1.5f);
    g.setColour(muted); g.drawVerticalLine(static_cast<int>(right), top, bottom);
    g.setFont(juce::FontOptions(10));
    g.drawText("-" + juce::String(length + gap, 2) + " s", 12, getHeight() - 20, 90, 16, juce::Justification::centredLeft);
    g.drawText("ORIGINAL", getWidth() - 106, getHeight() - 20, 90, 16, juce::Justification::centredRight);
    g.setColour(faint);
    g.drawText(settings.shape == 0 ? "Natural / no added envelope" : "Applied volume envelope", 110, getHeight() - 20, 290, 16, juce::Justification::centredLeft);
    if (playhead >= 0)
    {
        const float x = left + static_cast<float>(juce::jlimit(0.0, 1.0, playhead / (length + gap))) * (right - left);
        g.setColour(text.withAlpha(0.65f)); g.drawVerticalLine(static_cast<int>(x), top, bottom);
    }
}
void ShapeGraph::mouseDown(const juce::MouseEvent&)
{ dragStart = settings.shape; if (onGesture) onGesture(true); }
void ShapeGraph::mouseDrag(const juce::MouseEvent& e)
{
    const float value = juce::jlimit(0.0f, 1.0f, dragStart + e.getDistanceFromDragStartY() / 100.0f);
    settings.shape = value; if (onShape) onShape(value); repaint();
}
void ShapeGraph::mouseUp(const juce::MouseEvent&)
{ if (onGesture) onGesture(false); }
