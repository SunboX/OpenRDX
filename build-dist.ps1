# SPDX-FileCopyrightText: 2026 André Fiedler
#
# SPDX-License-Identifier: AGPL-3.0-or-later

[CmdletBinding()]
param(
    [string] $Version,
    [string] $PlatformIoPath,
    [string] $PythonPath,
    [string] $TemplatePath,
    [switch] $SkipTests
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$projectRoot = $PSScriptRoot
$versionFile = Join-Path $projectRoot 'VERSION'
$firmwareHeader = Join-Path $projectRoot 'include/rdx_mount/tusb9260.h'
$profileDirectory = [Environment]::GetFolderPath('UserProfile')
$platformIoEnvironment = Join-Path $profileDirectory '.platformio/penv'
if ($env:OS -eq 'Windows_NT') {
    $defaultPlatformIo = Join-Path $platformIoEnvironment 'Scripts/pio.exe'
    $defaultPython = Join-Path $platformIoEnvironment 'Scripts/python.exe'
} else {
    $defaultPlatformIo = Join-Path $platformIoEnvironment 'bin/pio'
    $defaultPython = Join-Path $platformIoEnvironment 'bin/python'
}
if ([string]::IsNullOrWhiteSpace($PlatformIoPath)) { $PlatformIoPath = $defaultPlatformIo }
if ([string]::IsNullOrWhiteSpace($PythonPath)) { $PythonPath = $defaultPython }
$recordedVersion = (Get-Content -Raw -LiteralPath $versionFile).Trim()
if ([string]::IsNullOrWhiteSpace($Version)) { $Version = $recordedVersion }
if ($Version -cne $recordedVersion) {
    throw "Release version $Version does not match VERSION $recordedVersion."
}

function Invoke-CheckedCommand {
    <# Run one external build command and stop immediately on failure. #>
    param(
        [Parameter(Mandatory = $true)] [string] $FilePath,
        [Parameter(Mandatory = $true)] [string[]] $Arguments,
        [Parameter(Mandatory = $true)] [string] $Description
    )

    Write-Host "==> $Description"
    & $FilePath @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "$Description failed with exit code $LASTEXITCODE."
    }
}

if ($Version -notmatch '^\d+\.\d{2}$') {
    throw "Release version '$Version' must use the firmware form MAJOR.MINOR, for example 1.06."
}
if (-not (Test-Path -LiteralPath $PlatformIoPath -PathType Leaf)) {
    throw "PlatformIO was not found at $PlatformIoPath"
}
if (-not (Test-Path -LiteralPath $PythonPath -PathType Leaf)) {
    throw "Python was not found at $PythonPath"
}
$headerText = Get-Content -Raw -LiteralPath $firmwareHeader
$majorMatch = [regex]::Match(
    $headerText,
    '(?m)^#define\s+FIRMWARE_MAJOR_VERSION\s+(?<value>\d+)\s*$')
$minorMatch = [regex]::Match(
    $headerText,
    '(?m)^#define\s+FIRMWARE_MINOR_VERSION\s+(?<value>\d+)\s*$')
if (-not $majorMatch.Success -or -not $minorMatch.Success) {
    throw 'Could not read the compiled firmware version from tusb9260.h.'
}
$compiledVersion = '{0}.{1}' -f `
    $majorMatch.Groups['value'].Value,
    $minorMatch.Groups['value'].Value.PadLeft(2, '0')
if ($Version -ne $compiledVersion) {
    throw "Release version $Version does not match compiled firmware version $compiledVersion."
}

# Forward the explicit template to both the test fixtures and normal build.
# The Python packager verifies and caches it for later plain PlatformIO builds.
$previousTemplate = $env:OPENRDX_TEMPLATE_PATH
if (-not [string]::IsNullOrWhiteSpace($TemplatePath)) {
    $env:OPENRDX_TEMPLATE_PATH = [System.IO.Path]::GetFullPath($TemplatePath)
}
Push-Location $projectRoot
try {
    # Run the release gate first: PlatformIO now replaces dist as part of its
    # normal build, so failed tests must stop before that replacement.
    if (-not $SkipTests) {
        Invoke-CheckedCommand `
            -FilePath $PythonPath `
            -Arguments @('-m', 'unittest', 'discover', '-s', 'tests', '-v') `
            -Description 'Running the complete automated firmware and release suite'
    }
    Invoke-CheckedCommand `
        -FilePath $PlatformIoPath `
        -Arguments @('run', '-e', 'tusb9261_ti_cgt') `
        -Description 'Building OpenRDX firmware and the complete dist bundle'
} finally {
    Pop-Location
    $env:OPENRDX_TEMPLATE_PATH = $previousTemplate
}
