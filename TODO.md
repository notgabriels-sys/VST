# Todo

## Before any release

- [ ] Leave `v0.1.0` and the older private-draft `v0.1.1-rc.1` untouched; use
      only a newly verified-unused `v0.1.2-rc.N` tag for a future candidate
- [ ] Run fresh CI for the exact candidate on `macos-15-intel` and
      `windows-2022`; both jobs must build, test, package, and upload artifacts
- [ ] Verify both downloaded ZIPs against `SHA256SUMS.txt` and inspect their
      contents before installing them
- [ ] Run exact-`0.1.2` VST3 functional checks in Ableton and Bitwig, plus AU
      checks in Ableton or another AU-capable macOS host. The older
      `v0.1.1-rc.1` assets passed a limited Ableton smoke test, but they predate
      Hold Length.
- [ ] Run the cross-version AU automation migration smoke from
      `docs/BUILD_AND_TEST.md`: save legacy Pitch/Freeze/Crossfade automation
      with the exact `v0.1.1-rc.1` AU, reopen it with exact `0.1.2`, verify each
      lane mapping, and separately automate Hold.
- [ ] Evaluate the exact candidate by ear and make the owner-controlled
      subjective musical/sound-quality decision
- [ ] Confirm the `holdMs` default (1000 ms) feels right by ear. Freeze now
      holds a recent window rather than looping the whole capture, per
      docs/PRODUCT_SPEC.md; the range and default are a judgement call.
- [ ] Validate the exact macOS and Windows assets from the draft release in
      target DAWs before publishing
- [ ] Confirm the JUCE 8 licensing basis for the intended distribution model
- [ ] Configure GitHub Secrets only after the required credentials exist, then
      run and verify the already-implemented Developer ID/notarization and
      Authenticode paths before public distribution. Ad-hoc macOS signatures
      and unsigned Windows builds are for engineering testing only.
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
- [x] Hold uses a later JUCE parameter-version generation intended to preserve
      the three legacy AU parameter indices; exact cross-version DAW migration
      validation remains open above
- [x] The physical capture buffer matches the advertised 10-second Hold range
