# Granular Freeze

Granular Freeze v0.2.0 is a compact stereo AU/VST3 live-input effect.

**Implemented and automatically tested. Not yet evaluated by Gabriel in Ableton
Live or Bitwig. It is not released. No commercial validation.** Automated tests and
renderer metrics do not establish musical quality, CPU suitability, DAW
compatibility, a release decision, pricing, or sales readiness.

## Implemented Grain Core

- Transparent stereo live pass-through records a chronological circular capture
  of up to eight seconds.
- Freeze snapshots valid capture and renders a deterministic fixed 64-voice
  engine with Hann windows, cubic pitched reads, and overlap normalization.
- Position 0.00 selects the oldest complete grain window; 1.00 selects the
  newest complete window, not the raw circular-buffer write location.
- Freeze/Unfreeze transitions are reversible from the current blend.
- v0.1 state supplies defaults for new controls; v0.2 state round-trips all
  six values. Tests cover migration, automation, finite guards, and chunking.

## Controls

All six AudioProcessorValueTreeState parameters are host-automatable.

| Host ID | Control | Range | Default |
| --- | --- | --- | --- |
| freeze | Freeze | off / on | off |
| pitch | Pitch | 0.50–2.00 ratio, 0.01 step | 1.00 |
| crossfadeMs | Crossfade | 1–500 ms, 1 ms step | 30 ms |
| grainSizeMs | Size | 5–200 ms, 1 ms step | 80 ms |
| densityHz | Density | 0–200 grains/s, 1 grain/s step | 20 grains/s |
| position | Position | 0.00–1.00, 0.01 step | 1.00 |

Size determines new-grain duration. Density is a deterministic launch rate and
zero settles frozen output to silence. Pitch is source-read rate. The editor
exposes Freeze, Pitch, Position, Size, Density, and Crossfade.

## Evidence and deferred scope

The offline GranularFreezeEngineTests and GranularFreezeTests executables need
no host/device. GranularFreezeRender writes thirteen controlled Freeze cases
plus dry-reference.wav: exactly fourteen stereo 48 kHz/24-bit WAV listening
aids. The CI workflow is configured to run both binaries on macOS and Windows;
a local run is not remote-CI or DAW-listening evidence.

The hardened candidate path uses pinned macOS and Windows runners, a universal
macOS 12 build, strict archive scripts, and a secret-free unsigned draft
workflow. Those paths still require remote execution and downloaded-asset
inspection; their presence in the repository is not release evidence.

Time-stretch, presets/performance banks, feedback, random scatter/modulation,
waveform UI, signing/notarization, store work, and all commercial/release
decisions are deferred, not current product claims. See
[docs/BUILD_AND_TEST.md](docs/BUILD_AND_TEST.md) for exact checks and human
listening boundaries.
