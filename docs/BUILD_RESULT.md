# Genuine FAP build results

**Owner physical update:** one requested USART information transaction displayed Connected on Momentum d3f89dfe/API87.1/target7. This confirms that transaction only; independent response-field verification, cleanup/restoration, repeatability and cancellation remain pending. Previous test artifact is preserved. The rebuilt cleanup/privacy/error-reporting artifact is not deployed. [Structured verification checklist and current artifact](USART_HARDWARE_CHECKLIST.md). Earlier all-pending statements below are historical.

**Current milestone:** owner confirms the Ultra V3 works with Momentum's existing Marauder app on Default USART13/14,115200/8N1. Permitted SDK direction mapping verifies TX13/PB6 and RX14/PB7; runtime guard checks it before init. A separate NS_ESP_INFO_ONLY FAP compiles/API87.1/target7/all62 imports validated; only one info query is permitted. Default GUI/storage FAP and first-device baseline are unchanged. No new deployment/execution; exact-artifact approval still required. [USART test-build details](ESP32_LIVE_INFO.md). Earlier wiring-only preparation gates below are historical; circuit-level details/physical NetSentinel timing remain unverified.

**Latest status milestone:** owner USB-C/CH340 info exchange at115200 confirms Marauder v1.11.0/v4, corrected IDF g8410210c9a, connected SD and unsupported battery monitor. Strict parser, sanitized fixture and UART-log redaction pass all regressions; clean Momentum API87.1/target7 build and all49 imports pass. Current FAP34888 bytes, SHA25627a73a79f5e3959d3d367a2cd2f12030f3fe048e6277ce8689f51893e2d38bf6, physically untested. All hardware adapters disabled and first-device artifact preserved. [Full evidence/remaining GPIO requirements](ESP32_PREPARATION.md).

**Current update:** USB CLI independently confirms installed API **87.1**, target 7, d3f89dfe; earlier 89.1/mismatch blockers below are superseded. Matching permitted Momentum SDK clean build, metadata and all 49 imports pass. Current FAP is 33,480 bytes, SHA256 `de38c1c7e08336a1ab5a3ebe6dcf2865ac8aa5ce9dedb97c8f8c17c024b859b5`. See [Momentum build result](MOMENTUM_BUILD.md). All radios remain disabled; physical installation/execution are pending explicit approval. For the device procedure use this artifact/hash; former fingerprint and metadata requests are historical.

## Latest enum-validation rebuild

The requested type-limit comparisons were already corrected in local commit `1249df0`. This follow-up bounds result-name indexing by the actual table with a completeness assertion, retains unsigned input/result range checks, and validates full 32-bit stored radio/result words before narrowing to enums. Normal/short-enum/sanitizer tests reject negative/out-of-range inputs and CRC-valid malformed history, preserve older valid snapshot recovery, and prohibit invalid persistence writes. No warning suppression or build-flag change was made.

`~/flipper-build-env/bin/ufbt` returned exit 0: changed sources compile, link, packaging and APPCHK pass, hardware target 7 / API 87.1. No compiler/linker errors remain. SDK remains official 1.4.3; installed Momentum compatibility and physical operation remain unverified. Current artifact `dist/netsentinel.fap` is **33,480 bytes**, SHA256 `06cd1717590bd6658d2914a128432b8fcb9d0110ca463e3c77222ac4e5996b27`. Full output is local in `logs/enum-validation-build.log`. The earlier clean-build evidence below remains historical.

## Initial successful clean build

Application source commit: `47ce6b9e1830768f85a68ab9543dc240529bec58`. Documentation updates in the result milestone do not change compiled application source.

## Result

| Stage | Observed result |
|---|---|
| Manifest/configuration | EXTERNAL app; netsentinel_app; 8 KiB stack; NS_GUI_STORAGE_ONLY; 12 C sources; no tests/legacy |
| Compiler | ARM GCC 12.3.1; all 12 translation units compiled successfully in the clean build |
| SDK/build command | Independently permitted official SDK 1.4.3; uFBT 0.2.6 |
| Link and app metadata | PASS; debug ELF and packaged FAP generated |
| FAP packaging | PASS; ARM ELF32 relocatable, EABI5 |
| SDK APPCHK | PASS; hardware target 7, API 87.1 |
| Final import table | 49 required imports, all enabled in official API table; no serial HAL imports |
| Momentum d3f89dfe compatibility | UNVERIFIED |
| Physical launch/GUI/SD tests | NOT RUN |
| Hardware adapters | Disabled; no hardware operation performed |

Artifact: `dist/netsentinel.fap`, **33,476 bytes**.

SHA256:

```text
0d8dcabeeb0fbf0d57108e6bfeba48df036b69e5839cc2445135818c6dea6fbb
```

Debug ELF: `dist/debug/netsentinel_d.elf`. Logs: `logs/official-sdk-first-build.log`, `logs/official-sdk-app-clean.log`, `logs/official-sdk-full-rebuild.log`. Local structured evidence: `logs/official-sdk-build-evidence.json`; final import listing: `logs/official-sdk-final-imports.txt`. All remain ignored/local; no release package or GitHub Release was created.

## Commands and errors

Run from NetSentinel-Ultra:

```sh
~/flipper-build-env/bin/ufbt
# App build outputs only: clean and recompile to verify all translation units.
~/flipper-build-env/bin/ufbt -c
~/flipper-build-env/bin/ufbt
```

Initial build and clean rebuild returned exit 0. No source compiler, linker or missing-import errors occurred in this milestone; earlier ARM short-enum fixes were already present. An intervening attempt to force rebuilding with `-B` returned exit 2 and `SCons Error: no such option: -B`. This was an unsupported build option, not a source failure. It was replaced by the successful app clean/build sequence above; the SDK and firmware were not cleaned or flashed.

The SDK-generated final FAP import list passes the official export table. The debug ELF also exposes `uxTopUsedPriority`, which is absent from the final SDK import list; do not mistake the debug-only listing for the packaged app's runtime requirements. Prefer the SDK's final import/APPCHK evidence when evaluating load compatibility.

## Remaining compatibility and physical requirements

Obtain permitted installed-Momentum target/API/export metadata or its exact revision SDK-only archive, with provenance and usage instructions. Compare hardware target, API major/minor compatibility, the 49 final function/data imports, declaration/ABI and loader/relocation support. Official SDK APPCHK proves official export resolution, not installed-Momentum compatibility. Do not remove UART/radio guards to try loading an unverified driver.

After that review, obtain explicit approval for deployment/launch. Transfer the FAP through the supported USB file workflow into `apps/GPIO`, then verify launch/exit, navigation, settings save/relaunch, history/state recovery and actual SD behavior. Record device/app/SDK revisions, artifact hash, expected/observed results and logs. No firmware replacement, storage erase or USB permission changes are needed. Restore adapters only after their independent interface/protocol/wiring reviews and receive-only physical tests. Full v1.0 and any Release remain pending.

## Regression checks

Nine host C suites, eight SDK-preflight cases and four publication cases pass. Host short-enum variants and ASan/UBSan pass, with leak detection disabled in the local traced environment. Staged publication and whitespace audits pass; runtime/compiler logs and FAP binaries are excluded from publication.

## First device-test preparation milestone

Direct package inspection and real official-SDK rebuild pass. Current FAP: 33,484 bytes, SHA256 `af74bfb1cdcc8fc3bf319d97b8fe056b563474cc39da223a1f87cd005ee37d30`, target 7 / API 87.1; all 49 packaged imports resolve officially. All 12 compiler commands retain GUI/storage-only mode. Nine host, short-enum and sanitizer suites plus eight preflight/four publication cases pass. No source changes, device operations or radio operations. Earlier binary fingerprints remain historical. See [first device-test procedure](FIRST_DEVICE_TEST.md) for exact compatibility evidence, missing permitted metadata and reversible, approval-gated GUI/storage checks. Momentum compatibility and physical verification remain pending. Remote master was verified at `42bdaec` before this milestone; earlier pending milestones are published.

## Downloaded Momentum SDK validation milestone

Downloaded ZIP SHA256 matches owner value `238db260ee7e8f6e9a79dd1c760e07a832351a7d92d896b4cb19082f8548b2c1`; all 626 members pass CRC and path/symlink checks. Component version is `mntm-dev-d3f89dfe` / f7; export table explicitly reports **87.1**, inconsistent with owner-confirmed installed API **89.1**. SDK switch/target rebuild stopped; no API 89.1 FAP generated. Official artifact/evidence preserved separately and existing cache untouched. [Exact inspected evidence and minimum resolution input](MOMENTUM_SDK.md). No radio changes, deployment, firmware operations or restricted-checkout access. Host, short-enum, sanitizer and publication regression suites remain passing.
# Hardware Integration Milestone 1 — 2026-10-13

Added per-capability diagnostics for ESP info, Wi-Fi, BLE, NRF24 and CC1101 with distinct interface/routing/receiver/simulation/physical-enable states and explicit blockers. The root GUI/storage baseline and the generated ESP-info-only development FAP both reject NRF24/CC1101 opens; the host radio-port contract now requires a test-only marker or explicit verified Ultra route evidence, with a separate provider evidence gate for CC1101. The existing USART info implementation and all earlier FAPs/settings remain preserved. No new physical test or deployment occurred.

Read-only review of the permitted Momentum API 87.1 / target 7 SDK confirms generic Sub-GHz registry lifecycle, `set_rx`, RSSI, idle/sleep and frequency functions are enabled exports. The declared `subghz_device_cc1101_ext_ep` is `Function,-`; its provider implementation is absent from the SDK archive. Public Sacred Labs docs describe the module inventory and NRF/CC selector but publish no Ultra V3 SPI/CE/CSN/IRQ/GDO routing. Generic standalone-driver mappings were explicitly rejected as board evidence. Wi-Fi remains blocked because the reviewed Marauder v1.11.0 start path may transmit AP beacons and PCAP defaults can persist raw ESP-side captures. Full evidence and the smallest next physical tests are in [HARDWARE_INTEGRATION_M1.md](HARDWARE_INTEGRATION_M1.md).

Validation: 14 C suites, 24 Python checks, short-enum suite, ASan/UBSan suite, Momentum uFBT compile/package/APPCHK, strict API/target/icon/import validation, SDK preflight and publication audit all pass. New undeployed artifact: `dist/netsentinel_development.fap`, 49,432 bytes, SHA-256 `e6f76caca37a00a6941fe9ae34208974921a8b7eefd56ba02654ce843e313938`, API 87.1, target 7, 63 resolved imports, `physical_test=false`. Import table contains no SPI or Sub-GHz registry operations. The overwritten development FAP is preserved under `builds/preserved-artifacts/08b3818328e61b61b1e6652818852ebee15625122d2ee016e9f16ea48b8a5d72/` with that SHA-256; other stable FAP checksums are unchanged. Not compatible evidence for any untested later firmware/API revision; no application deployment or radio initialization was attempted.

## NRF24 hardware integration candidate — 2026-10-13

The newest undeployed `dist/netsentinel_development.fap` is **49,444 bytes**, SHA-256 `72d50a813a54d2a111f796fb54609bf7a0636da47e868c5158dccc65557ddaca`, API 87.1 / target 7, icon present, 63 imports / zero weak. Momentum SDK compile/link/package/APPCHK succeeded; the strict FAP validator and export-name preflight passed. No SPI or Sub-GHz import is present because the physical adapters remain disabled. Fourteen host C suites, 24 Python checks, short-enum, ASan/UBSan and publication audits passed. The prior development FAP remains preserved locally byte-for-byte at 49,432 bytes, SHA-256 `e6f76caca37a00a6941fe9ae34208974921a8b7eefd56ba02654ce843e313938`.

See [NRF24_HARDWARE.md](NRF24_HARDWARE.md) for the public Sniffer audit, generic SDK pin evidence, the unbounded SDK acquire limitation, and exact physical dependencies. No NRF24 initialization, RF activity or deployment was performed.
