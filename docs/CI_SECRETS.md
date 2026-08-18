CI secrets & developer accounts required

Apple (macOS) notarization (required for frictionless mac installs):
- Apple Developer account (organization or individual)
- Create a Developer ID Application certificate and export as a p12 file (no password in repo)
- GitHub secrets to set:
  - APPLE_ID (email)
  - APP_SPECIFIC_PASSWORD (for altool/notarytool upload OR use App Store Connect API)
  - MACOS_CODESIGN_P12 (base64-encoded p12)
  - MACOS_CODESIGN_P12_PASSPHRASE (if any)
  - NOTARYTOOL_ISSUER and NOTARYTOOL_KEY (if using App Store Connect API key)

Windows code-signing (recommended to reduce SmartScreen warnings):
- Authenticode code signing certificate (.pfx)
- GitHub secrets to set:
  - WIN_CODESIGN_PFX (base64-encoded .pfx)
  - WIN_CODESIGN_PFX_PASSPHRASE

Gumroad / release
- Gumroad API key (used only if automating upload; otherwise upload manually)
  - GUMROAD_API_KEY (optional, keep secret)

GitHub
- GITHUB_TOKEN (Actions provided token is usually sufficient for uploads to Releases; for protected operations use personal access token scoped to repo)

Notes and best practices
- Never commit raw certificates, p12s or pfx files to the repository. Always store them as GitHub secrets and reference them in the workflow.
- Test local code-signing and notarization flow on a mac before adding to CI to ensure the certificate and keys are correct.
- If you prefer not to include code-signing in CI, CI can still produce unsigned builds that you sign locally.

