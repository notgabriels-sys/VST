# Todo

## Before any release

- [ ] Evaluate in a DAW by ear — nothing here has been listened to yet
- [ ] Decide whether freeze should hold the *most recent* audio rather than
      looping the whole capture from oldest first (see PluginProcessor.cpp,
      `readPosition = writePosition` on freeze entry). Current behaviour is
      closer to a looper than a freeze.
- [ ] Exercise the release workflow — it has never run
- [ ] Code signing + notarization, or accept Gatekeeper/SmartScreen warnings

## Feature work

- [ ] Grain engine: envelope, density, size
- [ ] Time-stretch independent of pitch
- [ ] Preset system and performance bank
- [ ] Fill `presets/` with example patches

## Done

- [x] Circular buffer and freeze trigger
- [x] Parameters via APVTS, mapped to GUI controls
- [x] Crossfade on freeze/unfreeze, and at the loop point
- [x] Local build and CI on macOS and Windows
- [x] Offline test suite running in CI
- [x] GitHub repo and CI pipeline
