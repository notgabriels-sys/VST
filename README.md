# Granular Freeze

A **VST3** audio plugin for macOS and Windows, with an additional **AU** build
for macOS, aimed at live time/freeze effects in Ableton and Bitwig.

Author: Gabriel García Alonso

Repository-authored source is offered under the MIT License; see `LICENSE`.
Binary builds incorporate JUCE 8.0.15, which is separately dual-licensed under
AGPLv3 or the commercial JUCE 8 licence. No distribution basis has been
recorded for this product yet, so production binaries must not be published or
sold. See [docs/RELEASE.md](docs/RELEASE.md).

## Status

Working prototype, not publicly released. Current internal version is `0.1.2`.
The exact older `v0.1.1-rc.1` private-draft assets passed AU validation and a
limited Ableton Live 12.4.2 AU/VST3 functional smoke test, but that candidate
predates Hold Length. The current Hold revision still requires exact-artifact
DAW validation and subjective musical/sound-quality approval.

The existing `v0.1.0` and `v0.1.1-rc.1` tags are immutable history and must not
be moved, deleted, or reused. A future candidate must use a newly verified-unused
`v0.1.2-rc.N` tag after all pre-tag gates in
[docs/RELEASE.md](docs/RELEASE.md) pass. The resulting draft assets then require
the document's separate post-tag verification before any release decision.

**Implemented**

- Circular-buffer freeze with a crossfaded transition in and out
- Crossfaded loop point intended to reduce repeat-boundary clicks
- Pitch control over frozen playback (0.5x–2.0x, cubic interpolation)
- Parameters via `AudioProcessorValueTreeState` — automatable, and saved with
  the session
- Adjustable hold length, so freeze captures the most recent slice rather
  than replaying the whole capture buffer
- UI with freeze toggle, pitch, crossfade-time and hold-length sliders
- CI workflow for macOS + Windows with an offline test suite

**Not production-complete** — the granular engine the name implies is still
ahead:

- Grain envelope, density and size controls
- Time-stretching independent of pitch
- Preset system and performance bank
- Credential-tested production signing and notarization. The workflow
  implementation exists, but no signing secrets are configured and no
  real-certificate run has completed. Verified engineering artifacts remain
  ad-hoc signed on macOS and unsigned on Windows.

## Parameters

| Parameter | Range | Default |
|---|---|---|
| `freeze` | on / off | off |
| `pitch` | 0.5x – 2.0x | 1.0x |
| `crossfadeMs` | 1 – 500 ms | 30 ms |
| `holdMs` | 50 ms – 10 s | 1000 ms |

Stereo in / stereo out only. The capture buffer is 10 seconds. Engaging Freeze
pins a window of the most recent `holdMs` of that capture and loops it, clamped
to however much has actually been captured so far. Hold changes while already
frozen are latched for the next off-to-on Freeze transition.

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
