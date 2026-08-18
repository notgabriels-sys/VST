Build & Test — Granular Freeze (prototype)

This document explains how to build the plugin locally and run a quick smoke test. It assumes the repository root is the project folder and that CMake-based JUCE setup is used.

Prerequisites
- macOS: Xcode (command line tools), CMake (>=3.20), Ninja (optional)
- Windows: Visual Studio 2022 with C++ workload, CMake
- JUCE: clone JUCE into third_party/JUCE or point CMake with -DJUCE_DIR

Quick steps (macOS)
1. Clone repository and ensure JUCE is available:
   git clone https://github.com/notgabriels-sys/VST.git
   cd VST

   # Option A: add JUCE as a submodule
   git submodule update --init --recursive

   # Option B: clone JUCE manually
   git clone https://github.com/juce-framework/JUCE.git third_party/JUCE

2. Configure and build (Xcode generator):
   mkdir -p build
   cd build
   cmake .. -G "Xcode" -DJUCE_DIR=../third_party/JUCE -DCMAKE_BUILD_TYPE=Release
   cmake --build . --config Release --parallel

3. Locate artifacts (example):
   # VST3 bundle
   find build -name "*.vst3" -maxdepth 3
   # AudioUnit (component) bundle on macOS
   find build -name "*.component" -maxdepth 3

4. Test in a host
   - Copy the .vst3 bundle to ~/Library/Audio/Plug-Ins/VST3/ or use the host's plugin scan locations.
   - For AU, install to ~/Library/Audio/Plug-Ins/Components/ and run AU validation if desired.
   - Open your DAW (Ableton, Bitwig, Reaper) and load the plugin. Try toggling Freeze and Pitch.

Quick steps (Windows)
1. Ensure Visual Studio and CMake are installed.
2. From project root:
   mkdir build
   cd build
   cmake .. -G "Visual Studio 17 2022" -A x64 -DJUCE_DIR=..\third_party\JUCE -DCMAKE_BUILD_TYPE=Release
   cmake --build . --config Release
3. Locate generated .dll or .vst3 files under build/ and copy to your VST scan path for testing.

Notes & Troubleshooting
- If CMake cannot find JUCE, set the JUCE_DIR variable to the path where JUCE's CMakeLists.txt lives.
- If the host does not show the plugin, rescan and check console/log for plugin scan errors.
- For automation and CI, see .github/workflows/ci.yml. Signing and notarization steps are intentionally omitted and must be added when secure secrets (Apple notarization key, Windows code-signing PFX) are available.

Recommended local tests
- Test at multiple sample rates (44.1k, 48k, 96k) and buffer sizes (64, 256, 1024) to validate buffer wrapping and read-head correctness.
- Toggle freeze rapidly and slowly to listen for clicks; if clicks persist, increase the crossfadeMs value in PluginProcessor::prepareToPlay.

If you want, I can attempt to run a CI build here and upload artifacts, but the execution environment may not have Xcode or Visual Studio installed. The GitHub Actions workflow will build on macOS and Windows runners automatically when you push.