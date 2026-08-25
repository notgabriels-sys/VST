# Build and test Granular Freeze DPF

## Requirements

- CMake 3.22 or newer
- C++17 compiler
- Git, including recursive submodule support for DPF's pinned Pugl dependency
- macOS: Xcode command-line tools
- Windows: Visual Studio 2022 C++ workload

DPF is fetched at commit
`4238e1c7f0351bbe488d79f0899c540543ac7583`; that tree pins Pugl at
`5e2621d714ddf1cb0f86e852f8ba5dffe04aa3a3`. For an audited local checkout:

```sh
cmake -S . -B build -DGRANULAR_FREEZE_DPF_SOURCE_DIR=/path/to/DPF
```

## Local development build

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Expected outputs are under `build/bin`:

- `GranularFreeze.vst3`
- `GranularFreeze.clap`
- `GranularFreeze.component` on macOS

The registered tests are `GranularFreezeEngine`, `GranularFreezeCore`, and
`GranularFreezeContract`. `No tests were found` is a failure.

## Universal macOS candidate

```sh
cmake -S . -B build-universal \
  '-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64' \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0
cmake --build build-universal --config Release --parallel
ctest --test-dir build-universal -C Release --output-on-failure
bash scripts/package_mac.sh build-universal build-universal/Granular-Freeze-macOS.zip
```

The packager requires exactly one VST3, AU, and CLAP bundle; exact arm64 and
x86_64 slices; macOS 12.0 minimum; valid signatures; README, project license,
and third-party notices; a clean ZIP root; and successful extraction/recheck.
It also rejects an AU `resourceUsage` declaration. The DPF exporter currently
emits broad network and arbitrary file read/write claims, but Granular Freeze
performs no network or file I/O; the project removes those claims immediately
after AU metadata generation and the packager checks the extracted final bytes.

## Windows candidate

```powershell
cmake -S . -B build
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
./scripts/package_win.ps1 -BuildDirectory build -OutputZip build/Granular-Freeze-Windows.zip
```

The Windows packager requires exactly one x86-64 VST3 and one x86-64 CLAP
bundle plus all required documents.

## macOS validation

After packaging, extract the exact ZIP being evaluated and install its bundles:

```sh
cp -R GranularFreeze.component ~/Library/Audio/Plug-Ins/Components/
cp -R GranularFreeze.vst3 ~/Library/Audio/Plug-Ins/VST3/
cp -R GranularFreeze.clap ~/Library/Audio/Plug-Ins/CLAP/
killall -9 AudioComponentRegistrar 2>/dev/null || true
auval -v aufx GF01 GFZP
```

Inspect each executable with `file`, `lipo -archs`, `vtool -show-build`,
`codesign --verify --deep --strict`, and `otool -L`. Validator success does not
prove DAW behavior or sound quality.

## DAW gate

Using the exact extracted candidate, record host/OS/format, sample rate, block
size, and commit/hash. Verify discovery, instantiation, editor opening, all
seven controls, automation, freeze/unfreeze/reversal, short/default/long Hold,
Position endpoints, stereo stability, project save/reopen, and realistic CPU.
No tag or publication follows automatically from this gate.
