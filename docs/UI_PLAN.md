# Compact gray/red interface

Status: the 900 × 560 gray/red interface, knobs, Shape graph, Curve/Wave tabs, and File/Track controls are implemented in version 0.2. Presets, A/B, and later advanced controls remain planned. The sections below also retain the design targets for future work.

## Direction

Small, clean, and practical. The source selection and the resulting buildup should be easy to understand at a glance. Use knobs to reduce space and restore the feel of an audio tool, with our own arrangement.

## Size and density

- Target default: **900 × 560 logical pixels**, reduced from the current 1060 × 760.
- Adjust the exact size only after rendered layout and scaling checks.
- Use approximately 14 px outer margins and 8–12 px between related areas.
- Knob diameter around 44–48 px; values remain readable and editable.
- Aim for 12–13 px body text and at least 11 px secondary labels.
- Avoid a large title, slogan, tall export card, or a separate sidebar.
- Keep the main workflow visible without scrolling.
- Later Pitch/Formant settings use a collapsed expandable section; they must not crowd the default view.

## Colors and finish

| Role | Starting color |
|---|---|
| Window | `#141414` |
| Panels | `#1D1D1D` |
| Fields / graph | `#181818` |
| Borders / inactive tracks | `#3A3A3A` |
| Main text | `#E8E8E8` |
| Secondary text | `#A4A4A4` |
| Accent | restrained red, around `#CB4848` |

Use red for active knob arcs, selected handles, the curve, and the main action. Leave most of the screen gray. Use flat surfaces, subtle borders, small corner radii, and no neon glow or decorative animation. Check contrast in the actual render. Errors need text or an icon, not just a different red.

## Proposed layout

Approximate bands within a 900 × 560 window:

| Area | Position | Contents |
|---|---|---|
| Header | y 14–44 | Small title; File/Track switch, compact preset field, A/B, and tempo as those features arrive. |
| Source | y 54–166 | File name, Open/Replace, source waveform, selection, original playback, zoom, and Select all. |
| Result | y 176–318 | Curve/Wave tabs sharing one canvas; time axis and original-sound marker. |
| Character / timing strip | y 328–356 | Character selection, length unit, and pulse rate. |
| Knobs | y 366–462 | Length, Size, Tone, Width, Shape, Gap, Depth, Output. |
| Actions | y 474–514 | Generate, preview choice/play, Stop, Save, and a compact draggable WAV area. |
| Status | y 524–544 | One short status or error line. |

This is a layout budget, not permission to shrink text until it fits. Rearrange small controls if testing shows a conflict.

## Knobs

- Compact flat knobs with a thin active red arc and a clear pointer.
- Label above, editable value below.
- Drag normally; use a fine-adjust modifier for small changes.
- Double-click resets the default.
- Provide useful units: seconds, milliseconds, percent, dB, and musical division where relevant.
- Shape knob and graph always show the same value.
- Use short labels, with plain-language tooltips for explanations.
- Pulse rate is a small selector; Depth is disabled clearly when pulsing is off.
- Tempo fallback belongs in a compact timing menu/field rather than another permanent large knob.

## Source waveform

- Always available for selecting the word or sound.
- Show the selected region with a subtle red tint and distinct boundary handles.
- Keep start/end time and selected duration readable.
- Keep zoom and whole-file selection in its compact toolbar.
- Before loading, the same area is the audio drop target. Avoid a separate large empty panel.
- Highlight valid file hover and show loading/error feedback.
- Automatic vertical display scaling must not change audio volume.

## Interactive Shape graph

- Horizontal axis: time toward the original sound. Vertical axis: the applied volume envelope.
- One draggable curvature handle for the first implementation.
- Dragging the handle updates Shape; do not trigger a full render for every mouse pixel.
- Keep original-sound timing visible with a labeled **Original** marker.
- Show Gap as a distinct quiet section before that marker.
- Do not allow dragging to create a negative length or Gap.
- Provide knob/value access so precise editing does not require dragging the graph.
- Keep the graph usable at its compact size: one meaningful handle is better than several tiny ones.
- A neutral Shape must be displayed truthfully; the natural reverb rise is not the same thing as an added volume envelope.

## Visualizations

Use visual feedback to explain a decision:

1. Source waveform: choose the input sound.
2. Curve view: understand and change Shape and Gap.
3. Wave view: inspect the generated audio on the same timeline.
4. Thin playback cursor: see where original/effect/combined audition is playing.
5. Optional small output meter: check loudness and overload when metering is implemented.

Curve and Wave are tabs in the same area, not two permanently expanded canvases. Label the view, units, and timing consistently. Skip an always-on spectrum analyzer or animated decoration unless it serves a specific editing need.

## Behavior

- Generate is the main action; the export area becomes usable once the result is ready.
- Do not export a stale result after settings or selection changes.
- Effect + original preview is an audition choice; export stays effect-only by default.
- Disabled controls remain readable and explain why they are disabled.
- Keep status messages short and use a tooltip/details area for long errors.
- Keep existing settings and source when merely changing views or opening an advanced section.

## File / Track modes

See [TRACK_MODE.md](TRACK_MODE.md) for processing and timing requirements.

- Keep the mode switch small and visible in the header.
- File mode uses the source area for loading and selection, with Generate and drag export in the action strip.
- Track mode reuses that area for incoming audio visualization. Editable selection handles and file audition actions are unavailable there.
- Reuse the result graph and knobs. Use the action strip for Effect Amount, readiness, and a compact latency readout; keep the default window size.
- Do not display an old File result as a printable Track result.
- Preserve loaded File work when switching modes.
- Show latency-changing edits as pending until safely applied; use inline status rather than repeated dialogs.

## Visual review

Inspect rendered empty, loaded, selected, generated, modified, busy, and error states. Check long filenames, parameter extremes, unavailable host tempo, inactive pulse, and the future preset/A/B header. Verify Windows scaling, keyboard focus, and hit targets. A screenshot should make the next action clear without a large instructional paragraph.
