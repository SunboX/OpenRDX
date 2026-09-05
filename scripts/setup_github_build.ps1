# SPDX-FileCopyrightText: 2026 André Fiedler
#
# SPDX-License-Identifier: AGPL-3.0-or-later

<# Install checksum-pinned build inputs on a disposable Windows CI runner. #>
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)] [string] $ToolsDirectory
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

function Get-VerifiedDownload {
    <# Download an input and reject any bytes outside the pinned release. #>
    param([string] $Uri, [string] $Path, [string] $Sha256)

    for ($attempt = 1; $attempt -le 4; $attempt++) {
        try {
            Invoke-WebRequest -Uri $Uri -OutFile $Path -TimeoutSec 120
            break
        } catch {
            if ($attempt -eq 4) { throw }
            Start-Sleep -Seconds (5 * $attempt)
        }
    }
    if ((Get-FileHash -Algorithm SHA256 -LiteralPath $Path).Hash -ne $Sha256) {
        throw "SHA-256 mismatch for $Uri"
    }
}

New-Item -ItemType Directory -Force -Path $ToolsDirectory | Out-Null
$archive = Join-Path $ToolsDirectory 'ti-cgt-arm-5.2.5.zip'
Get-VerifiedDownload `
    -Uri 'https://software-dl.ti.com/dsps/dsps_public_sw/sdo_ccstudio/codegen/Updates/p2win32/binary/com.ti.cgt.tms470.5.2.win32_root_5.2.5' `
    -Path $archive `
    -Sha256 '1c6dff3f131a22ec2dc3e9a52c2181b6c9d81a40964bfcf25879f9c0f5c700ee'
Expand-Archive -LiteralPath $archive -DestinationPath $ToolsDirectory -Force
$installer = Join-Path $ToolsDirectory 'downloads\ti_cgt_tms470_5.2.5_windows_installer.exe'
if ((Get-FileHash -Algorithm SHA256 -LiteralPath $installer).Hash -ne `
        'eabb8ca9c7376cdfdec9413edfbf426db5c88903ff6c1c05e68ad1c8c656e2fb') {
    throw 'TI compiler installer SHA-256 mismatch.'
}
$compilerRoot = Join-Path $ToolsDirectory 'ti-cgt-arm_5.2.5'
$process = Start-Process -FilePath $installer -Wait -PassThru -ArgumentList @(
    '--mode', 'unattended', '--prefix', ('"{0}"' -f $compilerRoot)
)
if ($process.ExitCode -ne 0) {
    throw "TI compiler installer failed with exit code $($process.ExitCode)."
}
# The 5.2.5 installer adds its own versioned child below --prefix. Resolve
# the one installed bin directory instead of assuming the 5.2.9 layout.
$installedCompilers = @(
    Get-ChildItem -LiteralPath $ToolsDirectory -Recurse -File -Filter 'armcl.exe' |
        Where-Object { $_.Directory.Name -eq 'bin' }
)
if ($installedCompilers.Count -ne 1) {
    throw "Expected one installed TI compiler below $ToolsDirectory; found $($installedCompilers.Count)."
}
$compilerRoot = $installedCompilers[0].Directory.Parent.FullName
Write-Host "Installed TI compiler root: $compilerRoot"
foreach ($name in @('armcl.exe', 'armhex.exe')) {
    $tool = Join-Path $compilerRoot "bin\$name"
    if (-not (Test-Path -LiteralPath $tool -PathType Leaf)) {
        throw "Required TI CGT 5.2.5 tool missing: $tool"
    }
}
$compilerVersion = & (Join-Path $compilerRoot 'bin\armcl.exe') --compiler_revision
if ($LASTEXITCODE -ne 0 -or "$compilerVersion" -notmatch '5\.2\.5') {
    throw "Unexpected TI compiler revision: $compilerVersion"
}
Write-Host "TI compiler revision: $compilerVersion"

$template = Join-Path $ToolsDirectory 'RDX2E__STD__F-0283.bin'
# This is an archived copy of the vendor's published download, pinned by the
# same digest enforced by the container builder and guarded updater.
Get-VerifiedDownload `
    -Uri 'https://web.archive.org/web/20220104152216if_/https://ftp1.overlandtandberg.com/rdx/RDX%20QuikStor/Firmware/USB/USB3.0/external/RDX2E__STD__F-0283.bin' `
    -Path $template `
    -Sha256 '73d528801aefc032d3a53637b035f65d72809151b2f127c6b2050e9bacc76f3b'

"TI_CGT_ROOT=$compilerRoot" | Out-File -FilePath $env:GITHUB_ENV -Append -Encoding utf8
"OPENRDX_TEMPLATE_PATH=$template" | Out-File -FilePath $env:GITHUB_ENV -Append -Encoding utf8
