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

Final JSON is in ignored logs/v1-final-package.json: filename, actual size/SHA-256, manifest name/version/icon/flags, API/target/stack, every actual import and physical_test=false. Validator checks exact 86-byte permitted Momentum manifest layout, magic/version, ARM ELF32 little-endian relocatable EABI5, API 87.1, target 7, stack 8192, development app version/name, the exact bundled 10x10 icon (or legacy no-icon metadata), zero flags, and Thumb entry matching defined netsentinel_app. Every required import must be enabled in the checksummed supplied table; weak imports are rejected. Unexpected SPI/Sub-GHz/power/USB/direct GPIO/registry or unbounded UART-wait imports fail. This supplements real SDK compile/link/APPCHK; it does not bypass loader checks or edit metadata.

Build evidence: logs/v1-final-host.log, v1-final-short.log, v1-final-sanitize.log, v1-final-build.log, live-info-build.log, v1-final-preflight.log and v1-final-publication.log. All are local/ignored. Build logs may be replaced by later verification; preserve a checksum/evidence snapshot before proposing a different artifact. Fresh packaging can produce a different checksum, so always validate the exact final file; no byte-for-byte reproducibility claim is made.

## Current packaged development result

| Field | Verified result |
|---|---|
| Source milestone | Application icon integration milestone (based on a65d0da source) |
| Artifact | dist/netsentinel_development.fap |
| Size | 43,800 bytes |
| SHA-256 | 4a070c082700ad7ec2188a360d98ce4ba8fb928a0d76ef0c20079b643c8cf82d |
| API / target / stack | 87.1 / 7 / 8192 |
| Required imports / weak imports | 63 / 0 |
| SDK compile/link/APPCHK | Passed; no compiler/linker errors |
| Host / short enums / sanitizer | All 12 C suites passed |
| Python checks | 8 preflight, 6 manifest, 5 publication tests passed |
| Hardware operations | USART info only; other physical features disabled |
| Physical validation | Not performed on this development artifact |

The pre-icon stable GUI/storage, ESP Info and ESP diagnostics binaries remain preserved with their original checksums under builds/preserved-v1-start and builds/preserved-before-icon; current dist variants now include the icon. The preceding unified development artifact and stable FAPs are also preserved under builds/preserved-0564846. No device data was accessed or rewritten. All seeded fuzz, counter/capacity, RXOFF and no-SD/stop-framing tests pass. Local sanitizer leak detection is disabled; ASan/UBSan ran. Public dependency findings are in DEPENDENCY_REVIEW.md. Packaging hashes can vary; this evidence applies to the exact file above, not another rebuild. No deployment/Release is authorized.

## Bundled application icon

All manifests use `fap_icon="assets/netsentinel_icon.png"`. The owner-supplied PNG is 10x10, one-bit grayscale, black/white only, with no ancillary metadata. The SDK encodes it into the FAP metadata; no runtime asset path is needed. The source artwork is retained for attribution/editing. Publication permits only the two reviewed artwork checksums; modified or unrelated binary assets fail the audit.

Build each variant with `python3 scripts/build_esp_info.py --variant gui|info|diagnostics|development` (choose one value). The default remains diagnostics and `--development` remains supported. GUI uses NS_GUI_STORAGE_ONLY; the other variants retain NS_ESP_INFO_ONLY. All staged builds copy the bundled icon. Prior overwritten artifacts are preserved by checksum under ignored builds/preserved-artifacts; the complete pre-icon snapshot is builds/preserved-before-icon. No settings or device files are touched.

The current icon development build is not physically tested or authorized for deployment. The prior artifact evidence remains in local snapshots and historical milestones.
