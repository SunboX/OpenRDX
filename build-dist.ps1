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
$buildDirectory = Join-Path $projectRoot '.pio\build\tusb9261_ti_cgt'
$distDirectory = Join-Path $projectRoot 'dist'
$versionFile = Join-Path $projectRoot 'VERSION'
$firmwareHeader = Join-Path $projectRoot 'include\rdx_mount\tusb9260.h'
$updaterSource = Join-Path $projectRoot 'scripts\rdx_manager_firmware_update.ps1'
$installationProcedureSource = Join-Path $projectRoot 'docs\getting-started\installation.md'
$templateSha256 = '73D528801AEFC032D3A53637B035F65D72809151B2F127C6B2050E9BACC76F3B'

if ([string]::IsNullOrWhiteSpace($PlatformIoPath)) {
    $PlatformIoPath = Join-Path $env:USERPROFILE '.platformio\penv\Scripts\pio.exe'
}
if ([string]::IsNullOrWhiteSpace($PythonPath)) {
    $PythonPath = Join-Path $env:USERPROFILE '.platformio\penv\Scripts\python.exe'
}
if ([string]::IsNullOrWhiteSpace($TemplatePath)) {
    $managerRoot = [System.IO.Path]::GetFullPath(
        (Join-Path $projectRoot '..\OpenRDXManager'))
    $templateCandidates = @()
    if (Test-Path -LiteralPath $managerRoot -PathType Container) {
        $templateCandidates = @(
            Get-ChildItem -LiteralPath $managerRoot -Recurse -File `
                -Filter 'RDX2E__STD__F-0283.bin' |
                Sort-Object -Property FullName |
                Where-Object {
                    (Get-FileHash -Algorithm SHA256 -LiteralPath $_.FullName).Hash `
                        -eq $templateSha256
                }
        )
    }
    if ($templateCandidates.Count -eq 0) {
        throw "No pinned compatibility template was found below $managerRoot. Pass -TemplatePath explicitly."
    }
    # Multiple byte-identical copies are equivalent inputs. Sorting makes the
    # default reproducible without encoding a workspace-specific subdirectory.
    $TemplatePath = $templateCandidates[0].FullName
}
if ([string]::IsNullOrWhiteSpace($Version)) {
    $Version = (Get-Content -Raw -LiteralPath $versionFile).Trim()
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

function Get-Sha256 {
    <# Return the uppercase SHA-256 digest for one release artifact. #>
    param([Parameter(Mandatory = $true)] [string] $Path)

    return (Get-FileHash -Algorithm SHA256 -LiteralPath $Path).Hash
}

function Reset-DistributionDirectory {
    <#
    Recreate only the project's generated dist directory.

    Removing the prior directory prevents retired filenames and metadata from
    leaking into a new release. The canonical-path comparison is deliberately
    performed before the recursive delete so a malformed path can never widen
    the cleanup beyond the project's named dist child.
    #>
    param(
        [Parameter(Mandatory = $true)] [string] $ProjectDirectory,
        [Parameter(Mandatory = $true)] [string] $DistributionDirectory
    )

    $expectedDirectory = [System.IO.Path]::GetFullPath(
        (Join-Path $ProjectDirectory 'dist'))
    $requestedDirectory = [System.IO.Path]::GetFullPath($DistributionDirectory)
    if (-not $requestedDirectory.Equals(
            $expectedDirectory, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to clean unexpected distribution path: $requestedDirectory"
    }
    if (Test-Path -LiteralPath $requestedDirectory) {
        Remove-Item -LiteralPath $requestedDirectory -Recurse -Force
    }
    New-Item -ItemType Directory -Path $requestedDirectory | Out-Null
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
if (-not (Test-Path -LiteralPath $TemplatePath -PathType Leaf)) {
    throw "The pinned compatibility template was not found at $TemplatePath"
}
if (-not (Test-Path -LiteralPath $updaterSource -PathType Leaf)) {
    throw "The guarded RDX updater was not found at $updaterSource"
}
if (-not (Test-Path -LiteralPath $installationProcedureSource -PathType Leaf)) {
    throw "The compatibility installation procedure was not found at $installationProcedureSource"
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

Push-Location $projectRoot
try {
    Invoke-CheckedCommand `
        -FilePath $PlatformIoPath `
        -Arguments @('run', '-e', 'tusb9261_ti_cgt') `
        -Description 'Building OpenRDX with TI ARM CGT'

    if (-not $SkipTests) {
        Invoke-CheckedCommand `
            -FilePath $PythonPath `
            -Arguments @(
                '-m', 'unittest', 'discover', '-s', 'tests', '-v'
            ) `
            -Description 'Running the complete automated firmware and release suite'
    }

    Reset-DistributionDirectory `
        -ProjectDirectory $projectRoot `
        -DistributionDirectory $distDirectory
    $versionFileName = $Version.Replace('.', '-')
    $releaseBaseName = "OpenRDX-v$versionFileName"
    $distBinary = Join-Path $distDirectory "$releaseBaseName.bin"
    $distRecoveryHex = Join-Path $distDirectory "$releaseBaseName-FlashBurner.hex"
    $distManifest = Join-Path $distDirectory "$releaseBaseName.json"
    $distChecksums = Join-Path $distDirectory "$releaseBaseName.sha256"
    $distUpdater = Join-Path $distDirectory 'rdx_manager_firmware_update.ps1'
    $distInstallationProcedure = Join-Path $distDirectory 'OPENRDX_USB_UPDATE.md'
    $firmwareHex = Join-Path $buildDirectory 'TUSB9261_RDX.hex'
    $flashBurnerHex = Join-Path $buildDirectory 'TUSB9261_RDX_flash.hex'

    Invoke-CheckedCommand `
        -FilePath $PythonPath `
        -Arguments @(
            'scripts\build_rdx_update_container.py',
            $firmwareHex,
            $TemplatePath,
            $distBinary,
            '--manifest',
            $distManifest
        ) `
        -Description 'Creating the versioned RDX Manager update container'

    if (-not (Test-Path -LiteralPath $flashBurnerHex -PathType Leaf)) {
        throw "The continuous FlashBurner image was not generated at $flashBurnerHex"
    }
    Copy-Item -LiteralPath $flashBurnerHex -Destination $distRecoveryHex -Force
    Copy-Item -LiteralPath $updaterSource -Destination $distUpdater -Force
    Copy-Item `
        -LiteralPath $installationProcedureSource `
        -Destination $distInstallationProcedure `
        -Force

    $manifest = Get-Content -Raw -LiteralPath $distManifest | ConvertFrom-Json
    $updaterHash = Get-Sha256 -Path $distUpdater
    $installationProcedureHash = Get-Sha256 -Path $distInstallationProcedure
    $manifest.container = Split-Path -Leaf $distBinary
    # The build manifest records its compiler input for developer diagnostics.
    # A release manifest must be relocatable, so remove that checkout-only path.
    $manifest.PSObject.Properties.Remove('firmware_hex')
    $manifest.flashburner_hex = Split-Path -Leaf $distRecoveryHex
    $manifest.flashburner_hex_sha256 = (Get-Sha256 -Path $distRecoveryHex).ToLowerInvariant()
    $manifest | Add-Member -NotePropertyName release_version -NotePropertyValue $Version
    $manifest | Add-Member -NotePropertyName release_name -NotePropertyValue $releaseBaseName
    # Record the on-device compatibility prerequisite without encoding release
    # history into the portable package metadata.
    $manifest | Add-Member `
        -NotePropertyName required_receiver_kind `
        -NotePropertyValue 'CompatibilityReceiver'
    $manifest | Add-Member `
        -NotePropertyName installation_updater `
        -NotePropertyValue (Split-Path -Leaf $distUpdater)
    $manifest | Add-Member `
        -NotePropertyName installation_updater_sha256 `
        -NotePropertyValue $updaterHash.ToLowerInvariant()
    $manifest | Add-Member `
        -NotePropertyName installation_procedure `
        -NotePropertyValue (Split-Path -Leaf $distInstallationProcedure)
    $manifest | Add-Member `
        -NotePropertyName installation_procedure_sha256 `
        -NotePropertyValue $installationProcedureHash.ToLowerInvariant()
    $manifest | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $distManifest -Encoding utf8

    $binaryHash = Get-Sha256 -Path $distBinary
    $recoveryHash = Get-Sha256 -Path $distRecoveryHex
    $manifestHash = Get-Sha256 -Path $distManifest
    @(
        "$($binaryHash.ToLowerInvariant()) *$(Split-Path -Leaf $distBinary)",
        "$($recoveryHash.ToLowerInvariant()) *$(Split-Path -Leaf $distRecoveryHex)",
        "$($updaterHash.ToLowerInvariant()) *$(Split-Path -Leaf $distUpdater)",
        "$($installationProcedureHash.ToLowerInvariant()) *$(Split-Path -Leaf $distInstallationProcedure)",
        "$($manifestHash.ToLowerInvariant()) *$(Split-Path -Leaf $distManifest)"
    ) | Set-Content -LiteralPath $distChecksums -Encoding ascii

    if ((Get-Item -LiteralPath $distBinary).Length -ne 62110) {
        throw 'The RDX Manager update container does not have the required 62,110-byte length.'
    }
    if ($binaryHash -ne ([string]$manifest.container_sha256).ToUpperInvariant()) {
        throw 'The release binary digest does not match its generated manifest.'
    }

    Write-Host ''
    Write-Host "Release $Version created successfully in $distDirectory"
    Get-Item -LiteralPath `
        $distBinary, $distRecoveryHex, $distUpdater, $distInstallationProcedure, `
        $distManifest, $distChecksums |
        Select-Object Name, Length |
        Format-Table -AutoSize
    Write-Host "RDX Manager binary SHA-256: $binaryHash"
} finally {
    Pop-Location
}
