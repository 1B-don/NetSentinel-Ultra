# NetSentinel Ultra — independent v1.0 development

Owner-authorized independent development for Flipper Zero with FlipMods Ultra V3. No v1.0 release or newly compiled FAP exists. No physical device or radio was tested. SDK integration remains incomplete.

## Build and tests

```sh
cd ~/Projects/NetSentinel-Ultra
make test
make sanitize
```

In a sandbox where LeakSanitizer cannot run under tracing, use `ASAN_OPTIONS=detect_leaks=0 make sanitize`. This still runs address and undefined-behavior checks, but does not check leaks. GitHub host CI is configured; no remote CI run is claimed.

## Current implementation

- Portable AP parsing, identity updates, channel counts, baseline comparison and CSV escaping.
- Independent NRF24 SPI RX driver: disables auto-ack/retries; samples channels 0–125; records RPD hits and sample counts. Activity percentages and graph heights are statistical samples, not calibrated RSSI or Wi-Fi identification.
- CC1101 external-radio adapter contract with frequency validation, receive tuning, RSSI extrema and cleanup. Concrete CC1101 SDK operations remain unimplemented.
- Firmware-profile-gated ESP32 UART receive sessions; unsupported by default. BLE is raw text, with no device parser or verified command profile yet.
- Exclusive radio session lifecycle, bounded UART line parser, timeout recovery, CSV session logging through storage callbacks, 16-entry RAM history, validated settings serialization and graph calculations.
- Host mocks for SPI, UART, storage and CC1101 measurements.

The portable modules live in `src/core` and `include/netsentinel`. They operate only through `NsPlatform`. The imported v0.2 source is retained in `legacy` for provenance; it is excluded from application builds because it sends unverified scanning commands. Original AP parser and analysis modules remain at the project root.

## Work remaining before v1.0

An authorized SDK adapter and GUI entry point; verified Ultra SPI/CE/CS routing; persistent settings and history connected to storage; installed ESP32 firmware identity and genuinely passive Wi-Fi/BLE command profiles; firmware-specific BLE parsing; RF timing/detection/cleanup validation; a genuine compatible FAP build; approved device deployment and hardware regression. Settings codec and logging callbacks alone do not constitute SD-card hardware verification. RAM history is not persistent session history.

See [adapter contract](adapters/README.md), [manual build](docs/MANUAL_BUILD.md), [verification matrix](docs/VERIFICATION.md), [provenance](docs/PROVENANCE.md) and [development log](logs/development.md).
