# Build & Test - Granular Freeze

## Prerequisites

- **macOS**: Xcode and command line tools, CMake >= 3.22
- **Windows**: Visual Studio 2022 with the C++ workload, CMake >= 3.22
- **JUCE**: fetched automatically as described below

CMake 3.22 is a hard floor because JUCE 8 requires it.

## JUCE

You do not need to install JUCE. `CMakeLists.txt` fetches the pinned JUCE
8.0.15 release on first configure, which can add a few minutes to a cold build.

To use a local JUCE checkout, set the project-specific override:

    cmake -S . -B build -DGRANULAR_FREEZE_JUCE_SOURCE_DIR=/path/to/JUCE

Do not use `JUCE_SOURCE_DIR` as caller input. FetchContent reserves it
internally. There is no JUCE submodule in this repository.

## Build

The release-candidate macOS build is universal2 and targets macOS 12.0 or
newer:

    cmake -S . -B build -G Xcode \
      '-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64' \
      -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0
    cmake --build build --config Release --parallel

On Windows, preserve CMake's default Visual Studio generator:

    cmake -S . -B build
    cmake --build build --config Release --parallel

The plugin bundles are normally written under:

    build/src/GranularFreeze_artefacts/Release/VST3/Granular Freeze.vst3
    build/src/GranularFreeze_artefacts/Release/AU/Granular Freeze.component   # macOS only

The product name contains a space, so quote these paths in shell commands.

## Offline behavioral tests

`tests/PluginTests.cpp` drives the AudioProcessor with generated signals. It
needs neither a host nor an audio device. Build it, locate the executable
without assuming a generator-specific output path, and execute it.

macOS or another POSIX shell:

    cmake --build build --config Release --target GranularFreezeTests --parallel
    test_binary=$(find build -type f -name GranularFreezeTests -print -quit)
    test -n "$test_binary" || { echo "GranularFreezeTests not found" >&2; exit 1; }
    "$test_binary"

Windows PowerShell:

    cmake --build build --config Release --target GranularFreezeTests --parallel
    $testBinary = Get-ChildItem build -Recurse -File -Filter GranularFreezeTests.exe | Select-Object -First 1
    if (-not $testBinary) { throw "GranularFreezeTests.exe not found" }
    & $testBinary.FullName

The nine assertions cover non-silent pass-through, stereo alignment,
freeze-held audio, click-safe freeze and unfreeze transitions, pitch-rate
behavior, finite output, and a sane output range. Exit code is non-zero on
failure. Add a behavioral case whenever the DSP changes.

The optional `GranularFreezeRender` target writes a dry reference plus five
freeze cases and prints objective measurements. It is a listening aid, not a
substitute for a DAW test:

    cmake --build build --config Release --target GranularFreezeRender --parallel

Locate and invoke it the same way as the test binary, passing an output
directory as its only argument.

## Package the candidate

Use the repository scripts rather than manually zipping build directories:

    bash ./scripts/package_mac.sh build build/Granular-Freeze-macOS.zip

    pwsh -File ./scripts/package_win.ps1 -BuildDirectory build -OutputZip build/Granular-Freeze-Windows.zip

The macOS script requires exactly one AU and one VST3, exact arm64 and x86_64
slices, macOS 12.0 in every slice, valid ad-hoc signatures, required documents,
and one clean archive root. The Windows script requires exactly one outer VST3
bundle, its expected x86_64 binary, required documents, and one clean archive
root. Both fail if an expected artifact is absent. These packages are not
production signed or notarized.

## AU validation (macOS)

    cp -R "build/src/GranularFreeze_artefacts/Release/AU/Granular Freeze.component" \
          ~/Library/Audio/Plug-Ins/Components/
    killall -9 AudioComponentRegistrar
    auval -v aufx GF01 GFZP

`aufx GF01 GFZP` comes from the plugin and manufacturer codes in
`src/CMakeLists.txt`. `auval` exercises loading and rendering, but it does not
establish that the effect sounds right.

## DAW smoke test

Install the exact extracted draft assets, rescan in Reaper or another host,
and record the host version, OS version, format, sample rate, and block size.
At minimum:

- Load VST3; on macOS also load AU in a host that supports it.
- Confirm dry pass-through before Freeze.
- Toggle Freeze, mute the input, and confirm captured audio continues.
- Move Pitch through 0.5x, 1.0x, and 2.0x and confirm playback rate responds.
- Exercise short and long crossfade settings and repeated Freeze transitions.
- Listen for clicks, dropouts, runaway level, denormals, or other glitches.
- Save, close, and reopen the session to check parameter restoration.
- Repeat representative checks at 44.1, 48, and 96 kHz and at small and large
  block sizes.

Do not mark the candidate validated until this has been performed by ear. See
`docs/RELEASE.md` for the complete release gate.

## Troubleshooting

- **CMake too old**: JUCE 8 needs >= 3.22.
- **Skip tests and render utility**: configure with
  `-DGRANULAR_FREEZE_BUILD_TESTS=OFF`.
- **Plugin does not appear**: rescan and check the host's plugin scan log.
- **Signing warnings**: production signing and notarization are not implemented;
  the macOS candidate is ad-hoc signed and Windows is unsigned. See
  `docs/CI_SECRETS.md`.
