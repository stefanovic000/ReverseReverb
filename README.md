# ReverseReverb

A small Windows audio tool that takes a sound, reverses it, and shapes the reverb tail that leads into it. Built with C++ and JUCE.

Drop a file in, pick the part you want, hit Generate, listen to the result, then save a WAV or drag it into FL Studio.

## Build

Needs Visual Studio with the C++ desktop workload, plus CMake.

```powershell
./scripts/build.ps1
```

The standalone app and the VST3 end up in `build/msvc-Release/ReverseReverb_artefacts/Release/`.

## Use

1. Drop a WAV, AIFF, FLAC or MP3 into the window.
2. Pick the word or sound you want to process.
3. Adjust Length, Size, Tone, Width, Shape, Gap, Depth and Output.
4. Generate → Play, then Save WAV or drag the result into FL Studio.

## More detail

- `AUDIO.md` — how the reverse-reverb tail is built
- `REVERB_CHARACTERS.md` — the room/hall/plate voicings
- `TRACK_MODE.md` — the beta track mode
- `UI_PLAN.md`, `ROADMAP.md`, `PLAN.md` — design notes and what comes next
