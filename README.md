# NetSentinel Ultra — v1.0 development

NetSentinel compiles and packages against the permitted Momentum Dev d3f89dfe SDK, **API 87.1 / target 7**. The unified development app implements dashboard/navigation, ESP32 information diagnostics, filters, session history, persistent settings, private CSV exports and receive-analysis cores. **Full three-radio v1.0 remains blocked** by verified board routing, bounded external-radio implementation and installed Marauder passive/storage behavior. No Release or additional deployment is authorized.

Owner-verified prior results: GUI launch/dashboard/exit/settings persistence; three consecutive ESP Info queries returning Connected; normal exit and original Marauder app functionality afterward; no identifiers found in inspected history/CSV. Back cancellation and new diagnostics/observation workflows remain physically unverified. Prior FAPs, device evidence and user data are preserved.

## Software and build

From this project directory:

```sh
scripts/verify_development.sh
```

The pipeline runs **12 C suites and 19 Python checks**, short-enum builds, ASan/UBSan, clean uFBT compilation/link/packaging/APPCHK, complete FAP metadata/entry/import checks and publication audit. Local sanitizer leak detection defaults off; address/undefined-behavior checks remain enabled. Host peripherals are mocks, not hardware evidence. See [build instructions](docs/V1_BUILD.md) for permitted SDK inputs, isolated cache and local build evidence.

Separate output: `dist/netsentinel_development.fap`. Root manifest remains GUI/storage-only; development manifest uses the info-only USART gate. Live USART is fixed TX13/PB6, RX14/PB7, 115200/8N1 and accepts one info command per acquisition. NRF24/CC1101/Wi-Fi/BLE physical workflows stay disabled. No device access or firmware operation occurs in the pipeline. Validate each generated checksum; packaging is not claimed byte-for-byte deterministic.

## Implemented boundaries

- GUI/controller own I/O on one worker; drawing reads bounded frames, Back is latched, queued/active queries cannot overlap, errors support safe retry and cleanup.
- Protocol/parser/transport separation provides strict echo/prompt validation, bounded lines/ISR buffers, malformed-response handling, timeout/cancellation and identifier redaction.
- NRF24 primary RX disables auto-ACK/retries; measured-channel activity summaries, plots and numeric CSV are implemented with SPI mocks.
- CC1101 status-only diagnostics, RXOFF policy checks, receive/RSSI contract, frequency exploration, traces and numeric logging are independently implemented; concrete external binding remains pending.
- Wi-Fi AP/channel/signal/baseline processing is host-tested; complete identity and stop acknowledgment are required by its dormant protocol. No encryption format is guessed. BLE active discovery is excluded.
- Channel/RSSI/age filters and radio/error history filters are volatile. Existing NSP1 dual-slot CRC settings/history remain compatible; writes are read back and corrupt state is not erased.
- Exclusive CSV creation reserves IDs before writes. Storage errors close resources and retain recoverable history; AP/counter exhaustion releases sessions. CSV/UI omit names, MACs and raw responses. Peer-side Marauder PCAP settings are independently reviewed, not assumed safe.

Use the [user guide](docs/USER_GUIDE.md), [current readiness matrix](docs/V1_READINESS.md), [dependency evidence](docs/DEPENDENCY_REVIEW.md), [SDK inventory](docs/SDK_INTERFACES.md) and [hardware procedure](docs/V1_HARDWARE_VERIFICATION.md). Development history and physical reports remain in [milestones](docs/MILESTONES.md) and the [USART checklist](docs/USART_HARDWARE_CHECKLIST.md). Licensing/provenance constraints are in [PROVENANCE.md](docs/PROVENANCE.md); no upstream implementation code was imported in this milestone.

## Publication

[GitHub](https://github.com/1B-don/NetSentinel-Ultra) is the primary repository. [AGENTS.md](AGENTS.md) requires tested, privacy-audited commits and normal fast-forward pushes. If DNS prevents publication, commits remain local without repeated retries. Binaries, runtime logs, captures, SDK inputs and private editor configuration are never published; no GitHub Release precedes complete hardware verification and explicit approval.

The application now bundles the owner-supplied 10x10 monochrome icon in GUI, ESP Info, ESP Diagnostics and development builds. See [build instructions](docs/V1_BUILD.md) for variant selection and preserved artifact evidence. Feature gates are unchanged; no new device deployment has occurred.

The [software completion audit](docs/SOFTWARE_COMPLETION.md) records corrected query outcomes, capture deadlines, storage recovery presentation, queue cancellation and all-screen restart coverage.
