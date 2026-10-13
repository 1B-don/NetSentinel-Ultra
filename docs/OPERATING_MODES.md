# Operating modes and laboratory simulation

This milestone adds four session-local modes selected from **Operating modes**:

| Mode | Behavior | Hardware access |
|---|---|---|
| Monitor | Existing dashboard and currently supported information/receive-gated screens | Existing capability gates only; no new radio is enabled |
| Lab | Local scenario workflow for repeatable controlled software exercises | None from the laboratory simulator |
| Simulation | Synthetic event generation, local replay, and an offline channel-overlap model | None |
| Hardware Diagnostics | Existing status and storage checks | Read-only state already collected by supported paths; no probing/init |

The selected mode and simulator dataset live only in RAM. Every launch starts in
Monitor. Neither a stored preference nor menu navigation exposes a transmit
adapter. The current Flipper build still has the ESP32 `info` transaction only;
Wi-Fi observation, BLE, NRF24 and CC1101 physical monitoring remain disabled.

## Synthetic event model

`src/core/simulation.c` produces at most 64 abstract records in batches of 16.
Each frame contains a generated sequence, abstract protocol class, neutral
laboratory-bin number, bounded pseudorandom test payload, fingerprint, and
CRC16. Its `NSIM` magic/version and private layout are not valid 802.11, BLE,
NRF24, or Sub-GHz frame encodings. It contains no MAC address, SSID, key
material, RF modulation parameters, or captured data. A strict validator
checks bounds and CRC before replay. The code has no platform, UART, SPI, radio,
filesystem, or persistence dependency. Local replay advances one in-memory
frame per app tick and Back cancels it. Clearing the screen dataset clears RAM
only.

The channel-overlap model accepts 14 synthetic 0–100 occupancy inputs and
reports a deterministic illustrative quality/interference score. The UI's
example vector is fixed simulation data. A high-overlap label is emitted at the
documented model threshold; this is classification of synthetic input only.
These values are not observations or predictions of the physical environment,
and must not be interpreted as measured RF interference. Intentional
interference/jamming is not implemented.

## Physical feature gates

Sacred Labs' Ultra V3 guide confirms the 18-pin form factor, ESP32-WROOM-32UE,
NRF24 and CC1101 components, the external radio selector, and antenna bands. It
does not publish a board-revision-specific electrical net table for SPI, NRF CE/
CSN, CC CS/GDO, signal directions, level shifting, or Flipper pin routing. The
image and generic module pinouts are not enough evidence to enable those
interfaces. See [dependency review](DEPENDENCY_REVIEW.md).

Marauder's public documentation describes both receive/capture and transmit
capabilities, and its settings documentation lists `SavePCAP` as enabled by
default. Because an observation command may initialize AP mode or persist raw
captures on the ESP SD card, the installed v1.11.0 scan path remains blocked.
Only the already verified `info` exchange is enabled. BLE active discovery is
excluded. No real packet construction, over-air replay, transmission, injection,
deauthentication, or jamming path exists in the application.

Public references reviewed for this milestone:

* [Sacred Labs Ultra V3 Board Guide](https://sacredlabstech.com/boards/ultra-v3/) — component inventory and selector, but no SPI signal map.
* [Marauder FAQ](https://github.com/justcallmekoko/ESP32Marauder/wiki/faq) — project supports frame capture and transmission; PCAP storage depends on board/SD configuration.
* [Marauder Settings](https://github.com/justcallmekoko/ESP32Marauder/wiki/marauder-settings) — current public wiki lists `SavePCAP` default true. This is corroborative risk evidence; installed v1.11.0 behavior is not inferred from a newer mutable wiki page.
* [Marauder Packet Count](https://github.com/justcallmekoko/ESP32Marauder/wiki/packet-count) — describes a receive-frame counter, but requires selected AP/station context and does not establish safe startup/stop behavior for the installed binary.
* [Flipper external CC1101 implementation reference](https://github.com/flipperdevices/flipperzero-firmware/blob/1.4.3/applications/drivers/subghz/cc1101_ext/cc1101_ext.c) — generic external-driver reference only; it does not establish Ultra V3 routing or Momentum runtime behavior.

The permitted Momentum SDK-only headers declare Sub-GHz device registry operations
such as lookup, begin/end, set-RX, RSSI, idle and sleep, and its API table exports
those registry functions. The packaged lab-sim FAP imports none of them. The
SDK has only the CC1101 external interconnect declaration; the external device
entry-point symbol is not an enabled export, and the archive has no provider
implementation to inspect for acquisition bounds, RX transitions, calibration
or cleanup. Thus declarations alone are not enough to enable the external
radio. The Nordic nRF24 receive-only core remains testable in isolation, but
CE/CSN/SPI board routes are still unverified. For receive-capable nRF24 hardware,
automatic ACK must remain off because acknowledgements themselves transmit.

## Validation scope

Host tests verify deterministic generation, input/output bounds, replay,
cancellation, tick wrap, model range validation, navigation, volatile mode
reset, and that the simulated paths call no peripheral/storage callbacks. SDK
build/import checks validate the packaged app against Momentum API 87.1 / target
7. The new `netsentinel_lab_sim.fap` is an undeployed development artifact
(49,004 bytes, SHA-256
`6be70f1db0176538e60486fc48789712bdcee14152f358cd9090a65581cb96fc`); its
successful package build does not validate a live radio feature or authorize
device installation.
