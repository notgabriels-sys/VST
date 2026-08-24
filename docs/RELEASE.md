# Release and publishing

## Current boundary

The DPF branch is an unreleased engineering candidate. Do not merge, tag,
publish, sell, or replace the private JUCE reference candidate until every gate
below is evidenced for one exact DPF commit and its downloaded artifacts.

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
the exact reviewed `main` commit. Draft creation does not authorize publication.

## Gumroad and other stores

Store creation/upload is manual and separate from engineering verification.
No store credential is read by the workflow. Do not upload until the production
gates and final publication approval are complete.
