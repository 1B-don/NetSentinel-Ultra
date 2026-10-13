# First device test preparation — approval pending

## Package evidence

The existing package was inspected directly, then rebuilt with permitted official SDK 1.4.3 and inspected again. Both inspections passed. The current `dist/netsentinel.fap` is 33,484 bytes, SHA256 `af74bfb1cdcc8fc3bf319d97b8fe056b563474cc39da223a1f87cd005ee37d30`. This supersedes earlier artifact fingerprints; identify the exact transferred binary by hash, not filename alone. No application source changed in this milestone.

Direct checks: ELF32 little-endian ARM relocatable/EABI5; `.fapmeta` magic 0x52474448, format 1, target 7, API 87.1, stack 8192 bytes. All 49 required imports in the packaged FAP resolve to enabled official SDK functions/data. No weak imports or serial/SPI/Sub-GHz/GPIO HAL imports were found. All 12 SDK compiler commands enable `NS_GUI_STORAGE_ONLY` and omit `NS_ENABLE_BOARD_PORT`; source capability initialization and UART stubs keep ESP32, NRF24 and CC1101 disabled. SDK compilation/link/packaging/APPCHK returned exit 0. Local evidence: `logs/predevice-package-evidence.json`, `logs/predevice-final-imports.txt`, `logs/predevice-build.log`. These files remain private and ignored.

## Installed Momentum compatibility gate

Status: **unverified; deployment is not recommended yet**. Owner reports installed revision `d3f89dfe2ef6b01839201598e9be1590cba80322`. The permitted [Momentum releases page](https://momentum-fw.dev/releases/) lists abbreviated revision d3f89dfe, but supplies no inspected API/export/ABI evidence. The public update index could not be retrieved in this environment. The restricted checkout was not accessed. No device connection, installation or execution was attempted.

Owner has now provided read-only device information: Momentum Dev, August 18, 2026, exact revision above, firmware API **89.1**, target **7**. The FAP API 87.1 differs in major version. Do not proceed to installation/launch. The next input is the public SDK ZIP described in [the exact SDK investigation](MOMENTUM_SDK.md); its contents must be verified before rebuilding. Earlier requests for the device API are satisfied.

For stronger predeployment evidence, supply an independently permitted SDK-only archive or API/export metadata attributable to that exact installed revision, with provenance and applicable instructions. Compare target 7, API compatibility, the 49 packaged imports including data symbols, declaration/layout/calling ABI and fast-relocation loader support. A matching API number alone is insufficient evidence that custom firmware retained every required export. Loader acceptance and successful GUI/storage execution remain physical tests even after metadata matches.

Remaining risks: loader/API rejection, missing symbols or ABI differences, allocation/input/lifecycle faults, SD errors and unintended persistence changes. Radios remain disabled, but mocks and package checks cannot prove physical behavior. Do not override loader warnings to proceed.

## Reversible test procedure — perform only after explicit approval

1. Record installed revision/API, application source commit, exact FAP hash, available SD space and expected results locally. Keep raw logs/screenshots/device identifiers out of Git. Verify no test requires firmware updates, permission changes or radio-selector changes.
2. Back up any existing `apps/GPIO/netsentinel.fap` and the entire `apps_data/netsentinel` directory through the supported USB file workflow. If either already exists, verify the backup before proceeding and plan restoration separately. Prefer an absent application-data directory for the first test; do not erase or replace existing data to obtain one. Settings tests overwrite alternating state slots inside that directory.
3. After installation approval, transfer only the verified FAP to `apps/GPIO/netsentinel.fap` through the supported file workflow, checking copied bytes/hash where available. Do not use firmware update/flash commands. Installation and launch each need approval unless explicitly authorized together.
4. After launch approval, open the app. Confirm the dashboard renders and Up/Down/OK/Back navigate Settings, History and Diagnostics. Back returns to dashboard and exits from dashboard. Check short/long Back and repeat navigation over three launch/exit cycles. Record crashes, freezing or unexpected status; stop on any failure.
5. Open Wi-Fi/BLE/NRF24/CC1101 pages only to confirm disabled/unsupported status. Do not press scan/start or confirm a selector. No RF observations or CSV radio exports are expected in this build.
6. For storage, record the current timeout in Settings; Right increases it by 1000 ms (up to its bound), then exit and relaunch to verify persistence. Left restores the original timeout; exit/relaunch and confirm it. Verify only `apps_data/netsentinel/state_0.bin` and `state_1.bin` changed as expected. No radio session CSV should be created. Avoid full-card, card-removal, corruption or destructive tests in this first pass.
7. Record actual results separately for loader, GUI/navigation, exit/relaunch and settings read/write. Restoring a setting value does not restore state-file bytes/generation. Rollback means exiting and, with specific owner authorization, restoring backed-up app/data or removing only newly created test files. No deletion, rename or restore is authorized by this document. Never erase storage or touch unrelated data.

Passing this procedure establishes only the GUI/storage build on that specific firmware and artifact. It does not verify any radio, provide a v1.0 release, or authorize enabling adapters.

## Automated verification

Nine host C suites, eight SDK-preflight cases and four publication cases pass; all nine short-enum and ASan/UBSan suites pass (local leak detection disabled). Real official-SDK build and APPCHK pass. Publication/whitespace audits are required before this milestone is committed. Physical tests remain not run.
