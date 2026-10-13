# Verification status — no release

**Current update:** USB CLI independently confirms installed API **87.1**, target 7, d3f89dfe; earlier 89.1/mismatch blockers below are superseded. Matching permitted Momentum SDK clean build, metadata and all 49 imports pass. Current FAP is 33,480 bytes, SHA256 `de38c1c7e08336a1ab5a3ebe6dcf2865ac8aa5ce9dedb97c8f8c17c024b859b5`. See [Momentum build result](MOMENTUM_BUILD.md). All radios remain disabled; physical installation/execution are pending explicit approval. For the device procedure use this artifact/hash; former fingerprint and metadata requests are historical.

| Area | Host evidence | Outstanding device/SDK evidence |
|---|---|---|
| Wi-Fi | 18 parser cases; gated protocol; UART ring loss; fragmented echoes/prompts; AP/RSSI/CSV/UI/cancellation integration | Owner identity/standalone Recon confirmed; installed raw sequence/labels, channel policy, receive-only implementation and NetSentinel UART/SDK still pending |
| BLE | Unsupported-by-default gate; raw line handling in mock profile | Passive capability verification; structured parser; real BLE observations |
| NRF24 | All 126 channels; ACK/retry disabled before CE; PRX-only; sample percentages; 17 transfer fault cases | Verified GPIO routing and chip; RPD timing/RF; activity plot source passes mock frame/GUI tests; actual GUI/RF pending |
| CC1101 | SIMULATED read-only SPI/identity/state diagnostics; RX-only state checks; rolling graph/mean/threshold CSV; preset persistence/live retune; faults/cadence/wrap/cleanup | User reports prior Ultra operation; NetSentinel external SDK binding, device SPI/RSSI, calibration and formal RF reception pending |
| Storage | CSV quoting; partial/error writes; sync error; path confinement | Source SD adapter passes shim tests; real file creation/export and card/removal behavior pending |
| Sessions | Radio exclusion; timeout across tick wrap; retry; bounded RAM history; no double release | Controller/UI allocation unwind and shutdown error propagation pass mocks; real device resource restoration pending |
| Settings | Strict versioned format; invalid/corrupt/range rejection; transactional decode | Dual-slot settings/history restart tests pass on mocks; actual SD relaunch pending |
| Build | GCC C11 with warnings as errors; AddressSanitizer + UBSan | Candidate manifest/shell pass host shim compile; real authorized SDK, ABI, FAP and launch pending |

Host tests operate exclusively on mocks. Neither mock SPI readback nor mocked RSSI establishes physical radio functionality. NRF probing is a conservative register readback test, not unique chip identification. Cleanup attempts may encounter bus failures; the eventual adapter must guarantee pins are made safe and report unrecoverable failures. No automatic power cycling or hardware reset is implemented.

## Required hardware scenarios

Record expected/observed values, device and application revisions, selector position, logs and pass/fail for every case:

1. App launch/exit repeatedly; no heap growth, stuck CE, UART/SPI ownership, power or expansion changes after exit.
2. Missing Ultra, incorrect selector, busy bus, UART silence and unplugged/broken connection: bounded error/recovery, no crash or internal CC fallback.
3. Verified passive ESP Wi-Fi/BLE operation and parsed results against recorded firmware output; no TX behavior in command implementation.
4. NRF activity on quiet and occupied channels; independently compare fixture activity, count bounds and plot/export agreement. Confirm automatic acknowledgments stay disabled.
5. External CC receive at permitted board frequencies; settling/calibration and RSSI agreement against reference; invalid-band rejection; exit returns external device to idle.
6. SD CSV contents for each radio, immutable earlier sessions, full/missing SD recovery, settings persistence and history across app restarts.
7. Global radio TX exclusion review and appropriate receive-only hardware verification; screenshots and logs for dashboard/plots/errors.

All physical scenarios are pending. Deployment/launch requires explicit owner approval. No intermediate release is produced.

## Application integration additions

All seven local suites pass. Added 420 torn-write boundaries, CRC rejection/recovery, full 16-entry history serialization, saved-settings/ID restoration, mandatory log setup, history retry/start blocking, explicit selector cancellation/start, disabled ESP/capabilities, readback mismatch, settings failure rollback, SD reload, shutdown/CSV-close error history, and Flipper shell/storage shims including four SDK allocation failure points. AddressSanitizer and UBSan pass with sandbox leak detection disabled. These are host outcomes, not completed physical scenarios.

## Marauder additions

See [command evidence and gates](MARAUDER.md). New mock tests cover busy serial ownership, ring wrap/overflow, serial framing failure, command allowlisting, provisional info/UI/history flow, source-format synthetic beacons, AP RSSI ranges, baseline/details navigation, stop acknowledgment and deferred exit, tick wrap, incomplete responses, control-line overflow and recovery. Owner standalone tests are recorded separately from NetSentinel hardware results. Security detail querying, verified BLE receive-only discovery and real SDK build remain unresolved.

## CC1101 evidence split

Prior Ultra V3 operation and Momentum automatic external selection are user-reported. No controlled signal is available; formal RF tests remain pending. Software proceeds using simulated SPI/RSSI faults and unified application integration tests. [CC1101 plan](CC1101.md) includes diagnostics/noise monitoring tests that do not require a controlled source once an authorized SDK port/build and deployment approval exist. The lack of a source does not block continued source development.

## Flipper API boundary milestone

Original seven suites remain passing; an eighth board-enabled Flipper contract suite covers isolated binding, incomplete/failed bind unwind, shared UART/NRF/CC ownership, wrong-owner calls, real GUI input callback cancellation, NRF transfer failure and forced CC cleanup during mutex failure. The original GUI suite now exercises actual callback/queue filtering, long Back, overflow cancellation, GUI/storage record failures and mutex failure. Both source variants compile under C11 warnings-as-errors and pass ASan/UBSan locally. Publication policy has four regression cases; privacy checks operate on the complete staged tree and never echo suspected data. No compatible firmware SDK build or physical verification was performed.

## Initial GUI/storage-only build configuration

A ninth host C suite compiles the actual Flipper/core sources with `NS_GUI_STORAGE_ONLY`, checks disabled radio acquisition/UART reads and writes, executes navigation/settings/exit and verifies the existing persistent-state save/load against SDK mocks. All previous suites remain unchanged. A native minimal UART object has no undefined serial HAL imports. These checks isolate the intended SDK build configuration; they do not prove an ARM FAP, SDK ABI or physical SD operation. Actual compatible SDK input remains unavailable.

## Official SDK build evidence

A real clean uFBT 0.2.6 build using permitted official SDK 1.4.3 compiled all 12 source files, linked, packaged `dist/netsentinel.fap` and passed APPCHK (target 7, API 87.1). All 49 final imports are enabled in the official export table, with no serial HAL imports. This is actual build evidence; physical GUI/SD/resource checks and Momentum compatibility remain unverified. BUILD_RESULT.md records artifact size/hash, source commit and ignored logs. No deployment, firmware operation or radio activity occurred.

## First device-test preparation milestone

Direct package inspection and real official-SDK rebuild pass. Current FAP: 33,484 bytes, SHA256 `af74bfb1cdcc8fc3bf319d97b8fe056b563474cc39da223a1f87cd005ee37d30`, target 7 / API 87.1; all 49 packaged imports resolve officially. All 12 compiler commands retain GUI/storage-only mode. Nine host, short-enum and sanitizer suites plus eight preflight/four publication cases pass. No source changes, device operations or radio operations. Earlier binary fingerprints remain historical. See [first device-test procedure](FIRST_DEVICE_TEST.md) for exact compatibility evidence, missing permitted metadata and reversible, approval-gated GUI/storage checks. Momentum compatibility and physical verification remain pending. Remote master was verified at `42bdaec` before this milestone; earlier pending milestones are published.
