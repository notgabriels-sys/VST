# CI secrets

## Current workflow state

The release workflow contains Developer ID signing, notarization and
Authenticode signing. Pull requests exercise the unsigned build/package path.
A manual run can exercise signing without creating a release, and only
newly created `v0.2.0-rc.*` tag pushes can create a private draft prerelease.
Non-creation updates and force-moves are rejected, and the publish job fails if
any release already exists for the tag instead of updating it. The workflow
cannot provide durable history for a tag that was deleted before being
recreated, so operators must never delete and recreate an old candidate tag;
use a new verified-unused suffix instead. GitHub supplies `GITHUB_TOKEN`
automatically; only the draft-release job receives `contents: write`.

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
on 2026-08-23, the repository has no Actions secrets. Treat the first signed
manual run as a debugging run, not a release.

Until secrets are added, macOS bundles remain ad-hoc signed and Windows bundles
unsigned. Do not publish or sell those candidate files.

The workflow is designed to remove temporary certificate/key files and the
temporary macOS keychain even when signing or notarization fails. The macOS
packager's `--preserve-signature` mode is designed to prevent replacement of
Developer ID signatures or stapled tickets and follows Apple's signed-ZIP
`ditto -c -k --keepParent` pattern. Non-secret preservation,
archive, extraction, and exact post-archive signature read-back mechanics have
been tested locally or in the workflow path; Developer ID import/signing,
Apple notarization/stapling, and Windows Authenticode have never been
exercised end to end with real credentials.

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

- `MAC_NOTARIZE_API_KEY` - base64-encoded `.p8` private key for an App Store
  Connect **team API key** usable by `notarytool`; individual keys are not
  supported for this route
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

The signing implementation is reviewed and merged. When the owner has the
required credentials and explicitly approves secret setup, add values under
GitHub: Settings -> Secrets and variables -> Actions -> New repository secret.
Do not paste values into chat or repository files. After a secret-name-only
confirmation, obtain separate approval before the first private
`sign_artifacts=true` run.

- Never commit certificates, private keys, passwords, or tokens.
- Never print decoded values or enable shell tracing around secret handling.
- Decode credentials only into runner-temporary files and remove them after
  use.
- Test signing and notarization with a non-public candidate before production.
- Confirm every required secret name exists without reading or exposing its
  value.

## Ordering, and why it matters

The intended credential-enabled order, not yet exercised end to end with real
credentials, is Developer ID signing, signature-preserving packaging,
notarization, stapling, signature-preserving repackaging, and validation of the
exact extracted distribution bundles. Stapling a ZIP itself does nothing, and
force-signing after stapling would destroy the production signature/ticket
relationship. Before recording `notarized=yes`, the workflow extracts the final
distribution ZIP and re-runs strict code-signature and stapler validation
against both extracted plugin bundles.

`--options runtime` is set during signing because notarization rejects binaries
without the hardened runtime.

Likely first-run friction: the exact `MAC_CODESIGN_IDENTITY` string, keychain
partition-list access on the runner, and notarization rejections.

To base64 a certificate without it landing in shell history:

    base64 -i Certificates.p12 | pbcopy
