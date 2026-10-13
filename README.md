# NetSentinel Ultra — independent v1.0 development

Source development for Flipper Zero and FlipMods Ultra V3. No v1.0 release, real SDK build, verified FAP, or physical hardware test exists. The restricted Momentum checkout remains untouched.

## Build and tests

```sh
cd ~/Projects/NetSentinel-Ultra
make test
make sanitize
```

In this traced sandbox use `ASAN_OPTIONS=detect_leaks=0 make sanitize`. Address/undefined-behavior checks run; leak detection is unavailable. Five suites cover the imported parser/analysis, portable radio/session core, application/persistence integration, and Flipper GUI/storage contract shims. Shims are independent test doubles and cannot validate actual SDK declarations or ABI. Remote CI is configured but not run here.

## Implemented in source

- Flipper GUI shell with dashboard, NRF sampled-activity plot, CC RSSI view, settings, history, diagnostics, explicit selector confirmation, worker-owned I/O, and allocation/exit cleanup.
- Portable application controller with exclusive sessions, durable session-ID reservations, exclusive CSV creation, failed-start cleanup, history retry and SD reload.
- Two checksummed state slots store validated settings and the latest 16 completed sessions. An incomplete new slot leaves the earlier valid snapshot recoverable. Every successful save is read back. Corrupt/unreadable slots are not automatically erased or reset.
- Flipper storage adapter for state slots and CSV files under `/ext/apps_data/netsentinel`; no storage erasing or firmware operations.
- Independent NRF24 RX register driver: automatic acknowledgments/retries disabled, primary RX only, channels 0–125, bounded dwell from persisted settings, activity counts and percentages.
- CC1101 receive/RSSI abstraction with band validation, extrema, shutdown and error propagation. Concrete external-radio SDK callbacks are still missing.
- Imported portable Wi-Fi AP parsing, channel counts, comparisons and CSV escaping. Raw BLE handling remains in the core for future firmware-specific integration.

**ESP32 scans are disabled in the application controller and Flipper port.** No installed firmware profile has been confirmed. The default hardware port also disables NRF/CC acquisition; an authorized board port must implement and verify their callbacks before enabling capabilities. Menus and mocked measurements do not establish hardware functionality.

The candidate `application.fam` includes only root parser/analysis, portable core and Flipper shell sources. Archived `legacy` code, tests and host SDK shims are excluded. Its v1.0 target metadata is not a release declaration.

## Navigation

Up/Down selects a dashboard item; OK opens it; Back returns or exits. On a supported radio page, OK requests physical-selector confirmation, a second OK confirms and starts RX; OK stops. Settings use Up/Down and Left/Right, saving each successful change. History uses Up/Down with newest sessions first. Diagnostics OK retries pending history or reloads SD state. Stop before changing the physical selector.

## Remaining verification

Real SDK compilation/exported ABI, on-device GUI/heap/thread checks, authorized external-radio callbacks and verified board wiring, actual SD/power-loss behavior, RF timing/detection/RSSI, passive ESP32 firmware commands and BLE parsing, and approved deployment/launch. The current shell performs no GPIO power, SPI, UART or RF operations by default.

See [adapter contract](adapters/README.md), [manual compilation](docs/MANUAL_BUILD.md), [verification matrix](docs/VERIFICATION.md), [persistence format](docs/PERSISTENCE.md), [provenance](docs/PROVENANCE.md) and [development log](logs/development.md).
