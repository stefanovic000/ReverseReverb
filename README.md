# ReverseReverb

A Windows audio tool that reverses a sound and shapes the reverb tail leading into it — the reverse-reverb lead-in trick, as a small app and a VST3.

Made with C++ and JUCE. Drop a file in, pick the part you want, generate, listen, then save it or drag it straight into FL Studio.

![ReverseReverb](docs/screenshot.png)

## Quick setup

1. Install **Visual Studio** with the *Desktop development with C++* workload. CMake and Ninja come with it, so nothing else is needed.
2. Clone and build — Release, with the checks run automatically:

   ```powershell
   git clone https://github.com/stefanovic000/ReverseReverb.git
   cd ReverseReverb
   ./scripts/build.ps1 -Configuration Release
   ```

   The script downloads JUCE into `.dependencies/` the first time, so the first build takes a few minutes.
3. Pick up the build:

   | Artefact | Path |
   |---|---|
   | Standalone app (4.9 MB) | `build/msvc-Release/ReverseReverb_artefacts/Release/Standalone/ReverseReverb.exe` |
   | VST3 (4.7 MB) | `build/msvc-Release/ReverseReverb_artefacts/Release/VST3/ReverseReverb.vst3` |

4. Run the `.exe` directly, or copy the whole `ReverseReverb.vst3` folder into `C:\Program Files\Common Files\VST3` and rescan plugins in FL Studio (Options → Manage plugins → Find more plugins).

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
