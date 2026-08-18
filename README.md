Granular Freeze — Creative Time/Freeze & Granular Texture Engine

Overview

This repository is a scaffold for "Granular Freeze", a cross-platform VST3 + AU audio plugin focused on live creative time/freeze and granular texture generation for Ableton and Bitwig users.

Goals for initial release
- Real-time freeze / buffer hold of audio with granular re-synthesis and time-stretching
- Performance-oriented UI for live tweaking (freeze, grain size, density, pitch shift, feedback)
- Preset system with performance bank (8 quick-load slots) and example project
- macOS: AU + VST3. Windows: VST3.

What is included in this repo
- Minimal JUCE/CMake project skeleton (expects JUCE added as submodule or JUCE_DIR provided)
- Basic plugin processor/editor skeleton (placeholder algorithm) in src/
- GitHub Actions CI workflow template for macOS + Windows builds (.github/workflows/ci.yml)
- Product spec and CI secret checklist in docs/
- Packaging script placeholders in scripts/
- Presets folder for example patches

Next steps (recommended)
1. Review product spec: docs/PRODUCT_SPEC.md
2. Add JUCE (submodule or set JUCE_DIR) and implement granular engine in src/
3. Provide signing & notarization secrets for CI (see docs/CI_SECRETS.md)
4. Run local builds on macOS and Windows to validate and iterate UI/algorithm
5. Create GitHub repository and push; use GitHub Actions to produce artifacts
6. Publish binaries to GitHub Releases and Gumroad; prepare marketing assets (screens, demo audio/video)

Paths
- Project root: /Users/notgabriels/Documents/VS Code/granular-freeze-plugin
- Docs: /Users/notgabriels/Documents/VS Code/granular-freeze-plugin/docs
- CI workflow: /Users/notgabriels/Documents/VS Code/granular-freeze-plugin/.github/workflows/ci.yml

Author: Gabriel García Alonso (project owner)

Notes
- This scaffold avoids embedding secrets. Signing and notarization must be added to CI as secrets under your control.
- The plugin code uses JUCE; include JUCE as a submodule or set JUCE_DIR in CI/locally.

License: MIT (see LICENSE)
