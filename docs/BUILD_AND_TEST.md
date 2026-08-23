# Build & Test — Granular Freeze

## Verification boundary

**Implemented and automatically tested. Not yet evaluated by Gabriel in Ableton
Live or Bitwig. It is not released. No commercial validation.** Local build, offline
tests, and renderer output prove implementation/file structure only. They do
not prove musical quality, practical CPU use, a DAW result, remote macOS/Windows
CI, signing/notarization, release readiness, or commercial validation.

## Prerequisites

- **macOS**: Xcode + command line tools, CMake >= 3.22
- **Windows**: Visual Studio 2022 with the C++ workload, CMake >= 3.22
- **JUCE**: fetched automatically, see below

CMake 3.22 is a hard floor because JUCE 8 requires it.

## JUCE

You do not need to install JUCE. `CMakeLists.txt` fetches JUCE 8.0.15 via
`FetchContent` on first configure, which adds a few minutes to a cold build.

To build against a local JUCE checkout instead, set `JUCE_SOURCE_DIR`:

    cmake -S . -B build -DJUCE_SOURCE_DIR=/path/to/JUCE

Note the variable is `JUCE_SOURCE_DIR`, not `JUCE_DIR`. There is no JUCE
submodule in this repository, so `git submodule update` does nothing.

## Build

macOS:

    cmake -S . -B build -G "Xcode"
    cmake --build build --config Release --parallel

Windows (default Visual Studio generator):

    cmake -S . -B build
    cmake --build build --config Release --parallel

## Artifacts

    build/src/GranularFreeze_artefacts/Release/VST3/Granular Freeze.vst3
    build/src/GranularFreeze_artefacts/Release/AU/Granular Freeze.component   # macOS only

Note the space in the product name — quote these paths in shell commands.

## Offline automated tests

Both binaries need no host/device and print ALL TESTS PASSED (0 failures) when
they pass:

    cmake --build build --config Release --target GranularFreezeEngineTests --parallel
    "build/tests/GranularFreezeEngineTests_artefacts/Release/GranularFreezeEngineTests"
    cmake --build build --config Release --target GranularFreezeTests --parallel
    "build/tests/GranularFreezeTests_artefacts/Release/GranularFreezeTests"

GranularFreezeEngineTests covers chronological circular reads, empty/short
capture, fixed 64 voices, Size/Hann behavior, Density scheduling,
oldest/newest Position, pitch, stereo timing, determinism, normalization, and
finite bounds. GranularFreezeTests covers pass-through/frozen output, six
controls and host attachments, v0.1 migration/v0.2 round-trip, automation,
reversible transitions, and chunking.

Processor preparation checks include 44.1 kHz fallback, 48 kHz behavior with
prepared 64/512-sample blocks, a 2048-sample oversized host block, and finite
rate bounds through 384 kHz. These are test points, not DAW certification.

The CI workflow is configured to build and execute both binaries on macOS and
Windows. A local result is not a remote CI result; obtain remote evidence from
the relevant commit/PR.

## Six controls

| ID | Range | Default |
| --- | --- | --- |
| freeze | off / on | off |
| pitch | 0.50–2.00 ratio, 0.01 step | 1.00 |
| crossfadeMs | 1–500 ms, 1 ms step | 30 ms |
| grainSizeMs | 5–200 ms, 1 ms step | 80 ms |
| densityHz | 0–200 grains/s, 1 grain/s step | 20 grains/s |
| position | 0.00–1.00, 0.01 step | 1.00 |

Position 1.00 selects the newest complete grain window in valid chronological
capture. All controls are host-automatable APVTS parameters.

## v0.2 listening renders

`GranularFreezeRender` drives the processor offline and produces a fixed
listening set. It is an artifact generator, not a CI quality threshold: it
returns non-zero only for output-directory or WAV file I/O failure, or when a
rendered sample or reported measurement is non-finite. Peak, RMS, DC,
maximum adjacent-sample step, the approximate brightness proxy, and the
maximum absolute L/R difference are diagnostics only.

Build and render to a new output directory:

    cmake --build build --config Release --target GranularFreezeRender --parallel
    GRAIN_RENDER_DIR=$(mktemp -d /tmp/granular-freeze-v02-renders.XXXXXX)
    "build/tests/GranularFreezeRender_artefacts/Release/GranularFreezeRender" "$GRAIN_RENDER_DIR"

The renderer writes exactly 14 non-empty stereo WAV files at 48 kHz and 24-bit:

- Size: `size-short` (10 ms), `size-long` (180 ms).
- Density: `density-low` (5 Hz), `density-high` (120 Hz).
- Position: `position-oldest` (0.0), `position-middle` (0.5),
  `position-newest` (1.0).
- Pitch: `pitch-down` (0.5x), `pitch-unity` (1.0x), `pitch-up` (2.0x).
- Transition: `transition-short` (1 ms), `transition-normal` (30 ms),
  `transition-long` (300 ms).
- `dry-reference`, generated once with the canonical 80 ms / 20 Hz / newest /
  unity / 30 ms configuration and Freeze off.

Every v0.2 case sets all six APVTS parameters, captures live input with Freeze
off, renders with Freeze on, then returns to live mode. Its reported metrics
cover the settled frozen region beginning after two configured crossfade
lengths; the dry reference metrics cover its complete render. The
`brightness-Hz` field is an approximate zero-crossing-derived brightness proxy,
not a spectral centroid.

Audit the exact file contract after rendering:

    for NAME in size-short size-long density-low density-high position-oldest position-middle position-newest pitch-down pitch-unity pitch-up transition-short transition-normal transition-long; do
      test -s "$GRAIN_RENDER_DIR/$NAME.wav"
    done
    test -s "$GRAIN_RENDER_DIR/dry-reference.wav"
    for WAV in "$GRAIN_RENDER_DIR"/*.wav; do afinfo "$WAV" | rg '2 ch, +48000 Hz, .*24-bit'; done

Successful rendering, non-empty files, and finite metrics do not prove musical
quality and do not authorize a release. Open the WAVs in Ableton Live or Bitwig
and complete the human listening gate before any tag, pricing, sales, or public
release decision.

## AU validation (macOS)

    cp -R "build/src/GranularFreeze_artefacts/Release/AU/Granular Freeze.component" \
          ~/Library/Audio/Plug-Ins/Components/
    killall -9 AudioComponentRegistrar
    auval -v aufx GF01 GFZP

`aufx GF01 GFZP` comes from the plugin/manufacturer codes in
`src/CMakeLists.txt`. auval renders at several sample rates and block sizes and
exercises parameter automation, but it does not check that the effect sounds
right — that still needs a DAW.

## Testing in a host

Copy the `.vst3` to `~/Library/Audio/Plug-Ins/VST3/` (macOS) or
`C:\Program Files\Common Files\VST3\` (Windows), rescan in your DAW, and load it.

Worth checking by ear:

- Sample rates 44.1k / 48k / 96k and block sizes 64 / 256 / 1024.
- Freeze within the first few seconds of loading, and after a long run — the
  captured region grows until it reaches the 8-second buffer.
- Fast and slow freeze toggling, listening at the transition and at the loop
  point.
- Pitch at the extremes (0.5x and 2.0x).

Crossfade time is a parameter (`crossfadeMs`, 1-500 ms) with a slider in the
UI; change it there rather than in code. Its default lives in
`createParameterLayout()` in `src/PluginProcessor.cpp`.

For the v0.2 human listening gate, also:

- Compare short capture and full eight-second capture; check Position 0.00,
  0.50, and 1.00 against changing input, confirming that 1.00 is the newest
  complete window.
- Audition Size 5/80/200 ms, Density 0/20/200 grains/s, and Pitch
  0.50/1.00/2.00 for useful behavior, not merely finite output.
- Toggle Freeze/Unfreeze and reversals for clicks, pumping, timeline
  discontinuity, and capture-resume issues.
- Use disparate stereo material and assess stereo stability and practical CPU
  use in a realistic live set.

No human DAW result exists yet. Do not infer compatibility, quality, CPU,
release, or commercial claims from automated checks.

## Troubleshooting

- **CMake too old**: JUCE 8 needs >= 3.22.
- **Plugin does not appear**: rescan, and check the host's plugin scan log.
- Signing and notarization are not automated. See `docs/CI_SECRETS.md`.
