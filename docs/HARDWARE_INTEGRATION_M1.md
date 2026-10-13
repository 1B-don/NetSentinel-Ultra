# Hardware Integration Milestone 1

Status: software interfaces and diagnostics prepared; experimental FAP not deployed. Physical NRF24, CC1101 and Wi-Fi observation remain disabled.

## Evidence review

| Item | Evidence | What it establishes | What it does not establish |
|---|---|---|---|
| Ultra V3 modules and selector | [Sacred Labs Ultra V3 guide](https://sacredlabstech.com/boards/ultra-v3/) | 18-pin Flipper connection, ESP32-WROOM-32UE, NRF24, CC1101, microSD; selector chooses NRF24 or CC1101 while ESP32 stays powered | No SPI MOSI/MISO/SCK, NRF CE/CSN/IRQ, CC CS/GDO, Flipper pin numbers, level-shifting or selector netlist |
| Generic Flipper NRF wiring | [vad7/nrf24scan](https://github.com/vad7/nrf24scan) and other independent scanner documentation | Generic standalone modules commonly use Flipper SPI and separate CE/CSN signals | These are not Ultra V3 schematics and conflict with other multi-radio board conventions; no Ultra V3 values are inferred |
| CC1101 pins and SPI | [TI CC1101 data sheet](https://www.ti.com/lit/ds/symlink/cc1101.pdf) | Chip-level pin and state semantics | No Ultra V3 routing, external-device provider or safe board initialization sequence |
| Owner-confirmed ESP UART | Prior physical information-only test and working Momentum Marauder app | USART 13/14, 115200 8N1, successful `info` query | Does not validate Wi-Fi observation startup, RF silence or ESP-side SD capture settings |

No authoritative public Ultra V3 signal table or schematic was found in the reviewed manufacturer material or targeted independent-driver searches. Generic Flipper GPIO pin tables and photographs are intentionally not used as routing proof.

## SDK API 87.1 / target 7 CC1101 inventory

The permitted SDK-only headers and bundled enabled-export table were inspected. `subghz_devices_init`, `subghz_devices_deinit`, `subghz_devices_get_by_name`, `subghz_devices_begin`, `subghz_devices_end`, `subghz_devices_is_connect`, `subghz_devices_load_preset`, `subghz_devices_is_frequency_valid`, `subghz_devices_set_frequency`, `subghz_devices_set_rx`, `subghz_devices_get_rssi`, `subghz_devices_idle`, and `subghz_devices_sleep` are declared and listed as enabled callable firmware exports (`Function,+`). The generic registry also exports packet/RX methods and TX methods; NetSentinel must not import or call the TX methods.

`subghz_device_cc1101_ext_ep` is declared by the external-device interconnect header but is listed `Function,-` in `api_symbols.csv`; it is not a callable application import. The permitted SDK package contains that header but no CC1101 external plugin implementation. The generic registry API therefore provides a candidate interface, not proof that the `cc1101_ext` provider is installed, selected, external-only, or safe to initialize. Several registry calls return `void`; declaration/export presence does not promise bounded waits, recoverable failure, receive-only preset behavior, or ownership. No SPI or Sub-GHz symbols are imported by the current FAP.

## Adapter contract and gates

- NRF24 core code implements bounded dwell/session sampling using primary RX configuration, disabled auto-acknowledgment/retries, register readback, and no payload/transmit API. It does not bind physical SPI, CSN, CE or IRQ. Each board transfer must be finite and serialized; acquisition failure must unwind; cancellation closes with CE low and the chip powered down; release restores the board's prior peripheral/pin ownership. The callback ABI has no deadline argument, so only the board implementation can enforce the transfer timeout and return `NS_TIMEOUT`; the caller cannot preempt a blocked callback.
- CC1101 core code accepts RSSI observations only after an external adapter reports successful acquisition, identity/diagnostics and settled RX. The adapter must be external-only, never fall back to the internal radio, avoid reset/TX operations, bound every wait, and idle/deassert CS/release even after errors. SDK registry calls without deadlines do not satisfy that contract on their own.
- Host board-port compilation now requires the explicit `NS_TEST_BOARD_PORT` marker. A non-test build must declare the individual verified route macro before a radio can be advertised; CC additionally requires `NS_VERIFIED_MOMENTUM_CC1101_PROVIDER`. At startup the Flipper boundary rejects an adapter that advertises a radio without its corresponding evidence gate. Root and generated FAP manifests do not enable the board-port gate. The host mock board test exercises driver/resource contracts only; it is not hardware evidence.
- Runtime descriptors separately report interface availability, routing verification, software receiver support, simulation availability, physical enablement and blockers. Simulated receiver cores never make a physical adapter appear ready.
- Wi-Fi observation remains blocked. Marauder v1.11.0's reviewed `StartScan` path initializes AP mode before beacon reception and opens PCAP handling; `SavePCAP` defaults on and stop teardown is conditional. A verified passive-only start/stop path and a confirmed policy preventing peer-side raw capture are required before enablement.

## Readiness matrix and smallest next verification

| Radio/capability | Software / simulation | Physical status | Smallest next evidence or test |
|---|---|---|---|
| ESP32 info | UART info-only transport and sanitized parser supported; previous artifact completed an owner-authorized query | Previously physically demonstrated; this milestone preserves it. No Wi-Fi observation enabled | No new test needed for info. Wi-Fi needs a reviewed installed-firmware passive-only lifecycle and peer-side SD capture policy before any observation test |
| BLE | Explicitly unavailable; synthetic UI/parser data only | Disabled; upstream path reviewed as active discovery | No test until an independently verified passive receive-only firmware interface exists |
| NRF24 | Portable RX sampler and deterministic SPI mocks work | Unverified route; physical adapter disabled | Obtain revision-specific manufacturer netlist for SPI/CE/CSN/IRQ/selector/voltage. Then, only after owner authorization and manual selector selection, perform a bounded read-only identity/RPD test while asserting no TX commands and cleanup |
| CC1101 | RSSI/session core and deterministic external-device mocks work | Route and external provider implementation unverified; physical adapter disabled | Obtain the same revision-specific netlist for SPI/CS/GDO/selector/voltage plus permitted provider identity/source. Then, after owner authorization and manual CC selector selection, perform a bounded external-only status/RX-state/RSSI test with teardown; no signal source is required to verify idle RSSI, but RF reception still needs a controlled legal source |

Do not move the selector during a running session. No selector, power, expansion or firmware settings are changed by NetSentinel. The next smallest project input is the manufacturer's Ultra V3 schematic/net table for this PCB revision and, for CC1101, the permitted external provider implementation/identity.
