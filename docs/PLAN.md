# ReverseReverb build plan

Status: version 0.2 adds the compact gray/red UI, Shape, Gap, combined preview, and buffered Track-mode beta to the original VST3/standalone foundation. See README.md for current usage and verification. Actual FL Track compensation, listening, and drag-and-drop checks remain a separate step.

## Purpose

A personal learning project: load a sound, choose a piece, turn it into a rising reverse-reverb effect, and drag the effect into FL Studio.

Use plain labels in the interface. Keep technical details in these documents.

## Documents

- [ROADMAP.md](ROADMAP.md): next-version feature priorities and implementation stages; current planning entry point.
- [UI_PLAN.md](UI_PLAN.md): requested compact gray/red design with knobs and interactive Shape graph.
- [REVERB_CHARACTERS.md](REVERB_CHARACTERS.md): IR sourcing, character processing, and verification.
- [TRACK_MODE.md](TRACK_MODE.md): File/Track modes, playing-audio processing, latency, and FL timing checks.
- [WORKFLOW.md](WORKFLOW.md): loading, selection, Capture, interface, and export behavior.
- [AUDIO.md](AUDIO.md): simple explanations of the sound algorithms and processing design.

## Confirmed context

- Target: FL Studio on Windows.
- This is for personal use and learning.
- The reference is Backverb's capture, preview, shaping, and WAV workflow.
- The user is interested in dragging audio into the plugin and selecting a piece inside it.

## Proposed first-version decisions

The first implementation follows these defaults after the user's instruction to start. They remain easy to change.

- Make drag-and-drop the main input method.
- Provide an Open audio file button as a fallback.
- Leave live Capture for a later version.
- Export the effect by itself. Keep the original vocal in FL Studio.
- Use a simple functional interface before adding visual decoration.
- Treat Vocal/Instrument modes as unnecessary until we can define a useful difference.

## Build order

### 1. Make a plugin FL can open

- Use C++ and JUCE to build a 64-bit VST3 audio effect.
- Pass incoming audio through without changing it.
- Open a basic window in FL Studio.
- Confirm it loads, closes, and reopens normally.

Done when: FL can scan and load the plugin, and the original track plays normally.

### 2. Load audio

- Accept a WAV dropped onto the window.
- Add a file picker.
- Keep an untouched source recording in memory.
- Handle mono and stereo, and retain the source sample rate.
- Show useful errors for unsupported or excessively large files.

Done when: the same recording loads through both methods and plays correctly.

### 3. Show and select the sound

- Draw a waveform: a picture of the sound.
- Select everything on initial load.
- Drag across the waveform to select a smaller piece.
- Let the user move the start and end edges, zoom, and play the selection.

Done when: a user can isolate and listen to a word without changing the source file.

### 4. Make the reverse reverb

- Use an existing reverb engine initially.
- Process the selected sound on a background worker.
- Produce an effect-only lead-in of a defined duration.
- Keep the original selection available for comparison and rerendering.
- Add brief edge fades to prevent clicks.

Done when: the result audibly grows toward its end and fits before the original sound.

### 5. Preview and save

- Add Play original, Play effect, and Stop.
- Save the result as WAV.
- Add Drag effect to FL Studio.
- Keep exported files available after the drag; do not delete files FL may still reference.

Done when: the exported sound matches the preview and survives saving and reopening an FL project.

### 6. Add musical timing and sound controls

- Read FL's tempo and time signature, with a manual fallback when unavailable.
- Add duration in seconds and musical beat/bar choices.
- Add Size, Tone, Width, Tremolo rate, Depth, and Output.
- Rerender when a setting changes the generated sound.
- Ensure a late background result cannot replace a newer result.

Done when: controls behave predictably and the exported WAV reflects the current settings.

### 7. Save the plugin's work with the project

- Save parameters and selection positions.
- Preserve loaded audio in the saved state, with a reasonable size limit.
- Rebuild the processed result after reopening.
- Keep instances independent.

Done when: a reopened FL project restores the source and settings without relying on an old temporary file.

### 8. Verify and refine

- Check silence, short selections, long files, mono, and stereo.
- Check different source and host sample rates.
- Check tempo changes and unavailable host timing.
- Check edits made during rendering and closing during rendering.
- Verify importing from FL's available drag sources rather than assuming they all behave alike.
- Listen for clicks, unexpectedly loud output, truncation, and unwanted dry voice.
- Improve appearance once the complete workflow works.

## Later features

The detailed roadmap is in [ROADMAP.md](ROADMAP.md). Version 0.2 implements its first UI/Shape/Gap/preview stage and a Track beta. [UI_PLAN.md](UI_PLAN.md) describes the compact gray/red interface and future layout additions.

- Live Capture for instruments and processed mixer audio.
- An audition that plays the effect followed by the original selection.
- Optional combined export of the effect and original selection.
- Presets and appearance refinements.

## Technical references

- [JUCE plugin setup](https://juce.com/tutorials/tutorial_create_projucer_basic_plugin/)
- [Accept dropped files](https://docs.juce.com/master/classjuce_1_1FileDragAndDropTarget.html)
- [Read host timing](https://docs.juce.com/master/classjuce_1_1AudioPlayHead.html)
- [Drag files out](https://docs.juce.com/master/classjuce_1_1DragAndDropContainer.html)
- [FL Studio plugin installation](https://www.image-line.com/fl-studio-learning/fl-studio-online-manual/html/basics_externalplugins.htm)
