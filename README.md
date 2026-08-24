# Granular Freeze

Granular Freeze 0.2.0 is a compact stereo live-input granular effect built with
the [DISTRHO Plugin Framework](https://github.com/DISTRHO/DPF). It targets VST3
and CLAP on macOS and Windows, plus Audio Unit on macOS.

Repository-authored source is MIT-licensed. DPF and its format/UI dependencies
use permissive licenses with attribution requirements; exact notices ship in
`THIRD_PARTY_NOTICES.md`. The DPF port does not incorporate JUCE.

## Behavior

- Transparent stereo pass-through continuously captures up to ten seconds.
- Freeze latches the latest Hold window and stops capture until the plug-in is
  fully live again.
- A deterministic fixed 64-voice grain engine uses Hann windows, cubic pitched
  reads, chronological Position, and overlap normalization.
- Freeze transitions reverse continuously from the current blend.
- Audio-callback work is bounded; capture and scratch buffers are allocated
  during activation rather than processing.

## Controls

| Symbol | Control | Range | Default |
| --- | --- | --- | --- |
| `freeze` | Freeze | off/on | off |
| `pitch` | Pitch | 0.50-2.00 | 1.00 |
| `crossfadeMs` | Crossfade | 1-500 ms | 30 ms |
| `holdMs` | Hold | 50-10,000 ms | 1,000 ms |
| `grainSizeMs` | Size | 5-200 ms | 80 ms |
| `densityHz` | Density | 0-200 Hz | 20 Hz |
| `position` | Position | 0.00-1.00 | 1.00 |

## Current verification boundary

The framework-neutral engine/core/contract tests pass locally and DPF produces
arm64 macOS VST3, AU, and CLAP bundles with the custom editor. This branch is
not yet a release: universal macOS packaging, Windows CI, validators,
exact-artifact DAW testing, signing/notarization, and owner listening approval
remain independent gates. See `docs/BUILD_AND_TEST.md` and `docs/RELEASE.md`.

---

<!-- catalog-footer -->
Part of **Gabriel Tools + Code**:
**[Open the full catalog](https://gabriel-tools-and-code.notgabriels960914.chatgpt.site)**
