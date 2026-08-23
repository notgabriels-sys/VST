# Release and publishing

## Candidate identity

The internal project version is `0.2.0`. The release workflow accepts newly
created candidate tags matching `v0.2.0-rc.*`.

Tags `v0.1.0` and `v0.1.1-rc.1` already exist and are immutable history. The
older `v0.1.1-rc.1` tag points to commit `561aaf8`; its associated GitHub
Release remains a private, unpublished draft candidate that predates Hold
Length and the Grain Core. Do not move, delete, reuse, or force-push either
tag. A future candidate must use the next verified-unused `v0.2.0-rc.N` tag
after all pre-tag gates pass.

## What the workflow does

`.github/workflows/release.yml` runs build/package validation for qualifying
pull requests, manual dispatches, and newly created tag pushes matching
`v0.2.0-rc.*`. Only a newly created matching tag push can run the draft-release
job. A manual run with `sign_artifacts=true` must be dispatched from `main` and
is artifact-only; it cannot create or mutate a release.

1. Validate that signed manual tests run from `main`, and that candidate tags
   are newly created and point to a commit contained in `origin/main`.
2. Build on `macos-15-intel` and `windows-2022`, producing universal
   arm64+x86_64 AU/VST3 bundles for macOS 12 or newer and an x86-64 Windows
   VST3.
3. Run both offline behavioral suites on both platforms: the Grain Engine
   contract and the complete processor/editor/state contract.
4. For matching tag pushes, or an opted-in manual signing test, conditionally
   attempt Developer ID signing, Apple notarization/stapling, and Windows
   Authenticode signing. Complete secret sets are required for signed manual
   runs; partial sets fail closed. No credential-dependent path has executed.
5. Package and upload `Granular-Freeze-macOS.zip` plus
   `signing-status-macos.txt` in `granular-freeze-macos`, and
   `Granular-Freeze-Windows.zip` plus `signing-status-windows.txt` in
   `granular-freeze-windows`.
6. For a newly created candidate tag, refuse to mutate any existing release,
   require both platforms to be either fully signed or fully unsigned, generate
   `SHA256SUMS.txt`, and create a private draft prerelease whose notes report
   verified status-file results.

The workflow conditionally reads the production-signing secrets documented in
`docs/CI_SECRETS.md`. Pull requests and ordinary unsigned manual builds skip
signing. A candidate-tag run with every secret set absent records an ad-hoc
signed macOS artifact and an unsigned Windows artifact. Partial secret sets
fail, and `sign_artifacts=true` requires all nine implemented secrets. The
credential-dependent paths have never run.

The workflow does not read Gumroad credentials. Every generated GitHub release
remains a private draft prerelease, while a pushed Git tag is visible in the
public repository.

## Create an engineering candidate

Do not recreate or move `v0.1.1-rc.1`. Before creating any `v0.2.0` candidate:

1. Confirm fresh macOS and Windows CI is green on the exact intended `main`
   commit.
2. Complete exact-artifact AU/VST3 DAW validation for all seven controls,
   Hold-window chronology, Grain Core behavior, transitions, and session
   restoration, and record the evidence.
3. Select and document one JUCE 8 distribution basis for the exact candidate.
4. Configure all nine signing/notarization secret names without exposing their
   values.
5. With separate explicit approval, run `workflow_dispatch` from `main` with
   `sign_artifacts=true`.
6. Require verified `mac_signed=yes`, `mac_notarized=yes`, and `win_signed=yes`
   results, then independently inspect the downloaded artifacts.
7. Choose the next candidate name matching `v0.2.0-rc.*`, such as
   `v0.2.0-rc.1`, only after confirming that no local tag, remote tag, or
   GitHub release already uses it.
8. Obtain a separate explicit approval to annotate the exact verified `main`
   commit and push only that new tag.

A newly created matching tag triggers another complete build and may create a
new private draft prerelease. Never reuse, move, or force-push a candidate tag.
If a candidate is abandoned, document it and use a new tag.

## Verify the draft

1. Confirm both matrix jobs and the draft-creation job are green.
2. Download the exact draft assets, not an older Actions artifact.
3. Verify `SHA256SUMS.txt` from the directory containing both ZIPs:

       shasum -a 256 -c SHA256SUMS.txt

4. Inspect both archives. Require one expected root, README, LICENSE, and the
   exact plugin bundles. On macOS, independently verify both architectures,
   every slice's minimum OS, Developer ID signatures, and stapled tickets.
5. Run the DAW checklist in `docs/BUILD_AND_TEST.md` on the exact extracted
   assets. Record commit/tag, hashes, host/OS/format versions, sample rate,
   block size, and every observed issue.

Recorded evidence: the exact older `v0.1.1-rc.1` private-draft AU and VST3
assets passed authoritative AU validation and a limited Ableton Live 12.4.2
functional smoke test at 48 kHz. That tag predates Hold Length and the Grain
Core. It is historical evidence only; the current v0.2.0 implementation still
requires exact-artifact validation in every target format/host, session
restoration, Bitwig checks, the full rate/block-size matrix, and subjective
musical/sound-quality approval.

## JUCE distribution gate

Repository-authored source is offered under the MIT License. Builds incorporate
JUCE 8.0.15, which JUCE separately offers under AGPLv3 or its commercial JUCE 8
licence. No distribution basis is currently recorded for Granular Freeze. Do
not publish or sell production binaries until the owner selects and verifies
one reviewed route for the exact release:

- **Commercial JUCE 8:** the owner privately verifies applicable product,
  Framework User, seat, and continued-distribution coverage. Record only a
  non-secret confirmation and date; never commit receipts, account identifiers,
  revenue data, or credentials.
- **AGPLv3:** release materials include the required licence/notices and provide
  clear access to the exact Corresponding Source and build/install materials
  for the distributed binaries.

The packaged third-party notice inventory must also be reviewed against the
exact modules and SDK code incorporated into each platform build. This is a
release gate, not legal advice.

## Production publication gate

Do not publish the GitHub draft or sell/upload files until all of these are
complete:

- Fresh native macOS and Windows CI for the exact release commit
- Downloaded-asset checksum and archive-content verification
- Exact VST3 and AU DAW loading and seven-parameter count/order checks
- Freeze, Pitch, Crossfade, short/default/long Hold, Size, Density, and
  chronological Position behavior
- Session restoration and automation checks, including v0.1 and v0.1.2 state
  migration into v0.2.0
- Listening checks without unresolved clicks, dropouts, runaway levels, or
  other glitches
- Owner-controlled subjective musical/sound-quality approval
- A documented JUCE 8 licensing basis and reviewed third-party notices
- Developer ID signing and notarization on macOS, and Authenticode signing on
  Windows, using the reviewed secret contract in `docs/CI_SECRETS.md`
- Product scope, price, listing copy, and delivery files explicitly approved
- A final explicit one-line approval to publish the release

## Gumroad

The current official Gumroad API does not support creating products or
uploading product content. After production approval, upload the signed,
verified files manually through the authenticated Gumroad product Content UI.
Do not add or log Gumroad credentials for an engineering candidate.

## Troubleshooting

- Build failures: inspect the Actions run and the failing platform job. JUCE is
  fetched via FetchContent unless
  `-DGRANULAR_FREEZE_JUCE_SOURCE_DIR=/path/to/JUCE` is supplied.
- Missing release assets: inspect the strict packaging and artifact-download
  steps. Product paths contain spaces and must remain quoted.
- No draft release: confirm the pushed tag is newly created, matches
  `v0.2.0-rc.*`, points to a commit contained in `main`, has no existing
  release object, and that both matrix jobs plus signing-status composition
  succeeded.
