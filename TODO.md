# Todo

## Before any release

- [ ] Leave the obsolete `v0.1.0` tag untouched; use `v0.1.1-rc.1` for this
      engineering candidate only
- [ ] Run fresh CI for the exact candidate on `macos-15-intel` and
      `windows-2022`; both jobs must build, test, package, and upload artifacts
- [ ] Verify both downloaded ZIPs against `SHA256SUMS.txt` and inspect their
      contents before installing them
- [ ] Evaluate in a DAW by ear — nothing here has been listened to yet
- [ ] Decide whether Freeze should hold a shorter recent window rather than
      loop the captured span. Current behaviour is closer to a looper than a
      conventional short freeze.
- [ ] Validate the exact macOS and Windows assets from the draft release in
      target DAWs before publishing
- [ ] Confirm the JUCE 8 licensing basis for the intended distribution model
- [ ] Implement and verify Developer ID signing + notarization on macOS and
      Authenticode signing on Windows before public distribution. Ad-hoc macOS
      signatures and unsigned Windows builds are for private candidate testing
      only.
- [ ] Obtain explicit approval before publishing a GitHub release or manually
      uploading release files through Gumroad's authenticated Content UI

## Feature work

- [ ] Grain engine: envelope, density, size
- [ ] Time-stretch independent of pitch
- [ ] Preset system and performance bank
- [ ] Fill `presets/` with example patches

## Done

- [x] Circular buffer and freeze trigger
- [x] Parameters via APVTS, mapped to GUI controls
- [x] Crossfade on freeze/unfreeze, and at the loop point
- [x] CI workflows configured for macOS and Windows
- [x] Offline test suite wired into both CI matrix jobs
- [x] GitHub repo and CI pipeline
