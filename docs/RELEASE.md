# Release and publishing

## Current boundary

`v0.2.0-rc.5` is the current private draft prerelease. Its immutable annotated
tag resolves to `4d031a1775bd7b9620f6f332f45186f6b64dae28`, the hardened `main`
commit that removes DPF's unnecessary AU network and arbitrary file
read/write resource declaration. The [release workflow run 32820071847](https://github.com/notgabriels-sys/VST/actions/runs/32820071847)
passed the annotated-tag/current-`main` guard, both platform builds and
behavioural suites, strict packaging, and draft creation. The downloaded
`rc.5` assets matched their `SHA256SUMS.txt` file, both ZIPs passed archive
testing, the macOS AU resource declaration was absent, and the exact
downloaded AU passed `auval` with `AU VALIDATION SUCCEEDED`.

The release asset SHA-256 values read back from the draft are:

- macOS ZIP: `51eebb0d6f3672060937d75c2c276a5e865e424da867d59b8a88a1d449735c51`
- Windows ZIP: `db55f4fed674a837513bffc58369ecd7501374c381dfb3a502df9b7e45176858`
- `SHA256SUMS.txt`: `ae69a51b36455a5feca4f3c491a73755bd61d159e9830876f4c31986bb9b65e8`

`rc.5` supersedes `rc.4`; `rc.4` was valid but was created before the AU
metadata hardening. The first `rc.3` tag was intentionally left immutable after
its fail-closed tag-source check exposed a checkout/ref assumption; it has no
release assets and must not be used. The earlier `v0.2.0-rc.1`, `v0.2.0-rc.2`,
failed `v0.2.0-rc.3`, and superseded `v0.2.0-rc.4` candidates must not be used
as the final release artifact.

The current candidate is still unsigned, not notarized, and not approved for
sale or public distribution. Treat it as an exact-artifact DAW and listening
candidate only. Do not publish, sell, or replace the draft until the remaining
gates below are evidenced and Gabriel gives explicit publication approval.

## Candidate workflow

The release workflow builds macOS universal VST3/AU/CLAP and Windows x86-64
VST3/CLAP, runs all registered tests, packages strict archives, records signing
status, and uploads private Actions artifacts. Only a newly created
`v0.2.0-rc.*` tag contained in `main` may create a private draft prerelease.
Manual signing tests are artifact-only and require explicit dispatch from
`main`. Partial secret sets fail closed.

## Framework distribution gate

Repository-authored source is MIT. DPF, Pugl, and CLAP components are
permissively licensed with attribution requirements. Every archive must contain
`LICENSE` and `THIRD_PARTY_NOTICES.md`; the exact dependency revisions and
notices must be reviewed against the candidate. The DPF candidate must contain
no JUCE binary or source material.

## Production gates

- Fresh macOS and Windows CI on the exact commit
- Complete test suite with zero failures
- Downloaded ZIP checksums and content verification
- VST3, AU, and CLAP validator results where tooling exists
- Exact-artifact DAW discovery, instantiation, editor, automation, and
  save/reopen evidence
- Freeze, Hold chronology, Size/Density/Pitch/Position, transitions, stereo,
  CPU, and listening approval
- Reviewed third-party notices
- Explicit signing/notarization decision and verified status files
- Explicitly approved scope, price, listing, support terms, and delivery files
- Final explicit approval to publish

Signing is not required for continued private development. If public macOS
distribution is chosen, Developer ID/notarization credentials remain an
owner-controlled external gate. Windows Authenticode is likewise separate.

## Tag safety

Never move, reuse, or force-push an existing tag. Before a candidate tag,
confirm it is absent locally, remotely, and from GitHub Releases; annotate only
the exact reviewed `main` commit. The release workflow rejects lightweight tags
and requires the annotated tag's commit to equal the current `origin/main` tip
at workflow start. Draft creation does not authorize publication.

## Gumroad and other stores

Store creation/upload is manual and separate from engineering verification.
No store credential is read by the workflow. Do not upload until the production
gates and final publication approval are complete.
