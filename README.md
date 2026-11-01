# ReverseReverb

A personal Windows audio plugin for reverse-reverb lead-ins, with File processing and a buffered Track-mode beta.

Version 0.2 adds the compact gray/red interface, Shape graph, Gap, combined preview, and Track-mode beta. [ROADMAP.md](ROADMAP.md) tracks completed and future stages; additional Room/Hall/Plate characters, presets, A/B, Pitch, Formant, and Capture remain planned.

Version 0.2.1 corrects Track history handling for repeated/rounded host timestamps and declared loop wraps. Real seeks still clear old audio. These fixes address reproducible offline silence cases; they do not establish the cause of an individual FL project's routing or device problems.

**Drop audio → select a sound → Generate effect → Play effect → drag into FL Studio.**

## Run the standalone version

After building, open:

```text
build/msvc-Release/ReverseReverb_artefacts/Release/Standalone/ReverseReverb.exe
```

Use its Options menu to select an audio output if preview is silent. Input is not needed for file processing. The standalone's feedback-protection setting can remain enabled.

## Load it in FL Studio

The complete VST3 bundle is this folder:

```text
build/msvc-Release/ReverseReverb_artefacts/Release/VST3/ReverseReverb.vst3
```

1. Copy the complete `ReverseReverb.vst3` folder into `C:\Program Files\Common Files\VST3`. This standard location is required; a custom VST search path is insufficient for FL's VST3 scanner.
2. Alternatively, run `./scripts/install.ps1` in PowerShell as administrator to copy and verify the bundle.
3. In FL Studio, open Options → Manage plugins. Enable Verify plugins, then click Find installed plugins.
4. Search for ReverseReverb, mark it as a favorite, and load it into a mixer effect slot.

Keep the complete `.vst3` folder together; its internal binary alone is not the full bundle. The build does not change FL settings or install files into Program Files. The separate install script copies the bundle to the standard VST3 location.

See [Image-Line's plugin location requirements](https://www.image-line.com/fl-studio-learning/fl-studio-online-manual/html/basics_externalplugins.htm#vstfolders).

## File mode

1. Drop a WAV into the large area, or click Open audio. Mono and stereo recordings are supported. The decoded recording must fit within 64 MB.
2. Drag across the waveform to select a word or sound. Adjust either edge. Scroll to zoom, use the +/- buttons, or double-click to select everything.
3. Use Play original to check the selection. Stop FL's normal playback while comparing, otherwise both may play together.
4. Choose the Length unit. Bar divisions use FL's tempo/time signature; Seconds enables the Length knob. The standalone uses manual BPM and 4/4.
5. Adjust Size, Tone, Width, Shape, Gap, pulse rate/Depth, and Output. Drag the curve up/down to edit Shape, or use its knob. Natural Shape adds no extra volume envelope.
6. Click Generate. Choose Effect only or Effect + original, then Play. The combined audition plays the selected original after the generated effect and Gap.
7. Drag the Drag effect to FL Studio button into the Playlist. Alternatively, use Save WAV with a new filename.
8. Place the exported file's **end** at the beginning of the original selected sound.

Curve shows the applied volume envelope. Wave shows the actual rendered audio. The source waveform remains available above. Knobs support direct value entry, fine adjustment with Shift, and double-click reset.

The exported WAV contains the growing reverb followed by Gap silence. It does not include the normal vocal, even when combined preview is selected. For example, a 2-second effect and 150 ms Gap produce a 2.15-second file. Align its end with the original word. Settings changes require generating again before preview or export.

Automatic drag files are stored in this project's `exports` folder. **Keep that folder:** FL projects may reference those WAVs. Save WAV lets you copy an effect into your song's own folder. Existing files are not overwritten.

## Track mode (beta)

Put the plugin on the individual source's mixer channel. Putting it on the Master or a shared bus processes every sound routed there. FL's automatic delay compensation can delay other channels to keep them aligned with Track's Length + Gap; those channels can feel late even though our effect receives only its own input. See [Image-Line's explanation of PDC](https://www.image-line.com/fl-studio-learning/fl-studio-online-manual/html/mixer_trackprops.htm).

1. Stop FL playback, then switch File to Track in the header.
2. Wait for Track ready. The status reports the added delay.
3. Start FL playback before the target sound so the complete buildup has room to play.
4. Use Amount to adjust the effect contribution and Output for final volume. Both can change during playback; Amount can be automated in FL.
5. Stop playback to apply changes to Length, Gap, Size, Tone, Width, Shape, or pulse settings. Preparation happens in the background and the new engine is applied while stopped.
6. Stop playback before switching back to File. Its loaded recording and selection are retained.

Track outputs the delayed original plus a convolution-based reverse effect made from a fixed Classic reverb response. It affects incoming mixer audio, so successive sounds can overlap. It is a different processing path from the isolated File render.

Track introduces real waiting time; it is intended for arrangement playback, not low-delay microphone/keyboard monitoring. It reports that latency to FL. Offline timing, bypass, seek recovery, and state checks pass, but actual FL delay compensation, automation, loops, and song-export behavior still need host testing. Keep FL processing the plugin; Smart Disable can prevent stopped-transport updates or previews.

There is no Track print/export button yet. Switch to File for generated WAV export. See [TRACK_MODE.md](TRACK_MODE.md) for the implementation and remaining verification.

For a quiet/missing original, check Output (0 dB preserves level) and try Amount at 0% to isolate the delayed original. Allow Length + Gap for startup. Version 0.2.1 keeps history across declared loop boundaries, including loops shorter than that delay. Real seeks and transport restarts clear history, so a new startup wait follows. Keep the FL effect-slot mix at 100% and use the plugin's Amount control to adjust the reverb. Shortening Length reduces the project-wide compensation delay; zero delay cannot provide a buildup before unknown incoming audio.

`./scripts/audio-diagnostics.ps1` reads Windows output and per-app volume/mute levels without changing settings or playing audio. It does not measure a hardware volume knob or prove that FL's internal routing is correct.

## What this first version includes

- File drop and file picker; waveform selection, edge adjustment, and zoom.
- Stereo reverse reverb using JUCE's Freeverb-based reverb.
- Musical or seconds-based duration; tone, width, tremolo, and gain controls.
- Original/effect preview with sample-rate conversion.
- Background rendering and persistent 32-bit float WAV export.
- Host parameters and project state with embedded source audio and selection.
- Compact 900 × 560 gray/red interface with knobs, Curve/Wave views, Shape, Gap, playback cursor, and combined preview.
- Buffered Track-mode beta with a reversed Classic response, host latency reporting, live Amount/Output, scopes, transport-history reset, and latency-preserving bypass.
- VST3 and standalone builds, plus offline audio/integration checks.

## Limits to verify in FL Studio

- Dragging from a particular FL window may behave differently from dropping a saved file. An edited clip may supply its full source recording and omit FL effects or stretching.
- Playlist placement is manual; a WAV drag does not communicate the intended track/time.
- Preview while the FL transport is stopped depends on FL continuing to process the plugin. If necessary, enable the host's processing or play an empty section.
- The Classic reverb still needs listening feedback, especially for long lead-ins from very short selections.
- No live Capture, automatic word recognition, or combined voice-plus-effect export yet.

## Build

Requires Git, Visual Studio with the Desktop development with C++ workload, and its CMake tools.

Run in PowerShell:

```powershell
./scripts/build.ps1
```

The script downloads JUCE tag `8.0.16-security` (commit `ea77444750de45e036e0279bb0291144bf8e3688`) into `.dependencies/JUCE` if absent. It uses the installed Visual C++ compiler, builds VST3 and standalone targets, and runs the checks. Upstream source notices remain in the dependency.

To build Debug instead:

```powershell
./scripts/build.ps1 -Configuration Debug
```

The JUCE source path can also be supplied through CMake's `JUCE_DIR` variable. The export directory is compiled to this checkout's `exports` folder for this personal build; moving the whole project requires rebuilding or using Save WAV.

## Verification

Version 0.2.1 passes 77 checks. These cover the original audio/file workflow plus Shape, exact Gap silence, combined audition, legacy state defaults, Track latency, wet/dry placement, block-size independence, live Amount, stopped-transport application, seek recovery, and latency-preserving bypass. Regression checks cover repeated/rounded host positions, short declared loops, seeks while looping, isolation between instances and File/Track channels, and sustained stereo dry levels at 44.1/48 kHz. The executable also loads the VST3 binary, initializes its audio component, checks its mixer buses, and renders editor images into `test-output`.

These checks do not establish FL Studio compatibility by themselves. Host loading, audio-device playback, and FL-specific dragging need an actual FL session.

## Source map

- `Source/AudioEngine.*`: file decoding, reverse reverb, controls, WAV writing.
- `Source/PluginProcessor.*`: FL integration, background tasks, preview, project state.
- `Source/Waveform.*`: waveform drawing and selection.
- `Source/PluginEditor.*`: interface and drag/drop.
- `Source/DarkTheme.*`: shared colors and custom button, slider, and field drawing.
- `Source/ShapeGraph.*`: interactive envelope and Gap timeline.
- `Source/TrackEngine.*`: prepared streaming convolution, dry delay, and live gain controls.
- `Tests/Checks.cpp`: offline audio/integration checks and interface snapshots.
- [PLAN.md](PLAN.md), [WORKFLOW.md](WORKFLOW.md), [AUDIO.md](AUDIO.md): design and learning notes.
