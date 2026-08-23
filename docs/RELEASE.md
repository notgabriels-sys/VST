# Release and publishing

## Candidate identity

The internal project version is `0.2.0`. The release workflow accepts exactly
the engineering-candidate tag `v0.2.0-rc.1`. That tag has not been created or
approved by this integration work.

An annotated `v0.1.0` tag already points to an older repository state. Treat it
as immutable obsolete history: do not move, delete, reuse, or force-push it.
If another v0.2 candidate is needed, first update and review the workflow for a
new tag such as `v0.2.0-rc.2`; never rewrite a published tag.

## What the workflow does

`.github/workflows/release.yml` runs only when `v0.2.0-rc.1` is pushed.

1. Build on `macos-15-intel` and `windows-2022`.
2. On macOS, create universal2 arm64+x86_64 AU and VST3 bundles with minimum
   macOS 12.0. On Windows, create the x86_64 VST3 with the default Visual
   Studio generator.
3. Execute `GranularFreezeEngineTests` and `GranularFreezeTests` on both
   platforms.
4. Run the strict platform packaging scripts. Upload the artifact container
   `granular-freeze-macos`, containing `Granular-Freeze-macOS.zip`, and
   `granular-freeze-windows`, containing `Granular-Freeze-Windows.zip`.
5. Download both artifacts, create `SHA256SUMS.txt`, and create a GitHub
   **draft prerelease** named after the tag with all three files attached.

The workflow does not use production signing, notarization, or Gumroad
credentials. macOS bundles are ad-hoc signed and the Windows bundle is
unsigned. The draft release is not public, but the pushed Git tag is visible in
the public repository.

## Create the engineering candidate

Each remote mutation needs explicit approval. First merge the reviewed changes
and require fresh green CI on the exact merged commit. Confirm that
`v0.2.0-rc.1` does not exist locally or remotely, then annotate the exact merge
commit rather than whichever commit happens to be checked out:

    set -euo pipefail
    candidate_tag=v0.2.0-rc.1
    approved_merge_sha=REPLACE_WITH_VERIFIED_40_CHARACTER_SHA
    git fetch origin --tags
    if git show-ref --verify --quiet "refs/tags/$candidate_tag"; then
      echo "Refusing to replace existing local tag $candidate_tag" >&2
      exit 1
    fi
    remote_tag=$(git ls-remote --tags origin "refs/tags/$candidate_tag")
    if [ -n "$remote_tag" ]; then
      echo "Refusing to replace existing remote tag $candidate_tag" >&2
      exit 1
    fi
    git rev-parse --verify "${approved_merge_sha}^{commit}"
    git tag -a "$candidate_tag" "$approved_merge_sha" \
      -m "Granular Freeze v0.2.0 release candidate 1"
    git push origin "refs/tags/$candidate_tag"

Pushing the tag triggers the release workflow. Do not publish the generated
draft.

If the workflow or candidate is wrong, delete only the draft release. Leave the
tag untouched, document the abandoned candidate, and prepare a new reviewed
tag/workflow pair.

## Verify the draft

1. Confirm both matrix jobs and the draft-creation job are green.
2. Download the exact draft assets, not an older Actions artifact.
3. Verify `SHA256SUMS.txt` from the directory containing both ZIPs:

       shasum -a 256 -c SHA256SUMS.txt

4. Inspect both archives. Require one expected root, README, LICENSE, and the
   exact plugin bundles. On macOS, independently verify both architectures,
   every slice's minimum OS, and signatures.
5. Run the DAW smoke test in `docs/BUILD_AND_TEST.md` on the exact extracted
   assets. Record host/OS/format versions and any issue in the PR.

## Production publication gate

Do not publish the GitHub draft or sell/upload the files until all of these are
complete:

- Fresh native macOS and Windows CI for the exact release commit
- Downloaded-asset checksum and archive-content verification
- VST3 and AU DAW loading, all six controls, Size/Density/Position/Pitch
  behavior, Freeze/Unfreeze/reversal transitions, session restore, stereo
  stability, and listening checks without unresolved glitches
- A documented JUCE 8 licensing basis for the intended distribution model
- Developer ID signing and notarization on macOS, and Authenticode signing on
  Windows, using the reviewed secret contract in `docs/CI_SECRETS.md`
- Product scope, price, listing copy, and delivery files explicitly approved
- A final explicit one-line approval to publish the release

## Gumroad

The current official Gumroad API does not support creating products or
uploading product content. After production approval, upload the signed,
verified files manually through the authenticated Gumroad product Content UI.
Do not add or log Gumroad credentials for this unsigned candidate.

## Troubleshooting

- Build failures: inspect the Actions run and the failing platform job. JUCE is
  fetched via FetchContent unless
  `-DGRANULAR_FREEZE_JUCE_SOURCE_DIR=/path/to/JUCE` is supplied.
- Missing release assets: inspect the strict packaging step and the artifact
  download step. Product paths contain spaces and must remain quoted.
- No draft release: check that the exact tag is `v0.2.0-rc.1` and that both
  build jobs succeeded before the publish job.
