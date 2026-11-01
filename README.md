# ReverseReverb

A Windows audio tool that reverses a sound and shapes the reverb tail leading into it — the reverse-reverb lead-in trick, as a small app and a VST3.

Made with C++ and JUCE. Drop a file in, pick the part you want, generate, listen, then save it or drag it straight into FL Studio.

![ReverseReverb](docs/screenshot.png)

## Build

Needs Visual Studio with the C++ desktop workload and CMake.

```powershell
./scripts/build.ps1
```

The app and the VST3 land in `build/msvc-Release/ReverseReverb_artefacts/Release/`.

## Use

1. Drop a WAV, AIFF, FLAC or MP3 into the window.
2. Pick the word or sound to process.
3. Adjust Length, Size, Tone, Width, Shape, Gap, Depth, Output.
4. Generate → Play → Save WAV (or drag into FL Studio).

## Docs

The long notes live in [`docs/`](docs):

| File | What's in it |
|---|---|
| [AUDIO.md](docs/AUDIO.md) | how the reverb tail is built |
| [REVERB_CHARACTERS.md](docs/REVERB_CHARACTERS.md) | room / hall / plate voicings |
| [TRACK_MODE.md](docs/TRACK_MODE.md) | the beta track mode |
| [UI_PLAN.md](docs/UI_PLAN.md) | interface decisions |
| [ROADMAP.md](docs/ROADMAP.md) | what's done, what's next |
| [PLAN.md](docs/PLAN.md) | original scope |
| [WORKFLOW.md](docs/WORKFLOW.md) | capture-to-FL workflow |
