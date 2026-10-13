# NetSentinel Ultra — independent v1.0 development

Source development for Flipper Zero and FlipMods Ultra V3. No v1.0 release, real SDK build, verified FAP, or NetSentinel physical hardware test exists. The restricted Momentum checkout remains untouched.

## Build and tests

```sh
cd ~/Projects/NetSentinel-Ultra
make test
make sanitize
```

In this traced sandbox use `ASAN_OPTIONS=detect_leaks=0 make sanitize`. Address/undefined-behavior checks run; leak detection is unavailable. Eight suites cover the imported parser/analysis, portable radio/session core, application/persistence integration, and Flipper GUI/storage contract shims. Shims are independent test doubles and cannot validate actual SDK declarations or ABI. Remote CI is configured but not run here.

## Implemented in source

- Flipper GUI shell with dashboard, NRF sampled-activity plot, CC RSSI view, settings, history, diagnostics, explicit selector confirmation, worker-owned I/O, and allocation/exit cleanup.
- Portable application controller with exclusive sessions, durable session-ID reservations, exclusive CSV creation, failed-start cleanup, history retry and SD reload.
- Two checksummed state slots store validated settings and the latest 16 completed sessions. An incomplete new slot leaves the earlier valid snapshot recoverable. Every successful save is read back. Corrupt/unreadable slots are not automatically erased or reset.
- Flipper storage adapter for state slots and CSV files under `/ext/apps_data/netsentinel`; no storage erasing or firmware operations.
- Independent NRF24 RX register driver: automatic acknowledgments/retries disabled, primary RX only, channels 0–125, bounded dwell from persisted settings, activity counts and percentages.
- CC1101 read-only SPI diagnostic helper, receive/RSSI contract, 64-sample plot, mean/extrema/threshold summaries, receive-only presets/retuning, rate-limited sampling and CSV metadata. Concrete external-radio SDK callbacks are still missing. [CC1101 evidence and integration](docs/CC1101.md) separates simulated tests, user-reported operation and pending hardware tests.
- Imported portable Wi-Fi AP parsing, channel counts, comparisons and CSV escaping. Raw BLE handling remains in the core for future firmware-specific integration.

**ESP32 scans remain disabled by the installed-command verification gate.** The owner confirmed Marauder v1.11.0 and standalone Wi-Fi Recon operation; raw UART command/output capture is still needed. Added a source UART driver, provisional info parser, gated passive beacon protocol, Wi-Fi Explorer AP/RSSI/baseline integration and channel-summary logging. See [Marauder integration](docs/MARAUDER.md) for precise evidence and limitations. The default hardware port also disables NRF/CC acquisition; an authorized board port must implement and verify their callbacks before enabling capabilities. Menus and mocked measurements do not establish hardware functionality.

The candidate `application.fam` includes only root parser/analysis, portable core and Flipper shell sources. Archived `legacy` code, tests and host SDK shims are excluded. Its v1.0 target metadata is not a release declaration.

## Navigation

Up/Down selects a dashboard item; OK opens it; Back returns or exits. On NRF/CC pages, OK requests physical-selector confirmation, a second OK confirms and starts RX; OK stops. Settings use Up/Down and Left/Right, saving each successful change. History uses Up/Down with newest sessions first. Diagnostics OK retries pending history or reloads SD state. Stop before changing the physical selector.

## Remaining verification

Real SDK compilation/exported ABI, on-device GUI/heap/thread checks, authorized external-radio callbacks and verified board wiring, actual SD/power-loss behavior, RF timing/detection/RSSI, passive ESP32 firmware commands and BLE parsing, and approved deployment/launch. The current GUI/storage-only manifest disables UART acquisition and all radio capabilities; the preserved nonminimal configuration supports explicit device-info requests while scanning stays gated. It does not change GPIO power or acquire NRF/CC by default.

See [adapter contract](adapters/README.md), [manual compilation](docs/MANUAL_BUILD.md), [verification matrix](docs/VERIFICATION.md), [persistence format](docs/PERSISTENCE.md), [provenance](docs/PROVENANCE.md) and [milestones](docs/MILESTONES.md).

## GitHub milestone workflow

[GitHub](https://github.com/1B-don/NetSentinel-Ultra) is the primary development repository. [AGENTS.md](AGENTS.md) makes tested, privacy-reviewed milestone commits and normal verified pushes persistent. Raw logs, captures, binaries and private configuration stay local. Published history is preserved; earlier local log-bearing checkpoints are not pushed. [SDK requirements](docs/SDK_INTERFACES.md) records the exact current symbol inventory and unresolved hardware bindings.

The application now isolates the optional board port from GUI/storage/UART, validates advertised callbacks, enforces shared radio ownership, routes tests through actual GUI input callbacks and preserves Back cancellation when the input queue is full. Both default and board-enabled Flipper source variants compile against mocks; real SDK compilation remains pending.

Milestone `ab8e708` was published by the owner and independently verified on GitHub. Automatic normal pushes remain enabled; shell Git still cannot resolve GitHub in this execution environment as of the current check.

The NRF SPI adapter now rejects payload/TX commands, carrier mode, auto-ACK/retries and unsupported register operations before board callbacks. Exhaustive command/value rejection checks and a complete 126-channel scan pass against mocks. This guard does not establish physical SPI operation or firmware ABI compatibility.

Build readiness now includes `make sdk-preflight` for a permitted owner-provided target export table, before the manual SDK compile/link stage. No target export input is currently available. GUI shutdown clears draw/input callbacks and detaches the viewport before session/storage teardown; existing mock lifecycle and failure tests enforce cleared callbacks at viewport free.

[Remaining build blockers](docs/BUILD_BLOCKERS.md) separates all seven requested categories and the minimum authorized SDK/toolchain inputs. The default shell can be compiled before physical RF tests or complete NRF/CC bindings once a permitted compatible SDK is available; a shell-only FAP does not complete the three-radio application. No target compile/link or installation has yet occurred.

SDK preflight can also check an actual ARM ELF undefined-symbol listing against enabled function and variable exports, covering runtime helpers missed by source-call discovery. No real ARM ELF exists yet. Public SPI/external-device interface review narrowed adapter requirements; bounded acquisition, external plugin lifecycle, target ABI and Ultra wiring remain unresolved.

The initial SDK build configuration now defines `NS_GUI_STORAGE_ONLY`: navigation/settings/history/storage remain available, all radio capabilities are false, and serial HAL headers/imports are excluded. Full adapter sources and their tests are preserved. [SDK-only build instructions](docs/MANUAL_BUILD.md) describe uFBT, the exact missing archive/provenance and the smallest owner action. No SDK-only archive was obtained and no ARM/FAP build is claimed.

A subsequently discovered SDK-only installation identifies official firmware **1.4.3**, not installed Momentum d3f89dfe. All 12 GUI/storage application translation units now compile to real ARM objects against those headers after correcting short-enum range-check warnings. This is ARM compile-only evidence; no link/FAP packaging, target compatibility or physical test is claimed. `make short-enums` preserves coverage for the actual compiler-flag failure.

For your first Kubuntu build, start with [FIRST_BUILD.md](docs/FIRST_BUILD.md). The current manifest is accepted by the available official 1.4.3 SDK parser, but the exact installed-Momentum SDK remains missing. No further compilation is attempted until a compatible permitted SDK is available.
