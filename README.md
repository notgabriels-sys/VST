# Granular Freeze

Granular Freeze catches the last seconds of a live signal and holds them just
before they vanish. Inside that suspended memory, a deterministic granular
engine can stretch the moment into weight, motion, or atmosphere without
breaking the continuity of the performance.

Version 0.2.0 is a compact stereo effect built with the
[DISTRHO Plugin Framework](https://github.com/DISTRHO/DPF). It targets VST3 and
CLAP on macOS and Windows, plus Audio Unit on macOS.

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

Click the central state control to capture or release the memory. Drag any
continuous parameter vertically; hold Shift while dragging for fine control.

## Visual themes

The interface keeps the same restrained geometry while offering three quiet
color atmospheres. Select a palette from the three swatches in the upper-right
corner, or press `T` while the editor is focused to cycle through them:

- **Obsidian / Sage** — the default, cool and mineral.
- **Ember / Copper** — warmer, earthen, and slightly more nocturnal.
- **Nocturne / Violet** — a restrained ultraviolet accent for darker sessions.

Themes are visual-only: they do not add audio parameters, automation lanes, or
changes to the signal path. A shallow material gradient and sparse grain give
the surfaces depth without turning the editor glossy or visually busy.

## Current verification boundary

The framework-neutral engine/core/contract suites pass on macOS and Windows.
CI produces universal arm64/x86-64 macOS VST3, AU, and CLAP bundles and x86-64
Windows VST3 and CLAP binaries. Apple AU validation passes, and the exact VST3
has been observed loading with its editor open. Dedicated VST3/CLAP validator
runs, final listening approval, production signing/notarization, tagging, and
publication remain independent release gates. See `docs/DPF_PORT_VERIFICATION.md`,
`docs/BUILD_AND_TEST.md`, and `docs/RELEASE.md`.

---

<!-- catalog-footer -->
Part of **Gabriel Tools + Code**:
**[Open the full catalog](https://gabriel-tools-and-code.notgabriels960914.chatgpt.site)**
