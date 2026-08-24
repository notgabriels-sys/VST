[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidateNotNullOrEmpty()]
    [string]$BuildDirectory,

    [Parameter(Mandatory = $true)]
    [ValidateNotNullOrEmpty()]
    [string]$OutputZip
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

function Fail([string]$Message) {
    throw "error: $Message"
}

if (-not (Test-Path -LiteralPath $BuildDirectory -PathType Container)) {
    Fail "build directory does not exist: $BuildDirectory"
}

$buildRoot = (Resolve-Path -LiteralPath $BuildDirectory).Path
$repositoryRoot = Split-Path -Parent $PSScriptRoot
foreach ($document in @('README.md', 'LICENSE', 'THIRD_PARTY_NOTICES.md')) {
    $documentPath = Join-Path $repositoryRoot $document
    if (-not (Test-Path -LiteralPath $documentPath -PathType Leaf)) {
        Fail "required document is missing: $documentPath"
    }
}

$archiveRoot = 'Granular Freeze-Windows'
$stagingDirectory = Join-Path $buildRoot $archiveRoot
if (Test-Path -LiteralPath $stagingDirectory) {
    Remove-Item -LiteralPath $stagingDirectory -Recurse -Force
}

# Select a bundle directory only. The inner Windows binary also ends in .vst3,
# so extension-only recursive matching is deliberately forbidden here.
$bundles = @(
    Get-ChildItem -LiteralPath $buildRoot -Directory -Filter 'Granular Freeze.vst3' -Recurse
)
if ($bundles.Count -ne 1) {
    Fail "expected exactly one outer Granular Freeze.vst3 directory; found $($bundles.Count)"
}

$bundle = $bundles[0]
$innerBinary = Join-Path $bundle.FullName 'Contents/x86_64-win/Granular Freeze.vst3'
if (-not (Test-Path -LiteralPath $innerBinary -PathType Leaf)) {
    Fail "expected Windows VST3 binary is missing: $innerBinary"
}

$outputZip = [System.IO.Path]::GetFullPath($OutputZip)
$outputParent = Split-Path -Parent $outputZip
if (-not (Test-Path -LiteralPath $outputParent -PathType Container)) {
    Fail "output directory does not exist: $outputParent"
}
if (Test-Path -LiteralPath $outputZip -PathType Leaf) {
    Remove-Item -LiteralPath $outputZip -Force
}

New-Item -ItemType Directory -Path $stagingDirectory | Out-Null
Copy-Item -LiteralPath $bundle.FullName -Destination (Join-Path $stagingDirectory 'Granular Freeze.vst3') -Recurse
Copy-Item -LiteralPath (Join-Path $repositoryRoot 'README.md') -Destination (Join-Path $stagingDirectory 'README.md')
Copy-Item -LiteralPath (Join-Path $repositoryRoot 'LICENSE') -Destination (Join-Path $stagingDirectory 'LICENSE')
Copy-Item -LiteralPath (Join-Path $repositoryRoot 'THIRD_PARTY_NOTICES.md') -Destination (Join-Path $stagingDirectory 'THIRD_PARTY_NOTICES.md')

Add-Type -AssemblyName System.IO.Compression.FileSystem
[System.IO.Compression.ZipFile]::CreateFromDirectory(
    $stagingDirectory,
    $outputZip,
    [System.IO.Compression.CompressionLevel]::Optimal,
    $true
)

$archive = [System.IO.Compression.ZipFile]::OpenRead($outputZip)
try {
    $archiveEntries = @($archive.Entries | ForEach-Object { $_.FullName })
}
finally {
    $archive.Dispose()
}

foreach ($entry in $archiveEntries) {
    if ($entry -and -not $entry.StartsWith("$archiveRoot/", [System.StringComparison]::Ordinal)) {
        Fail "archive contains an unexpected root entry: $entry"
    }
}

foreach ($requiredEntry in @(
    "$archiveRoot/README.md",
    "$archiveRoot/LICENSE",
    "$archiveRoot/THIRD_PARTY_NOTICES.md",
    "$archiveRoot/Granular Freeze.vst3/Contents/x86_64-win/Granular Freeze.vst3"
)) {
    if ($archiveEntries -notcontains $requiredEntry) {
        Fail "archive is missing required entry: $requiredEntry"
    }
}

Write-Output "Packaged Windows candidate: $outputZip"
$archiveEntries
