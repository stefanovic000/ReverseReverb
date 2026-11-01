# Workflow and interface plan

Status: File loading/selection/export follows this workflow. Version 0.2 adds Shape, Gap, combined preview, and a buffered Track-mode beta. FL-specific drag sources and Track host timing still require testing.

The current 0.2 layout is the compact gray/red, knob-based design in [UI_PLAN.md](UI_PLAN.md). [ROADMAP.md](ROADMAP.md) tracks what is implemented and what remains planned.

## Main workflow

This is the File workflow. [TRACK_MODE.md](TRACK_MODE.md) describes the implemented beta alternative: select Track while stopped, play the FL arrangement, and process incoming mixer audio with reported latency. Amount and Output stay live; engine changes apply while stopped.

1. Drop a recording into the plugin, or click Open audio file.
2. Listen to it.
3. Highlight the word or sound to use.
4. Choose the effect's duration and sound settings.
5. Click Generate effect.
6. Listen using Play effect.
7. Drag the finished WAV into FL Studio.
8. Place its end at the beginning of the original word or sound.

The original recording stays in FL Studio. Loading it into this plugin creates a working copy.

## Why drag-and-drop comes first

It avoids recording the same audio again. The user can see what is loaded, choose a piece, and process it at their own pace.

The main empty-window message should be:

> Drop an audio file here
>
> or Open audio file

Once audio is loaded, use that area for the waveform and selection. A Replace audio button allows loading a different recording without an accidental drop discarding work.

## Does Capture have a use?

Yes. Capture copies audio playing through the plugin rather than opening a saved recording.

It is useful for:

- A synth or other instrument played live.
- A vocal after FL's other effects have changed it.
- Several sounds mixed together on one mixer channel.
- Audio that is inconvenient to save as a separate file first.

Example: dropping a vocal's source file loads the recording from disk. Capturing the vocal after distortion and delay records what those effects produce.

Recommendation: leave Capture out of version one. Later, add it as a secondary input option beside Open audio file. A future Capture mode would show Record, Stop, and a recording indicator.

Capture is not needed to generate reverse reverb from a loaded file.

## What FL drag-and-drop must be checked

- A saved WAV from Windows is the basic supported input.
- Dragging from FL's Browser, Edison, or other audio windows needs direct testing.
- Ordinary dragging in the Playlist may move a clip rather than export audio to a plugin.
- A drag may supply the entire underlying file rather than a trimmed section.
- A dropped source file may omit FL's clip stretching, pitch changes, automation, and mixer effects.

Therefore the plugin includes its own selection controls. For an exact processed version, the user can render a file in FL first; Capture can offer another route later.

Do not promise direct Playlist selection import until it works in the actual plugin.

## Selecting a word

- The full recording is selected initially.
- Click and drag across the waveform to highlight a region.
- Drag either edge to refine it.
- Play original plays that region only.
- Zoom makes short words easier to select.
- Select all restores the whole-file selection.

The plugin does not recognize words automatically. The user identifies a word by listening.

## Previous 0.1 window layout

Top: compact title and song speed.

Left, upper panel: recording name, Open/Replace audio, source waveform, selection handles, zoom, and Play original.

Left, lower panel: Generate, effect waveform, Play effect, Stop, and Save WAV.

Right: Length and tempo fields, then a vertical stack of horizontal sliders for Size, Tone, Width, Pulse depth, and Output. Pulse rate sits beside its label. Sliders replace the original knob row.

Bottom left: large drag-to-FL area. Bottom edge: a single status line.

This previous gray/green slider layout has been replaced by the 0.2 compact gray/red design in UI_PLAN.md. Waveforms still fit their visible height automatically; this only changes the drawing, not audio volume.

Make only the controls relevant to the current state available. Before generation, explain that an effect must be generated before it can be played or exported.

## Changes and generation

For the first version, use an explicit Generate effect button. This makes the sequence easy to understand.

Changing the selection or sound settings marks the previous result as out of date. Show Generate again and prevent exporting a result that no longer matches the visible settings. Later we can consider automatic generation after the user stops moving a knob.

## What the exported file contains

Default proposal: only the growing reverb before the selected sound, without the normal voice mixed in.

The effect still comes from the vocal, so it can contain recognizable voice-like textures. Effect-only does not mean it is unrelated noise.

Example placement:

```text
Time:          8 seconds       9 seconds       10 seconds
Effect track:  quiet --------- louder ------> ends
Vocal track:                                  Hello...
```

The file's end is the alignment point. The user places that end where the original selection starts. An ordinary audio-file drag does not automatically tell the plugin the target Playlist position.

If a word was selected from the middle of a phrase, align to that word rather than the start of the entire recording.

## Choices still open

- Capture remains a later secondary input option.
- Effect + original audition is recommended in the next feature stage; default export stays effect-only.
- The user has specified gray with red accents, a smaller minimal interface, less empty space, and knobs. See UI_PLAN.md for the layout budget.
