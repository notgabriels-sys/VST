# CI secrets

## Current workflow state

The current CI and release workflows read no user-provided repository secrets.
GitHub supplies `GITHUB_TOKEN` automatically; the release job limits it to
`contents: write` so it can create an unsigned draft prerelease.

Production signing, notarization, and Gumroad delivery are not implemented.
macOS bundles are ad-hoc signed for structural verification only, and Windows
bundles are unsigned. Do not publish or sell these candidate files.

## Future production-signing contract

Nothing reads the names below today. If production signing is approved, make
these the canonical GitHub Actions secret names and update the workflow to
match them exactly.

Developer ID signing requires the certificate and private key, not only its
identity name:

- `MAC_CODESIGN_P12_BASE64` - base64-encoded Developer ID Application `.p12`
- `MAC_CODESIGN_P12_PASSWORD` - password protecting the `.p12`
- `MAC_KEYCHAIN_PASSWORD` - password for the temporary CI keychain
- `MAC_CODESIGN_IDENTITY` - exact Developer ID Application identity

Choose exactly one complete notarization route.

App Store Connect API key route:

- `MAC_NOTARIZE_API_KEY` - base64-encoded `.p8` private key
- `MAC_NOTARIZE_KEY_ID` - App Store Connect key ID
- `MAC_NOTARIZE_ISSUER_ID` - App Store Connect issuer ID

Apple ID route:

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
