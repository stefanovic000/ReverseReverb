# Audio processing plan

This describes our proposed implementation, not Backverb's undocumented internal code.

Version 0.2 adds Shape, trailing Gap silence, combined audition, and a separate Track engine. See [ROADMAP.md](ROADMAP.md) for stages and [REVERB_CHARACTERS.md](REVERB_CHARACTERS.md) for future character assets. The core File algorithm below remains the foundation.

## Basic idea

Sound is a list of numbers called samples. Stereo audio has separate lists for the left and right speakers. The sample rate says how many numbers are played each second.

Keep three separate things: the untouched recording, the selected piece, and the generated effect.

## Load and select

Decode the file into sound numbers. Convert selection times into sample positions using the source sample rate. Clamp positions to the recording's bounds and require a nonempty selection.

For the waveform, divide the recording into small sections and draw each section's lowest and highest values. Reuse this drawing data when zooming where practical.

## Reverse reverb

One standard method is to reverse the source, apply fully wet reverb, and reverse the rendered output. Fully wet means keeping the reverb without mixing in the direct source.

That full result can include both a lead-in and reverberation overlapping the source. Our proposed default export keeps the lead-in only, so its end can align with the original sound's start.

For a selected source of S samples and a requested lead-in of L samples:

1. Make a reversed copy of the S source samples.
2. Reset the reverb so no previous recording remains in it.
3. Process the reversed copy through the reverb, keeping only wet output.
4. Continue processing L samples of silence to collect the reverb after the reversed source ends.
5. Reverse that collected tail to create the L-sample lead-in.
6. Apply shaping, output gain, and brief edge fades.

This is equivalent to taking the pre-source portion of the full reversed wet render. Listen to the result and refine the reverb settings: a mathematically valid result still needs to sound useful.

The normal original voice is never mixed into this default output. Retain it separately for comparison or a future combined audition.

## Duration

Convert the chosen duration into seconds, then into processing samples.

- Seconds = number of quarter-note beats × 60 / BPM.
- One bar contains numerator × 4 / denominator quarter-note beats.
- Samples = seconds × processing sample rate, rounded to a whole sample.

Example: at 120 BPM in 4/4 time, one bar lasts two seconds.

Use host tempo and time signature when available. Allow manual values when they are unavailable. Changing duration rerenders from the original selection; it does not repeatedly stretch an already processed file.

## Sound controls

| Control | Basic operation |
|---|---|
| Size | Change the reverb's decay and/or delay structure, depending on the chosen engine. |
| Tone | Use a filter to reduce or retain high frequencies. |
| Width | Split stereo into shared sound (mid) and differences (side); adjust the side amount. |
| Tremolo rate | Use a repeating smooth curve to change volume at a chosen musical rate. |
| Tremolo depth | Control how far that volume falls during each pulse. Zero means no pulsing. |
| Output | Multiply samples by a gain value to change loudness. |

Width alone cannot turn two identical channels into a rich stereo recording. The reverb should create a stereo field first.

Tremolo changes the existing sound's volume; it does not add another sound. Define its starting phase consistently so repeated renders are predictable.

Suggested order: reverse reverb generation → tone → width → tremolo → gain → edge fades.

## Playback and sample rates

Read original or processed audio with a playback cursor. Convert sample rates when the recording and FL's output use different rates, so speed and pitch remain correct.

Keep the source path through the mixer unchanged. Preview is a separate playback source. Verify preview while the host transport is stopped, since hosts may suspend some plugin processing.

## Background work

Rendering and WAV writing happen outside FL's time-critical audio callback. The callback handles pass-through and playback using prepared buffers.

Each render gets a source-and-settings snapshot and a version number. Only publish a result if it still belongs to the latest request. Hand completed buffers to playback safely; never modify a buffer while it is being played.

## Edges, levels, and export

Apply short fades at boundaries to avoid sudden jumps that cause clicks. Reject invalid values and silence rather than producing broken output. Avoid unexpectedly excessive levels; do not silently change loudness with automatic normalization unless that becomes an explicit feature.

Write the processed buffer as a WAV with the correct channel count and sample rate. Use a unique persistent export file for each result. Dragging passes that file to FL; saving offers a user-chosen destination.

## Saving with the FL project

Save settings, selection, and source audio with a size limit. Regenerate the effect on restore. Do not rely solely on a temporary source path that may disappear later.

## Processing of playing audio — 0.2 beta

[TRACK_MODE.md](TRACK_MODE.md) describes the separate streaming engine for mixer input. It uses a finite reversed Classic impulse response through JUCE convolution, a buffered original path, measured/reported latency, and transport recovery. Its continuous reverb can overlap successive sounds, unlike the isolated lead-in extracted by the File renderer.

File rendering stays on the background worker. Track DSP must be prepared in advance and run within the audio callback's time budget, without file operations or allocation. Amount automation and mode switching need timing tests in FL.

## Shape, Gap, and combined preview — 0.2

Shape zero adds no envelope. Positive Shape multiplies the audible effect by `u^(4 * shape)`, where `u` runs from zero to one across the lead-in. It attenuates the early part without boosting the result. Edge fades remain in place.

Append Gap silence after all audible-effect processing, so effect-only export lasts Length + Gap. Combined preview appends only the selected original after that buffer, with short fades on its boundaries. The exported WAV stays effect-only regardless of preview choice.
