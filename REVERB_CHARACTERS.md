# Reverb characters plan

Status: Room/Hall/Plate assets are not implemented; none have been selected, downloaded, or bundled yet. Version 0.2 uses JUCE convolution for Track mode with a finite response generated from Classic. File mode retains its existing Classic engine.

## What a character means

A character is a different echo pattern, not only a different name or Tone setting.

An impulse response (IR) is a short audio file describing the echo pattern of a space or reverb device. Convolution applies that pattern to the source recording.

Keep the current Freeverb-based implementation as **Classic**. Add three new choices:

| Character | Sound to look for |
|---|---|
| Room | Compact, close reflections. |
| Hall | Spacious, diffuse reflections with a lingering tail. |
| Plate | Dense, smooth reflections from a plate reverb or a convincing plate model. |

A real room library does not automatically provide a Plate. Find a suitable device/model IR or generate one from a suitable algorithm. Do not relabel a hall recording as Plate.

## How to get the fingerprints

### Preferred first route: existing IR files

- Start with [EchoThief's downloadable library](https://www.echothief.com/downloads/) for real spaces.
- Research a separate suitable source for Plate.
- Pick a small candidate set rather than downloading large unrelated collections.
- Prefer clean mono or stereo WAVs for the first implementation.
- Keep the original filename, source URL, space/device description, sample rate, and preparation notes alongside each asset.
- Store the upstream information with the project; the user should not need to source files manually to use bundled characters.

### Alternative: generate from a reverb engine

Send a one-sample impulse through a chosen reverb algorithm and record the fully wet response until it has faded. This makes an IR of that particular setting. A capture reproduces a fixed response; it does not preserve every time-varying behavior of a modulated or nonlinear reverb.

### Later learning exercise: measure a real room

Play a known test sweep through a speaker, record it through a microphone, and extract the room response. This needs equipment and measurement work, so it is not a dependency for the first character update.

## Choose and prepare assets

1. Inspect sample rate, channel layout, duration, noise, silence, and peak level.
2. Remove irrelevant leading silence and unwanted direct sound where appropriate, preserving meaningful early reflections.
3. Fade the ending if needed to avoid an abrupt cut.
4. Compare candidates on the same vocal, short hit, and sustained sound.
5. Compare at sensible matched levels so the loudest choice does not automatically seem best.
6. Choose one useful Room, Hall, and Plate. Record any fixed level adjustment explicitly.
7. Embed the selected files in the build with stable character IDs.

Do not silently normalize every exported effect. Character preparation gains and the user's Output control have separate purposes.

## Processing integration

Use [JUCE's convolution engine](https://docs.juce.com/master/classjuce_1_1dsp_1_1Convolution.html). Add the `juce_dsp` module to the build.

For each render:

1. Snapshot the selected source, character, settings, and IR data.
2. Load the chosen IR and prepare the convolution engine before processing the first source sample.
3. Reverse the selected source and feed it through the fully wet engine.
4. Collect the required tail, reverse it, and produce the lead-in.
5. Apply Tone, Width, Shape, pulse, Output, and boundary fades.
6. Append Gap silence and publish only if the render request is still current.

Check latency and tail handling explicitly. An IR that is shorter than the requested lead-in can produce a long quiet beginning. Choose sufficiently useful tails and define how Size changes them rather than hiding that behavior.

## Size with convolution

The current Size value changes feedback/decay in an algorithmic reverb. A fixed IR needs a different interpretation.

First investigate changing its decay envelope without stretching time. This can shorten the tail cleanly, but cannot invent a longer tail that was never recorded. Use a sufficiently long baseline IR. Do not present file resampling as a size-only change: it can also change the sound's pitch/color.

Define and listen to the Size range for each character before exposing it. Classic retains its existing behavior for old projects.

## Interface and saving

- Use a compact character strip or selector in [UI_PLAN.md](UI_PLAN.md).
- Existing projects default to Classic.
- Presets refer to stable bundled character IDs.
- Save the character and sound settings in project state.
- Add Custom IR later as an import choice. Embed bounded custom IR data in the project rather than relying only on an external file path.
- Keep original source audio and IR audio distinct in code and in file-drop behavior.

## Completion checks

- Each character is audibly distinct at comparable levels.
- Silence remains silent and output is finite.
- Mono/stereo sources and differing sample rates behave correctly.
- Duration, Gap, preview, and export alignment agree.
- Cancellation and character switching cannot publish an old render.
- Saved projects restore the exact bundled character without missing-file prompts.
- Old projects still use Classic and reproduce the old behavior.
- Short selections, long lengths, and extreme Size settings give understandable results.
