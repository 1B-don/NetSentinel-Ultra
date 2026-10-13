# Reproducible Momentum development build

Target: supplied Momentum Dev d3f89dfe SDK ZIP, API 87.1, hardware target 7. uFBT 0.2.6 and ARM GCC 12.3.1/toolchain 39 are used through the isolated cache. No restricted firmware repository is read. This packages development source with pending physical radio adapters disabled, not a full hardware-verified v1.0 release.

## Inputs and setup

Keep the supplied ZIP at builds/sdk-input/flipper-z-f7-sdk-mntm-dev-d3f89dfe.zip, SHA-256 238db260ee7e8f6e9a79dd1c760e07a832351a7d92d896b4cb19082f8548b2c1. Its API table is copied unmodified to builds/sdk-input/api_symbols.csv, SHA-256 a2bdb9893ba57ac6ccc4eae2e66ab201fa84f48508347f4c52685cfb3c3a8d4f. SDK installation was already completed and validated. For a new authorized environment use the supported local update described in MOMENTUM_BUILD.md with isolated UFBT_HOME ~/.ufbt-netsentinel-momentum, FBT_TOOLCHAIN_PATH ~/.ufbt and --hw-target f7. No internet/firmware update is part of application verification.

The build helper validates the ZIP checksum, installed SDK component identity, hardware target, API version and exact installed SDK build metadata. Missing/mismatched inputs stop the build. Use only a reviewed SDK distribution; do not copy headers or exports from a restricted checkout. Tests/flipper_mock is never an ARM include path.

## Single-command verification

From the application directory:

```sh
scripts/verify_development.sh
```

This runs the twelve host C suites and Python checks, all short-enum suites and ASan/UBSan, clean uFBT build, actual-package validation, strict actual-import preflight and publication audit. Tests run sequentially because Python preflight fixtures share temporary project-local inputs. Local leak detection defaults off for the current environment; ASan/UBSan remain enabled. Override ASAN_OPTIONS to enable leak detection where supported. CI retains its regular sanitizer configuration.

Only builds/, logs/ and the separately named dist/netsentinel_development.fap are created/updated. The helper preserves checksums for existing GUI/storage, ESP Info and ESP diagnostics FAPs. No device connection, deployment, launch, peripheral energizing, SDK update, firmware flashing or network command is performed. Publication audit reads the Git index; stage intended source/docs before the final publication audit.

To build/validate only:

```sh
python3 scripts/build_esp_info.py --development
python3 scripts/validate_fap.py dist/netsentinel_development.fap
```

Generated staged application.fam uses appid netsentinel_development, title NetSentinel Ultra Dev, entry netsentinel_app, stack 8192 and NS_ESP_INFO_ONLY. Root application.fam remains NS_GUI_STORAGE_ONLY. All core modules compile; firmware-facing USART is info-only, NRF/CC ports absent and Wi-Fi/BLE commands denied. Runtime UI does not elevate blocked capabilities. Existing data and state schema are unchanged.

## Package evidence

Final JSON is in ignored logs/v1-final-package.json: filename, actual size/SHA-256, manifest name/version/icon/flags, API/target/stack, every actual import and physical_test=false. Validator checks exact 86-byte permitted Momentum manifest layout, magic/version, ARM ELF32 little-endian relocatable EABI5, API 87.1, target 7, stack 8192, development app version/name, no icon/flags, and Thumb entry matching defined netsentinel_app. Every required import must be enabled in the checksummed supplied table; weak imports are rejected. Unexpected SPI/Sub-GHz/power/USB/direct GPIO/registry or unbounded UART-wait imports fail. This supplements real SDK compile/link/APPCHK; it does not bypass loader checks or edit metadata.

Build evidence: logs/v1-final-host.log, v1-final-short.log, v1-final-sanitize.log, v1-final-build.log, live-info-build.log, v1-final-preflight.log and v1-final-publication.log. All are local/ignored. Build logs may be replaced by later verification; preserve a checksum/evidence snapshot before proposing a different artifact. Fresh packaging can produce a different checksum, so always validate the exact final file; no byte-for-byte reproducibility claim is made.

## Current packaged development result

| Field | Verified result |
|---|---|
| Artifact | dist/netsentinel_development.fap |
| Size | 43,268 bytes |
| SHA-256 | 79e92c0b7677d3fa7f32e4e0a09c540d2ce57d38b55875c5e0d15363ba97899c |
| API / target / stack | 87.1 / 7 / 8192 |
| Required imports / weak imports | 63 / 0 |
| SDK compile/link/APPCHK | Passed; no compiler/linker errors |
| Host / short enums / sanitizer | All 12 C suites passed |
| Python checks | 8 preflight, 5 manifest, 4 publication tests passed |
| Hardware operations | ESP info only; other physical adapters disabled |
| Physical validation | Not performed on this development artifact |

The preserved stable GUI, ESP Info and ESP diagnostics FAP checksums still match builds/preserved-v1-start/checksums.json. No device storage was accessed. Sanitizer leak detection was disabled locally; ASan/UBSan ran. Packaging hashes can vary on a clean rebuild; the table applies to the exact final artifact above. No release/deployment is authorized.
