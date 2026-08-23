# Todo

## Remaining gates before any release decision

- [ ] Gabriel evaluates the build by ear in Ableton Live or Bitwig.
- [ ] Run fresh CI for the exact candidate on `macos-15-intel` and
      `windows-2022`; both jobs must build, run both offline suites, package,
      and upload artifacts.
- [ ] Exercise the v0.2 release-candidate workflow; it has not been run for
      this Grain Core.
- [ ] Verify both downloaded ZIPs against `SHA256SUMS.txt` and inspect their
      contents before any installation or DAW validation.
- [ ] Validate the exact macOS and Windows candidate assets in target DAWs.
- [ ] Confirm the JUCE 8 licensing basis for the intended distribution model.
- [ ] Implement and verify Developer ID signing and notarization on macOS and
      Authenticode signing on Windows before public distribution.
- [ ] Make commercial and release decisions after the listening and workflow
      gates. No price, date, store, or commercial validation is set.
- [ ] Obtain explicit approval before creating a tag, publishing a GitHub
      release, or uploading release files to a storefront.

## Deferred feature work

- [ ] Time-stretch independent of pitch.
- [ ] Presets and a performance bank.
- [ ] Feedback or recursive grain capture.
- [ ] Random scatter, jitter, or probability.

## Done and evidenced locally

- [x] Grain Engine: deterministic fixed 64-voice playback, Hann envelopes,
      cubic pitched reads, and overlap normalization.
- [x] Size (grainSizeMs): 5–200 ms, default 80 ms.
- [x] Density (densityHz): 0–200 grains/s, default 20 grains/s; zero settles
      to silence and positive Density launches deterministically.
- [x] Position (position): 0.00–1.00, default 1.00; chronological
      oldest-to-newest complete windows, with 1.00 selecting the newest.
- [x] Eight-second chronological capture, immutable frozen view, and reversible
      Freeze/Unfreeze transitions.
- [x] Six APVTS parameters, host automation attachments, v0.1 state migration,
      v0.2 state round-trip, automation and finite-output evidence.
- [x] Two offline test binaries and the fourteen-file renderer listening set.
- [x] CI workflow configured for both offline suites on pinned macOS and
      Windows runners, with strict platform packaging and artifact failure
      gates. Remote execution remains unverified for this merge.

Local automated evidence does not replace remote macOS/Windows CI, Gabriel's
DAW listening evaluation, signing/notarization, or commercial/release work.
