<!-- SPDX-FileCopyrightText: 2026 André Fiedler -->
<!-- SPDX-License-Identifier: AGPL-3.0-or-later -->

# Eject mechanism reference audit

On 2026-09-05, the first foreground/ADC timing correction was installed on the
test receiver. Windows Explorer eject still caused a continuous motor run of
roughly 31 seconds, and the mechanism prevented reinsertion afterward. The
timing correction alone therefore did not resolve the physical failure.

A read-only E4/E5/E7 probe of the empty receiver subsequently reported firmware
1.06. Six 64-byte flash samples, including the vector table and mechanism/main
code regions, matched the previously flashed timing-fix image. This was sampled
identity evidence, not a complete-image hash check. The 256-byte manufacturing
record at `0x3E000` contained `FF` throughout and failed its checksum, confirming
the effective fallback profile `0x38`. The probe did not alter that record.

## Output behavior

The mechanism controller uses these hardware profile mappings:

| Control | OpenRDX behavior |
| --- | --- |
| Powered travel | GPIO3 low during powered states; high when stopped. The pin name does not imply an electrical direction. |
| Profile-dependent GPIO0 | Logical output 11 uses active-high GPIO0 on non-38h profiles. Profile 38h assigns the pin to output 14. Identity must be loaded before setting direction so the correct level can be preloaded. |
| Return travel | Profiles 35h/37h assert GPIO0 before PWM1 and clear it during cleanup. Profile 36h has no assigned output-12 drive phase. |
| Profile 38h startup | GPIO0 starts high and stays high through connected USB operation and mechanism travel. Motor cleanup cannot lower it. |

Profile-38h idle deassertion would require coordination of startup timing, ADC,
cartridge standby, USB reconnect settling, and cartridge monitoring. OpenRDX
currently holds the connected-operation level and does not implement that
separate low-power policy.

## Foreground servicing

SATA receiver-error interrupts previously could run synchronous engine-stop
and link-reset waits while a powered mechanism phase needed foreground GPIO2
polling. The interrupt now uses the existing deferred media-discovery path
during active mechanism states, clears the receiver-error count, and retains
required terminal callbacks. The wait-for-I/O stage still uses normal recovery.
Mechanism ownership is volatile and published before applying motor power.

The first correction remains: skip CPU sleep during eject, bound an entire live
MCP3008 sample to one millisecond, avoid ADC work when digital media presence
already answers the predicate, seed GPIO2 history before applying power, and
handle an expired initial travel window before optional ADC/grace work.

## Validation boundary

Source regression tests cover the output levels, profile mapping, preload and
cleanup order, interrupt deferral, callback/discovery barriers, endpoint history,
and deadline priority. The build uses the required Windows TI ARM CGT 5.2.5.
These checks cannot establish motor timing or reinsertion on the receiver.
The GPIO mismatches and blocking interrupt path are verified defects, but the
exact cause of the reported 31-second physical run is not yet proven. A complete
Windows Explorer eject followed by reinsertion remains the acceptance check.

The corrected package passed all 215 Python tests and the Windows TI ARM CGT
5.2.5 build. It was installed through the validated in-place route on 2026-09-05
at 20:52 CEST, with container SHA-256
`1297b787a1eec95537aecd934e24b7e14b128371c785eb997f31c5cf64d28c4f`.
The receiver disappeared and returned at the same physical USB location.
Six subsequent flash samples matched this corrected image, the empty-bay
check passed, and the manufacturing-record bytes were unchanged. This confirms
installation; it does not yet establish physical eject/reinsertion success.

## Follow-up: long drive and insertion during recovery

The next test still produced about 30 seconds of eject motion from both Windows
Explorer and the physical button, followed by a pause and return motion. The
first reinsertion triggered another short eject; the second stayed inserted.
The controller's recovery-settle state 7 continues into state 1 without a new
host/button request, so return motion alone is not evidence of a finished cycle.

The PWM driver now disables the channel while loading PER/PH1D and issues START
last, following the retained TI `pwm_init` source. Stop disables the channel
before changing its duty register. Initialization also requires a
START after its timing writes. This removes an uninitialized/stale first
period without asserting that it explains the full observed delay. Existing
period and duty arithmetic remain unchanged.

A debounced insertion during an active cycle previously committed the new
cartridge state while skipping its GPIO11 assertion; terminal cleanup could
then hide that cartridge without another input transition to restore it.
Insertion publication is now deferred until the mechanism coordinator is idle.
Raw-input tracking and physical removal handling continue during travel.

### Read-only mechanism snapshot

OpenRDX adds a six-byte vendor CDB `E8 00 00 00 00 80` on BOT. It returns exactly
128 bytes from RAM and live register reads, with no flash, ATA, or motor operation.
Nonzero reserved bytes and other requested lengths are rejected. Numeric
multi-byte fields are little-endian. The command is available with an empty bay.

| Offset | Field |
| --- | --- |
| 0 | Four-byte `RDXM` signature |
| 4, 5, 6, 7 | Schema version, current phase, retry count, saved event count |
| 8 | 16-bit effective hardware profile |
| 10, 11 | GPIO input/output low bytes |
| 12, 16 | 32-bit RTI microsecond/millisecond readings |
| 20, 24, 28, 32 | Deadline, last service, maximum service gap, phase-entry time in milliseconds |
| 36, 40, 44, 48 | Software start, boot-homing, success, and failure counters |
| 52, 56, 60 | PWM1 CFG, PER, and PH1D register values |
| 64–127 | Up to eight committed phase events, oldest first; each is a 32-bit millisecond timestamp followed by phase, GPIO input, GPIO output, and retry bytes |

Comparing the two RTI readings with host timestamps distinguishes timer scale
from a missed foreground deadline. The phase events and start/completion counts
distinguish recovery continuation from an additional eject request. Snapshots
do not themselves establish actual physical output levels at the motor.

The trace-enabled package passed all 224 Python tests and the TI ARM CGT 5.2.5
build, then installed successfully on 2026-09-05 at 21:16 CEST. Its container
SHA-256 is
`9a7c273f2f59964519b95b3e91738813ff5718031996486988a36500ca202279`.
The same USB target returned and the live E8 snapshot responded with schema 1.
Initial empty-bay samples showed phase 0, PWM1 disabled, GPIO3 high, profile-38h
GPIO0 high, and matching microsecond/millisecond timebases. Physical-cycle
capture is the next validation step.

## Captured foreground stall

The live trace captured one accepted software/coordinator start and no boot
homing or extra request during the reported repeat-eject cycle. The first drive
began at 75095 ms and did not enter its stop/pause state until 82131 ms. The
maximum foreground-service gap was exactly 7000 ms, and a concurrent diagnostic
request took 7411 ms. FRC0 and FRC1 continued advancing at the expected ratio.
The later route was state 6 at 84131 ms, state 7 at 84686 ms, state 1 again at
86131 ms, successful settle at 87002 ms, and idle at 87502 ms.

The caching/all-pages MODE SENSE handler unconditionally called
`ahci_identify_device`, whose command-completion timeout is 7000 ms. MODE SENSE
6/10 deliberately bypass the generic no-media gate to keep management queries
available. That combination issued IDENTIFY to a withdrawing cartridge in USB
interrupt context, delaying the foreground endpoint/deadline checks. It also
explains why the AHCI discovery and receiver-error guards were insufficient.
The trace proves one seven-second stall; repeated queries are a possible
explanation for longer user-observed runs, not a measured claim from this capture.

The refresh is now permitted only for a ready, logically loaded medium with no
pending or active eject. Other requests retain the existing mode-page response
format using cached information. The eject-reservation predicate now includes
the queued request, so disk refresh is excluded before foreground preparation
begins. The phase sequence itself is unchanged.

This MODE SENSE correction passed all 229 Python tests and the TI ARM CGT 5.2.5
build. The package installed successfully at 21:25 CEST on 2026-09-05 with
container SHA-256
`fb38051929ea2c2c075e05118da382231188ac403c72e5e77afdde6809e50fd6`.
Six flash samples matched that package after the same receiver returned at its
previous USB location. A fresh live trace was started for the acceptance test.

The user then confirmed normal eject and mechanical reinsertion. The final
diagnostic session captured 465 idle snapshots before a native SPTI request
failed, so it does not supply a measured duration for that successful cycle.

## Follow-up: inserted-media access and fan demand

After the mechanical acceptance test, the user reported that the fan ran
continuously with a cartridge inserted and stopped on eject. Explorer also
failed to open F: with "A device which does not exist was specified." Windows
reported the receiver as present but its disk as No Media. A System disk event
153 at 21:28:38 CEST recorded a retried read at LBA zero, before the diagnostic
session ended. The last E8 request failed in approximately 15 ms after a roughly
48.5-second delay in Windows device enumeration; this was not an E8 command
waiting through its own 45-second timeout.

The fan policy explicitly selected 50 percent duty for cool, ready media and
delayed the first temperature sample for five minutes. Threshold-only cooling
is an intentional policy change requested by the user: start at 45°C, stop at
40°C, and preserve demand within the intervening band. Unknown temperature
cannot create a new cooling request; a failed sample preserves active cooling.

The reinsertion audit also found two lifecycle gaps. An empty-bay discovery can
consume its pending request while the SATA PHY still reports ready; a later
debounced GPIO5 insertion did not schedule discovery and relied on a new SATA
connect-change interrupt. Separately, BOT registered its idle/reset function
as the SATA initialization callback. Completing foreground discovery could
therefore clear an active USB data/status exchange or arm a duplicate CBW.
The follow-up changes explicitly queue deferred discovery for an unready
insertion and separate removable-LUN command reception from SATA completion.
These are source-established defects; their role in the observed access
failure and the combined correction still require a device test.

The user also reported missing RDX Manager identity information, with F: shown
as Unknown / Error / No Media. A subsequent bounded TEST UNIT READY probe failed
at the Windows transport layer with error 1167 (device not connected), without
receiving SCSI status or sense. The current session therefore cannot establish
the cartridge's readiness through SCSI or read the Manager identity.

The combined follow-up passed all 240 Python tests and the Windows TI ARM CGT
5.2.5 build. Its prepared container SHA-256 is
`978697e2f5ac180dea0e7597f209d40172dfa3213b2e38429bd59a97e0dc212f`.
After the user removed the cartridge and reconnected USB, installation completed
at 21:44 CEST on 2026-09-05. Six flash samples matched the new package. Bounded
post-flash queries returned TANDBERG RDX identity, successful REQUEST SENSE and
E8 responses, and the expected NOT READY / MEDIUM NOT PRESENT sense for TEST
UNIT READY and both READ CAPACITY forms with the bay empty. The mechanism was
idle with PWM1 disabled. The next device checks must cover Explorer access and
Manager identity after insertion, fan demand, and another complete
eject/reinsertion cycle.

The user subsequently answered "looks good now" to the requested check of
Explorer access, RDX Manager information, cool-drive fan behavior, and a further
eject/reinsertion cycle. An independent Windows observation showed the same
receiver's 320 GB medium online (`IsOffline=false`, reported capacity
320072933376 bytes). This establishes the reported acceptance check on this
receiver and medium. Actual hot/cool threshold transitions, repeated cold
starts, and broader media regression remain unmeasured.
