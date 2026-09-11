# SPDX-FileCopyrightText: 2026 André Fiedler
#
# SPDX-License-Identifier: AGPL-3.0-or-later

[CmdletBinding(DefaultParameterSetName = 'Validate')]
param(
    [Parameter(ParameterSetName = 'Validate')]
    [switch] $ValidateOnly,

    [Parameter(ParameterSetName = 'Validate')]
    [ValidateSet('OpenRDX', 'CompatibilityReceiver')]
    [string] $ValidateFirmwareKind = 'OpenRDX',

    [Parameter(Mandatory = $true, ParameterSetName = 'Eject')]
    [switch] $EjectOnly,

    [Parameter(Mandatory = $true, ParameterSetName = 'Update')]
    [switch] $Update,

    [Parameter(Mandatory = $true, ParameterSetName = 'Restore')]
    [switch] $RestoreOpenRDX,

    [Parameter(Mandatory = $true, ParameterSetName = 'InstallOpenRDX')]
    [switch] $InstallOpenRDX,

    [Parameter(Mandatory = $true, ParameterSetName = 'InstallOpenRDXOnCompatibilityReceiver')]
    [switch] $InstallOpenRDXOnCompatibilityReceiver,

    [Parameter(Mandatory = $true, ParameterSetName = 'Repair')]
    [switch] $RepairCompatibilityReceiver,

    [Parameter(ParameterSetName = 'Update')]
    [Parameter(ParameterSetName = 'Restore')]
    [Parameter(Mandatory = $true, ParameterSetName = 'InstallOpenRDX')]
    [Parameter(ParameterSetName = 'InstallOpenRDXOnCompatibilityReceiver')]
    [Parameter(Mandatory = $true, ParameterSetName = 'Repair')]
    [string] $ImagePath,

    [Parameter(ParameterSetName = 'Restore')]
    [Parameter(ParameterSetName = 'InstallOpenRDX')]
    [Parameter(ParameterSetName = 'InstallOpenRDXOnCompatibilityReceiver')]
    [string] $ManifestPath,

    [Parameter(ParameterSetName = 'InstallOpenRDXOnCompatibilityReceiver')]
    [string] $FlashBurnerPath = 'C:\Program Files (x86)\Texas Instruments Inc\Command Line FlashBurner\TUSB926x_CL_Burner.exe',

    [Parameter(ParameterSetName = 'InstallOpenRDXOnCompatibilityReceiver')]
    [ValidateRange(30, 900)]
    [int] $ReenumerationTimeoutSeconds = 300,

    [string] $TargetSerialNumber
)

$ErrorActionPreference = 'Stop'

$expectedImageLength = 62110
$compatibilityImageSha256 = '73D528801AEFC032D3A53637B035F65D72809151B2F127C6B2050E9BACC76F3B'
$expectedModel = 'TANDBERG RDX USB Device'
# Retain discovery of legacy builds while admitting the release-based revision.
$openRdxFirmwareRevisions = @('0001', '0106', '0107', '0108')
$openRdxUsbPnpPrefix = 'USB\VID_1A5A&PID_0005\'
$compatibilityRevision = '0283'
$compatibilityDiskPnpPrefix = 'USBSTOR\DISK&VEN_TANDBERG&PROD_RDX&REV_0283\'
$chunkLength = 4096
$developmentManifestPath = Join-Path $PSScriptRoot '..\.pio\build\tusb9261_ti_cgt\TUSB9261_RDX_update.json'
$romBootloaderPnpPrefix = 'USB\VID_0451&PID_926B\'

$windowsIdentity = [Security.Principal.WindowsIdentity]::GetCurrent()
$windowsPrincipal = [Security.Principal.WindowsPrincipal]::new($windowsIdentity)
$isAdministrator = $windowsPrincipal.IsInRole(
    [Security.Principal.WindowsBuiltInRole]::Administrator)
if ($PSCmdlet.ParameterSetName -ne 'Validate' -and -not $isAdministrator) {
    throw 'This operation requires an elevated PowerShell session (Run as administrator).'
}

$nativeSource = @'
using System;
using System.ComponentModel;
using System.Runtime.InteropServices;
using Microsoft.Win32.SafeHandles;

public static class RdxManagerScsiTransport
{
    private const uint GenericRead = 0x80000000U;
    private const uint GenericWrite = 0x40000000U;
    private const uint ShareReadWrite = 3U;
    private const uint OpenExisting = 3U;
    private const uint IoctlScsiPassThroughDirect = 0x4D014U;
    private const byte ScsiIoctlDataOut = 0;
    private const byte ScsiIoctlDataUnspecified = 2;
    private const int SenseLength = 32;

    [StructLayout(LayoutKind.Sequential)]
    private struct ScsiPassThroughDirect
    {
        public ushort Length;
        public byte ScsiStatus;
        public byte PathId;
        public byte TargetId;
        public byte Lun;
        public byte CdbLength;
        public byte SenseInfoLength;
        public byte DataIn;
        public uint DataTransferLength;
        public uint TimeOutValue;
        public IntPtr DataBuffer;
        public uint SenseInfoOffset;

        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 16)]
        public byte[] Cdb;
    }

    [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    private static extern SafeFileHandle CreateFile(
        string fileName,
        uint desiredAccess,
        uint shareMode,
        IntPtr securityAttributes,
        uint creationDisposition,
        uint flagsAndAttributes,
        IntPtr templateFile);

    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern bool DeviceIoControl(
        SafeFileHandle device,
        uint controlCode,
        IntPtr input,
        uint inputLength,
        IntPtr output,
        uint outputLength,
        out uint bytesReturned,
        IntPtr overlapped);

    /** Send one command through the same Windows SPTI transport used by RDX Manager. */
    public static string Send(string devicePath, byte[] cdb, byte[] payload, uint timeoutSeconds)
    {
        if (cdb == null || cdb.Length == 0 || cdb.Length > 16)
        {
            throw new ArgumentException("CDB length must be between one and sixteen bytes.");
        }

        using (SafeFileHandle handle = CreateFile(
            devicePath,
            GenericRead | GenericWrite,
            ShareReadWrite,
            IntPtr.Zero,
            OpenExisting,
            0,
            IntPtr.Zero))
        {
            if (handle.IsInvalid)
            {
                throw new Win32Exception(Marshal.GetLastWin32Error(), "Opening " + devicePath);
            }

            int headerLength = Marshal.SizeOf(typeof(ScsiPassThroughDirect));
            int packetLength = headerLength + SenseLength;
            IntPtr packet = Marshal.AllocHGlobal(packetLength);
            IntPtr data = IntPtr.Zero;

            try
            {
                for (int index = 0; index < packetLength; index++)
                {
                    Marshal.WriteByte(packet, index, 0);
                }

                if (payload != null && payload.Length != 0)
                {
                    data = Marshal.AllocHGlobal(payload.Length);
                    Marshal.Copy(payload, 0, data, payload.Length);
                }

                byte[] paddedCdb = new byte[16];
                Array.Copy(cdb, paddedCdb, cdb.Length);
                ScsiPassThroughDirect command = new ScsiPassThroughDirect
                {
                    Length = (ushort)headerLength,
                    CdbLength = (byte)cdb.Length,
                    SenseInfoLength = SenseLength,
                    DataIn = payload == null ? ScsiIoctlDataUnspecified : ScsiIoctlDataOut,
                    DataTransferLength = payload == null ? 0U : (uint)payload.Length,
                    TimeOutValue = timeoutSeconds,
                    DataBuffer = data,
                    SenseInfoOffset = (uint)headerLength,
                    Cdb = paddedCdb
                };

                Marshal.StructureToPtr(command, packet, false);
                uint returned;
                if (!DeviceIoControl(
                    handle,
                    IoctlScsiPassThroughDirect,
                    packet,
                    (uint)packetLength,
                    packet,
                    (uint)packetLength,
                    out returned,
                    IntPtr.Zero))
                {
                    throw new Win32Exception(Marshal.GetLastWin32Error(), "SCSI pass-through");
                }

                command = (ScsiPassThroughDirect)Marshal.PtrToStructure(
                    packet,
                    typeof(ScsiPassThroughDirect));
                if (command.ScsiStatus != 0)
                {
                    byte[] sense = new byte[SenseLength];
                    Marshal.Copy(IntPtr.Add(packet, headerLength), sense, 0, sense.Length);
                    throw new InvalidOperationException(
                        "SCSI status 0x" + command.ScsiStatus.ToString("X2") +
                        ", sense " + BitConverter.ToString(sense).Replace('-', ' '));
                }

                return "GOOD";
            }
            finally
            {
                if (data != IntPtr.Zero)
                {
                    Marshal.FreeHGlobal(data);
                }
                Marshal.FreeHGlobal(packet);
            }
        }
    }
}
'@

if (-not ('RdxManagerScsiTransport' -as [type])) {
    Add-Type -TypeDefinition $nativeSource
}

function Find-RdxCompatibilityImage {
    <#
    Locate the pinned compatibility image in a development workspace.

    Release bundles intentionally omit this image. A release user must pass
    -ImagePath explicitly; a developer gets a safe convenience lookup only
    when the adjacent OpenRDXManager tree contains matching pinned bytes.
    #>
    $managerRoot = [System.IO.Path]::GetFullPath(
        (Join-Path $PSScriptRoot '..\..\OpenRDXManager'))
    if (-not (Test-Path -LiteralPath $managerRoot -PathType Container)) {
        throw 'Compatibility image not found; pass its exact path with -ImagePath.'
    }

    $candidates = @(
        Get-ChildItem -LiteralPath $managerRoot -Recurse -File `
            -Filter 'RDX2E__STD__F-0283.bin' |
            Sort-Object -Property FullName |
            Where-Object {
                (Get-FileHash -Algorithm SHA256 -LiteralPath $_.FullName).Hash `
                    -eq $compatibilityImageSha256
            }
    )
    if ($candidates.Count -eq 0) {
        throw "No pinned compatibility image was found below $managerRoot. Pass -ImagePath explicitly."
    }
    # Multiple byte-identical copies are interchangeable; stable path sorting
    # makes the convenience lookup deterministic without weakening validation.
    return $candidates[0].FullName
}

function Get-RdxCustomManifestPath {
    <# Resolve one custom manifest beside an explicit image, in a release bundle, or in the local build tree. #>
    param([string] $CandidateImagePath)

    if (-not [string]::IsNullOrWhiteSpace($CandidateImagePath)) {
        $siblingManifest = [System.IO.Path]::ChangeExtension(
            $CandidateImagePath, '.json')
        if (Test-Path -LiteralPath $siblingManifest -PathType Leaf) {
            return $siblingManifest
        }
    }

    $bundledManifests = @(
        Get-ChildItem -LiteralPath $PSScriptRoot -Filter 'OpenRDX-v*.json' -File |
            Sort-Object -Property Name
    )
    if ($bundledManifests.Count -eq 1) {
        return $bundledManifests[0].FullName
    }
    if ($bundledManifests.Count -gt 1) {
        throw 'More than one OpenRDX release manifest is beside the updater; keep exactly one release in the installation directory.'
    }
    if (Test-Path -LiteralPath $developmentManifestPath -PathType Leaf) {
        return $developmentManifestPath
    }
    throw 'No custom manifest was supplied and no single OpenRDX release manifest was found beside the updater.'
}

function Get-RdxManifestContainerPath {
    <# Resolve the leaf container named by a custom manifest without accepting a path outside its directory. #>
    param([Parameter(Mandatory = $true)] [string] $CustomManifestPath)

    if (-not (Test-Path -LiteralPath $CustomManifestPath -PathType Leaf)) {
        throw "Custom image manifest not found: $CustomManifestPath"
    }
    $customManifest = Get-Content -Raw -LiteralPath $CustomManifestPath |
        ConvertFrom-Json
    $containerName = [string]$customManifest.container
    if ([string]::IsNullOrWhiteSpace($containerName) -or
        [System.IO.Path]::GetFileName($containerName) -ne $containerName) {
        throw 'The custom manifest must name its container as a file in the same directory.'
    }
    $manifestDirectory = Split-Path -Parent (
        Resolve-Path -LiteralPath $CustomManifestPath)
    return Join-Path $manifestDirectory $containerName
}

if ($RestoreOpenRDX) {
    $InstallOpenRDXOnCompatibilityReceiver = $true
}
if (($InstallOpenRDX -or $InstallOpenRDXOnCompatibilityReceiver) -and
    (-not $PSBoundParameters.ContainsKey('ManifestPath') -or
        [string]::IsNullOrWhiteSpace($ManifestPath))) {
    $ManifestPath = Get-RdxCustomManifestPath -CandidateImagePath $ImagePath
}
if ($InstallOpenRDXOnCompatibilityReceiver -and
    (-not $PSBoundParameters.ContainsKey('ImagePath') -or
        [string]::IsNullOrWhiteSpace($ImagePath))) {
    $ImagePath = Get-RdxManifestContainerPath -CustomManifestPath $ManifestPath
}
if ($Update -and
    (-not $PSBoundParameters.ContainsKey('ImagePath') -or
        [string]::IsNullOrWhiteSpace($ImagePath))) {
    $ImagePath = Find-RdxCompatibilityImage
}

function Test-RdxSignatureFailure {
    <# Recognize fixed-format DATA PROTECT / 74h / 08h sense exactly. #>
    param([Parameter(Mandatory = $true)] [string] $Message)

    $match = [regex]::Match($Message, 'sense (?<bytes>(?:[0-9A-Fa-f]{2} ?)+)$')
    if (-not $match.Success) {
        return $false
    }
    $sense = @($match.Groups['bytes'].Value.Trim().Split(' ') | Where-Object { $_ })
    return $sense.Count -ge 14 -and
        (([Convert]::ToByte($sense[2], 16) -band 0x0F) -eq 0x07) -and
        $sense[12].ToUpperInvariant() -eq '74' -and
        $sense[13].ToUpperInvariant() -eq '08'
}

function Get-RdxUsbAncestorInstanceId {
    <#
    Walk a disk LUN's PnP parent chain to the physical USB device node.

    Disk instance identifiers change when firmware changes the product revision
    or serial descriptor. The physical USB node's location path identifies the
    connector used by the selected adapter and therefore survives each
    re-enumeration. A composite-interface node contains an MI component and is
    deliberately skipped because its interface-specific location path is not
    shared by the ROM-loader device.
    #>
    param([Parameter(Mandatory = $true)] [string] $InstanceId)

    $currentInstanceId = $InstanceId
    for ($depth = 0; $depth -lt 16; $depth++) {
        if ($currentInstanceId.StartsWith(
                'USB\VID_', [StringComparison]::OrdinalIgnoreCase) -and
            $currentInstanceId.IndexOf(
                '&MI_', [StringComparison]::OrdinalIgnoreCase) -lt 0) {
            return $currentInstanceId
        }

        $parentProperty = Get-PnpDeviceProperty `
            -InstanceId $currentInstanceId `
            -KeyName 'DEVPKEY_Device_Parent' `
            -ErrorAction Stop
        $parentInstanceId = [string]$parentProperty.Data
        if ([string]::IsNullOrWhiteSpace($parentInstanceId) -or
            $parentInstanceId.Equals(
                $currentInstanceId, [StringComparison]::OrdinalIgnoreCase)) {
            break
        }
        $currentInstanceId = $parentInstanceId
    }

    throw "Could not locate the physical USB ancestor for PnP device $InstanceId"
}

function Get-RdxPnpLocationPaths {
    <# Return every non-empty physical location path exposed by one PnP node. #>
    param([Parameter(Mandatory = $true)] [string] $InstanceId)

    $locationProperty = Get-PnpDeviceProperty `
        -InstanceId $InstanceId `
        -KeyName 'DEVPKEY_Device_LocationPaths' `
        -ErrorAction Stop
    $locationPaths = @(
        @($locationProperty.Data) |
            ForEach-Object { ([string]$_).Trim() } |
            Where-Object { -not [string]::IsNullOrWhiteSpace($_) } |
            Select-Object -Unique
    )
    if ($locationPaths.Count -eq 0) {
        throw "PnP device $InstanceId does not expose a physical location path."
    }
    return $locationPaths
}

function Get-RdxDiskUsbLocationPaths {
    <# Resolve a disk LUN to the stable location paths of its physical USB node. #>
    param([Parameter(Mandatory = $true)] [string] $DiskPnpDeviceId)

    $usbInstanceId = Get-RdxUsbAncestorInstanceId -InstanceId $DiskPnpDeviceId
    return @(Get-RdxPnpLocationPaths -InstanceId $usbInstanceId)
}

function Test-RdxLocationPathIntersection {
    <# Test two location-path sets with Windows instance-name case semantics. #>
    param(
        [Parameter(Mandatory = $true)] [string[]] $CandidatePaths,
        [Parameter(Mandatory = $true)] [string[]] $RequiredPaths
    )

    foreach ($candidatePath in $CandidatePaths) {
        foreach ($requiredPath in $RequiredPaths) {
            if ($candidatePath.Equals(
                    $requiredPath, [StringComparison]::OrdinalIgnoreCase)) {
                return $true
            }
        }
    }
    return $false
}

function Wait-RdxPnpPrefix {
    <#
    Wait for a PnP role at the selected adapter's physical USB location.

    FlashBurner addresses ROM loaders by a numeric index, so its caller also
    requires there to be exactly one loader system-wide before using index 0.
    Other roles may coexist because their location path is checked explicitly.
    #>
    param(
        [Parameter(Mandatory = $true)] [string] $Prefix,
        [Parameter(Mandatory = $true)] [int] $TimeoutSeconds,
        [Parameter(Mandatory = $true)] [string[]] $RequiredLocationPaths,
        [switch] $RequireSingleGlobalMatch
    )

    $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
    do {
        $matches = @(
            Get-PnpDevice -PresentOnly -ErrorAction SilentlyContinue |
                Where-Object {
                    $_.InstanceId.StartsWith(
                        $Prefix, [StringComparison]::OrdinalIgnoreCase)
                }
        )
        if ($RequireSingleGlobalMatch -and $matches.Count -gt 1) {
            throw "More than one present device matched PnP prefix $Prefix; index-based programming is unsafe."
        }

        $locationMatches = @()
        foreach ($match in $matches) {
            try {
                $candidatePaths = @(
                    Get-RdxPnpLocationPaths -InstanceId $match.InstanceId)
                if (Test-RdxLocationPathIntersection `
                        -CandidatePaths $candidatePaths `
                        -RequiredPaths $RequiredLocationPaths) {
                    $locationMatches += $match
                }
            } catch {
                # A just-enumerated node can briefly lack its location property.
                # Poll again rather than ever accepting an unbound device.
            }
        }
        if ($locationMatches.Count -eq 1) {
            return $locationMatches[0]
        }
        if ($locationMatches.Count -gt 1) {
            throw "More than one present device matched the selected USB location for prefix $Prefix"
        }
        Start-Sleep -Milliseconds 500
    } while ([DateTime]::UtcNow -lt $deadline)
    return $null
}

function Get-RdxTargetCandidates {
    <#
    Enumerate RDX disk LUNs for one firmware family and optional identity pins.

    The serial filter selects among multiple compatible adapters at startup.
    The location filter is used after reset, when firmware may intentionally
    expose a different serial or revision while remaining on the same USB port.
    #>
    param(
        [Parameter(Mandatory = $true)]
        [ValidateSet('OpenRDX', 'CompatibilityReceiver')]
        [string] $FirmwareKind,
        [string] $SerialNumber,
        [string[]] $RequiredLocationPaths
    )

    if ($FirmwareKind -eq 'CompatibilityReceiver') {
        $firmwareRevisions = @($compatibilityRevision)
    } else {
        $firmwareRevisions = $openRdxFirmwareRevisions
    }

    $trimmedSerial = if ([string]::IsNullOrWhiteSpace($SerialNumber)) {
        $null
    } else {
        $SerialNumber.Trim()
    }
    $candidates = @(
        Get-CimInstance Win32_DiskDrive |
            Where-Object {
                # CIM and the PnP instance must describe the same accepted
                # revision; the USB parent, serial, and location checks follow.
                $diskPnpPrefix = if ($FirmwareKind -eq 'CompatibilityReceiver') {
                    $compatibilityDiskPnpPrefix
                } else {
                    'USBSTOR\DISK&VEN_TANDBERG&PROD_RDX&REV_' +
                        ([string]$_.FirmwareRevision) + '\'
                }
                $_.Model -eq $expectedModel -and
                $_.FirmwareRevision -cin $firmwareRevisions -and
                ([string]$_.PNPDeviceID).StartsWith(
                    $diskPnpPrefix, [StringComparison]::OrdinalIgnoreCase) -and
                ($null -eq $trimmedSerial -or
                    ([string]$_.SerialNumber).Trim().Equals(
                        $trimmedSerial, [StringComparison]::OrdinalIgnoreCase))
            }
    )

    if ($null -eq $RequiredLocationPaths -or
        $RequiredLocationPaths.Count -eq 0) {
        return $candidates
    }

    $boundCandidates = @()
    foreach ($candidate in $candidates) {
        try {
            $candidatePaths = @(
                Get-RdxDiskUsbLocationPaths `
                    -DiskPnpDeviceId ([string]$candidate.PNPDeviceID))
            if (Test-RdxLocationPathIntersection `
                    -CandidatePaths $candidatePaths `
                    -RequiredPaths $RequiredLocationPaths) {
                $boundCandidates += $candidate
            }
        } catch {
            # During enumeration the disk can precede its complete PnP parent
            # chain. A later polling iteration will evaluate it again.
        }
    }
    return $boundCandidates
}

function Get-RdxTarget {
    <# Select exactly one compatible RDX adapter, optionally by serial number. #>
    param(
        [Parameter(Mandatory = $true)]
        [ValidateSet('OpenRDX', 'CompatibilityReceiver')]
        [string] $FirmwareKind,
        [string] $SerialNumber
    )

    $targets = @(
        Get-RdxTargetCandidates `
            -FirmwareKind $FirmwareKind `
            -SerialNumber $SerialNumber)
    if ($targets.Count -eq 1) {
        return $targets[0]
    }
    if ($targets.Count -gt 1 -and [string]::IsNullOrWhiteSpace($SerialNumber)) {
        throw "Found $($targets.Count) compatible $FirmwareKind RDX adapters; pass -TargetSerialNumber or disconnect all but the intended adapter."
    }
    $selection = if ([string]::IsNullOrWhiteSpace($SerialNumber)) {
        ''
    } else {
        " with serial '$($SerialNumber.Trim())'"
    }
    throw "Expected exactly one compatible $FirmwareKind RDX target$selection; found $($targets.Count)."
}

function Wait-RdxTarget {
    <# Wait for one compatible disk LUN at the selected physical USB location. #>
    param(
        [Parameter(Mandatory = $true)]
        [ValidateSet('OpenRDX', 'CompatibilityReceiver')]
        [string] $FirmwareKind,
        [Parameter(Mandatory = $true)] [string[]] $RequiredLocationPaths,
        [Parameter(Mandatory = $true)] [int] $TimeoutSeconds
    )

    $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
    do {
        $targets = @(
            Get-RdxTargetCandidates `
                -FirmwareKind $FirmwareKind `
                -RequiredLocationPaths $RequiredLocationPaths)
        if ($targets.Count -eq 1) {
            return $targets[0]
        }
        if ($targets.Count -gt 1) {
            throw "More than one $FirmwareKind RDX disk LUN matched the selected USB location."
        }
        Start-Sleep -Milliseconds 500
    } while ([DateTime]::UtcNow -lt $deadline)
    return $null
}

function Confirm-RdxSelectedTarget {
    <#
    Revalidate the exact disk selected before any state-changing command.

    PhysicalDrive numbers can be reused after device removal, and a cartridge
    can be inserted while release files are being hashed. Require the same disk
    PnP identity, reported serial, firmware family, and physical USB location
    immediately before opening the SPTI path. Re-enumeration checks later in the
    workflow use only the physical location because the programmed firmware is
    expected to change the logical identity.
    #>
    param(
        [Parameter(Mandatory = $true)]
        [ValidateSet('OpenRDX', 'CompatibilityReceiver')]
        [string] $FirmwareKind,
        [Parameter(Mandatory = $true)] [string] $PnpDeviceId,
        [Parameter(Mandatory = $true)] [string] $SerialNumber,
        [Parameter(Mandatory = $true)] [string[]] $RequiredLocationPaths
    )

    $candidates = @(
        Get-RdxTargetCandidates `
            -FirmwareKind $FirmwareKind `
            -SerialNumber $SerialNumber `
            -RequiredLocationPaths $RequiredLocationPaths |
            Where-Object {
                ([string]$_.PNPDeviceID).Equals(
                    $PnpDeviceId, [StringComparison]::OrdinalIgnoreCase)
            }
    )
    if ($candidates.Count -ne 1) {
        throw 'The selected RDX disk identity changed before transfer; no command was sent.'
    }
    return $candidates[0]
}

function Assert-RdxEmptyBay {
    <#
    Refuse firmware transfer while the selected adapter exposes cartridge data.

    Disk size and mounted-volume associations must both be empty. Some OpenRDX
    revisions leave Win32_DiskDrive.MediaLoaded asserted after cartridge
    removal, so an identity-matched Get-Disk "No Media" result may override
    only that stale flag. Every other inconsistent state remains fail-closed.
    #>
    param([Parameter(Mandatory = $true)] $Disk)

    $volumes = @(Get-RdxMountedVolumes -Disk $Disk)
    $storageReportsNoMedia = Test-RdxStorageDiskNoMedia -Disk $Disk
    if (([bool]$Disk.MediaLoaded -and -not $storageReportsNoMedia) -or
        $volumes.Count -ne 0 -or
        [UInt64]$Disk.Size -ne 0) {
        throw 'Firmware update refused: eject and physically remove the cartridge first.'
    }
}

function Test-RdxStorageDiskNoMedia {
    <# Confirm that the matching Windows storage disk explicitly reports no media. #>
    param([Parameter(Mandatory = $true)] $Disk)

    $storageDisk = Get-Disk -Number ([UInt32]$Disk.Index) -ErrorAction SilentlyContinue
    if ($null -eq $storageDisk) {
        return $false
    }

    $expectedSerial = ([string]$Disk.SerialNumber).Trim()
    $storageSerial = ([string]$storageDisk.SerialNumber).Trim()
    $operationalStatus = @(
        $storageDisk.OperationalStatus | ForEach-Object { [string]$_ })
    return ($expectedSerial.Length -ne 0) -and
        $storageSerial.Equals($expectedSerial, [StringComparison]::Ordinal) -and
        ($operationalStatus -contains 'No Media') -and
        ([UInt64]$storageDisk.Size -eq 0)
}

function Get-RdxMountedVolumes {
    <# Return mounted logical disks backed by the selected physical disk. #>
    param([Parameter(Mandatory = $true)] $Disk)

    $partitions = @(Get-CimAssociatedInstance -InputObject $Disk -Association Win32_DiskDriveToDiskPartition)
    $volumes = foreach ($partition in $partitions) {
        Get-CimAssociatedInstance -InputObject $partition -Association Win32_LogicalDiskToPartition
    }
    return @($volumes)
}

function New-WriteBufferCdb {
    <# Build the exact ten-byte WRITE BUFFER CDB emitted by RDX Manager. #>
    param(
        [Parameter(Mandatory = $true)] [byte] $Mode,
        [Parameter(Mandatory = $true)] [int] $Offset,
        [Parameter(Mandatory = $true)] [int] $Length,
        [byte] $BufferId = 0x00
    )

    return [byte[]] @(
        0x3B,
        $Mode,
        $BufferId,
        (($Offset -shr 16) -band 0xFF),
        (($Offset -shr 8) -band 0xFF),
        ($Offset -band 0xFF),
        (($Length -shr 16) -band 0xFF),
        (($Length -shr 8) -band 0xFF),
        ($Length -band 0xFF),
        0x00
    )
}

function Enable-RdxCompatibilityHeaderCheckBypass {
    <#
    Authorize the compatibility receiver and disable its firmware header gate.

    The authorization bytes depend on the selected adapter's reported serial;
    deriving them here prevents a valid request from being replayed to another
    device merely because it exposes the same model and firmware revision.
    #>
    param(
        [Parameter(Mandatory = $true)] [string] $DevicePath,
        [Parameter(Mandatory = $true)] [string] $SerialNumber
    )

    $serial = $SerialNumber.Trim()
    if ($serial -notmatch '^[\x20-\x7E]{10}$') {
        throw 'The selected compatibility receiver serial must contain exactly ten printable ASCII characters.'
    }
    $xorKey = [byte[]] @(0x55, 0xAA, 0x25, 0x46, 0x04, 0xC1, 0x1D, 0xB7)
    $authorization = New-Object byte[] 10
    for ($index = 0; $index -lt 8; $index++) {
        $authorization[$index + 2] =
            [byte](([byte][char]$serial[$index + 2]) -bxor $xorKey[$index])
    }

    $enableCdb = New-WriteBufferCdb -Mode 0x02 -BufferId 0x80 -Offset 0 -Length 10
    [void] [RdxManagerScsiTransport]::Send(
        $DevicePath, $enableCdb, $authorization, 15)
    Write-Host '  mode 02  buffer 80  serial authorization  GOOD'

    $disableCdb = New-WriteBufferCdb -Mode 0x02 -BufferId 0x82 -Offset 0x020FF8 -Length 2
    [void] [RdxManagerScsiTransport]::Send(
        $DevicePath, $disableCdb, [byte[]] @(0x00, 0x00), 15)
    Write-Host '  mode 02  buffer 82  header check bypass  GOOD'
}

$targetKind = if ($RepairCompatibilityReceiver -or $InstallOpenRDXOnCompatibilityReceiver -or
    ($PSCmdlet.ParameterSetName -eq 'Validate' -and
        $ValidateFirmwareKind -eq 'CompatibilityReceiver')) {
    'CompatibilityReceiver'
} else {
    'OpenRDX'
}
$target = Get-RdxTarget `
    -FirmwareKind $targetKind `
    -SerialNumber $TargetSerialNumber
$devicePath = "\\.\PhysicalDrive$($target.Index)"
$mountedVolumes = @(Get-RdxMountedVolumes -Disk $target)
$storageReportsNoMedia = Test-RdxStorageDiskNoMedia -Disk $target
$selectedTargetSerialNumber = ([string]$target.SerialNumber).Trim()
$selectedTargetPnpDeviceId = [string]$target.PNPDeviceID
$selectedLocationPaths = @()
if ($Update -or $InstallOpenRDX -or $InstallOpenRDXOnCompatibilityReceiver -or
    $RepairCompatibilityReceiver) {
    # Capture the physical connector before any command can reset the device.
    # Subsequent identities are accepted only when they appear at this path.
    $selectedLocationPaths = @(
        Get-RdxDiskUsbLocationPaths `
            -DiskPnpDeviceId ([string]$target.PNPDeviceID))
}

Write-Host "Selected target: $devicePath"
Write-Host "  Model:    $($target.Model)"
Write-Host "  Firmware: $($target.FirmwareRevision)"
Write-Host "  PNP ID:   $($target.PNPDeviceID)"
if ($storageReportsNoMedia -and $mountedVolumes.Count -eq 0 -and
    [UInt64]$target.Size -eq 0) {
    Write-Host '  Media:    empty bay; Windows storage stack reports No Media'
} elseif ([bool]$target.MediaLoaded) {
    $volumeSummary = if ($mountedVolumes.Count -ne 0) {
        (($mountedVolumes | ForEach-Object DeviceID) -join ', ')
    } else {
        'no mounted volume'
    }
    Write-Host "  Media:    cartridge reported present; $volumeSummary"
} elseif ($mountedVolumes.Count -ne 0) {
    Write-Host "  Media:    $((($mountedVolumes | ForEach-Object DeviceID) -join ', '))"
} else {
    Write-Host '  Media:    empty bay; no mounted cartridge volume'
}

if ($EjectOnly) {
    # Clear PREVENT MEDIUM REMOVAL before asking the dock to unload/eject.
    [void] [RdxManagerScsiTransport]::Send(
        $devicePath,
        [byte[]] @(0x1E, 0x00, 0x00, 0x00, 0x00, 0x00),
        $null,
        15)
    [void] [RdxManagerScsiTransport]::Send(
        $devicePath,
        [byte[]] @(0x1B, 0x00, 0x00, 0x00, 0x02, 0x00),
        $null,
        30)
    Write-Host 'Cartridge unload/eject command completed with SCSI GOOD.'
    exit 0
}

if (-not $Update -and -not $InstallOpenRDX -and
    -not $InstallOpenRDXOnCompatibilityReceiver -and -not $RepairCompatibilityReceiver) {
    Assert-RdxEmptyBay -Disk $target
    Write-Host 'Validation only; no SCSI command was sent.'
    exit 0
}

Assert-RdxEmptyBay -Disk $target
if (-not (Test-Path -LiteralPath $ImagePath -PathType Leaf)) {
    throw "Firmware image not found: $ImagePath"
}

$image = [System.IO.File]::ReadAllBytes($ImagePath)
$hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $ImagePath).Hash
if ($InstallOpenRDX -or $InstallOpenRDXOnCompatibilityReceiver) {
    if (-not (Test-Path -LiteralPath $ManifestPath -PathType Leaf)) {
        throw "Custom image manifest not found: $ManifestPath"
    }
    $manifest = Get-Content -Raw -LiteralPath $ManifestPath | ConvertFrom-Json
    if ($image.Length -ne $expectedImageLength -or
        $hash -ne ([string]$manifest.container_sha256).ToUpperInvariant() -or
        $manifest.container_length -ne $expectedImageLength -or
        ([string]$manifest.authentication_scheme) -ne 'openrdx-sha256-v1' -or
        -not $manifest.requires_openrdx_receiver -or
        -not $manifest.installation_requires_rom_loader -or
        ([string]$manifest.template_sha256).ToUpperInvariant() -ne $compatibilityImageSha256) {
        throw "Custom container or manifest validation failed (length $($image.Length), SHA-256 $hash)."
    }
    if ($InstallOpenRDXOnCompatibilityReceiver) {
        if (-not $manifest.flashburner_hex -or -not $manifest.flashburner_hex_sha256) {
            throw 'Custom manifest does not identify the continuous FlashBurner HEX artifact.'
        }
        $flashHexName = [string]$manifest.flashburner_hex
        if ([string]::IsNullOrWhiteSpace($flashHexName) -or
            [System.IO.Path]::GetFileName($flashHexName) -ne $flashHexName) {
            throw 'The custom manifest must name its FlashBurner HEX as a file in the same directory.'
        }
        $manifestDirectory = Split-Path -Parent (
            Resolve-Path -LiteralPath $ManifestPath)
        $flashHexPath = Join-Path $manifestDirectory $flashHexName
        if (-not (Test-Path -LiteralPath $flashHexPath -PathType Leaf)) {
            throw "FlashBurner HEX not found: $flashHexPath"
        }
        $flashHexHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $flashHexPath).Hash
        if ($flashHexHash -ne ([string]$manifest.flashburner_hex_sha256).ToUpperInvariant()) {
            throw "FlashBurner HEX digest does not match the custom manifest: $flashHexHash"
        }
        if (-not (Test-Path -LiteralPath $FlashBurnerPath -PathType Leaf)) {
            throw "TI command-line FlashBurner not found: $FlashBurnerPath"
        }
    }
} elseif ($image.Length -ne $expectedImageLength -or $hash -ne $compatibilityImageSha256) {
    throw "Firmware image does not match the required compatibility image (length $($image.Length), SHA-256 $hash)."
}

Write-Host "Validated image: $ImagePath"
Write-Host "  Length:   $($image.Length) bytes"
Write-Host "  SHA-256:  $hash"

# Artifact validation may take long enough for a disk number to be reused or a
# cartridge to be inserted. Bind the SPTI path to the exact selected identity
# again and repeat the empty-bay test immediately before the first mutation.
$target = Confirm-RdxSelectedTarget `
    -FirmwareKind $targetKind `
    -PnpDeviceId $selectedTargetPnpDeviceId `
    -SerialNumber $selectedTargetSerialNumber `
    -RequiredLocationPaths $selectedLocationPaths
$devicePath = "\\.\PhysicalDrive$($target.Index)"
Assert-RdxEmptyBay -Disk $target
if ($InstallOpenRDXOnCompatibilityReceiver) {
    Enable-RdxCompatibilityHeaderCheckBypass `
        -DevicePath $devicePath `
        -SerialNumber $selectedTargetSerialNumber
}
Write-Host 'Beginning Manager-compatible WRITE BUFFER transfer. Do not disconnect power.'

$transferComplete = $false
for ($attempt = 1; $attempt -le 2 -and -not $transferComplete; $attempt++) {
    $offset = 0
    $commandNumber = 0
    $compatibilityInstallationStaged = $false
    if ($attempt -eq 2) {
        Write-Warning 'Restarting the complete firmware transfer once from offset zero.'
        Start-Sleep -Milliseconds 500
    }
    try {
        while ($offset -lt $image.Length) {
            $length = [Math]::Min($chunkLength, $image.Length - $offset)
            $payload = New-Object byte[] $length
            [Array]::Copy($image, $offset, $payload, 0, $length)
            $cdb = New-WriteBufferCdb -Mode 0x04 -Offset $offset -Length $length
            $commandNumber++
            try {
                [void] [RdxManagerScsiTransport]::Send($devicePath, $cdb, $payload, 60)
                Write-Host ("  {0,2}/16  mode 04  offset 0x{1:X6}  length 0x{2:X6}  GOOD" -f $commandNumber, $offset, $length)
            } catch [System.InvalidOperationException] {
                $isFinalChunk = $offset + $length -eq $image.Length
                if (-not $InstallOpenRDXOnCompatibilityReceiver -or -not $isFinalChunk -or
                    -not (Test-RdxSignatureFailure -Message $_.Exception.Message)) {
                    throw
                }
                Write-Host ("  {0,2}/16  mode 04  offset 0x{1:X6}  length 0x{2:X6}  expected 07/74/08 authentication result" -f $commandNumber, $offset, $length)
                $compatibilityInstallationStaged = $true
            }
            $offset += $length
        }
        $transferComplete = $true
    } catch {
        if ($attempt -eq 2) {
            throw
        }
        Write-Warning "Firmware transfer attempt one failed: $($_.Exception.Message)"
    }
}

if ($InstallOpenRDXOnCompatibilityReceiver) {
    if (-not $compatibilityInstallationStaged) {
        throw 'Compatibility installation did not end with the required 07/74/08 authentication result.'
    }
    Write-Host @'
Compatibility installation staging completed safely. The OpenRDX payload is in
SPI flash and the four-byte boot marker remains erased after the expected
authentication result. Disconnect and reconnect USB now, without bridging J7.
The controller should enumerate its ROM loader as VID 0451 / PID 926B.
'@
    $bootloader = Wait-RdxPnpPrefix `
        -Prefix $romBootloaderPnpPrefix `
        -TimeoutSeconds $ReenumerationTimeoutSeconds `
        -RequiredLocationPaths $selectedLocationPaths `
        -RequireSingleGlobalMatch
    if ($null -eq $bootloader) {
        throw @'
The ROM loader did not enumerate after compatibility installation staging. The flash
boot marker is intentionally invalid and the device remains recoverable. Try
one complete USB power cycle; use the validated J7 procedure only if the ROM
loader still does not appear.
'@
    }
    Write-Host "Selected ROM loader: $($bootloader.InstanceId)"
    Write-Host "Validated FlashBurner HEX: $flashHexPath"
    Write-Host "  SHA-256: $flashHexHash"
    Write-Host 'Starting TI FlashBurner. Accept the Windows elevation prompt.'
    $burner = Start-Process -FilePath $FlashBurnerPath `
        -ArgumentList @('/s', '0', '/f', ('"{0}"' -f $flashHexPath)) `
        -Verb RunAs -WindowStyle Hidden -Wait -PassThru
    if ($burner.ExitCode -ne 0) {
        throw "TI FlashBurner failed with exit code $($burner.ExitCode); the ROM loader remains recoverable."
    }
    Write-Host 'TI FlashBurner returned success.'
    Write-Host 'Disconnect and reconnect USB once more, with J7 open, to boot OpenRDX.'
    $openRdxUsbDevice = Wait-RdxPnpPrefix `
        -Prefix $openRdxUsbPnpPrefix `
        -TimeoutSeconds $ReenumerationTimeoutSeconds `
        -RequiredLocationPaths $selectedLocationPaths
    if ($null -eq $openRdxUsbDevice) {
        throw 'OpenRDX did not re-enumerate before the timeout after FlashBurner completed.'
    }
    $openRdxDevice = Wait-RdxTarget `
        -FirmwareKind 'OpenRDX' `
        -RequiredLocationPaths $selectedLocationPaths `
        -TimeoutSeconds $ReenumerationTimeoutSeconds
    if ($null -eq $openRdxDevice) {
        throw 'OpenRDX re-enumerated, but its stable no-media RDX disk LUN did not appear at the selected USB location.'
    }
    Write-Host 'OpenRDX USB update completed without J7:'
    $openRdxDevice |
        Select-Object Index, Model, SerialNumber, FirmwareRevision, PNPDeviceID |
        Format-List
    exit 0
}

$activateCdb = New-WriteBufferCdb -Mode 0x05 -Offset $offset -Length 0
try {
    [void] [RdxManagerScsiTransport]::Send($devicePath, $activateCdb, $null, 60)
    Write-Host ("  17/17  mode 05  offset 0x{0:X6}  length 0x000000  GOOD" -f $offset)
} catch [System.ComponentModel.Win32Exception] {
    # Mode-05 activation resets and disconnects the adapter. If Windows loses
    # the handle during that reset, verify disappearance/re-enumeration below.
    Write-Warning "Activation disconnected before IOCTL completion: $($_.Exception.Message)"
}

Write-Host 'Transfer complete; waiting for the adapter to re-enumerate.'
$null = Start-Sleep -Seconds 1
if ($InstallOpenRDX) {
    $usbDevice = Wait-RdxPnpPrefix `
        -Prefix $openRdxUsbPnpPrefix `
        -TimeoutSeconds 30 `
        -RequiredLocationPaths $selectedLocationPaths
    if ($null -eq $usbDevice) {
        throw 'The custom transfer finished, but the USB adapter did not re-enumerate within 30 seconds.'
    }
    $openRdxDevice = Wait-RdxTarget `
        -FirmwareKind 'OpenRDX' `
        -RequiredLocationPaths $selectedLocationPaths `
        -TimeoutSeconds 30
    if ($null -eq $openRdxDevice) {
        throw 'The USB adapter re-enumerated, but the required stable no-media RDX disk LUN did not.'
    }
    Write-Host 'Custom firmware adapter and no-media RDX disk LUN re-enumerated:'
    $openRdxDevice | Select-Object Index, Model, SerialNumber, FirmwareRevision, PNPDeviceID | Format-List
} else {
    $compatibilityDevice = Wait-RdxTarget `
        -FirmwareKind 'CompatibilityReceiver' `
        -RequiredLocationPaths $selectedLocationPaths `
        -TimeoutSeconds 30
    if ($null -eq $compatibilityDevice) {
        throw 'The transfer finished, but the selected compatibility receiver did not re-enumerate within 30 seconds.'
    }
    Write-Host 'Compatibility receiver re-enumerated:'
    $compatibilityDevice | Select-Object Index, Model, SerialNumber, FirmwareRevision, PNPDeviceID | Format-List
}
