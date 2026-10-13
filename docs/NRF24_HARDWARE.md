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

### Owner-supplied FAP import cross-check

The owner-supplied [static-analysis report](NetSentinel_FAP_Static_Analysis.md) lists the following direct SPI/GPIO dependencies for the two requested binaries. Every listed symbol is present as `+` (enabled) in the permitted API 87.1 / target 7 `api_symbols.csv`:

| FAP | SPI handle variables | SPI functions | GPIO references / functions | Other selection reference |
|---|---|---|---|---|
| `nrf24sniff.fap` | `furi_hal_spi_bus_handle_external`, `furi_hal_spi_bus_handle_external_extra` | `furi_hal_spi_bus_handle_init/deinit`, `furi_hal_spi_acquire/release`, `furi_hal_spi_bus_trx` | `gpio_ext_pa4`, `gpio_ext_pb2`, `gpio_ext_pc3`, `furi_hal_gpio_init`, `furi_hal_gpio_init_simple` | `momentum_settings` |
| `nrf24channelscanner.fap` | `furi_hal_spi_bus_handle_external` only | `furi_hal_spi_bus_handle_init/deinit`, `furi_hal_spi_acquire/release`, `furi_hal_spi_bus_trx` | `gpio_ext_pb2`, `gpio_ext_pc3`, `furi_hal_gpio_init`, `furi_hal_gpio_init_simple` | None listed |

The report also lists `furi_hal_power_enable_otg`, `furi_hal_power_disable_otg`, and `furi_hal_power_is_otg_enabled` in both FAPs. Their import presence is not proof that a particular control-flow path toggles power; NetSentinel does not import or use them.

The SDK-only Momentum settings header refines the API-level mapping: `SpiDefault` is documented as CS on PA4, `SpiExtra` as CS on PC3, and `MomentumSettings` contains `spi_nrf24_handle`. The external handle declarations match those CS choices while sharing MISO PA6, MOSI PA7 and SCK PB3. Thus the SDK supplies an API-supported way for an application to select a handle from the Momentum NRF24 setting. `nrf24sniff.fap`'s imports of both handles and `momentum_settings` are consistent with such selection, but imports alone do not prove the setting is read, which branch is taken, or how `gpio_ext_pc3`/`gpio_ext_pa4` are used. `nrf24channelscanner.fap`'s standard-handle import together with PC3 GPIO import likewise does not identify runtime CS behavior. No binary was executed or disassembled in this review.

This resolves the **SDK setting-to-generic-CS association**, not the Ultra V3 electrical routing: the owner's “Default 4” selection is consistent with the SDK's `SpiDefault`/PA4 designation, but does not prove the onboard module is connected to PA4 (or PB2 CE), or explain any selector/switching circuitry. The report's imported helper names and internal function names are static dependency evidence only, not proof of call order, receive behavior, teardown restoration, or packet reception.

All two binaries' listed SPI/GPIO/API names are enabled exports; there is no missing SPI symbol blocker for compiling against this API table. The actual safety blocker remains semantic: the API's `furi_hal_spi_acquire()` is a void, blocking call with no timeout. `furi_hal_spi_bus_trx()` has a per-transfer timeout and boolean result, which starts only after acquisition. An application mutex can serialize NetSentinel's own requests but cannot bound wait time against other users of the shared HAL bus. Do not substitute a detached worker, elapsed-time check after acquire, or transfer timeout for bounded acquisition.

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

## Read-only SPI sanity probe candidate

The permitted API 87.1 SDK exposes the generic external SPI handle on PA6 MISO, PA7 MOSI, PB3 SCK and software-controlled PA4 CS, plus an `external_extra` handle using PC3 CS. It exports handle init/deinit, acquire/release and timed bus transfer. The header states the handles are not initialized by default. `furi_hal_spi_acquire()` is explicitly blocking and has no timeout/cancellation parameter; `furi_hal_spi_bus_trx()` has a per-transfer timeout. Therefore this SDK alone cannot satisfy NetSentinel's finite acquisition contract and is not a ready physical adapter.

Nordic's [nRF24L01 Product Specification v2.0](https://cdn.sparkfun.com/datasheets/Wireless/Nordic/nRF24L01_Product_Specification_v2_0.pdf), register map §9.1, describes STATUS (0x07), CONFIG (0x00), RF_CH (0x05) and RF_SETUP (0x06). Their reads use `R_REGISTER` transactions; no register write, CONFIG change, CE transition, RX entry or payload command is needed. The same specification says STATUS bit 7 is reserved zero, STATUS `RX_P_NO` value 6 is unused, CONFIG bit 7 is reserved zero, RF_CH bit 7 is reserved zero, and RF_SETUP bits 7:5 are reserved zero. There is no chip identity/WHOAMI register. A no-write read of those four registers can only yield a *plausible SPI response*, not prove an authentic nRF24 device: a floating-low bus can still return plausible zero values.

NetSentinel now has a separately testable `ns_nrf_probe_readonly()` contract. It submits only reads of those four registers, passes a maximum 100 ms acquisition budget and at most 20 ms to each transfer, checks cancellation between transactions and releases ownership on every post-acquisition path. Elapsed time and cancellation are checked again after release. The release callback is required to restore the bus promptly; this core cannot forcibly interrupt an SDK release call. It does not touch CE. Its caller-supplied acquisition callback must honor a timeout, which the SDK's blocking acquire cannot guarantee. The function is not wired into the Flipper app, is not exposed in a manifest, and does not alter any feature gate. Host tests prove its command envelope and cleanup only; they do not establish bus behavior on an Ultra V3.

The public Sniffer's PA6/PA7/PB3 plus PA4 and PB2 selections, and Momentum's generic external handle pin notes, agree at the *Flipper connector/HAL* level. They do not establish the Ultra V3's internal module nets. “Default 4” has not been mapped by an authoritative Ultra V3 net table. Thus the required board-specific reconciliation is still missing, including whether the selected path routes CSN to PA4 and CE to PB2, any switching/buffering, and selector interaction.

### Evidence required before an owner-approved physical probe

1. A revision-matched Sacred Labs schematic/netlist or manufacturer statement identifying the onboard NRF24's SCK/MOSI/MISO/CSN/CE connections, selector path, and signal-voltage circuitry. The generic Flipper SDK mapping and the Sniffer source are not substitutes. If no such record exists, a qualified, owner-approved power-off continuity trace of the relevant nets is the smallest alternative; do not change selector or power controls during development.
2. Confirmation that Momentum “Default 4” selects that exact documented route and that no other board peripheral owns the same bus/CS path.
3. A bounded adapter ownership plan. The current exported SPI acquire is blocking, so either an authorized API with finite acquisition/cancellation or a separately reviewed proof that the app can safely and exclusively acquire the bus without an unbounded wait is needed.
4. Separate approval of the exact read-only candidate artifact/checksum and test procedure. The test should read the four registers with CE untouched, report only “plausible SPI response” or failure, and exit/release immediately. No RF capture or transmission is involved. A plausible response is not component authentication; no unique identity can be obtained from these registers.

Until those items are available, physical NRF24 access remains disabled. The standalone Sniffer's channel-finding/stop path transmits and is neither copied nor part of this probe.

## Reference-app comparison update — 2026-10-13

At the time of the prior review, `NetSentinel_FAP_Static_Analysis.md` was not available in the development workspace, so that earlier comparison was pending. The owner has since supplied it at [the project report](NetSentinel_FAP_Static_Analysis.md); the exact Sniffer and Channel Scanner imports are cross-checked below against the API 87.1 table.

The public [NRF24 Channel Scanner README](https://raw.githubusercontent.com/htotoo/NRF24ChannelScanner/main/README.md) documents generic Flipper expansion wiring: external SPI on pins 2/A7 MOSI, 3/A6 MISO, and 5/B3 SCK, with 4/A4 CSN and 6/B2 CE; it says IRQ is unconnected. This agrees with the generic HAL signals and the Sniffer's selected CE/CS choices at the Flipper connector level. It does not identify Ultra V3 onboard nets or Momentum's runtime “Default 4” route. The catalog pins the scanner source to [`6bd1a9362a22d580a3ec1e2490cea4eab1103d6b`](https://raw.githubusercontent.com/flipperdevices/flipper-application-catalog/main/applications/GPIO/nrf24channelscanner/manifest.yml), but its source files were not retrievable through the permitted public source view in this session. Therefore scanner C-level acquisition, initialization and teardown cannot be compared reliably here; README pinout text is not treated as proof of the implementation.

The Sniffer C source and its driver provide a separate, concrete warning. Its driver initializes `furi_hal_spi_bus_handle_external`, performs `furi_hal_spi_acquire()` (a blocking SDK operation), then uses software CS and PB2 CE. The stop/finalization path releases the bus after invoking channel-finding logic that transmits a ping packet. This is unsuitable as a receive-only lifecycle template. Source-level use does not prove a bounded ownership strategy: the API call itself has no deadline and returns no acquisition status. Adding a timeout around subsequent `furi_hal_spi_bus_trx()` calls cannot bound time spent waiting to acquire the bus.

NetSentinel's portable `ns_nrf_probe_readonly()` remains the candidate probe, and its injected I/O callbacks remain the candidate transport boundary. No Flipper SPI provider was added: implementing acquisition with the available blocking API would falsely advertise the 100 ms contract. The probe remains absent from the application entry path and FAP imports. No candidate GPIO, CE, CSN or SPI initialization was attempted.

To resume exact reference-FAP analysis, provide the missing report in this repository (for example `docs/NetSentinel_FAP_Static_Analysis.md`) or paste its symbol table. Before a hardware probe, also obtain board-specific Ultra V3 routing and a safe bounded/exclusive acquisition design as listed above. The physical feature gate remains disabled.
