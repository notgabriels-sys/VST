# DPF port verification record

**Date:** 2026-08-24  
**Branch:** `codex/dpf-port`  
**Working checkpoint before this record:** `d86383d`

## Dependency integrity

A cold CMake FetchContent configure read back:

- DPF `4238e1c7f0351bbe488d79f0899c540543ac7583`
- Pugl `5e2621d714ddf1cb0f86e852f8ba5dffe04aa3a3`

The ordinary GitHub commit archive was rejected for UI builds because it omits
the Pugl submodule. The committed configuration uses an immutable recursive Git
fetch.

## Build and tests

The Xcode CMake generator was tested and rejected: DPF's AU wrapper source was
compiled as C++ instead of Objective-C++, causing Foundation syntax errors.
macOS CI now uses CMake's default Unix Makefiles generator.

A cold Unix Makefiles Release build completed for arm64 and x86_64 with macOS
12.0 as the minimum deployment target. DPF/Pugl emitted upstream deprecation and
integer-conversion warnings; project source emitted no recorded errors.

CTest result:

```text
3/3 tests passed
GranularFreezeEngine  PASS
GranularFreezeCore    PASS
GranularFreezeContract PASS
```

The suites cover chronological/cubic reads, scheduler launch count, fixed voice
bound, stereo alignment, zero density, non-finite automation sanitization,
transparent live capture, frozen output, unfreeze, oversized block chunking,
and exact parameter symbols/defaults. This remains narrower than the historical
JUCE suite and is not DAW evidence.

## Artifacts and package

The strict macOS packager successfully produced and re-extracted:

`build-cold-universal-make/Granular-Freeze-macOS.zip`

SHA-256:

`381d13256196a12c649483c8d51f15d70dddfa3093b7772f9d677dc836c96d90`

The ZIP contains one VST3, one AU, one CLAP, README, LICENSE, and
THIRD_PARTY_NOTICES. All three binaries contain exactly arm64 and x86_64, report
macOS 12.0 for both slices, and pass strict ad-hoc code-signature verification.
The packager was also pressure-tested with an arm64-only build and correctly
failed with exit 1 before creating a candidate.

## Audio Unit validation

The exact universal AU was installed and validated with:

`auval -v aufx GF01 GFZP`

Apple reported `AU VALIDATION SUCCEEDED`. Evidence included:

- Manufacturer `Gabriel Garcia Alonso`, product `Granular Freeze`, version 0.2.0
- Custom Cocoa view available and passed
- Exactly seven parameters with expected ranges/defaults
- Stereo 2-in/2-out layout
- Parameter retention across reset/initialization
- 64, 137, 512, and 4096-frame render checks
- 11.025, 22.05, 44.1, 48, 96, and 192 kHz render checks

DPF printed TODO messages for unsupported AU selectors/properties, and auval
warned that recommended latency, tail-time, and bypass properties are absent.
These warnings were not hidden; auval still passed every required section.

## Windows CI compatibility

GitHub's Windows runner demonstrated that DPF's unchecked download can produce
a zero-byte `glext.h` while configuration still succeeds. Before DGL is added,
the project now downloads `glext.h` and `khrplatform.h` from immutable commits
in the official Khronos OpenGL/EGL registries with TLS verification and exact
SHA-256 checks. A failed, empty, or changed transfer is therefore a configure
error rather than a later opaque compiler failure.
DPF then uses its native OpenGL extension loader; no GLEW or runtime DLL is
added.

## Remote CI and downloaded artifacts

Commit `18de27e` completed both GitHub Actions matrices successfully:

- CI Build run `32760912128`: macOS and Windows passed
- Release Candidate Build run `32760912064`: macOS and Windows passed; the
  draft-release job was intentionally skipped for the pull-request event

All Windows DSP tests passed on MSVC before packaging. The Windows build also
confirmed that DPF emits CLAP as one `.clap` file (rather than a bundle
directory), and the packager enforces that platform-specific layout.

The CI Build artifacts were downloaded, tested with `unzip -t`, extracted, and
inspected independently. Their SHA-256 values were:

- macOS: `75c94998b67e30e3bbcc180c6dbc724e2e777c7713408165bdc3353a348ec10a`
- Windows: `22b7fdb64157cc35d8ab53456f1a9bce899e6c7d8fc0eec53f00a58d51ae549a`

The macOS archive contains VST3, AU, and CLAP plus the required documents. All
three executable slices are exactly `x86_64 arm64`, and all extracted bundles
pass `codesign --verify --deep --strict`. These remain ad-hoc signatures, not
Developer ID/notarization evidence. The Windows archive contains VST3 and CLAP
plus the required documents; both binaries identify as PE32+ x86-64.

## Remaining gates

- VST3/CLAP validator tooling (none installed locally)
- Exact DPF VST3/CLAP/AU DAW discovery, editor, automation, and save/reopen
- Full listening/CPU approval
- Production signing/notarization decision and evidence, if selected
- Final third-party notice review
- Explicit merge, tag, and publication approvals
