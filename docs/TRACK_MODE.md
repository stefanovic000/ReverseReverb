# File and Track modes

Status: a Track-mode beta is implemented in version 0.2. Offline checks pass for measured delay, dry/wet placement, Gap, block sizes, seeking, bypass, mode restoration, and File preservation. Actual FL host compensation, automation, loops, listening, and song export still require host verification.

Version 0.2.1 fixes two reproducible silence cases: repeated/rounded host timestamps no longer reset audio history, and a declared loop wrap preserves history so short loops can fill the dry delay. A genuine seek or transport restart still clears it. Synthetic host tests verify these cases; the user's FL project routing has not been observed directly.

## Purpose

Add a small **File / Track** switch so the plugin can also make reverse reverb from audio playing through its FL mixer channel.

| Mode | Input | What the user does |
|---|---|---|
| File | A loaded recording | Select a word, generate, audition, and drag out a WAV. |
| Track | Incoming mixer audio | Play the FL project and hear the effect on that channel. |

File remains the default and the best mode for precisely placed individual effects. Track is useful while arranging and adjusting a song. Keep the existing source and selection when switching modes.

## The waiting time

An effect cannot precede an unknown sound without waiting or advance scheduling.

Track mode will buffer the incoming sound. It can then output the growing effect before the buffered original. This introduces waiting time, called latency. Report the actual latency to FL so its plugin delay compensation can keep mixer tracks aligned.

- A longer lead-in generally requires more waiting time.
- FL may delay other mixer channels to align them with this plugin. That changes the project's responsiveness; it does not feed those channels into our reverb. An instance on the Master or a shared bus does process all audio routed there.
- FL compensation does not make the plugin predict a microphone performance.
- Singing or playing a keyboard through it may feel noticeably delayed.
- Playback needs enough time before the target sound (preroll) to hear the whole buildup.
- Starting exactly on the target word, seeking, or looping can interrupt the required history. Handle these cases explicitly.
- Declared loop wraps now keep wet/dry history across passes. The first pass still needs startup time. Hosts without valid loop points cannot reliably distinguish a wrap from a seek.

First target: playback of an existing FL arrangement. Low-delay live monitoring is not a requirement for this mode.

## Proposed streaming engine

Track mode needs its own bounded processing path. Do not run the current offline file renderer on every audio block.

Prototype a reversed finite reverb fingerprint (IR) through streaming convolution:

1. Prepare the selected character's IR and shape its rise outside the audio callback.
2. Process incoming audio through that reversed response to produce the wet signal.
3. Delay the original by the measured lead-in, Gap, and any additional processing latency needed for alignment.
4. Combine the delayed original with the effect at a controlled Amount.
5. Apply final Output gain and report the real output delay to the host.

The streaming result can overlap words or successive hits. It is not automatically identical to File mode's isolated, effect-only lead-in. Listen and measure the timing rather than assuming both methods produce identical files.

Keep Classic available in File mode. For Track mode, prototype a finite response captured from Classic if needed, and verify its behavior before presenting it as the same engine.

## Length, Shape, and Gap

- Length controls the intended buildup duration, independently of any word's duration.
- Shape controls its volume envelope.
- Gap places the buildup earlier relative to the delayed original.
- Measure kernel, dry-path, and convolution latency together; avoid counting the same delay twice.
- Do not append file-style silence to each processing block to implement Gap. Use the streaming timing relationship.
- Keep the graph's Original marker consistent with what is heard.

Current beta: prepare Length, Gap, Size, Tone, Width, Shape, and pulse changes in the background, then apply the engine while transport is stopped. Show pending edits while playing. Amount and Output update smoothly during playback. Later work can crossfade equal-latency engine changes while playing after host timing and continuity tests establish a reliable approach.

## What receives the effect

Initially, all audio arriving at the selected mixer channel can contribute to the reverb. The plugin does not identify words automatically.

Provide an automatable **Effect Amount** control so the user can raise or lower the effect along the song timeline. Verify automation timing against the compensated wet output. This controls what is heard; it is not automatic selection of a particular source word.

Later, input gating or hit targeting could choose which events excite the reverb. That is separate from volume automation and needs its own design.

## Output policy

- Default Track output: delayed original plus the wet effect.
- Effect Amount adjusts the wet contribution; it does not unexpectedly lower the original.
- Output adjusts the final signal.
- A future Wet only option can support mixer sends, but its latency and routing must be tested.
- Do not play File preview audio on top of Track processing accidentally.
- The existing drag button only exports a valid File render.
- Printing Track output to WAV requires an explicit later Print/Capture workflow; it must not silently export an unrelated old File result.

## Compact interface

Follow [UI_PLAN.md](UI_PLAN.md): preserve the 900 × 560 target and gray/red style.

- Add a small File / Track switch in the header.
- File mode retains source selection, Generate, audition, and drag export.
- Track mode uses the source area as a rolling input waveform with no editable file-selection handles.
- Reuse the shared Curve/Wave area and character/knob controls.
- File-only controls give way to Amount and a short latency/readiness readout. Do not add a permanent extra panel.
- Show messages such as Waiting for audio, Playing, or Length change applies when stopped.
- Show a brief contextual explanation of the delay when entering Track mode; avoid a repeating modal warning.
- Keep Capture separate: Capture records audio into a source, while Track continuously processes it.

## Engineering requirements

- Preallocate ring buffers and convolution resources for supported bounds.
- Keep file I/O, allocation, blocking locks, and heavy engine preparation off the audio callback.
- Execute only prepared, bounded streaming DSP on the callback.
- Use safe engine handoff and smoothing/crossfades for sound changes.
- Clear history on genuine seeks, sample-rate changes, and transport restart; retain it for verified wraps inside the declared loop range.
- Do not leak an old passage into a new playback position.
- Save the mode, controls, character, and any pending timing configuration with the project. Do not serialize transient playback buffers.
- Old saved states restore File mode with its existing zero-latency pass-through behavior.
- Switching back to File mode restores the appropriate host latency through a safe transition.

## Build and verification order

1. Prototype wet/dry timing using an impulse and a finite reversed IR.
2. Measure and report latency. Confirm FL keeps another mixer track aligned.
3. Verify startup/preroll, stops, seeks, loops, and offline song export.
4. Add smooth parameter updates and Amount automation.
5. Integrate the compact mode-dependent UI.
6. Save/restore and test multiple instances, different sample rates, and block sizes.
7. Listen to vocals, drums, sustained notes, and dense passages for overlap and artifacts.

Do not claim zero-delay live input or promise exact File/Track sound equivalence. Release Track mode when its timing, sound, and transport behavior are verified in FL.

## References

- [FL Studio plugin delay compensation](https://www.image-line.com/fl-studio-learning/fl-studio-online-manual/html/mixer_mixermenu.htm)
- [JUCE processor latency reporting](https://docs.juce.com/master/classjuce_1_1AudioProcessor.html)
- [JUCE convolution](https://docs.juce.com/master/classjuce_1_1dsp_1_1Convolution.html)
