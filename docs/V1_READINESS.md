# NetSentinel Ultra v1.0 readiness and implementation plan

This is a development audit, not a release declaration. Target: supplied Momentum Dev d3f89dfe SDK, API 87.1, target 7. No deployment is authorized. Preserved stable GUI/storage, ESP Info and diagnostics artifacts and checksums are under ignored builds/preserved-v1-start; prior physical evidence remains unchanged. Application SD data was not accessed.

## Audit at 0b84e05

| Capability | Implemented | Simulated | SDK validated | Physical evidence | Blocker / remaining work |
|---|---|---|---|---|---|
| GUI/navigation/lifecycle | Worker/frame/queue, Back latch, cleanup | Allocation unwind/navigation/exit | Yes | Baseline launch/dashboard/exit | Full navigation polish and soak pending |
| ESP USART info | Exclusive USART, bounded ISR ring, strict one-info allowlist | Fragment/error/timeout/cancel/repeat | Yes | Three consecutive Connected, normal exit, original app works | Physical cancellation and exhaustive restoration pending |
| ESP diagnostics | Matched constants, typed SD state, timing/retry | Yes | Yes | Diagnostics v0.2 not tested | Full readiness/connection presentation |
| Wi-Fi observations | AP parser, channel counts, baseline comparisons, signal range | Synthetic beacon output | Core compiled, adapter disabled | Other app AP discovery only | Installed passive initialization/stop behavior not proved |
| BLE observations | Explicit unsupported state | Exclusion tests | Disabled | None | No verified passive command/output contract |
| NRF24 activity | PRX, auto-ACK/retry off, RPD counters, plot/CSV | SPI readback/CE/error mocks | Core compiled, adapter disabled | Standalone app works | Ultra SPI/CE/CS routing and bounded physical port unknown |
| CC1101 RSSI | Band validation, receive contract, status-read SPI, trace/CSV | Identity/RX/state/RSSI mocks | Core compiled, adapter disabled | Prior operation reported | External-only port, routing, RX preset/calibration and RF reception pending |
| Persistence/history | CRC dual slots, ID reservation, exclusive CSV, error recovery | Torn writes/readback/history wrap | Yes | Settings persist; inspected info CSV/history private | Filtering/export UX and broader fault/stress coverage |
| Privacy | Info UI constants, raw UART redaction | Identifier tests | Yes | Owner inspected records | Audit dormant AP UI/CSV paths before enabling observation |
| Filters | Missing unified filters | No | No | None | Bounded history/channel/signal filters |
| Reproducible full development build | Separate info artifact helper | Build guards | Yes for current helper | No new artifact authorized | Unified dev helper and mandatory package evidence |

## Sequential milestones

1. Audit and preserve baseline (this document). Record precise dependencies and passive policy.
2. Implement bounded filtering, privacy-safe observation/CSV presentation, history navigation and NRF activity summaries. Preserve NSP1 persistence schema/data compatibility.
3. Add deterministic malformed-input fuzz/stress, cancellation and ownership regressions; fix findings. Existing suites remain mandatory.
4. Prepare unified development FAP using reviewed SDK-only APIs, preserve all stable artifacts, strictly validate actual metadata/imports, document user guide and final hardware procedure.
5. Physical adapters are separate dependency work: only implement bindings after routing and receive configuration evidence exists. Hardware testing waits until software work is complete and separately approved.

## Authoritative hardware investigation

Reviewed manufacturer [Ultra V3 guide](https://sacredlabstech.com/boards/ultra-v3/) and [component guide](https://sacredlabstech.com/guides/components/). They identify ESP32-WROOM-32UE/NRF24/CC1101, the 18-pin connector, NRF/CC selector and 433 MHz emphasis; neither supplies an SPI netlist, CE/CS/GDO pin assignments, switch truth table or voltage circuitry. Module datasheets cannot establish PCB routing. Required input: an authorized manufacturer schematic/netlist or permitted working external-radio source identifying exact pin routes and bus ownership. Continuity measurements, selector changes and hardware access require owner action/approval; none performed here.

NRF register-level receive behavior is encoded and tested: CONFIG PRIM_RX before CE, EN_AA=0, SETUP_RETR=0, no payload/TX/carrier commands. [Nordic specification](https://docs.nordicsemi.com/r/bundle/pdf_ps_nrf24l01p/) defines chip behavior, not Ultra wiring. CC SPI diagnostics issue status reads only; [TI datasheet](https://www.ti.com/lit/ds/symlink/cc1101.pdf) defines silicon bands and RX state. Physical cc_receive must additionally supply a reviewed preset, calibration/settling and automatic-state policy excluding TX, with external-only selection and cleanup. An opaque receive callback name is not evidence of passive operation.

[Marauder beacon sniff](https://github.com/justcallmekoko/ESP32Marauder/wiki/beacon-sniff) documents received beacon processing. It does not prove installed build initialization emits no probes/association traffic or BLE scan requests. Hardware UART remains info-only until exact installed command/init/stop source evidence demonstrates the full passive lifecycle. Successful Wi-Fi Recon in another app is functional evidence, not TX exclusion evidence. No speculative scan/BLE commands will be enabled.

## Release gate

Full three-radio functionality is blocked by the above inputs. Software mocks and a packaged development FAP do not establish hardware completeness. No v1.0 Release until implemented capabilities, passive review, compatible SDK build, physical checklist and explicit owner approval are all satisfied. Existing licensing/provenance requirements remain applicable.

## Observation and storage milestone

Implemented bounded channel/RSSI/age filters, filtered newest-first history and numeric history CSV exports with pre-create ID reservation, exclusive creation and cleanup on every storage fault. Added NRF measured-channel peak/quiet summaries, blocked-adapter explanations, and a private AP UI/CSV path. Filters are deliberately volatile; existing NSP1 settings/history schema remains compatible. Added tests for age wrap, invalid measurements, small-buffer transactionality, filtered UI, storage failures, ownership exclusion and actual FAP metadata mutation. Eleven C suites, short enums, sanitizer, preflight/publication tests and matching SDK build/import checks pass. Physical bindings remain blocked; full feature completeness is not claimed.

## Stress, diagnostics and remaining gates

Twelve C suites now pass, including deterministic 20,000 parser mutations, 2,000 fragmented/interrupted UART transactions across tick wrap, 25,200 mocked NRF primary-RX samples, and 10,000 UI events. UART line staging is scrubbed at teardown. GUI rendering measures real SDK string width and marks truncation instead of drawing outside the 128-pixel screen; host draw mocks assert width bounds. Unified diagnostics has storage, USART and blocked-radio views. Full visual navigation remains physically untested.

Observed vs implemented status remains separate: ESP info has owner-reported repeated success and restored original app functionality; Back cancellation is still pending. NRF/CC processing is implemented and simulated, compiled into the development FAP, but physical adapters remain disabled. Wi-Fi channel/AP processing and filtering are implemented/simulated; installed initialization/receive/stop TX exclusion remains unresolved. BLE remains unsupported because no verified passive protocol exists. No complete three-radio v1.0 claim is justified.

Permitted SDK headers/export table provide SPI and SubGhzDevice APIs (see SDK_INTERFACES.md). Transfer deadlines exist, but SPI bus acquisition has no deadline parameter. External plugin implementation, exact device identity, pin routes and ownership semantics are still required. Manufacturer guides contain no netlist; no inferred binding was created.

## Updated passive-protocol investigation

Read-only [upstream v1.11.0 WiFiScan.cpp](https://raw.githubusercontent.com/justcallmekoko/ESP32Marauder/v1.11.0/esp32_marauder/WiFiScan.cpp) review finds receive modes calling setWiFiMode with WIFI_MODE_NULL, a promising passive configuration. It does not establish the exact installed binary/options or complete initialization/stop path. RunAPScan also starts PCAP handling; its default storage effects need review before privacy-safe live integration. The upstream Bluetooth scan path explicitly enables active scan (setActiveScan(true)), so it cannot be treated as passive discovery. No commands were sent or upstream code copied. These are source facts; installed-build correspondence and side effects remain unresolved.

## Current readiness summary

| Area | Implemented / simulated | SDK build | Physical / blocked |
|---|---|---|---|
| Dashboard, lifecycle, GUI width/recovery | Implemented; navigation/fault/stress mocks pass | Pass | Prior dashboard/exit pass; new full navigation pending |
| ESP info/diagnostics | Implemented; bounded transport/parser/cancellation mocks pass | Pass | Prior three queries/exit/interop/privacy pass; new diagnostics and cancellation pending |
| Filters/history/settings/private exports | Implemented; torn-write/export/filter mocks pass | Pass | Prior settings/privacy pass; new workflows pending |
| NRF RPD sampling, graphical/statistical analysis, CSV | Implemented; receive-only SPI mocks pass | Compiled; no physical SPI imports | Board routing/ownership port blocked |
| CC status/RSSI trace/tune/CSV | Implemented; simulated responses pass | Compiled; no external device imports | External plugin/routing/preset/calibration and RF verification blocked |
| Wi-Fi AP/channel/baseline/signal processing | Implemented with synthetic observations | Compiled; live scan gate off | Exact installed passive/privacy lifecycle blocked |
| BLE discovery | Unsupported; no guessed parser/commands | Disabled | Upstream active scan unsuitable for receive-only requirement |
| Software verification pipeline | Implemented; strict metadata/import/privacy checks pass | Pass | Full hardware release gate remains open |

Smallest authorized next input is documentation/source, not another experimental deployment: manufacturer Ultra V3 SPI/CE/CS/GDO netlist and switch circuitry, permitted external CC1101 plugin implementation/identity and receive preset, and installed Marauder build correspondence/options plus receive/stop/PCAP-default evidence. No additional radio binding can responsibly be completed without those facts. Inputs must be independently permitted and outside the restricted checkout. No firmware replacement is proposed. Security information is currently absent in the reviewed AP fixtures; arbitrary new field formats are not guessed.

## Software completion audit fixes

Found and corrected age filtering that advanced only on input, AP-table capacity errors that retained UART ownership, and unbounded total observation counters. Filter expiry now follows worker/feed time across tick wrap, including idle screens. AP-table/counter exhaustion stops, closes logs and releases resources with Full recorded in history. No-measurement NRF statistics and empty filtered history have explicit states. History result CSV values now use enum units. Wi-Fi mock observations produce only session-local AP numbers, channel and RSSI in CSV; no names/MAC/raw responses. The development FAP still cannot issue scans.

Fuzz now mixes valid seeds, seeded mutations and random inputs, asserts more than 3,000 successful parses for each AP/settings path, and round-trips valid settings. UART stress includes complete sanitized replies, mutations and garbage with random fragment sizes. All 12 C suites, 17 Python checks, short enums, ASan/UBSan and real Momentum build/metadata/63-import validation pass. No hardware operation was performed; preserved-0564846 retains all prior artifacts/checksums.
