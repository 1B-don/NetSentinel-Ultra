# Hardware Integration Milestone 1

Status: software interfaces and diagnostics prepared; experimental FAP not deployed. Physical NRF24, CC1101 and Wi-Fi observation remain disabled.

## Evidence review

| Item | Evidence | What it establishes | What it does not establish |
|---|---|---|---|
| Ultra V3 modules and selector | [Sacred Labs Ultra V3 guide](https://sacredlabstech.com/boards/ultra-v3/) | 18-pin Flipper connection, ESP32-WROOM-32UE, NRF24, CC1101, microSD; selector chooses NRF24 or CC1101 while ESP32 stays powered | No SPI MOSI/MISO/SCK, NRF CE/CSN/IRQ, CC CS/GDO, Flipper pin numbers, level-shifting or selector netlist |
| Generic Flipper NRF wiring | [mothball187/flipperzero-nrf24](https://github.com/mothball187/flipperzero-nrf24) and [vad7/nrf24scan](https://github.com/vad7/nrf24scan) | Generic standalone module wiring and one reference app's SPI/CE selection | These are not Ultra V3 schematics; no Ultra V3 values are inferred |
| CC1101 pins and SPI | [TI CC1101 data sheet](https://www.ti.com/lit/ds/symlink/cc1101.pdf) | Chip-level pin and state semantics | No Ultra V3 routing, external-device provider or safe board initialization sequence |
| Owner-reported Sub-GHz operation | Owner reports selector=CC1101, Momentum auto-selected External, Read RAW entered RX UI at 433.92 MHz and showed low RSSI fluctuations; no transmit/replay | Existing app UI/RSSI behavior under that configuration | Does not prove decoded packets, calibrated RSSI, direct SPI traffic or NetSentinel adapter behavior |
| Owner-confirmed ESP UART | Prior physical information-only test and working Momentum Marauder app | USART 13/14, 115200 8N1, successful `info` query | Does not validate Wi-Fi observation startup, RF silence or ESP-side SD capture settings |

No authoritative public Ultra V3 signal table or schematic was found in the reviewed manufacturer material or targeted independent-driver searches. Generic Flipper GPIO pin tables and photographs are intentionally not used as routing proof. A focused review of the public Sniffer source is recorded in [NRF24 hardware readiness](NRF24_HARDWARE.md).

## SDK API 87.1 / target 7 CC1101 inventory

The permitted SDK-only headers and bundled enabled-export table were inspected. `subghz_devices_init`, `subghz_devices_deinit`, `subghz_devices_get_by_name`, `subghz_devices_begin`, `subghz_devices_end`, `subghz_devices_is_connect`, `subghz_devices_load_preset`, `subghz_devices_is_frequency_valid`, `subghz_devices_set_frequency`, `subghz_devices_set_rx`, `subghz_devices_get_rssi`, `subghz_devices_idle`, and `subghz_devices_sleep` are declared and listed as enabled callable firmware exports (`Function,+`). The generic registry also exports packet/RX methods and TX methods; NetSentinel must not import or call the TX methods.

`SUBGHZ_DEVICE_CC1101_EXT_NAME` is declared as `cc1101_ext`; `subghz_device_cc1101_ext_ep` is also declared by the external-device interconnect header but is listed `Function,-` in `api_symbols.csv`, so it is not a callable application import. Enabled registry exports include generic lookup, begin/end, connected-state, set-RX, RSSI, frequency and idle/sleep operations. The permitted SDK package contains no external provider implementation. Therefore the SDK has a generic device-provider contract but does not prove the `cc1101_ext` provider is exposed to FAPs, selected, external-only, bounded, or safe to initialize. Several calls return `void`, no relevant calls accept deadlines, and no errors can be reported by set-RX/idle. No SPI or Sub-GHz symbols are imported by the current FAP.

## Adapter contract and gates

- NRF24 core code implements bounded dwell/session sampling using primary RX configuration, disabled auto-acknowledgment/retries, register readback, and no payload/transmit API. It does not bind physical SPI, CSN, CE or IRQ. The callback now receives a 20 ms SPI transfer timeout; acquisition must be nonblocking or finite and cleanup on any timeout must release CE and ownership. The permitted SDK acquire API is blocking and has no timeout, so it does not currently meet the contract by itself. See [NRF24 hardware readiness](NRF24_HARDWARE.md).
- CC1101 core code accepts RSSI observations only after an external adapter reports successful acquisition, identity/diagnostics and settled RX. Candidate callbacks now carry 100 ms probe, 500 ms tune and 200 ms total diagnostic and 20 ms idle/RSSI budgets; mock tests require failures to release ownership. These are adapter contracts, not guarantees from the SDK. The provider must be external-only, avoid reset/TX operations, honor bounded waits, and idle/deassert CS/release after errors. Generic SDK calls lack the deadlines/error returns needed to prove this, so the physical gate stays off.
- Host board-port compilation now requires the explicit `NS_TEST_BOARD_PORT` marker. A non-test build must declare the individual verified route macro before a radio can be advertised; CC additionally requires `NS_VERIFIED_MOMENTUM_CC1101_PROVIDER`. At startup the Flipper boundary rejects an adapter that advertises a radio without its corresponding evidence gate. Root and generated FAP manifests do not enable the board-port gate. The host mock board test exercises driver/resource contracts only; it is not hardware evidence.
- Runtime descriptors separately report interface availability, routing verification, software receiver support, simulation availability, physical enablement and blockers. Simulated receiver cores never make a physical adapter appear ready.
- Wi-Fi observation remains blocked. Marauder v1.11.0's reviewed `StartScan` path initializes AP mode before beacon reception and opens PCAP handling; `SavePCAP` defaults on and stop teardown is conditional. A verified passive-only start/stop path and a confirmed policy preventing peer-side raw capture are required before enablement.

## Readiness matrix and smallest next verification

| Radio/capability | Software / simulation | Physical status | Smallest next evidence or test |
|---|---|---|---|
| ESP32 info | UART info-only transport and sanitized parser supported; previous artifact completed an owner-authorized query | Previously physically demonstrated; this milestone preserves it. No Wi-Fi observation enabled | No new test needed for info. Wi-Fi needs a reviewed installed-firmware passive-only lifecycle and peer-side SD capture policy before any observation test |
| BLE | Explicitly unavailable; synthetic UI/parser data only | Disabled; upstream path reviewed as active discovery | No test until an independently verified passive receive-only firmware interface exists |
| NRF24 | Portable RX sampler and deterministic SPI mocks work | Unverified route; physical adapter disabled | Obtain revision-specific manufacturer netlist for SPI/CE/CSN/IRQ/selector/voltage. Then, only after owner authorization and manual selector selection, perform a bounded read-only identity/RPD test while asserting no TX commands and cleanup |
| CC1101 | RSSI/session core and deterministic external-device mocks work; callbacks have explicit operation budgets | Owner reports Momentum Read RAW receive UI at 433.92 MHz and fluctuating RSSI with selector CC1101/External; decoded RF and NetSentinel driver remain unverified | Obtain the revision-specific CS/GDO/SPI/selector/voltage netlist and an authorized external provider with bounded, cancellable lifecycle. Then, only after separate approval, perform a NetSentinel external-only ID/RX/RSSI/teardown test; decoding still needs a known controlled signal |

Do not move the selector during a running session. No selector, power, expansion or firmware settings are changed by NetSentinel. The next smallest project input is the manufacturer's Ultra V3 schematic/net table for this PCB revision and, for CC1101, the permitted external provider implementation/identity.
