# NetSentinel Ultra — independent v1.0 development

NetSentinel now compiles and packages a real GUI/storage-only FAP using the permitted **Momentum mntm-dev-d3f89dfe SDK**, matching the running firmware’s independently confirmed **API 87.1 / hardware target 7**. Clean compilation, linking, packaging and all 49 import checks pass. The owner physically verified launch, dashboard, Back exit, settings persistence and disabled-radio status on the first build. Radio operation and remaining navigation/error paths are unverified; no v1.0 release occurred.

Current rebuilt artifact: `dist/netsentinel.fap` (34,888 bytes; not physically tested). See [Momentum build evidence and reproducible commands](docs/MOMENTUM_BUILD.md). The earlier official SDK artifact/evidence are preserved separately.

[ESP32 preparation](docs/ESP32_PREPARATION.md) records UART topology/protocol blockers, deadline fixes and meaningful status UI. [The bridge connection procedure](docs/ESP32_CONNECTION_TEST.md) incorporates owner-confirmed photographs and remains blocked on exact board nets/circuitry and explicit hardware approval.

The earlier API 89.1 report is superseded by the owner’s direct USB CLI result. All radio adapters remain disabled. [First on-device test](docs/FIRST_DEVICE_TEST.md) requires explicit approval before installation or execution.

UART logs now contain fixed redacted telemetry rather than raw serial bytes; owner MACs and other identifiers are excluded. USB-C communication does not verify Flipper GPIO routing.

A separate **ESP32 information-only** test FAP now compiles with `NS_ESP_INFO_ONLY`; it permits only one info query and keeps NRF24/CC1101/scanning disabled. The owner confirmed the intended Flipper USART path; [test-build evidence and approval boundary](docs/ESP32_LIVE_INFO.md) describe the supported API, exact artifact and remaining physical checks. The owner reports one authorized information request displayed Connected. The owner also confirms clean exit and the original Marauder app working afterward. Independent field checks, exhaustive restoration, repeatability and cancellation checks remain pending. The newly rebuilt cleanup/privacy artifact has not been deployed; [hardware verification checklist](docs/USART_HARDWARE_CHECKLIST.md). The default FAP remains unchanged.

## Build and tests

```sh
cd ~/Projects/NetSentinel-Ultra
make test
make sanitize
```

In this traced sandbox use `ASAN_OPTIONS=detect_leaks=0 make sanitize`. Address/undefined-behavior checks run; leak detection is unavailable. Ten C suites (the prior nine plus the exact info-only configuration) plus eight SDK-preflight and four publication cases cover the imported parser/analysis, portable radio/session core, application/persistence integration, and Flipper GUI/storage contract shims. Shims are independent test doubles and cannot validate actual SDK declarations or ABI. Remote CI is configured but not run here.

## Implemented in source

- Flipper GUI shell with dashboard, NRF sampled-activity plot, CC RSSI view, settings, history, diagnostics, explicit selector confirmation, worker-owned I/O, and allocation/exit cleanup.
- Portable application controller with exclusive sessions, durable session-ID reservations, exclusive CSV creation, failed-start cleanup, history retry and SD reload.
- Two checksummed state slots store validated settings and the latest 16 completed sessions. An incomplete new slot leaves the earlier valid snapshot recoverable. Every successful save is read back. Corrupt/unreadable slots are not automatically erased or reset.
- Flipper storage adapter for state slots and CSV files under `/ext/apps_data/netsentinel`; no storage erasing or firmware operations.
- Independent NRF24 RX register driver: automatic acknowledgments/retries disabled, primary RX only, channels 0–125, bounded dwell from persisted settings, activity counts and percentages.
- CC1101 read-only SPI diagnostic helper, receive/RSSI contract, 64-sample plot, mean/extrema/threshold summaries, receive-only presets/retuning, rate-limited sampling and CSV metadata. Concrete external-radio SDK callbacks are still missing. [CC1101 evidence and integration](docs/CC1101.md) separates simulated tests, user-reported operation and pending hardware tests.
- Imported portable Wi-Fi AP parsing, channel counts, comparisons and CSV escaping. Raw BLE handling remains in the core for future firmware-specific integration.

**ESP32 scans remain disabled by the installed-command verification gate.** The owner confirmed Marauder v1.11.0 and standalone Wi-Fi Recon operation; owner USB-C/CH340 status now confirms installed identity/SD/battery labels, while header wiring, framing/help and passive scan behavior remain pending. Added a source UART driver, strict bounded info parser, gated passive beacon protocol, Wi-Fi Explorer AP/RSSI/baseline integration and channel-summary logging. See [Marauder integration](docs/MARAUDER.md) for precise evidence and limitations. The default hardware port also disables NRF/CC acquisition; an authorized board port must implement and verify their callbacks before enabling capabilities. Menus and mocked measurements do not establish hardware functionality.

The candidate `application.fam` includes only root parser/analysis, portable core and Flipper shell sources. Archived `legacy` code, tests and host SDK shims are excluded. Its v1.0 target metadata is not a release declaration.

## Navigation

Up/Down selects a dashboard item; OK opens it; Back returns or exits. On NRF/CC pages, OK requests physical-selector confirmation, a second OK confirms and starts RX; OK stops. Settings use Up/Down and Left/Right, saving each successful change. History uses Up/Down with newest sessions first. Diagnostics OK retries pending history or reloads SD state. Stop before changing the physical selector.

## Remaining verification

Installed Momentum exported ABI/loader compatibility, on-device GUI/heap/thread checks, authorized external-radio callbacks and verified board wiring, actual SD/power-loss behavior, RF timing/detection/RSSI, passive ESP32 firmware commands and BLE parsing, and approved deployment/launch. The current GUI/storage-only manifest disables UART acquisition and all radio capabilities; the preserved nonminimal configuration supports explicit device-info requests while scanning stays gated. It does not change GPIO power or acquire NRF/CC by default.

See [adapter contract](adapters/README.md), [manual compilation](docs/MANUAL_BUILD.md), [verification matrix](docs/VERIFICATION.md), [persistence format](docs/PERSISTENCE.md), [provenance](docs/PROVENANCE.md) and [milestones](docs/MILESTONES.md).

## GitHub milestone workflow

[GitHub](https://github.com/1B-don/NetSentinel-Ultra) is the primary development repository. [AGENTS.md](AGENTS.md) makes tested, privacy-reviewed milestone commits and normal verified pushes persistent. Raw logs, captures, binaries and private configuration stay local. Published history is preserved; earlier local log-bearing checkpoints are not pushed. [SDK requirements](docs/SDK_INTERFACES.md) records the exact current symbol inventory and unresolved hardware bindings.

The initial manifest defines `NS_GUI_STORAGE_ONLY`: GUI/navigation/settings/history/storage are compiled, while all radio capabilities and serial HAL imports are disabled. The complete radio/UART source and receive-only guards remain available for later verified integration. Both preserved hardware source variants and the minimal configuration pass host tests.

uFBT 0.2.6 is installed at `~/flipper-build-env/bin/ufbt`; ARM GCC 12.3.1 and the official 1.4.3 SDK reside in the existing SDK-only cache. A clean real build compiled all 12 translation units, linked, generated the FAP and passed APPCHK with 49 final imports supported by official API 87.1. This does not certify the Momentum target. `make short-enums` retains coverage for the ARM compiler failure fixed earlier.

For first-build steps see [FIRST_BUILD.md](docs/FIRST_BUILD.md). Remaining target/radio gaps are in [BUILD_BLOCKERS.md](docs/BUILD_BLOCKERS.md) and [SDK_INTERFACES.md](docs/SDK_INTERFACES.md). Binaries, editor configuration, full compiler output and private build evidence remain local and are not published as release packages.

Latest enum-validation rebuild passes with `-Werror` unchanged. Result-name indexing is bounded by its table and checked for enum completeness; serialized history radio/result words are validated before enum conversion. CRC-valid malformed-record tests verify rejection, older-slot recovery and no invalid-record writes under normal and short-enum compilers.

ESP32 Diagnostics v0.2 is a separate, information-only experimental build with validated identity/SD details, query timing and an idle-only retry action. The preceding ESP Info hardware checklist has 7/8 owner-reported passes; active-request Back cancellation is pending. See [hardware checklist](docs/USART_HARDWARE_CHECKLIST.md) for artifact evidence and remaining tests. This build has not been deployed.

Current v1.0 work follows the [readiness audit and sequential plan](docs/V1_READINESS.md). Stable artifacts are preserved; no intermediate deployment is authorized.

The [development user guide](docs/USER_GUIDE.md) covers observation/history filters, private CSV exports and radio readiness. Build the separate unified development app with `python3 scripts/build_esp_info.py --development`, then validate it with `python3 scripts/validate_fap.py dist/netsentinel_development.fap`. Stable artifacts remain untouched.

The development suite now includes deterministic parser/UART/RX/navigation stress and three unified diagnostic views. Full radio readiness remains gated by external wiring/plugin evidence and installed ESP32 passive-lifecycle verification; see the current readiness checklist.
