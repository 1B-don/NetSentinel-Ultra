# Unified ESP diagnostics and gated Wi-Fi development

Baseline: `22dd593a716f740b9cf1abce2822a442dae5a67b`. Target: permitted Momentum d3f89dfe SDK, API 87.1, target 7. No hardware test, deployment, selector/settings change, flash or RF command was performed in this milestone.

## Owner evidence and recovery

On 2026-10-13 the owner reported a successful ESP information query, successful relaunch/requery, and original Marauder application interoperation for SHA-256 `7751b712e71b84b396a13a6ca2f33649e7d3b65031bd4dceb99d5b732ff11cf7`, 49,924 bytes. These are physical owner observations, not automated tests, exhaustive cleanup evidence or a Wi-Fi/passivity result. Exact local bytes match. The FAP remains at `dist/netsentinel_esp_status.fap` and has a checksum-recorded recovery copy under ignored `builds/preserved-owner-20261013/`. All seven existing FAPs were copied and rehashed for recovery; previous preservation directories remain intact.

The build helper permanently refuses the owner-verified Status filename and checks its expected checksum before building other variants. New candidates have different filenames. Binaries, SDK material, captures and runtime/compiler logs remain ignored and are not published. The saved NSP1 settings/history and CSV schema are unchanged; no device data was accessed.

## Pinned discovery source and correction

Rechecked public Marauder v1.11.0 commit `2ff3716e2307f8a1c3d2503936728b8cbc18acd8`. References: [CommandLine.cpp](https://github.com/justcallmekoko/ESP32Marauder/blob/2ff3716e2307f8a1c3d2503936728b8cbc18acd8/esp32_marauder/CommandLine.cpp), [command definitions](https://github.com/justcallmekoko/ESP32Marauder/blob/2ff3716e2307f8a1c3d2503936728b8cbc18acd8/esp32_marauder/CommandLine.h), [WiFiScan.cpp](https://github.com/justcallmekoko/ESP32Marauder/blob/2ff3716e2307f8a1c3d2503936728b8cbc18acd8/esp32_marauder/WiFiScan.cpp), [Buffer.cpp](https://github.com/justcallmekoko/ESP32Marauder/blob/2ff3716e2307f8a1c3d2503936728b8cbc18acd8/esp32_marauder/Buffer.cpp), [settings.cpp](https://github.com/justcallmekoko/ESP32Marauder/blob/2ff3716e2307f8a1c3d2503936728b8cbc18acd8/esp32_marauder/settings.cpp). Local public review files remain under ignored builds; no source was copied into the application.

**Correction:** earlier reports attributed hidden-AP initialization to discovery's initWiFi. In the rechecked file, initWiFi loads settings and changes channel. RunAPScan/RunBeaconScan select NULL/promiscuous mode through setWiFiMode; AP configuration occurs in a separate attack routine. Historical milestone records retain the earlier finding as history; this correction and current readiness labels supersede it. No physical gate was relaxed.

| Command / path | Verified public behavior | Classification / open gate |
|---|---|---|
| `scanap` | Starts TARGET_AP via RunAPScan; startup acknowledgment is `Starting AP scan. Stop with stopscan` | NULL/promiscuous receive path in reviewed source; installed passivity/start-stop still not independently verified |
| `sniffbeacon` | Starts WIFI_SCAN_AP via RunBeaconScan; streams RSSI/channel/BSSID/ESSID | “Sniff” is not permission; peer PCAP handling remains a privacy dependency |
| `list -a` | Lists `[index][CH:channel] SSID RSSI`, optional selected suffix, then selected count | Metadata output; no BSSID/security in this format, so neither is inferred |
| `info -a index` | Reports ESSID/BSSID/channel/RSSI and named Security plus unrelated metadata | Pure detail parser supports the reviewed result fields. No FAP sends this command; identity data stay volatile |
| `stopscan` | Dispatches OFF and emits `Stopping WiFi tran/recv` plus prompt; shutdown depends on state | A framed reply alone does not prove installed RF shutdown or restoration |

No explicit probe/management transmit or saveSetting call was identified in the reviewed discovery startup itself. That does not establish installed binary behavior or complete RF silence: initial state, settings, callbacks, background behavior and source correspondence remain unverified. Other public routines use active WiFi.scanNetworks calls and AP/TX APIs; they are outside this design. BLE active discovery remains excluded. No attack, credential, association or packet-capture command interface is added.

Both reviewed discovery routines open PCAP handling. Buffer openFile consults SavePCAP; when enabled it can create/write peer SD files or emit capture data. The public setting defaults true. NetSentinel redaction cannot protect those peer captures. No peer setting is queried or changed to work around this. Existing connection/state-dependent shutdown requires independent review before any real command is permitted. The source's RAM Wi-Fi storage selection is not a guarantee about every installed persistent-setting effect.

## Bounded parser and state model

New `wifi_discovery.c/.h` provides independently authored pure list/detail parsers and a bounded command-intent model. It exposes no UART callback, pin operation or transmission interface. Shipping builds do not define `NS_TEST_WIFI_DISCOVERY_PROTOCOL`; begin always returns Unsupported and records Unavailable. `NS_WIFI_DEVELOPMENT` enables only the explicit offline parser self-check in Network Results, not protocol start or command permissions. A test-only flag exercises the model on the host and is rejected from generated variant definitions by build tests.

The result model retains up to 48 records with session-local provider index, SSID, optional BSSID, channel, RSSI and a typed security classification. List output leaves BSSID/security unreported. Detail requires all five emitted result fields; unknown security labels fail Unsupported rather than invent a class. Missing/duplicate fields, non-printable bytes, invalid MAC/ranges and overlong/truncated SSIDs fail closed without changing parser outputs. Supported SSIDs are printable ASCII up to 32 bytes, including empty/hidden values; binary/non-ASCII SSIDs are explicitly unsupported by this candidate. No truncation is silently accepted. Duplicate list indices may update RSSI only for consistent SSID/channel; conflicting identity is rejected. Index matching is session-local, not persistent BSSID authentication.

Lines are capped at 191 characters, detail input at 1024 bytes, feed calls at 1024 bytes, and aggregate line accounting at 16,384 bytes including empty lines. CRLF and split prompts are supported; corrupt control bytes and embedded CR cannot repair a response. A result snapshot becomes valid only after a correlated list echo, valid records/count summary and final prompt. An empty snapshot requires the summary too. Raw identity remains in RAM for parsing/comparison only and never reaches GUI/history/CSV. The renderer uses local AP numbers, numeric channel/RSSI and typed security. The experimental self-check uses labeled invented input and never populates network results or history.

Model states: Unavailable, Starting, Observing, Stopping, Reading Results, Complete, Empty, Error, Timeout and Cancelled. Intents describe start, stop and list dispatch; no physical consumer exists. Dwell and reply deadlines use wrapping application milliseconds. Stream chatter is bounded/drained while the model waits to stop and retrieve its list; packet payloads are not parsed or captured. Stop intent is requested after finite dwell or cancellation. Repeated cancel does not extend the current stop deadline. Failed/timeout start-stop retains peer_may_scan and blocks restart until a framed stop response completes the **host model's stop-contract assumption**. That flag is not physical RF-silence evidence; installed stop semantics remain unverified even after an acknowledgment.

Local exit cannot stop an independently running ESP. A future physical adapter must establish start/stop dispatch, loss-of-link handling, callback/bus restoration and a verified passive peer stop policy before binding these intents. It must report cleanup unknown and prohibit automatic restart after unconfirmed stop; it cannot send a late stop after USART release, claim a local timeout stopped RF, reset the ESP, or force another UART owner. Marauder has no request nonce, so late complete replies remain unauthenticated as fresh. No software timer preempts the existing void UART HAL TX call.

## Unified interface and variant differences

Both new candidates open a four-row scrolling dashboard: ESP32 Diagnostics, Wi-Fi Discovery, Network Results, Session History, Settings, Hardware Readiness, then More tools. More tools retains the previous eleven-entry navigation including BLE/NRF/CC unavailable screens, filters, exports, storage diagnostics and offline modes. No existing tool, preset or data schema is deleted. Back from a detail/tool screen stops local info/simulation work and returns to the dashboard; Back on the dashboard exits. Original manifests retain their existing home layout unless the new UI flag is selected.

Diagnostics retain four views/six typed states. Labels distinguish an active Live info query, a queued request, Last Successful Reading, Not Tested and Unavailable; the query result is separate from general UI/storage status. Storage errors remain visible with a path to Tools/Diagnostics recovery. Timing now explicitly labels the latest attempt, separate from historical success ID. No unvalidated firmware text is displayed. Current binaries do not restore historical diagnostic values from prior device state files.

| Variant | Defines / behavior | Physical capabilities |
|---|---|---|
| Preserved esp-status | Frozen owner-verified exact file, prior four-view diagnostics | Previously owner-tested info only; no new build |
| esp-stable | NS_ESP_INFO_ONLY + NS_UNIFIED_UI; polished diagnostics/dashboard; discovery/results unavailable | New candidate, physically untested; info only |
| wifi-dev | Same plus NS_WIFI_DEVELOPMENT; offline list/detail parser self-check accessible with OK in Network Results | New candidate, physically untested; info only; every Wi-Fi/scan/detail/list/stop command denied |

The offline self-check reports pass/failure only. It does not show invented APs as hardware observations. No live Wi-Fi result collection is available in either candidate. To inspect the software protocol model, run host tests; do not define its test flag in an ARM build.

## Validation and remaining approval

All original suites are retained, plus discovery/parser and unified-UI suites. Coverage includes empty/malformed/missing/security/duplicate/truncated/capacity records, one-byte response fragments, deadlines/tick wrap, repeated cancellation/sessions, stop uncertainty, UART contention/denial, private rendering, dashboard/tools navigation and protected build output. Standard, short-enum and ASan/UBSan suites run with warnings as errors; local leak detection remains disabled. Matching Momentum API 87.1 / target 7 compilation/link/APPCHK, strict metadata/icon/entry, 63 imports and staged publication audit supplement host evidence. Exact two-candidate hashes/results are in MILESTONES.md and ignored package JSON; no Release is authorized.

Both artifacts require normal GUI/storage and exclusive info-only USART access, with no new permission/settings/power operation. Future approved execution may write existing numeric/redacted session data under the normal app directory; development accessed no device files. Preserved recovery bytes are not overwritten. New candidate deployment/launch needs separate approval of exact checksum and procedure. Wi-Fi additionally needs installed passive/capture-policy/stop review and separate command approval; NRF/CC remain blocked on their existing route/provider/ownership requirements. No physical test is requested or executed here.
