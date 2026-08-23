# Build & Test — Granular Freeze

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

## Tests

`tests/PluginTests.cpp` drives the AudioProcessor directly with generated
signals and asserts on the output. No host and no audio device are needed, and
it runs in CI on both platforms.

    cmake --build build --config Release --target GranularFreezeTests --parallel
    "build/tests/GranularFreezeTests_artefacts/Release/GranularFreezeTests"

It covers pass-through fidelity, L/R alignment while frozen, that freeze holds
audio when the input goes silent, that neither freeze nor unfreeze produces a
discontinuity, the pitch ratio, and the absence of NaN/Inf. Exit code is
non-zero on failure.

Both defects found during the initial bring-up — freeze outputting silence, and
a click at the loop point — were invisible to the compiler and to auval, and
were caught here. Add a case when you change the DSP.

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

## Troubleshooting

- **CMake too old**: JUCE 8 needs >= 3.22.
- **Plugin does not appear**: rescan, and check the host's plugin scan log.
- Signing and notarization are not automated. See `docs/CI_SECRETS.md`.
