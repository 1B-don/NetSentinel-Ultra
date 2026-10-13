# Verification status — no release

| Area | Host evidence | Outstanding device/SDK evidence |
|---|---|---|
| Wi-Fi | 15 parser cases; identity update; bounded line processing; unknown/overflow rejection | Installed ESP32 identity; passive scan commands; observed AP fixtures; UART routing |
| BLE | Unsupported-by-default gate; raw line handling in mock profile | Passive capability verification; structured parser; real BLE observations |
| NRF24 | All 126 channels; ACK/retry disabled before CE; PRX-only; sample percentages; 17 transfer fault cases | Verified GPIO routing and chip; RPD timing/RF; activity plot source passes mock frame/GUI tests; actual GUI/RF pending |
| CC1101 | Band validation; mocked tuning/RSSI extrema; absence handling; cleanup | External SDK adapter; calibration and RX state; RSSI measurements; exploration UI |
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

All five local suites pass. Added 420 torn-write boundaries, CRC rejection/recovery, full 16-entry history serialization, saved-settings/ID restoration, mandatory log setup, history retry/start blocking, explicit selector cancellation/start, disabled ESP/capabilities, readback mismatch, settings failure rollback, SD reload, shutdown/CSV-close error history, and Flipper shell/storage shims including four SDK allocation failure points. AddressSanitizer and UBSan pass with sandbox leak detection disabled. These are host outcomes, not completed physical scenarios.
