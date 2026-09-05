# SPDX-FileCopyrightText: 2026 André Fiedler
#
# SPDX-License-Identifier: AGPL-3.0-or-later

<#
Exercise the updater's in-place main block with simulated Windows devices.
Only constant initialization, function definitions, and the main block are
loaded from the production AST. Native transport and privilege initialization
are excluded. This validates host control flow, not SPTI or receiver firmware.
#>
[CmdletBinding(DefaultParameterSetName = 'InstallOpenRDX')]
param(
    [Parameter(Mandatory = $true)] [string] $UpdaterPath,
    [Parameter(Mandatory = $true)] [string] $ImagePath,
    [Parameter(Mandatory = $true)] [string] $ManifestPath,
    [ValidateSet('success', 'media', 'identity', 'authentication')]
    [string] $Scenario = 'success'
)

$ErrorActionPreference = 'Stop'
$tokens = $null
$parseErrors = $null
$ast = [System.Management.Automation.Language.Parser]::ParseFile(
    $UpdaterPath, [ref] $tokens, [ref] $parseErrors)
if ($parseErrors.Count -ne 0) { throw 'Updater did not parse.' }
$source = $ast.Extent.Text
$constantStart = $source.IndexOf("`$ErrorActionPreference = 'Stop'")
$constantEnd = $source.IndexOf('$windowsIdentity =')
$mainStart = $source.IndexOf('$targetKind = if')
if ($constantStart -lt 0 -or $constantEnd -le $constantStart -or
    $mainStart -le $constantEnd) { throw 'Updater structure changed.' }
foreach ($statement in $ast.EndBlock.Statements) {
    if ($statement.Extent.StartOffset -ge $constantStart -and
        $statement.Extent.EndOffset -le $constantEnd -and
        $statement -is [System.Management.Automation.Language.AssignmentStatementAst] -and
        $statement.Left.Extent.Text -ne '$developmentManifestPath') {
        # Explicit fixture paths make the checkout-dependent default unnecessary.
        . ([scriptblock]::Create($statement.Extent.Text))
    }
    if ($statement -is [System.Management.Automation.Language.FunctionDefinitionAst]) {
        . ([scriptblock]::Create($statement.Extent.Text))
    }
}

# No native library or device handle is available to the simulated transport.
Add-Type -TypeDefinition @'
using System;
using System.Collections.Generic;
public static class RdxManagerScsiTransport
{
    public static List<string> Commands = new List<string>();
    public static bool RejectFinalChunk;

    /** Record a command without performing any device I/O. */
    public static string Send(string path, byte[] cdb, byte[] payload, uint timeout)
    {
        Commands.Add(path + "|" + BitConverter.ToString(cdb) + "|" +
            (payload == null ? 0 : payload.Length));
        if (RejectFinalChunk && cdb[1] == 4 && cdb[4] == 0xF0)
            throw new InvalidOperationException("SCSI authentication failure 07/74/08");
        return "GOOD";
    }
}
'@

$InstallOpenRDX = $true
$InstallOpenRDXOnCompatibilityReceiver = $false
$RepairCompatibilityReceiver = $false
$Update = $false
$EjectOnly = $false
$TargetSerialNumber = 'TESTSERIAL12'
$script:diskQueries = 0
$script:sleepCalls = 0
[RdxManagerScsiTransport]::RejectFinalChunk = $Scenario -eq 'authentication'

function Get-CimInstance {
    <# Return a synthetic empty OpenRDX receiver; never query Windows. #>
    param([string] $ClassName)
    if ($ClassName -ne 'Win32_DiskDrive') { throw 'Unexpected CIM class.' }
    $script:diskQueries++
    $serial = if ($Scenario -eq 'identity' -and $script:diskQueries -gt 1) {
        'CHANGED00000'
    } else { $TargetSerialNumber }
    [pscustomobject] @{
        Index = 99
        Model = 'TANDBERG RDX USB Device'
        FirmwareRevision = '0001'
        SerialNumber = $serial
        PNPDeviceID = "USBSTOR\DISK&VEN_TANDBERG&PROD_RDX&REV_0001\$serial&0"
        Size = 0
        MediaLoaded = $true
    }
}

function Get-CimAssociatedInstance {
    <# Simulate no partition or mounted-volume associations. #>
    param($InputObject, [string] $Association)
}

function Get-Disk {
    <# Exercise the stale WMI flag exception using identity-matched storage. #>
    param([uint32] $Number, [string] $ErrorAction)
    if ($Number -ne 99) { throw 'Unexpected disk selection.' }
    [pscustomobject] @{
        SerialNumber = $TargetSerialNumber
        OperationalStatus = if ($Scenario -eq 'media') { 'Online' } else { 'No Media' }
        Size = 0
    }
}

function Get-PnpDeviceProperty {
    <# Supply a synthetic physical USB parent and stable connector. #>
    param([string] $InstanceId, [string] $KeyName, [string] $ErrorAction)
    $data = switch ($KeyName) {
        'DEVPKEY_Device_Parent' { "USB\VID_1A5A&PID_0005\$TargetSerialNumber" }
        'DEVPKEY_Device_LocationPaths' { 'TEST_USB_PORT_1' }
        default { throw 'Unexpected PnP property.' }
    }
    [pscustomobject] @{ Data = $data }
}

function Get-PnpDevice {
    <# Simulate application enumeration at the selected connector. #>
    param([switch] $PresentOnly, [string] $ErrorAction)
    [pscustomobject] @{ InstanceId = "USB\VID_1A5A&PID_0005\$TargetSerialNumber" }
}

function Start-Sleep {
    <# Skip deliberate delays and fail promptly on unexpected polling loops. #>
    param([int] $Seconds, [int] $Milliseconds)
    $script:sleepCalls++
    if ($script:sleepCalls -gt 3) { throw 'Unexpected simulated polling loop.' }
}

$failure = $null
try {
    . ([scriptblock]::Create($source.Substring($mainStart)))
} catch {
    $failure = $_.Exception.Message
}
$result = [ordered] @{
    error = $failure
    commands = @([RdxManagerScsiTransport]::Commands)
    disk_queries = $script:diskQueries
}
Write-Output ('WORKFLOW_RESULT ' + ($result | ConvertTo-Json -Depth 4 -Compress))
