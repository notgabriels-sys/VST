# Granular Freeze

A cross-platform **VST3 + AU** audio plugin for live time/freeze effects,
aimed at Ableton and Bitwig users.

Author: Gabriel García Alonso · License: MIT

## Status

Working prototype. An earlier baseline passed macOS and Windows CI, but this
candidate still requires fresh CI on both platforms. It has **not been
released** and has not yet been evaluated by ear in a DAW.
The current engineering candidate uses internal version `0.1.1` and the
reserved candidate tag `v0.1.1-rc.1`. The existing `v0.1.0` tag is obsolete
history and must not be moved, deleted, or reused. See
[docs/RELEASE.md](docs/RELEASE.md) for the candidate gates.

**Implemented**

- Circular-buffer freeze with a crossfaded transition in and out
- Crossfaded loop point, so held audio does not click on repeat
- Pitch control over frozen playback (0.5x–2.0x, cubic interpolation)
- Parameters via `AudioProcessorValueTreeState` — automatable, and saved with
  the session
- UI with freeze toggle, pitch and crossfade-time sliders
- CI workflow for macOS + Windows with an offline test suite

**Not implemented** — the granular engine the name implies is still ahead:

- Grain envelope, density and size controls
- Time-stretching independent of pitch
- Preset system and performance bank
- Production signing and notarization (the macOS candidate is ad-hoc signed;
  Windows is unsigned)

## Parameters

| Parameter | Range | Default |
|---|---|---|
| `freeze` | on / off | off |
| `pitch` | 0.5x – 2.0x | 1.0x |
| `crossfadeMs` | 1 – 500 ms | 30 ms |

Stereo in / stereo out only. The capture buffer is 8 seconds; frozen playback
loops whatever has been captured so far, which grows up to that limit.

## Build

    cmake -S . -B build -G "Xcode"      # macOS; omit -G on Windows
    cmake --build build --config Release --parallel

JUCE 8.0.15 is fetched automatically. Requires CMake >= 3.22. Full instructions,
including how to run the tests and validate the AU, are in
[docs/BUILD_AND_TEST.md](docs/BUILD_AND_TEST.md).

## Layout

    src/            plugin processor and editor
    tests/          offline behavioural tests (run in CI)
    docs/           build, release and CI secret guides
    scripts/        packaging helpers
    presets/        example patches (empty)
    .github/        CI and release workflows

## Docs

- [docs/PRODUCT_SPEC.md](docs/PRODUCT_SPEC.md) — intended product
- [docs/BUILD_AND_TEST.md](docs/BUILD_AND_TEST.md) — building, testing, validating
- [docs/RELEASE.md](docs/RELEASE.md) — tagging and publishing
- [docs/CI_SECRETS.md](docs/CI_SECRETS.md) — secrets, signing status
