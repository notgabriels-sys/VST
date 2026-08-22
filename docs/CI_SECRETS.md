# CI secrets

## Current workflow state

The release workflow now contains Developer ID signing, notarization and
Authenticode signing. Every step is gated on its own secrets: with none set,
each logs a warning and exits 0, so the unsigned candidate path behaves exactly
as it did before. GitHub supplies `GITHUB_TOKEN` automatically; the release job
limits it to `contents: write`.

**None of the signing paths has ever executed.** No certificates exist, so only
the skip branches have run. Treat the first signed tag as a debugging run, not
a release.

Until secrets are added, macOS bundles remain ad-hoc signed and Windows bundles
unsigned. Do not publish or sell those candidate files.

The draft release notes report which of the three actually happened rather than
asserting the build is unsigned, so a signed build is not mislabelled.

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

After the signing implementation is reviewed, add values under GitHub:
Settings -> Secrets and variables -> Actions -> New repository secret.

- Never commit certificates, private keys, passwords, or tokens.
- Never print decoded values or enable shell tracing around secret handling.
- Decode credentials only into runner-temporary files and remove them after
  use.
- Test signing and notarization with a non-public candidate before production.
- Confirm every required secret name exists without reading or exposing its
  value.

## Ordering, and why it matters

Signing runs before packaging. Notarization and stapling run after, and then the
macOS zip is rebuilt so it contains the stapled tickets — stapling a zip does
nothing, so an archive built before stapling ships without the ticket and
Gatekeeper still consults Apple at launch.

`--options runtime` is set during signing because notarization rejects binaries
without the hardened runtime.

Likely first-run friction: the exact `MAC_CODESIGN_IDENTITY` string, keychain
partition-list access on the runner, and notarization rejections.

To base64 a certificate without it landing in shell history:

    base64 -i Certificates.p12 | pbcopy
