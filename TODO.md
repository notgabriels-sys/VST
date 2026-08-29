# Granular Freeze release tracker

## Current private candidate

- [x] `v0.2.0-rc.5` is the current immutable private draft prerelease. Its
      annotated tag resolves to `4d031a1775bd7b9620f6f332f45186f6b64dae28`.
- [x] The
      [rc.5 release workflow](https://github.com/notgabriels-sys/VST/actions/runs/32820071847)
      passed its macOS build, Windows build, and draft-creation jobs. It built,
      ran all three behavioral suites, packaged VST3/AU/CLAP for macOS and
      VST3/CLAP for Windows, and created the private draft.
- [x] The exact release ZIPs and `SHA256SUMS.txt` were downloaded and verified
      on 2026-08-29. Both checksums matched; both ZIPs passed integrity tests;
      each archive has one expected root, required project and third-party
      notices, and the expected plug-in payloads.
- [x] The extracted macOS VST3, AU, and CLAP binaries are universal arm64 and
      x86_64 builds targeting macOS 12.0. The AU has no `resourceUsage`
      declaration. The extracted Windows VST3 and CLAP binaries are PE32+
      x86-64.
- [x] The candidate manifests record `mac_signed=no`, `mac_notarized=no`, and
      `win_signed=no`. The macOS bundles are ad-hoc signed only; this candidate
      is not suitable for public distribution or sale.

See [docs/RELEASE.md](docs/RELEASE.md) for the source commit, asset digests,
candidate history, and release workflow rules. See
[docs/DPF_PORT_VERIFICATION.md](docs/DPF_PORT_VERIFICATION.md) for the DPF
port, platform build, and Audio Unit validation record.

## Remaining release gates

- [ ] Perform exact-asset DAW tests in Ableton Live or Bitwig for VST3, AU, and
      CLAP where each format is supported. Record host and OS version, sample
      rate, block size, candidate hash, discovery, instantiation, editor,
      automation, and project save/reopen results.
- [ ] Evaluate Freeze/unfreeze/reversal, short/default/long Hold, Pitch, Size,
      Density, Position endpoints, stereo behavior, realistic CPU use, and
      audible behavior by ear. Give an explicit owner sound-quality approval.
- [ ] Run available VST3 and CLAP validator tools against the exact candidate
      assets and resolve any substantive findings.
- [ ] Review [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) against the
      exact DPF, Pugl, and CLAP material in the candidate archive.
- [ ] Decide whether public distribution requires Developer ID/notarization and
      Windows Authenticode signing. If it does, configure the required GitHub
      Actions secrets and first run the artifact-only signing test described in
      [docs/CI_SECRETS.md](docs/CI_SECRETS.md).
- [ ] Approve final product scope, price, listing copy, support terms, and
      delivery assets. Store uploads remain manual and separate from the
      workflow.
- [ ] Give explicit final approval before publishing a GitHub release or
      uploading the product to any storefront.

## Candidate safety

- [ ] Keep `v0.1.0`, `v0.1.1-rc.1`, and every `v0.2.0-rc.*` tag immutable.
      Do not publish, replace, or sell the `rc.5` draft.
- [ ] If code changes require another candidate, use a new verified-unused
      `v0.2.0-rc.N` annotated tag only after fresh CI and the relevant
      validation gates have been completed.

## Deferred product work

- [ ] Time-stretch independent of pitch.
- [ ] Presets and a performance bank.
- [ ] Feedback or recursive grain capture.
- [ ] Random scatter, jitter, or probability.

The project now uses DPF, not JUCE. Automated and packaging evidence is
valuable engineering evidence, but it does not replace host validation,
listening evaluation, signing, commercial decisions, or publication approval.
