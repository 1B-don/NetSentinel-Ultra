# NRF24 hardware integration readiness

Status: receive-only sampling core and deterministic adapter mocks are software-tested. The Ultra V3 physical adapter remains disabled. This review uses only the permitted Momentum API 87.1 / target 7 SDK-only headers/export table and public application references; it does not inspect the restricted firmware checkout or import third-party implementation code.

## Owner-confirmed observations

- Board: Sacred Labs FlipMods Ultra V3 with EBYTE E01-2G4M27SX module.
- Momentum NRF24 SPI option selected: “Default 4”.
- The owner's installed NRF24 Sniffer can enter its “Sniffing: Yes” state and repeat after app restart.
- Packet reception and the Ultra V3's actual board routing have not been confirmed. The state label proves only that the application reached its software state.

## Reference application inspected

The public [`mothball187/flipperzero-nrf24` Sniffer](https://github.com/mothball187/flipperzero-nrf24/blob/main/nrfsniff/nrfsniff.c) is the exact standalone `nrfsniff` implementation reviewed. Its initialization calls `nrf24_init()`, and `start_sniffing()` enters its promiscuous receive setup. Its ordinary loop checks `nrf24_sniff_address()`. However, stopping sniffing calls `wrap_up()`, and that function invokes `nrf24_find_channel()`. In the companion [`nrf24.c`](https://github.com/mothball187/flipperzero-nrf24/blob/main/lib/nrf24.c), `nrf24_find_channel()` constructs a ping packet and passes it to `nrf24_txpacket()`; that helper writes a TX payload, enters TX mode, and waits for completion. Thus this public application's complete sniff/stop workflow is not receive-only. NetSentinel does not reuse or call that workflow. The repository README also describes its apps as including sniffing and mousejack functionality ([README](https://github.com/mothball187/flipperzero-nrf24/blob/main/README.md)). No source was copied. The checked repository page did not expose a license file, so reuse of its code is not assumed permitted.

The inspected `nrf24.h` selects `furi_hal_spi_bus_handle_external` and `gpio_ext_pb2` for CE. The handle's SDK-only declaration documents SPI signals MISO PA6, MOSI PA7, SCK PB3, and software CS PA4; the alternate external-extra handle uses PC3 for CS. These are Flipper generic external pins and the reference app's choices, not evidence that those nets reach the Ultra V3's onboard NRF24 or that they correspond electrically to the owner's selected “Default 4” path. The SDK does not expose a named “Default 4” setting in application metadata. The same SPI bus selection reported by a working app is not a board schematic or a verified packet-reception test.

## Permitted Momentum API 87.1 evidence

The local authorized SDK-only `furi_hal_spi_config.h` documents `furi_hal_spi_bus_handle_external` with PA6/PA7/PB3, software CS PA4, 2 MHz preset; `furi_hal_spi_bus_handle_external_extra` uses the same data/clock pins and CS PC3. The SDK's `api_symbols.csv` marks `furi_hal_spi_bus_handle_init`, `furi_hal_spi_acquire`, `furi_hal_spi_release`, and `furi_hal_spi_bus_trx` as enabled callable exports. `furi_hal_spi_bus_trx` accepts a timeout and returns success/failure. The SPI header explicitly marks `furi_hal_spi_acquire()` as blocking and declares no timeout/cancellation parameter. GPIO declarations include `gpio_ext_pb2`, but API declaration/export does not prove the board route.

These SDK artifacts establish which Flipper HAL contracts an application may call. They do not provide the Ultra V3 netlist, runtime value of the “Default 4” configuration, safe interaction with the board's selector, chip presence, or packet-reception proof. The lack of a bounded acquire operation is a concrete adapter-contract gap; until ownership can be guaranteed without an uninterruptible wait, the physical adapter remains off.

## NetSentinel receive-only candidate contract

The existing portable `ns_nrf_open/sample/close` core is the candidate driver behind an injected `NsPlatform` interface. It writes only CONFIG/channel/receive-register configuration, verifies register readback, keeps `PRIM_RX=1`, clears auto-ack and retry registers, samples the RPD register, and exposes no payload, TX, ACK-payload, carrier, or retry operation. The Flipper SPI guard allows only this register envelope. This is software behavior under a mock peripheral, not a board-specific adapter or RF test.

Each SPI callback now receives a 20 ms transfer timeout (`NS_NRF_SPI_TIMEOUT_MS`) and must return `NS_TIMEOUT` when exceeded. Every failed initialization/sample unwinds CE low and releases ownership; repeated sessions and cancellation between bounded core calls are covered by deterministic mocks. The API's blocking acquire remains outside this bound. A future physical provider must demonstrate a safe finite/nonblocking acquisition strategy, CS handling, handle/GPIO lifecycle, finite transfer error propagation, selector compatibility, and restoration after normal, timeout, cancellation, and app-exit paths. The route feature macro stays unset in all application manifests.

## Readiness matrix and smallest next test

| Item | Classification | Remaining evidence / smallest safe next step |
|---|---|---|
| Portable RX-only sampling | Implemented and host-tested | None for software core; still not live RF evidence |
| SDK SPI interface | API 87.1 declarations and exports inspected | Resolve blocking acquire ownership/lifecycle before using it in an interruptible app path |
| Generic Sniffer pins | Public implementation documents external SPI + PB2 CE | Does not establish Ultra V3 mapping or selected Default 4 implementation |
| Ultra V3 SPI, CSN, CE, IRQ and selector nets | Blocked | Obtain a revision-matched Sacred Labs schematic/net table or explicit manufacturer pin map. If unavailable, owner-authorized, power-off continuity tracing by a qualified person is the minimum physical alternative; NetSentinel does not change selector/power settings |
| Packet reception | Unverified | After route and initialization review, and a separately approved test, observe a known controlled nRF24 source without transmitting from NetSentinel; verify a measurement changes and cleanup leaves the existing Momentum app usable |
| TX-free lifecycle | Not inherited from Sniffer | Do not use Sniffer stop/finalization path as receive-only precedent; verify NetSentinel's guarded SPI transcript contains no payload/TX/carrier commands |

Do not switch the Ultra V3 selector or energize the candidate adapter during this milestone. No physical NRF24 initialization, scan or deployment was performed.
