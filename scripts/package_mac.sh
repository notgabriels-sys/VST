#!/usr/bin/env bash
# Package the macOS release candidate from an existing Release build.

set -euo pipefail

die() {
  echo "error: $*" >&2
  exit 1
}

if [[ $# -lt 2 || $# -gt 3 ]]; then
  die "usage: $0 <build-directory> <output-zip> [--preserve-signature]"
fi

build_directory=$1
output_zip=$2
preserve_signature=false
if [[ $# -eq 3 ]]; then
  [[ $3 == '--preserve-signature' ]] || die "unknown option: $3"
  preserve_signature=true
fi
script_directory=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)
repository_root=$(cd "$script_directory/.." && pwd -P)

[[ -d "$build_directory" ]] || die "build directory does not exist: $build_directory"
build_directory=$(cd "$build_directory" && pwd -P)

if [[ "$output_zip" != /* ]]; then
  output_zip="$PWD/$output_zip"
fi
output_parent=$(dirname "$output_zip")
[[ -d "$output_parent" ]] || die "output directory does not exist: $output_parent"

for document in README.md LICENSE; do
  [[ -f "$repository_root/$document" ]] || die "required document is missing: $repository_root/$document"
done

archive_root='Granular Freeze-macOS'
staging_directory="$build_directory/$archive_root"
rm -rf "$staging_directory"
rm -f "$output_zip"

vst3_bundles=()
while IFS= read -r -d '' bundle; do
  vst3_bundles+=("$bundle")
done < <(find "$build_directory" -type d -name 'Granular Freeze.vst3' -print0)

component_bundles=()
while IFS= read -r -d '' bundle; do
  component_bundles+=("$bundle")
done < <(find "$build_directory" -type d -name 'Granular Freeze.component' -print0)

[[ ${#vst3_bundles[@]} -eq 1 ]] || die "expected exactly one outer Granular Freeze.vst3 directory; found ${#vst3_bundles[@]}"
[[ ${#component_bundles[@]} -eq 1 ]] || die "expected exactly one outer Granular Freeze.component directory; found ${#component_bundles[@]}"

verify_bundle() {
  local bundle=$1
  local binary="$bundle/Contents/MacOS/Granular Freeze"
  local architectures normalized_architectures minos arch
  local -a parsed_architectures=()

  [[ -f "$binary" ]] || die "expected bundle binary is missing: $binary"
  [[ -x "$binary" ]] || die "expected bundle binary is not executable: $binary"

  architectures=$(lipo -archs "$binary")
  read -r -a parsed_architectures <<< "$architectures"
  normalized_architectures=$(printf '%s\n' "${parsed_architectures[@]}" | LC_ALL=C sort -u)
  if [[ "$normalized_architectures" != $'arm64\nx86_64' ]]; then
    die "$binary must contain exactly arm64 and x86_64 slices (found: $architectures)"
  fi

  while IFS= read -r arch; do
    minos=$(vtool -arch "$arch" -show-build "$binary" | awk '$1 == "minos" { print $2 }')
    [[ "$minos" == "12.0" ]] || die "$binary $arch slice must declare macOS 12.0 (found: ${minos:-none})"
  done <<< "$normalized_architectures"
}

verify_signature() {
  codesign --verify --deep --strict "$1"
}

# Unsigned engineering candidates receive a valid ad-hoc bundle seal. A
# Developer ID workflow passes --preserve-signature so packaging cannot replace
# the production signature (or a stapled notarization ticket) with an ad-hoc one.
for bundle in "${vst3_bundles[@]}" "${component_bundles[@]}"; do
  if [[ $preserve_signature == false ]]; then
    codesign --force --deep --sign - "$bundle"
  fi
  verify_signature "$bundle"
  verify_bundle "$bundle"
done

mkdir -p "$staging_directory"
ditto "${vst3_bundles[0]}" "$staging_directory/Granular Freeze.vst3"
ditto "${component_bundles[0]}" "$staging_directory/Granular Freeze.component"
ditto "$repository_root/README.md" "$staging_directory/README.md"
ditto "$repository_root/LICENSE" "$staging_directory/LICENSE"

for bundle in "$staging_directory/Granular Freeze.vst3" "$staging_directory/Granular Freeze.component"; do
  verify_signature "$bundle"
  verify_bundle "$bundle"
done

if [[ $preserve_signature == true ]]; then
  # Follow Apple's signed-ZIP recipe so resource data and extended attributes,
  # including a stapled ticket, survive extraction by macOS tools.
  ditto -c -k --keepParent "$staging_directory" "$output_zip"
else
  ditto -c -k --norsrc --keepParent "$staging_directory" "$output_zip"
fi

archive_entries=$(unzip -Z1 "$output_zip")
while IFS= read -r entry; do
  if [[ $preserve_signature == true ]]; then
    [[ -z "$entry" || "$entry" == "$archive_root/"* ||
       "$entry" == '__MACOSX/' || "$entry" == "__MACOSX/._$archive_root" ||
       "$entry" == "__MACOSX/$archive_root/"* ]] ||
      die "archive contains an unexpected root entry: $entry"
  else
    [[ -z "$entry" || "$entry" == "$archive_root/"* ]] ||
      die "archive contains an unexpected root entry: $entry"
    case "/$entry/" in
      */._*/) die "archive contains AppleDouble metadata: $entry" ;;
    esac
  fi
done <<< "$archive_entries"

for required_entry in \
  "$archive_root/README.md" \
  "$archive_root/LICENSE" \
  "$archive_root/Granular Freeze.vst3/Contents/MacOS/Granular Freeze" \
  "$archive_root/Granular Freeze.component/Contents/MacOS/Granular Freeze"; do
  grep -Fxq "$required_entry" <<< "$archive_entries" || die "archive is missing required entry: $required_entry"
done

unzip -tq "$output_zip"

# Validate the exact bytes a user receives, not only the pre-archive sources.
verification_directory=$(mktemp -d "${TMPDIR:-/tmp}/granular-freeze-macos-verify.XXXXXX")
cleanup_verification() {
  [[ -n "${verification_directory:-}" && -d "$verification_directory" ]] &&
    rm -rf "$verification_directory"
}
trap cleanup_verification EXIT
ditto -x -k "$output_zip" "$verification_directory"
for bundle in \
  "$verification_directory/$archive_root/Granular Freeze.vst3" \
  "$verification_directory/$archive_root/Granular Freeze.component"; do
  [[ -d "$bundle" ]] || die "archive extraction is missing expected bundle: $bundle"
  verify_signature "$bundle"
  verify_bundle "$bundle"
done

echo "Packaged macOS candidate: $output_zip"
unzip -Z1 "$output_zip"
