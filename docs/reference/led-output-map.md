# RDX LED Output Mapping

This is an electrical and firmware reference for maintainers. Owners looking
for normal media and eject behavior should use
[Using OpenRDX](../getting-started/using-openrdx.md); the product README does not
duplicate pin, polarity, or timing details.

## Outcome

OpenRDX uses two bicolor LED controllers and all four color channels. The
firmware registers logical outputs `(6, 7)` as the
cartridge/activity controller and `(8, 9)` as the dock/eject-button controller.
Its built-in diagnostic command puts both controllers into alternating-color
test mode.

The controller state machine stores a generic first/second-output selector;
the two controllers do not share the same physical color order. The cartridge
pair is first-green/second-amber, while board wiring establishes
the dock pair as first-amber/second-green. The logical-output table and setter
define the exact selector, register, and active-low polarity below.

Hardware behavior fixes the otherwise-generic pair identity:

- With no cartridge, the dock/eject-button LED is steady amber and the
  cartridge LED is off.
- Insertion makes the cartridge LED steady green and then blink green while
  the dock/eject-button LED remains steady amber.
- An accepted eject-button press blinks the dock/eject-button LED amber.
- The diagnostic command places both controllers in the same alternating
  off/second/off/first mode without taking temporary ownership of their pins.

## Logical-pin table

Each logical entry binds one physical selector and an active-low output. The
controller keeps colors out of its transition logic by referring only to each
pair's first and second members.

| Logical index | Physical selector | Config word | LED role |
| ---: | ---: | ---: | --- |
| 6 | 7 | `0x00000100` | Cartridge green |
| 7 | 6 | `0x00000100` | Cartridge amber |
| 8 | 8 | `0x00000100` | Dock amber (first output) |
| 9 | 9 | `0x00000100` | Dock green (second output) |

For polarity byte zero, `spi_set_logical_output()` writes the physical bit low
when the logical state is asserted and high when it is deasserted. All four
bindings are therefore active-low; the safe all-off register value has every
one of these four bits set.

## DOCK_GREEN

- Register: `0xFFF7E548`
- Selector: `SCI GPIO9 via logical output 9`
- Polarity: `active-low`

## DOCK_AMBER

- Register: `0xFFF7E548`
- Selector: `SCI GPIO8 via logical output 8`
- Polarity: `active-low`

## CARTRIDGE_GREEN

- Register: `0xFFF7BC3C`
- Selector: `GPIO7 via logical output 6`
- Polarity: `active-low`

## CARTRIDGE_AMBER

- Register: `0xFFF7BC3C`
- Selector: `GPIO6 via logical output 7`
- Polarity: `active-low`

## Normal firmware implementation

The production port permanently claims all four active-low pins, preloads the
off level before changing their direction/function, and models two generic
controller states rather than writing colors directly. One shared timer
advances both controllers. It reloads for 100 ms only while the dock controller
is in mode two; otherwise both controllers advance every 500 ms. A dock
mode-two confirmation therefore advances the cartridge controller every
100 ms until the dock returns to mode zero.

Normal stable and transient behavior follows these state transitions:

- the no-media baseline is dock first/amber steady and cartridge off;
- insertion enables cartridge first/green steady without changing the dock;
- storage activity starts the six-half-phase normal-selector blink on the
  cartridge controller;
- an accepted eject starts the same six-half-phase blink on the dock, so its
  first/amber channel blinks;
- the forced-second operation starts eight half-phases on the controller's
  second channel; on the dock that channel is green;
- mechanism terminal case four clears its validation selector and chooses
  dock first/amber, while terminal case eight sets the selector and chooses
  dock second/green;
- diagnostic mode 1 preserves the controller fields and cycles
  off, second, off, first at the ordinary 500-ms cadence.

## Hidden no-cartridge button display

Entry requires one uninterrupted five-second press while no cartridge is
present; release before the timer expires cancels. Once entered,
the dock LED repeats a second-output off/on/off preamble followed by one to
seven first-output pulses for callback-table selections zero through six.
The service immediately following the final pulse renders the next
second-output-off preamble because the cycle state advances with the final
count. There is no duplicate 500-ms first-output-off frame between cycles.

A short press/release opens a one-second second-click window. Expiry advances
the selection with `0..6` wraparound. A second press inside the window confirms
the current entry. Confirmation begins at the next ordinary controller
service, toggles the currently rendered channel for twenty 100-ms half-phases,
returns mode 2 to mode 0 on the following 100-ms service, and dispatches the
callback on the next 500-ms controller service. A five-second hold cancels a
selection press; every stable press refreshes the independent 60-second menu
timeout.

Selection zero persists operation mode one, selection one persists mode two,
entries two through five perform no operation, and entry six resets the system.
Mode selection schedules the separate reconnect state machine; it does not
disconnect USB in the callback. With no media, the next reconnect service
detaches, waits 1000 ms, reconnects, and arms a separate 2000-ms transfer-settle
timer. Mode two also clears the scoped SCSI PREVENT interlock.

[Back to the firmware behavior reference](firmware-behavior.md) ·
[Documentation index](../README.md)
