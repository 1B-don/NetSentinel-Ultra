# NetSentinel Ultra — v1.0 software RC1

The [unified ESP/Wi-Fi milestone](docs/WIFI_DISCOVERY.md) records owner-reported query/relaunch/original-app interoperation for the frozen ESP Status SHA-256 `7751b712e71b84b396a13a6ca2f33649e7d3b65031bd4dceb99d5b732ff11cf7`. New `esp-stable` and `wifi-dev` candidates use a unified dashboard; Wi-Fi is software-tested and physically disabled. The latter adds an explicitly offline parser self-check, with no fake network readings. Both transports remain info-only and new candidates await separate hardware approval.

NetSentinel compiles and packages against the permitted Momentum Dev d3f89dfe SDK, **API 87.1 / target 7**. The unified hardware-gated candidate implements dashboard/navigation, ESP32 information diagnostics, filters, session history, persistent settings, private CSV exports and receive-analysis cores. **Full three-radio v1.0 remains blocked** by verified board routing, bounded external-radio implementation and installed Marauder passive/storage behavior. No Release or additional deployment is authorized.

Owner-verified prior results: GUI launch/dashboard/exit/settings persistence; three consecutive ESP Info queries returning Connected; normal exit and original Marauder app functionality afterward; no identifiers found in inspected history/CSV. Back cancellation and new diagnostics/observation workflows remain physically unverified. Prior FAPs, device evidence and user data are preserved.

## Software and build

From this project directory:

```sh
scripts/verify_development.sh rc
```

The pipeline runs **17 C suites and 27 Python checks**, short-enum builds, ASan/UBSan, clean uFBT compilation/link/packaging/APPCHK, complete FAP metadata/entry/import checks and publication audit. Local sanitizer leak detection defaults off; address/undefined-behavior checks remain enabled. Host peripherals are mocks, not hardware evidence. See [build instructions](docs/V1_BUILD.md) for permitted SDK inputs, isolated cache and local build evidence. New candidate commands are `scripts/verify_development.sh esp-stable` and `scripts/verify_development.sh wifi-dev`; rebuilding the owner-verified esp-status filename is refused.

Candidate output: `dist/netsentinel_ultra_v1_0_rc1.fap`. Root manifest remains GUI/storage-only; candidate manifest uses the info-only USART gate. Live USART is fixed TX13/PB6, RX14/PB7, 115200/8N1 and accepts one info command per acquisition. NRF24/CC1101/Wi-Fi/BLE physical workflows stay disabled. No device access or firmware operation occurs in the pipeline. Validate each generated checksum; packaging is not claimed byte-for-byte deterministic.

The current development artifact adds Monitor, Lab (offline), Simulation, and Hardware Diagnostics modes. Lab/Simulation can generate bounded abstract synthetic events, replay them in RAM, and run a toy channel-overlap model. These are explicitly not RF frames or live measurements. Build it with `scripts/verify_development.sh development`; output is `dist/netsentinel_development.fap`. Details and boundaries are in [Operating Modes](docs/OPERATING_MODES.md). The tested owner-reported ESP32 `info` path is retained. No real transmit or packet replay adapter is included.

Hardware Diagnostics reports radio interface availability, route verification, software receiver core, simulation availability, physical enablement and explicit blockers separately. NRF24/CC1101 remain host/mock-only. Wi-Fi stays blocked on installed passive lifecycle and peer PCAP privacy. The pinned-source recheck corrects the earlier AP-start attribution: discovery selects NULL/promiscuous mode; AP setup is in another routine. No installed RF-silence claim follows. Owner Read RAW/RSSI observations do not verify NetSentinel's CC adapter. See [hardware integration evidence](docs/HARDWARE_INTEGRATION_M1.md), [NRF24 readiness](docs/NRF24_HARDWARE.md) and [CC1101 notes](docs/CC1101.md). No generic pin map establishes Ultra V3 routing.

The supplied seven-FAP static-analysis report is now cross-checked against Momentum API 87.1: the Sniffer and Channel Scanner's listed SPI/GPIO imports are all enabled exports. The SDK maps Momentum's NRF24 `SpiDefault`/`SpiExtra` setting to generic CS PA4/PC3, but that does not verify Ultra V3 wiring or prove the reference FAPs' runtime path. The source-level Channel Scanner init/teardown comparison remains incomplete; the physical adapter is still gated on bounded acquisition and board routing. See the NRF24 review for details.

## Implemented boundaries

The [ESP32 status candidate](docs/ESP32_STATUS_DIAGNOSTICS.md) adds independent typed connection/result diagnostics, a fourth redacted response view and stale-buffer/duplicate-echo rejection. Build separately with `scripts/verify_development.sh esp-status`; previous FAPs are preserved. Only `info` is permitted, and this exact candidate awaits separate physical approval. Delayed complete stale replies cannot be authenticated because the protocol has no request ID.

The [public external-provider implementation decision](docs/CC1101_PROVIDER_IMPLEMENTATION.md) compares the upstream lifecycle with API 87.1. It identifies unbounded calibration, fatal transitions and unverified SPI transfer bounds; no physical adapter or additional radio mock is added. An app-owned metadata loader is a conditional discovery alternative, pending exact plugin/loader provenance and safe finite lifecycle evidence.

The [provider-discovery decision](docs/CC1101_DISCOVERY_READINESS.md) finds no supported safe registry initialization/borrowing lifecycle for a metadata-only diagnostic. No discovery FAP is generated and no registry calls are made. Source preflight now inventories Sub-GHz/plugin-manager calls so future changes cannot omit them from names-only checks.

The [CC1101 registry readiness report](docs/CC1101_REGISTRY_READINESS.md) confirms an exported named-discovery path independent of the disabled external entrypoint import. Registered, initialized, connected, RX and received-data states remain distinct. Registry lifetime and provider deadlines/passivity are unverified, so physical access stays disabled; deterministic future-provider mocks exercise missing/wrong-name devices and cleanup.

The [hardware architecture decision](docs/HARDWARE_ARCHITECTURE.md) ranks existing external CC registry integration, a separate synchronized read-only diagnostic, firmware services and a coprocessor bridge. None currently closes the bounded-ownership and board-routing gates. Typed board regression tests now cover 24 alternating NRF/CC contention, cancellation and cleanup cycles; no new physical provider is enabled.

The [API 87.1 acquisition review](docs/NRF24_SPI_ACQUISITION.md) found no exported bounded SPI ownership mechanism: timed mutex calls lack access to the shared SPI lock, and bus event callbacks have no timeout/result contract. Physical NRF24 stays disabled. Expanded probe mocks cover acquisition failure/cancellation and recovery without releasing another client's ownership.

- GUI/controller own I/O on one worker; drawing reads bounded frames, Back is latched, queued/active queries cannot overlap, errors support safe retry and cleanup.
- Protocol/parser/transport separation provides strict echo/prompt validation, bounded lines/ISR buffers, malformed-response handling, timeout/cancellation and identifier redaction.
- NRF24 primary RX disables auto-ACK/retries; measured-channel activity summaries, plots and numeric CSV are implemented with SPI mocks.
- CC1101 status-only diagnostics, RXOFF policy checks, receive/RSSI contract, frequency exploration, traces and numeric logging are independently implemented; concrete external binding remains pending.
- Wi-Fi AP/channel/signal/baseline parsing is software-tested only. Reviewed discovery paths select NULL/promiscuous mode and open peer PCAP handling with SavePCAP defaulting on. Installed passivity, storage privacy and stop behavior remain unverified; physical commands stay blocked. BLE active discovery is excluded.
- Channel/RSSI/age filters and radio/error history filters are volatile. Existing NSP1 dual-slot CRC settings/history remain compatible; writes are read back and corrupt state is not erased.
- Exclusive CSV creation reserves IDs before writes. Storage errors close resources and retain recoverable history; AP/counter exhaustion releases sessions. CSV/UI omit names, MACs and raw responses. Peer-side Marauder PCAP settings are independently reviewed, not assumed safe.

Use the [user guide](docs/USER_GUIDE.md), [current readiness matrix](docs/V1_READINESS.md), [dependency evidence](docs/DEPENDENCY_REVIEW.md), [SDK inventory](docs/SDK_INTERFACES.md) and [hardware procedure](docs/V1_HARDWARE_VERIFICATION.md). Development history and physical reports remain in [milestones](docs/MILESTONES.md) and the [USART checklist](docs/USART_HARDWARE_CHECKLIST.md). Licensing/provenance constraints are in [PROVENANCE.md](docs/PROVENANCE.md); no upstream implementation code was imported in this milestone.

## Publication

[GitHub](https://github.com/1B-don/NetSentinel-Ultra) is the primary repository. [AGENTS.md](AGENTS.md) requires tested, privacy-audited commits and normal fast-forward pushes. If DNS prevents publication, commits remain local without repeated retries. Binaries, runtime logs, captures, SDK inputs and private editor configuration are never published; no GitHub Release precedes complete hardware verification and explicit approval.

The application now bundles the owner-supplied 10x10 monochrome icon in GUI, ESP Info, ESP Diagnostics and development builds. See [build instructions](docs/V1_BUILD.md) for variant selection and preserved artifact evidence. Feature gates are unchanged; no new device deployment has occurred.

The [software completion audit](docs/SOFTWARE_COMPLETION.md) records corrected query outcomes, capture deadlines, storage recovery presentation, queue cancellation and all-screen restart coverage.

Current CC1101 owner observations and mock-enforced callback budgets are documented in [CC1101.md](docs/CC1101.md). Momentum's exported generic Sub-GHz registry does not establish a usable bounded external provider: its `cc1101_ext` entry point is disabled in the API table and receive/tuning operations do not expose cancellation deadlines. The current development FAP remains provider-free.

RC1 is a built software candidate, not a fully operational three-radio release. All presently supported workflows are implemented and tested; remaining hardware-dependent features stay explicitly blocked. See [known limitations](docs/V1_LIMITATIONS.md). Use `scripts/verify_development.sh` without an argument to retain the separate development workflow.
