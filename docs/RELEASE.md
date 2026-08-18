# Release & publishing

## What the workflow does

`.github/workflows/release.yml` (`Release Build & Publish`) runs on a pushed tag
matching `v*.*.*`.

1. Builds macOS and Windows in a matrix, runs the test suite, and packages each
   platform's plugin bundles into a zip.
2. The `publish` job downloads both artifacts and creates a **draft** GitHub
   release for the tag with the zips attached, via
   `softprops/action-gh-release@v2`.
3. If `GUMROAD_ACCESS_TOKEN` and `GUMROAD_PRODUCT_ID` are both set, it uploads
   the zip to Gumroad as a new product version. If either is missing it logs a
   skip and exits cleanly.

The release is created as a draft, so nothing is public until you publish it.

## Making a release

    git tag v0.1.0
    git push origin v0.1.0

Then review the draft on GitHub, write the notes, and publish.

To undo before publishing: delete the draft release and the tag
(`git push origin :refs/tags/v0.1.0`).

## Before the first release

- **The workflow has never run.** No tag has been pushed, so the first one is
  its first real exercise. Expect to iterate.
- **Builds are unsigned.** See `docs/CI_SECRETS.md`. macOS users get Gatekeeper
  warnings, Windows users get SmartScreen warnings. This is worth resolving
  before charging for it.
- **The plugin has not been evaluated by ear.** CI, the offline tests and auval
  all pass, but that only establishes it runs correctly — not that it sounds
  good. Load it in a DAW first.

## Secrets

See `docs/CI_SECRETS.md`, which is the single source for secret names. Only
`GUMROAD_ACCESS_TOKEN` and `GUMROAD_PRODUCT_ID` are read by any workflow today.

## Manual Gumroad upload

If you would rather not automate it, leave the Gumroad secrets unset, download
the zips from the draft release, and upload them at https://gumroad.com.

## Troubleshooting

- Build failures: check the Actions log. macOS uses the Xcode generator,
  Windows the default Visual Studio generator; JUCE is fetched via FetchContent
  (override with `-DJUCE_SOURCE_DIR=...`, not `JUCE_DIR`).
- No assets on the release: check the packaging step. The product name contains
  a space, so paths must stay quoted.
