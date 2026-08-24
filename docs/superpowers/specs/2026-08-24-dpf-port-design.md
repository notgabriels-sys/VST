# Granular Freeze DPF Port Design

**Status:** Approved by Gabriel

**Date:** 2026-08-24

## Product contract

The DPF implementation preserves Granular Freeze 0.2's audible contract:
transparent stereo capture while live; a latched 50 ms to 10 s history window;
deterministic 64-voice granular rendering; Hann envelopes; cubic reads;
envelope-weight normalization; bounded chunk processing; reversible half-cosine
crossfades; finite output guards; and no allocation, lock, I/O, or unbounded
loop in the audio callback.

The parameter order, symbols, names, ranges, steps, and defaults are fixed:

| Index | Symbol | Name | Range | Step | Default |
| ---: | --- | --- | --- | ---: | ---: |
| 0 | `freeze` | Freeze | 0/1 | 1 | 0 |
| 1 | `pitch` | Pitch | 0.50-2.00 | 0.01 | 1.00 |
| 2 | `crossfadeMs` | Crossfade (ms) | 1-500 | 1 | 30 |
| 3 | `holdMs` | Hold (ms) | 50-10000 | 1 | 1000 |
| 4 | `grainSizeMs` | Size (ms) | 5-200 | 1 | 80 |
| 5 | `densityHz` | Density (Hz) | 0-200 | 1 | 20 |
| 6 | `position` | Position | 0-1 | 0.01 | 1 |

## Boundaries

- `AudioBufferView` is a non-owning planar float view used by the core and
  tests. It performs no allocation.
- `GrainEngine` accepts that view and contains no JUCE/DPF headers.
- `GranularFreezeCore` owns preallocated stereo vectors, capture metadata,
  transition state, parameters, and `GrainEngine`.
- `GranularFreezePlugin` maps DPF callbacks to the core and declares the exact
  host contract.
- `GranularFreezeUI` renders a compact seven-control editor and emits DPF edit
  gestures. It does not own authoritative parameter state.

## Identity and formats

- Product: `Granular Freeze`
- Maker: `Gabriel Garcia Alonso`
- Version: `0.2.0`
- CLAP ID: `com.gabrielgarciaalonso.granularfreeze`
- DPF brand ID: `GFZP`
- DPF unique ID: `GF01`
- Audio: stereo input, stereo output, no MIDI
- Targets: VST3 and CLAP on macOS/Windows; AU additionally on macOS

## Dependency and license

DPF is fetched at an immutable commit with its exact tree-pinned Pugl submodule.
The shipped notices must reproduce DPF's ISC notice and identify CLAP's MIT
attribution. No JUCE code, binary, notice, or fetched dependency is included in
the DPF artifacts.

## Verification gates

1. Framework-neutral engine/core tests pass and cover deterministic output,
   stereo alignment, capture/hold, freeze/unfreeze/reversal, parameter bounds,
   oversized blocks, and non-finite inputs.
2. DPF adapter tests prove the parameter metadata and direct audio behavior.
3. CMake builds VST3, CLAP, and AU locally on macOS.
4. Artifact bundles contain the expected binaries and no JUCE strings/files.
5. CI builds/tests macOS and Windows from a cold dependency fetch.
6. Format validators and at least one DAW instantiate the exact DPF candidate.
7. Publication remains a separate explicit approval.
