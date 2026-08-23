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

The twelve assertions cover non-silent pass-through, stereo alignment,
freeze-held audio, click-safe freeze and unfreeze transitions, pitch-rate
behavior, finite output, sane output range, recent-window Hold behavior, stable
AU parameter generations, and a sample-exact check that the physical Hold
endpoint reaches 10,000 ms. Exit code is non-zero on failure. Add or update a
behavioral case whenever DSP or host-visible parameter behavior changes.

The optional `GranularFreezeRender` target writes a dry reference plus nine
freeze cases: six Hold lengths, octave-up and octave-down pitch cases, and a
minimum-crossfade case. It prints objective measurements and produces listening
material, but it is not a substitute for exact-artifact DAW validation or
owner sound-quality approval:

    cmake --build build --config Release --target GranularFreezeRender --parallel

Locate and invoke it the same way as the test binary, passing an output
directory as its only argument.

## Package the candidate

Use the repository scripts rather than manually zipping build directories:

    bash ./scripts/package_mac.sh build build/Granular-Freeze-macOS.zip

    pwsh -File ./scripts/package_win.ps1 -BuildDirectory build -OutputZip build/Granular-Freeze-Windows.zip

In default engineering mode, the macOS script requires exactly one AU and one
VST3, applies and verifies ad-hoc bundle seals, verifies exact arm64 and x86_64
slices and macOS 12.0 deployment targets, and validates the final extracted
archive. With `--preserve-signature`, it does not re-sign the bundles, allowing
the surrounding release workflow to preserve Developer ID signatures and
stapled-ticket data. The workflow separately verifies Developer ID identity,
hardened runtime, and stapled tickets after packaging and extraction. It uses
preservation mode after signing and again after notarization.

The Windows packager validates the exact outer VST3 bundle, expected x86_64
binary, required documents, and archive root. Both scripts fail if an expected
artifact is absent. The credential-dependent production paths are implemented
but have never run with real credentials; all currently verified artifacts
remain ad-hoc signed on macOS and unsigned on Windows.

## AU validation (macOS)

First extract the exact Actions or draft ZIP being evaluated. Replace the path
below with that extraction directory. Move any existing component aside, then
use `ditto`; `cp -R` can merge bundles and leave stale files behind.

    candidate_root="/absolute/path/to/extracted/Granular Freeze-macOS"
    candidate_component="$candidate_root/Granular Freeze.component"
    installed_component="$HOME/Library/Audio/Plug-Ins/Components/Granular Freeze.component"
    backup_component="${installed_component}.before-v0.1.2"
    test -d "$candidate_component"
    test ! -e "$backup_component"
    if test -e "$installed_component"; then mv "$installed_component" "$backup_component"; fi
    ditto "$candidate_component" "$installed_component"
    diff -qr "$candidate_component" "$installed_component"
    codesign --verify --deep --strict "$installed_component"
    killall -9 AudioComponentRegistrar
    auval -v aufx GF01 GFZP

`aufx GF01 GFZP` comes from the plugin and manufacturer codes in
`src/CMakeLists.txt`. `auval` exercises loading and rendering, but it does not
establish that the effect sounds right.

Recorded evidence: the installed AU from the older `v0.1.1-rc.1` private draft
passed `auval -v aufx GF01 GFZP` outside the restricted filesystem sandbox with
exit status 0 and `AU VALIDATION SUCCEEDED`. Sandboxed empty-catalog failures
are non-authoritative. That candidate predates Hold Length, so rerun `auval`
against the exact future candidate and require four global parameters in the
preserved order: Pitch, Freeze, Crossfade, Hold.

## DAW smoke test

Install the exact extracted artifacts being evaluated: use GitHub draft assets
for a tagged candidate and exact Actions artifacts for an unreleased branch or
`main` revision. Record the commit/tag, artifact identity, host version, OS
version, format, sample rate, and block size. At minimum:

- Load VST3; on macOS also load AU in a host that supports it.
- Confirm the corrected AU exposes four product parameters in the preserved
  order Pitch, Freeze, Crossfade, Hold. Confirm VST3 exposes those four plus
  JUCE's wrapper-owned Bypass parameter.
- Confirm dry pass-through before Freeze.
- Toggle Freeze, mute the input, and confirm captured audio continues.
- Move Pitch through 0.5x, 1.0x, and 2.0x and confirm playback rate responds.
- Move Hold through representative short, default, and long values such as
  80 ms, 1000 ms, and 9000 ms. Refreeze newly captured material and confirm the
  held window changes and continues to select recent audio rather than replaying
  the entire capture history.
- Confirm the intentional latch behavior: moving Hold while already frozen
  affects the next off-to-on Freeze transition, not the active held window.
- Exercise short and long crossfade settings and repeated Freeze transitions.
- Listen for clicks, dropouts, runaway level, denormals, or other glitches.
- Save, close, and reopen the session with a distinctive non-default Hold value
  to check parameter restoration and automation. Only parameter values are
  serialized; the captured frozen audio itself is not restored.
- Run a separate cross-version AU automation migration smoke. With the exact
  older `v0.1.1-rc.1` AU installed, save a session containing distinct Pitch,
  Freeze, and Crossfade automation lanes. Close the host, replace that AU with
  the exact `0.1.2` candidate, reopen the session, and verify every legacy lane
  still controls the same parameter. Then add and exercise a separate Hold
  automation lane. Record the old/new artifact hashes and observed lane mapping.
- Repeat representative checks at 44.1, 48, and 96 kHz and at small and large
  block sizes.

Recorded functional result: the exact older `v0.1.1-rc.1` AU and VST3 passed a
limited Ableton Live 12.4.2 smoke test at 48 kHz, including loading, dry
pass-through, Freeze hold with silent input, Pitch response, crossfade-control
operation, rapid automation, bypass/re-enable, remove/reload, and zero
host-reported dropouts. That test did not cover Hold, Bitwig, session
restoration, all listed rates/block sizes, or owner sound-quality judgment.

Do not mark the current revision or production candidate fully validated until
the remaining checks and every gate in `docs/RELEASE.md` are complete.

## Troubleshooting

- **CMake too old**: JUCE 8 needs >= 3.22.
- **Skip tests and render utility**: configure with
  `-DGRANULAR_FREEZE_BUILD_TESTS=OFF`.
- **Plugin does not appear**: rescan and check the host's plugin scan log.
- **Signing warnings**: production signing, notarization, and Authenticode are
  implemented in the release workflow but have never executed with real
  credentials. Without complete secret sets, verified macOS artifacts are
  ad-hoc signed and Windows artifacts are unsigned. See `docs/CI_SECRETS.md`.
