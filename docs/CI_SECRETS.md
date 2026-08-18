# CI secrets

## Wired up today

Only two secrets are read by any workflow. Both are optional and both are
consumed by the Gumroad step in `.github/workflows/release.yml`:

| Secret | Purpose |
|---|---|
| `GUMROAD_ACCESS_TOKEN` | Gumroad API access token |
| `GUMROAD_PRODUCT_ID` | Product to attach the release zip to |

Use these names exactly. If either is missing the step logs that it is skipping
and exits cleanly, so the release still succeeds without them.

An earlier version of this document named `GUMROAD_API_KEY`. Nothing reads that
— setting it silently skips the upload.

`GITHUB_TOKEN` is provided automatically by Actions; the publish job requests
`permissions: contents: write` so it can create the release. No setup needed.

**The Gumroad upload has never been exercised.** No tag has been pushed, so the
first release will be its first real run.

## Not implemented

Code signing and notarization are **not** in any workflow. CI produces unsigned
builds. macOS users will see Gatekeeper warnings and Windows users SmartScreen
warnings.

If you add signing later, these are the credentials you will need. The names are
a suggestion — nothing reads them yet, so pick names and make the workflow match:

**macOS** — Developer ID Application certificate, exported as .p12:

- `MACOS_CODESIGN_P12` — base64-encoded .p12
- `MACOS_CODESIGN_P12_PASSPHRASE`
- `APPLE_ID` and `APP_SPECIFIC_PASSWORD` for notarytool, or
  `NOTARYTOOL_ISSUER` + `NOTARYTOOL_KEY` for an App Store Connect API key

**Windows** — Authenticode certificate:

- `WIN_CODESIGN_PFX` — base64-encoded .pfx
- `WIN_CODESIGN_PFX_PASSPHRASE`

A previous version of `docs/RELEASE.md` listed a second, conflicting set of
names (`MACOS_CODESIGN_IDENTITY`, `WINDOWS_PFX_BASE64`, ...). Ignore those; this
file is the single source.

## Adding secrets

Settings → Secrets and variables → Actions → New repository secret. Names must
match what the workflow reads, exactly.

## Practices

- Never commit certificates, .p12 or .pfx files. Store them as secrets.
- Test signing and notarization locally on a Mac before wiring it into CI.
- Unsigned builds are a legitimate starting point; signing can be added later
  without changing the build.
