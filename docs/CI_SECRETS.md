# CI secrets

## Current workflow state

The release workflow contains Developer ID signing, notarization and
Authenticode signing. Pull requests exercise the unsigned build/package path.
A manual run can exercise signing without creating a release, and only
newly created `v0.1.1-rc.*` tag pushes can create a private draft prerelease.
Force-moved/reused tags are rejected, and the publish job fails if any release
already exists for the tag instead of updating it. GitHub supplies
`GITHUB_TOKEN` automatically; only the draft-release job receives
`contents: write`.

Secret handling is fail-closed:

- an entirely absent secret set produces an explicitly unsigned tag candidate;
- a partial secret set fails the corresponding platform job;
- a manual run with `sign_artifacts` enabled fails unless every signing and
  notarization secret below is configured;
- a tag run with mixed whole-platform outcomes (for example, signed Windows
  but unsigned macOS) fails before any draft release is created;
- the release notes are composed from status files written after successful
  operations, never from secret presence alone.

**None of the secret-dependent signing paths has ever executed.** As verified
on 2026-08-22, the repository has no Actions secrets. Treat the first signed
manual run as a debugging run, not a release.

Until secrets are added, macOS bundles remain ad-hoc signed and Windows bundles
unsigned. Do not publish or sell those candidate files.

The workflow removes temporary certificate/key files and the temporary macOS
keychain even when a signing or notarization command fails. The macOS packager
uses `--preserve-signature` after Developer ID signing and stapling so it cannot
replace those signatures with ad-hoc ones. In that mode it follows Apple's
signed-ZIP `ditto -c -k --keepParent` recipe so resource data and extended
attributes are not deliberately stripped.

## Production-signing contract — now read by the workflow

These are the exact names `.github/workflows/release.yml` reads. They are no
longer suggestions; changing one means changing the workflow too.

Developer ID signing requires the certificate and private key, not only its
identity name:

- `MAC_CODESIGN_P12_BASE64` - base64-encoded Developer ID Application `.p12`
- `MAC_CODESIGN_P12_PASSWORD` - password protecting the `.p12`
- `MAC_KEYCHAIN_PASSWORD` - password for the temporary CI keychain
- `MAC_CODESIGN_IDENTITY` - exact Developer ID Application identity

Two notarization routes were documented. **The App Store Connect API key route
is the one implemented**, because it needs no interactive Apple ID and no
app-specific password rotation. The Apple ID route below is left for reference;
using it would mean rewriting the notarize step.

App Store Connect API key route (implemented):

- `MAC_NOTARIZE_API_KEY` - base64-encoded `.p8` private key
- `MAC_NOTARIZE_KEY_ID` - App Store Connect key ID
- `MAC_NOTARIZE_ISSUER_ID` - App Store Connect issuer ID

Apple ID route (documented, NOT implemented):

- `MAC_NOTARIZE_APPLE_ID` - Apple ID used for notarization
- `MAC_NOTARIZE_APP_PASSWORD` - app-specific password
- `MAC_NOTARIZE_TEAM_ID` - Apple Developer Team ID

Windows Authenticode signing requires:

- `WIN_PFX_BASE64` - base64-encoded signing `.pfx`
- `WIN_PFX_PASSWORD` - password protecting the `.pfx`

## Gumroad

The current official Gumroad API does not support creating products or
uploading product content. The workflows therefore do not read
`GUMROAD_ACCESS_TOKEN` or `GUMROAD_PRODUCT_ID`. Upload approved production
files manually through the authenticated Gumroad product Content UI. Do not
add Gumroad secrets merely for the current candidate workflow.

## Adding secrets safely

After the signing implementation is reviewed and merged, add values under GitHub:
Settings -> Secrets and variables -> Actions -> New repository secret.

- Never commit certificates, private keys, passwords, or tokens.
- Never print decoded values or enable shell tracing around secret handling.
- Decode credentials only into runner-temporary files and remove them after
  use.
- Test signing and notarization with a non-public candidate before production.
- Confirm every required secret name exists without reading or exposing its
  value.

## Ordering, and why it matters

Signing runs before packaging. The signed ZIP is submitted to Apple;
notarization and stapling run next, and then the macOS ZIP is rebuilt with
`--preserve-signature` so it contains the stapled tickets. Stapling a ZIP itself
does nothing, and force-signing after stapling would destroy the production
signature/ticket relationship. Before recording `notarized=yes`, the workflow
extracts the final distribution ZIP and re-runs strict code-signature and
stapler validation against both extracted plugin bundles.

`--options runtime` is set during signing because notarization rejects binaries
without the hardened runtime.

Likely first-run friction: the exact `MAC_CODESIGN_IDENTITY` string, keychain
partition-list access on the runner, and notarization rejections.

To base64 a certificate without it landing in shell history:

    base64 -i Certificates.p12 | pbcopy
