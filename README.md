# Granular Freeze

Granular Freeze v0.2.0 is a compact stereo AU/VST3 live-input effect.

It builds VST3 for macOS and Windows and AU for macOS. Repository-authored
source is MIT-licensed. Binary builds incorporate JUCE 8.0.15, which is
separately dual-licensed under AGPLv3 or the commercial JUCE 8 licence. No
distribution basis has been recorded for this product, so production binaries
must not be published or sold. See [docs/RELEASE.md](docs/RELEASE.md).

**Implemented and automatically tested. Not yet evaluated by Gabriel in Ableton
Live or Bitwig. It is not released. No commercial validation.** Automated tests and
renderer metrics do not establish musical quality, CPU suitability, DAW
compatibility, a release decision, pricing, or sales readiness.

## Implemented Grain Core

- Transparent stereo live pass-through records a chronological circular capture
  of up to ten seconds.
- Hold selects the most recent 50 ms–10 s chronological window, clamped to
  available capture, and latches that view when Freeze engages.
- Freeze renders that held view through a deterministic fixed 64-voice
  engine with Hann windows, cubic pitched reads, and overlap normalization.
- Position 0.00 selects the oldest complete grain window; 1.00 selects the
  newest complete window, not the raw circular-buffer write location.
- Freeze/Unfreeze transitions are reversible from the current blend.
- v0.1 state receives Hold and Grain Core defaults; v0.1.2 state retains Hold
  while receiving Grain Core defaults; v0.2 state round-trips all seven values.
  Tests cover migration, automation, finite guards, and chunking.

## Controls

All seven AudioProcessorValueTreeState parameters are host-automatable.

| Host ID | Control | Range | Default |
| --- | --- | --- | --- |
| freeze | Freeze | off / on | off |
| pitch | Pitch | 0.50–2.00 ratio, 0.01 step | 1.00 |
| crossfadeMs | Crossfade | 1–500 ms, 1 ms step | 30 ms |
| holdMs | Hold | 50–10,000 ms, 1 ms step | 1,000 ms |
| grainSizeMs | Size | 5–200 ms, 1 ms step | 80 ms |
| densityHz | Density | 0–200 grains/s, 1 grain/s step | 20 grains/s |
| position | Position | 0.00–1.00, 0.01 step | 1.00 |

Size determines new-grain duration. Density is a deterministic launch rate and
zero settles frozen output to silence. Pitch is source-read rate. The editor
exposes Freeze, Pitch, Position, Size, Density, Hold, and Crossfade.

## Evidence and deferred scope

The offline GranularFreezeEngineTests and GranularFreezeTests executables need
no host/device. GranularFreezeRender writes fifteen controlled Freeze cases
plus dry-reference.wav: exactly sixteen stereo 48 kHz/24-bit WAV listening
aids. The CI workflow is configured to run both binaries on macOS and Windows;
a local run is not remote-CI or DAW-listening evidence.

The hardened candidate path uses pinned macOS and Windows runners, a universal
macOS 12 build, strict archive scripts, fail-closed conditional signing and
notarization, signing-status manifests, and private draft prereleases. No
credential-dependent signing path has executed. Those paths still require
remote execution and downloaded-asset inspection; their presence in the
repository is not release evidence.

The exact older `v0.1.1-rc.1` private-draft AU/VST3 assets passed a limited
Ableton Live 12.4.2 functional smoke test, but they predate Hold and the Grain
Core and are not evidence for v0.2.0.

Time-stretch, presets/performance banks, feedback, random scatter/modulation,
waveform UI, credential-tested production signing/notarization, store work, and
all commercial/release decisions are deferred, not current product claims. See
[docs/BUILD_AND_TEST.md](docs/BUILD_AND_TEST.md) for exact checks and human
listening boundaries.

---

<!-- catalog-footer -->
Part of **Gabriel Tools + Code** — a public catalog of audio products, studio utilities, design systems, software and repositories by Gabriel García Alonso:

**[Open the full catalog →](https://gabriel-tools-and-code.notgabriels960914.chatgpt.site)**

Related free tools: [theme-contrast](https://github.com/notgabriels-sys/theme-contrast) · [htmlshot](https://github.com/notgabriels-sys/htmlshot) · [50 dark themes for Claude Code](https://github.com/notgabriels-sys/claude-code-50-dark-themes).
