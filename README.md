# NetSentinel Ultra — v1.0 software RC1

NetSentinel compiles and packages against the permitted Momentum Dev d3f89dfe SDK, **API 87.1 / target 7**. The unified hardware-gated candidate implements dashboard/navigation, ESP32 information diagnostics, filters, session history, persistent settings, private CSV exports and receive-analysis cores. **Full three-radio v1.0 remains blocked** by verified board routing, bounded external-radio implementation and installed Marauder passive/storage behavior. No Release or additional deployment is authorized.

Owner-verified prior results: GUI launch/dashboard/exit/settings persistence; three consecutive ESP Info queries returning Connected; normal exit and original Marauder app functionality afterward; no identifiers found in inspected history/CSV. Back cancellation and new diagnostics/observation workflows remain physically unverified. Prior FAPs, device evidence and user data are preserved.

## Software and build

From this project directory:

```sh
scripts/verify_development.sh rc
```

The pipeline runs **15 C suites and 24 Python checks**, short-enum builds, ASan/UBSan, clean uFBT compilation/link/packaging/APPCHK, complete FAP metadata/entry/import checks and publication audit. Local sanitizer leak detection defaults off; address/undefined-behavior checks remain enabled. Host peripherals are mocks, not hardware evidence. See [build instructions](docs/V1_BUILD.md) for permitted SDK inputs, isolated cache and local build evidence.

Candidate output: `dist/netsentinel_ultra_v1_0_rc1.fap`. Root manifest remains GUI/storage-only; candidate manifest uses the info-only USART gate. Live USART is fixed TX13/PB6, RX14/PB7, 115200/8N1 and accepts one info command per acquisition. NRF24/CC1101/Wi-Fi/BLE physical workflows stay disabled. No device access or firmware operation occurs in the pipeline. Validate each generated checksum; packaging is not claimed byte-for-byte deterministic.

The current development artifact adds Monitor, Lab (offline), Simulation, and Hardware Diagnostics modes. Lab/Simulation can generate bounded abstract synthetic events, replay them in RAM, and run a toy channel-overlap model. These are explicitly not RF frames or live measurements. Build it with `scripts/verify_development.sh development`; output is `dist/netsentinel_development.fap`. Details and boundaries are in [Operating Modes](docs/OPERATING_MODES.md). The tested owner-reported ESP32 `info` path is retained. No real transmit or packet replay adapter is included.

Hardware Diagnostics now reports radio interface availability, route verification, software receiver core, simulation availability, physical enablement and explicit blockers separately. NRF24 and CC1101 cores remain host/mock-only; Wi-Fi observation remains blocked by Marauder's AP initialization and peer-side PCAP storage behavior. Owner reports the existing Momentum Read RAW app entered its external CC1101 receive UI at 433.92 MHz and showed RSSI fluctuation; this does not verify decoded RF or NetSentinel's CC adapter. See the [Hardware Integration Milestone 1 evidence](docs/HARDWARE_INTEGRATION_M1.md), [NRF24 hardware readiness review](docs/NRF24_HARDWARE.md) and [CC1101 development notes](docs/CC1101.md). The NRF read-only probe is software-tested but not integrated or physically enabled. Generic SDK/Sniffer pins do not establish the Ultra V3 SPI route.

## Implemented boundaries

- GUI/controller own I/O on one worker; drawing reads bounded frames, Back is latched, queued/active queries cannot overlap, errors support safe retry and cleanup.
- Protocol/parser/transport separation provides strict echo/prompt validation, bounded lines/ISR buffers, malformed-response handling, timeout/cancellation and identifier redaction.
- NRF24 primary RX disables auto-ACK/retries; measured-channel activity summaries, plots and numeric CSV are implemented with SPI mocks.
- CC1101 status-only diagnostics, RXOFF policy checks, receive/RSSI contract, frequency exploration, traces and numeric logging are independently implemented; concrete external binding remains pending.
- Wi-Fi AP/channel/signal/baseline parsing is software-tested only. Marauder v1.11.0 initializes AP mode before the beacon receiver and may transmit; beacon PCAP capture may persist raw frames with SavePCAP defaulting on. Production start is blocked. BLE active discovery is excluded.
- Channel/RSSI/age filters and radio/error history filters are volatile. Existing NSP1 dual-slot CRC settings/history remain compatible; writes are read back and corrupt state is not erased.
- Exclusive CSV creation reserves IDs before writes. Storage errors close resources and retain recoverable history; AP/counter exhaustion releases sessions. CSV/UI omit names, MACs and raw responses. Peer-side Marauder PCAP settings are independently reviewed, not assumed safe.

Use the [user guide](docs/USER_GUIDE.md), [current readiness matrix](docs/V1_READINESS.md), [dependency evidence](docs/DEPENDENCY_REVIEW.md), [SDK inventory](docs/SDK_INTERFACES.md) and [hardware procedure](docs/V1_HARDWARE_VERIFICATION.md). Development history and physical reports remain in [milestones](docs/MILESTONES.md) and the [USART checklist](docs/USART_HARDWARE_CHECKLIST.md). Licensing/provenance constraints are in [PROVENANCE.md](docs/PROVENANCE.md); no upstream implementation code was imported in this milestone.

## Publication

[GitHub](https://github.com/1B-don/NetSentinel-Ultra) is the primary repository. [AGENTS.md](AGENTS.md) requires tested, privacy-audited commits and normal fast-forward pushes. If DNS prevents publication, commits remain local without repeated retries. Binaries, runtime logs, captures, SDK inputs and private editor configuration are never published; no GitHub Release precedes complete hardware verification and explicit approval.

The application now bundles the owner-supplied 10x10 monochrome icon in GUI, ESP Info, ESP Diagnostics and development builds. See [build instructions](docs/V1_BUILD.md) for variant selection and preserved artifact evidence. Feature gates are unchanged; no new device deployment has occurred.

The [software completion audit](docs/SOFTWARE_COMPLETION.md) records corrected query outcomes, capture deadlines, storage recovery presentation, queue cancellation and all-screen restart coverage.

Current CC1101 owner observations and mock-enforced callback budgets are documented in [CC1101.md](docs/CC1101.md). Momentum's exported generic Sub-GHz registry does not establish a usable bounded external provider: its `cc1101_ext` entry point is disabled in the API table and receive/tuning operations do not expose cancellation deadlines. The current development FAP remains provider-free.

RC1 is a built software candidate, not a fully operational three-radio release. All presently supported workflows are implemented and tested; remaining hardware-dependent features stay explicitly blocked. See [known limitations](docs/V1_LIMITATIONS.md). Use `scripts/verify_development.sh` without an argument to retain the separate development workflow.
