# ReverseReverb: next-version plan

Status: version 0.2 implements the compact gray/red interface, Shape, Gap, combined preview, and a buffered Track-mode beta. New reverb characters and the other later stages remain planned. Track's real FL timing/listening verification is still open.

This is the main plan for the recommendations discussed after the first build. Read [UI_PLAN.md](UI_PLAN.md) for the compact interface and [REVERB_CHARACTERS.md](REVERB_CHARACTERS.md) for the reverb sounds.

[TRACK_MODE.md](TRACK_MODE.md) specifies the planned File / Track switch for processing audio playing through FL, including latency, automation, and transport behavior.

## User requirements

- A slick, minimal interface, including a smaller window.
- Gray surfaces with restrained red accents.
- Less empty space and compact knobs instead of the large slider column.
- An interactive Shape graph and useful sound visualizations.
- Our own layout and appearance.
- Keep drag-and-drop as the main input method.
- Add a planned Track mode for playing mixer audio alongside the existing File workflow. This requires a separate processing stage.

## Current working foundation

We already have file loading, waveform selection and zoom, one reverb engine, separate original/effect playback, duration, Size, Tone, Width, pulse rate/depth, Output, WAV export, and saved source/selection/settings.

The original build passed 39 checks. Version 0.2 adds focused processing and compatibility checks. It preserves plugin identity, existing parameters, source files, saved projects, and effect-only export. See README.md for current behavior.

## Recommended features

| Priority | Feature | Why it helps |
|---|---|---|
| First | Shape | Control whether the buildup happens gradually or mostly near the end. |
| First | Gap | Leave breathing room before the original word or hit. |
| First | Effect + original preview | Hear the complete transition before placing anything in FL. |
| First | Compact gray/red UI | Make the tool smaller and easier to use while arranging a song. |
| Next | Reverb characters | Choose genuinely different echo textures. Start with Room, Hall, and Plate. |
| Next | Presets | Save useful settings and recall them quickly. |
| Next | A/B comparison | Compare two ideas using the same source and selection. |
| Later | Pitch | Make the effect higher or lower without changing the chosen duration. |
| Later | Formant | Change its voice-like character independently of its musical note. |
| Later | Capture | Load sound by recording the mixer input, including FL's upstream effects. |
| Later experiment | Automatic generation | Update the result after a short pause in editing, with cancellation of old renders. |
| Separate planned stage | File / Track modes | Process playing mixer audio with reported latency and Effect Amount automation; retain precise File generation. |

## Build sequence

### 1. Make the compact interface — implemented in 0.2

Follow [UI_PLAN.md](UI_PLAN.md). Replace the slider column with knobs, reduce the window, and use a shared Curve/Wave view for the result. Keep existing controls connected to their existing parameters.

First render empty and loaded screenshots. Check readability, selection handles, knob values, focus, and disabled states. Do not add new sound controls as working features before their processing exists.

Done when: the interface fits the target size without clipped controls, and current loading, preview, selection, and export still work.

### 2. Add Shape — implemented in 0.2

Shape is a volume envelope: a curve that changes the effect's loudness over time.

- Apply it to the generated reverb before adding Gap silence.
- Start with one simple curvature control, not an unrestricted multi-point editor.
- Increasing end emphasis makes the buildup quieter earlier and stronger near the end.
- Provide a neutral setting that leaves the current sound unchanged.
- The graph and Shape knob edit the same parameter. Neither keeps a separate hidden value.
- The Curve view shows the applied envelope. The Wave view shows the actual rendered sound. They must be labeled clearly because they are different things.
- Never boost unexpectedly just to force the sound to match a drawing.
- Keep the short boundary fades that prevent clicks.

Done when: changing the curve audibly changes the rise, both controls stay synchronized, and neutral Shape preserves the existing result.

### 3. Add Gap and define timing — implemented in 0.2

Gap is silence between the end of the effect and the start of the original sound.

Let L be the audible effect length and G be the gap:

- Generate L seconds of effect.
- Append G seconds of silence to the effect-only WAV.
- The exported file lasts L + G seconds.
- Its end remains the placement point for the original word.
- Label audible length, gap, and total duration clearly; do not silently shorten the effect to make room for Gap.
- Musical Length remains independent of the selected word's duration.
- Use zero Gap by default so existing projects retain their timing.

Example: 2 seconds of effect + 0.2 seconds of silence = a 2.2-second file. Place its end where the original word begins.

Done when: preview, graph, WAV length, and placement agree at different tempos and sample rates.

### 4. Add the complete-transition preview — implemented in 0.2

- Provide a compact preview choice: Effect or Effect + original.
- For the combined preview, play the effect, the chosen Gap, then the selected original audio.
- Do not include the rest of the source recording.
- Stop always stops the full audition.
- Keep ordinary export effect-only by default.
- If combined export is added later, expose it as an explicit choice rather than changing the existing drag button.
- Explain that normal FL playback can also be heard if the host transport is running; avoid surprising double playback.

Done when: users can judge the transition without arranging a test WAV first, and the drag output remains predictable.

### 5. Add reverb characters

Follow [REVERB_CHARACTERS.md](REVERB_CHARACTERS.md). Use bundled echo fingerprints (impulse responses / IRs) through JUCE convolution. Keep the current algorithm available as Classic for old projects.

Done when: Room, Hall, and Plate are meaningfully different, bundled, level-checked, saved correctly, and usable without locating files manually.

### 6. Add presets and A/B

Presets store sound settings, not the user's vocal recording.

- Start with a small set made through listening: short lead-in, spacious build, soft room, bright plate, and pulsed build.
- Support saving and naming personal presets.
- Show when settings differ from the selected preset.
- A and B share the same source and selection, but hold separate sound settings.
- Cache each slot's result when practical. Changing the source or selection invalidates both.
- Preview and export always refer to the visibly active slot.
- Save both slots and the active slot with the FL project.
- Keep preset and comparison controls in the compact header.

Done when: A/B can compare two effects without losing either, and loading a preset never replaces the source recording.

### 7. Add Pitch, then Formant

- Evaluate an existing pitch-shifting implementation before integrating it.
- Preserve the requested effect duration when shifting pitch.
- Start with Pitch only; zero must leave the sound unchanged.
- Add independent Formant later, with an optional link between the two.
- Put these controls in an expandable section so they do not enlarge the default window.
- Compare different amounts on real vocals and instruments for artifacts and level changes.

Done when: duration and alignment stay correct, default settings preserve the sound, and the creative results are useful.

### 8. Add Capture as another way to load audio

- Keep file drop and Open available.
- Add Capture under the source area's import choices.
- Record only audio arriving at the plugin's mixer input, with a visible recording indicator and duration limit.
- Use prepared buffers; avoid file writing or allocation in the audio callback.
- Stop recording into the same source/selection workflow as a loaded file.
- Replacing the source invalidates the old effect and A/B render caches.

Capture is separate from live reverse-reverb processing. Recording a mixer input does not automatically create an effect ahead of incoming audio.

### 9. Evaluate automatic File generation

Automatic generation can improve the current file workflow: wait briefly after edits, cancel superseded work, and publish only the latest settings. Keep an explicit Generate option if automatic work feels distracting or slow.

### 10. Add Track mode through a separate processing path — beta implemented in 0.2

Follow [TRACK_MODE.md](TRACK_MODE.md). Add a compact File / Track switch. File mode retains loading, selection, Generate, and WAV export. Track mode processes the incoming mixer channel continuously and introduces latency that must be reported and verified with FL's delay compensation.

Plan a streaming reversed-response engine, a correctly delayed original path, and automatable Effect Amount. Support arrangement playback first. Verify preroll, transport changes, loops, and song rendering. Live microphone/keyboard monitoring will retain noticeable delay; compensation does not remove that physical limitation.

Done when: timing is measured and stable in FL, mode switching preserves File work, and the interface fits the compact layout without misleading file export or selection controls in Track mode.

Current beta: finite reversed Classic response through JUCE convolution, buffered original, reported latency, scopes, live Amount/Output, stopped-transport application of engine changes, transport reset, and saved mode. Offline checks cover delay, Gap, block-size independence, bypass, seeking, and state. Actual FL host compensation, automation, loops, and song export remain to be checked. Other character assets are not bundled yet.

## Shared engineering requirements

- Reuse existing parameter IDs; append new parameters without reordering old ones.
- Read old saved states. Missing Shape and Gap values must preserve the old result. Missing character means Classic.
- Version new state data, including custom IRs and A/B settings.
- Rendering uses an immutable source/settings/IR snapshot. Discard late results after edits.
- Keep offline rendering, file writing, and engine preparation off the playback callback. Track mode uses only prepared, bounded streaming DSP there.
- Preserve persistent exported WAVs that FL projects may reference.
- Do not replace the installed binary while FL has it loaded. Finish and verify the build first, then update it after the user saves and closes FL.

## Verification at each stage

- Run the existing checks after relevant code changes.
- Add focused checks for new processing: Shape neutral behavior, Gap length/alignment, combined preview order, character loading, and preset/A/B restoration.
- Render and inspect empty, loaded, generated, changed, and error interface states.
- Check the compact UI at normal and increased Windows scaling.
- Listen to vocals, a short hit, a sustained sound, and silence.
- Check FL playback and file dragging after installing a verified build.
- For Track mode, additionally verify reported latency, automation timing, seek/loop recovery, mode changes, and offline FL export.

## Other comparison features

Wet/dry Mix, custom-IR import, level meters, hit targeting in a playing loop, and Mac/AU builds were mentioned during the comparison. They are not all required for the next update.

Custom-IR import fits naturally after bundled characters. A small output meter fits the UI if useful. Mix needs an explicit preview/export policy. Loop targeting belongs with live processing. Mac/AU requires a separate platform build and testing.

## First delivery

Compact gray/red UI, working Shape graph, Gap, and Effect + original preview. Then add the three new reverb characters, presets, and A/B. Pitch, Formant, Capture, and the separately planned Track mode follow in later stages. Track mode does not replace File mode.
