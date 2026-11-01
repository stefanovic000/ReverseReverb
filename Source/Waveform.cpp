#include "Waveform.h"
#include "DarkTheme.h"
#include <cmath>

namespace
{
using namespace rr::ui;
}

void Waveform::setAudio(std::shared_ptr<const rr::Audio> next)
{
    if (audio == next) return;
    audio = std::move(next); peaks.clear(); displayScale = 1.0f;
    if (audio)
    {
        const auto& buffer = audio->samples;
        const int count = buffer.getNumSamples();
        peaks.resize(static_cast<size_t>((count + groupSize - 1) / groupSize));
        for (size_t i = 0; i < peaks.size(); ++i)
        {
            auto& p = peaks[i];
            for (int c = 0; c < buffer.getNumChannels(); ++c)
            {
                const auto range = buffer.findMinMax(c, static_cast<int>(i) * groupSize,
                    juce::jmin(groupSize, count - static_cast<int>(i) * groupSize));
                p.low = juce::jmin(p.low, range.getStart()); p.high = juce::jmax(p.high, range.getEnd());
            }
        }
        start = 0; end = count;
        float largest = 0.0f;
        for (const auto& p : peaks) largest = juce::jmax(largest, std::abs(p.low), std::abs(p.high));
        // Fit the drawing vertically; this does not change playback or export volume.
        displayScale = 0.9f / juce::jmax(0.0001f, largest);
    }
    showAll();
}

void Waveform::setSelection(int first, int last)
{ if (start != first || end != last) { start = first; end = last; repaint(); } }

void Waveform::showAll()
{ viewStart = 0; viewEnd = audio ? audio->samples.getNumSamples() : 1; repaint(); }

void Waveform::zoom(double factor)
{
    if (!audio) return;
    const double count = audio->samples.getNumSamples();
    const double width = juce::jlimit(juce::jmin(count, audio->sampleRate * 0.02), count, (viewEnd - viewStart) * factor);
    const double centre = editable ? (start + end) * 0.5 : (viewStart + viewEnd) * 0.5;
    viewStart = juce::jlimit(0.0, count - width, centre - width * 0.5); viewEnd = viewStart + width;
    repaint();
}

double Waveform::sampleAt(float x) const
{ return viewStart + juce::jlimit(0.0, 1.0, (x - 16.0) / juce::jmax(1, getWidth() - 32)) * (viewEnd - viewStart); }
float Waveform::xAt(double sample) const
{ return 16.0f + static_cast<float>((sample - viewStart) / juce::jmax(1.0, viewEnd - viewStart)) * (getWidth() - 32.0f); }

void Waveform::paint(juce::Graphics& g)
{
    const auto box = getLocalBounds().toFloat();
    g.setColour(field); g.fillRoundedRectangle(box, 5.0f);
    g.setColour(dropHighlight ? accent : border.withAlpha(0.7f));
    g.drawRoundedRectangle(box.reduced(0.5f), 5.0f, 1.0f);
    if (!audio)
    {
        g.setColour(editable ? text : faint);
        g.setFont(juce::FontOptions(editable ? 16.0f : 12.0f));
        const auto titleArea = editable ? getLocalBounds().translated(0, -10) : getLocalBounds();
        g.drawText(editable ? "Drop a recording here" : "Your lead-in will appear here", titleArea, juce::Justification::centred);
        if (editable)
        {
            g.setColour(faint); g.setFont(juce::FontOptions(11.0f));
            g.drawText("WAV, AIFF, FLAC or MP3", 0, getHeight() / 2 + 9, getWidth(), 20, juce::Justification::centred);
        }
        return;
    }
    const float plotTop = editable ? 12.0f : 8.0f;
    const float plotBottom = getHeight() - 28.0f;
    const float middle = (plotTop + plotBottom) * 0.5f;
    const float height = juce::jmax(4.0f, (plotBottom - plotTop) * 0.5f);
    const float selectionLeft = juce::jlimit(16.0f, getWidth() - 16.0f, xAt(start));
    const float selectionRight = juce::jlimit(16.0f, getWidth() - 16.0f, xAt(end));
    if (editable)
    {
        g.setColour(accent.withAlpha(0.08f));
        g.fillRect(juce::Rectangle<float>(selectionLeft, 12.0f, selectionRight - selectionLeft, getHeight() - 40.0f));
    }
    g.setColour(border); g.drawHorizontalLine(static_cast<int>(middle), 16, getWidth() - 16.0f);
    for (int x = 16; x < getWidth() - 16; ++x)
    {
        const int a = juce::jlimit(0, static_cast<int>(peaks.size()) - 1, static_cast<int>(sampleAt(static_cast<float>(x))) / groupSize);
        const int b = juce::jlimit(a + 1, static_cast<int>(peaks.size()), static_cast<int>(sampleAt(static_cast<float>(x + 1))) / groupSize + 1);
        float low = 0, high = 0;
        // At high zoom use exact samples, not a coarse cached peak.
        if ((viewEnd - viewStart) / juce::jmax(1, getWidth() - 32) < groupSize)
        {
            const int first = juce::jlimit(0, audio->samples.getNumSamples() - 1, static_cast<int>(sampleAt(static_cast<float>(x))));
            const int last = juce::jlimit(first + 1, audio->samples.getNumSamples(), static_cast<int>(sampleAt(static_cast<float>(x + 1))) + 1);
            for (int c = 0; c < audio->samples.getNumChannels(); ++c)
            {
                const auto range = audio->samples.findMinMax(c, first, last - first);
                low = juce::jmin(low, range.getStart()); high = juce::jmax(high, range.getEnd());
            }
        }
        else for (int i = a; i < b; ++i)
        { low = juce::jmin(low, peaks[static_cast<size_t>(i)].low); high = juce::jmax(high, peaks[static_cast<size_t>(i)].high); }
        g.setColour(!editable || (x >= selectionLeft && x <= selectionRight) ? accent : faint);
        g.drawVerticalLine(x, middle - juce::jmin(1.0f, high * displayScale) * height,
                          middle - juce::jmax(-1.0f, low * displayScale) * height + 1);
    }
    if (editable)
    {
        for (const auto position : { start, end })
        {
            const float x = xAt(position);
            if (x < 15 || x > getWidth() - 15) continue;
            g.setColour(accent); g.drawVerticalLine(static_cast<int>(x), 12, getHeight() - 28.0f);
            g.fillRoundedRectangle(x - 2, 12, 4, 12, 1);
        }
    }
    g.setColour(faint); g.setFont(juce::FontOptions(10.0f));
    for (int i = 0; i <= 4; ++i)
    {
        const double position = viewStart + (viewEnd - viewStart) * i / 4.0;
        const int x = static_cast<int>(16 + (getWidth() - 90) * i / 4.0);
        g.drawText(juce::String(position / audio->sampleRate, 2) + " s", x, getHeight() - 25, 74, 18, juce::Justification::centredLeft);
    }
    if (playhead >= viewStart && playhead <= viewEnd)
    { g.setColour(text.withAlpha(0.7f)); g.drawVerticalLine(static_cast<int>(xAt(playhead)), 8, getHeight() - 24.0f); }
}

void Waveform::mouseDown(const juce::MouseEvent& e)
{
    if (!editable || !audio) return;
    if (e.getNumberOfClicks() > 1)
    { start = 0; end = audio->samples.getNumSamples(); if (onSelection) onSelection(start, end); repaint(); return; }
    if (std::abs(e.position.x - xAt(start)) < 9) dragMode = 1;
    else if (std::abs(e.position.x - xAt(end)) < 9) dragMode = 2;
    else { dragMode = 3; anchor = static_cast<int>(sampleAt(e.position.x)); updateDrag(e.position.x); }
}
void Waveform::updateDrag(float x)
{
    if (!audio || dragMode == 0) return;
    const int count = audio->samples.getNumSamples();
    const int position = juce::jlimit(0, count, static_cast<int>(sampleAt(x)));
    if (dragMode == 1) start = juce::jlimit(0, end - 1, position);
    else if (dragMode == 2) end = juce::jlimit(start + 1, count, position);
    else
    {
        start = juce::jlimit(0, count - 1, juce::jmin(anchor, position));
        end = juce::jlimit(start + 1, count, juce::jmax(anchor, position));
    }
    if (onSelection) onSelection(start, end);
    repaint();
}
void Waveform::mouseDrag(const juce::MouseEvent& e) { updateDrag(e.position.x); }
void Waveform::mouseUp(const juce::MouseEvent&) { dragMode = 0; }
void Waveform::mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& wheel)
{ if (editable && wheel.deltaY != 0) zoom(wheel.deltaY > 0 ? 0.8 : 1.25); }
