Release & CI publishing guide

This document describes the automated release workflow and the secrets needed to publish signed artifacts and (optionally) upload builds to Gumroad.

Automatic release workflow
- A workflow named `Release Build & Publish` runs when you push a tag matching v*.*.* (for example `v0.1.0`).
- The workflow builds macOS and Windows artifacts in separate jobs, packages them (zip) and uploads them as workflow artifacts.
- A `publish` job collects artifacts, creates a draft GitHub release for the tag, and uploads packaged assets to the release.
- If Gumroad secrets are present, the `publish` job will attempt to upload the zip to Gumroad as a new product version.

Required GitHub repository secrets
- GITHUB_TOKEN — provided automatically in Actions; no manual setup needed for creating releases.
- (Optional) GUMROAD_PRODUCT_ID — ID of the Gumroad product where releases should be uploaded.
- (Optional) GUMROAD_ACCESS_TOKEN — Gumroad API access token (keep this secret). If both GUMROAD_PRODUCT_ID and GUMROAD_ACCESS_TOKEN are set, CI will attempt to upload the zip during release creation.

Signing and notarization (recommended for production builds)
- Code signing and notarization are not automated by default because they require sensitive credentials.
- Recommended secrets to add if you want CI to sign and notarize artifacts:
  - MACOS_CODESIGN_IDENTITY: The name of your macOS Developer ID Application identity (e.g., "Developer ID Application: Example Corp (TEAMID)")
  - MACOS_NOTARIZE_USERNAME: Apple ID (email) used for notarization (or use an App-Specific password)
  - MACOS_NOTARIZE_PASSWORD: App-specific password (or API key) — store this as a secret
  - WINDOWS_PFX_BASE64: Base64-encoded PFX file for Authenticode signing. (Don't upload .pfx directly as a secret.)
  - WINDOWS_PFX_PASSWORD: Password for the PFX file

How to add secrets
1. Go to your GitHub repository > Settings > Secrets and variables > Actions > New repository secret.
2. Add the values above (names must match exactly).

How to make a release
1. Create a tag locally and push it:
   git tag v0.1.0
   git push origin v0.1.0
2. The `Release Build & Publish` workflow will run. It will produce a draft release and attach built assets.
3. Review the draft release on GitHub, update release notes, and publish it.
4. If you use Gumroad and provided the required secrets, the workflow will also attempt to upload the packaged zip as a new version.

Manual upload to Gumroad
- If you prefer not to automate Gumroad uploads, download the signed artifacts from the release and upload them manually at https://gumroad.com

Notes & troubleshooting
- The runner requires a valid CMake + toolchain for the target OS. macOS builds require Xcode; Windows builds require Visual Studio.
- If builds fail due to missing JUCE, add JUCE as a submodule or adjust CMake variable DJUCE_DIR.
- This workflow creates a draft release; this gives you a chance to verify artifacts before publishing publicly.

If you'd like, I can also:
- Add signing/notarization steps to the build jobs that decode the certificates from secrets and run codesign/signtool.
- Add a workflow that automatically publishes the draft release (mark as published) and triggers an optional Gumroad upload after your manual confirmation.
- Create GitHub Release notes automatically from CHANGELOG.md

Security reminder
- Never paste private keys, passwords, or tokens in chat. Use GitHub Secrets. If any token was pasted earlier into an open chat, revoke it immediately.