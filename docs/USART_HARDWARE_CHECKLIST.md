# USART information-test verification checklist

## Recorded physical result

Owner reports the first authorized NetSentinel ESP Info test on Momentum Dev d3f89dfe, API87.1, target7, attached Ultra V3: requesting Marauder information resulted in **Connected**. Record this as one successful information transaction only. No independent audit of every response field, callback teardown, pin/serial/logging restoration, SD contents, repeatability or cancellation was reported. No RF functionality is verified.

The previously prepared local test FAP is preserved at `builds/preserved-first-esp-info/netsentinel_esp_info.fap`:37384 bytes, SHA256 `c27cefddc3379404b4bd689a3196574f858d31a91af262d0a71700a39f45db67`. The owner did not supply an independently checked on-device transfer checksum; this is preserved local build identity, not proof of the exact transferred bytes.

## Record format and approval

For each case record: artifact hash/source milestone, firmware API/target/revision, existing selector/attachment configuration, expected/observed state, number of attempted/completed transactions, elapsed time when available, pass/fail/not-exercised and sanitized notes. Keep identifiers/raw responses out of both logs and public documentation. Record field checks independently as pending until observed; Connected alone is not a field-by-field audit.

The revised FAP requires new deployment/launch approval for its exact hash. Back up existing application-data files using the supported workflow before an approved test; queries change local session IDs, CSV/history and state slots. No erase or automatic restoration is authorized. Keep the working power/attachment/selector/settings unchanged. Do not open another radio mode, unplug a powered board, reset, force ownership, change USB permissions or disable firmware logging/expansion to create a fault. Any such setup alteration requires separate authorization and is outside this checklist.

| Case | Expected result / evidence to record | Physical status |
|---|---|---|
| One info request | Connecting → Waiting → Connected/query passed | Owner reports PASS once |
| Identity/status fields | Independently compare permitted firmware/version/profile/IDF/SD/battery fields without addresses | Pending |
| Cleanup after success | Original Marauder app remains functional; detailed restoration checks separate | Owner operational PASS; exhaustive restoration pending |
| Exit after success | App exits and original Marauder app works afterward | Owner reports PASS for one sequence |
| Repeated queries | Ten manual info requests; one submission per acquisition, no Busy/error buildup, expected history count | Pending |
| Repeated launch/exit | Ten manual launch/query/exit cycles, resource stability; count each completed cycle | Pending |
| Cancel while Connecting | Back cancels pending acquisition; no new query/history if acquisition never occurred | Pending |
| Cancel while Waiting | Back cancels active query, ordinary exit/reacquisition works; no stop/scan command | Pending |
| Timing-sensitive cancellation | Record actual phase; if reply finished before Back, mark cancellation Not exercised | Pending |
| Timeout/error cleanup | Bounded error and subsequent safe reacquisition; do not create hardware faults without separate approval | Mock-tested only |
| Already busy controller | Other owner preserved, no forced takeover; use only an independently approved safe setup | Mock-tested only |
| Callback/resource restoration | No stale callbacks, stuck ownership or unwanted setting/power changes; observation scope stated | Mock/API-review only |
| SD/privacy | Fixed redacted line telemetry and numeric session outcomes only; no addresses/raw identity strings | Mock/source-review only |
| Heap/resource stability | Compare pre/post metrics if already available through approved read-only tools; no growing leak trend | Pending |

Perform each approved manual action once; do not use automatic retries or generate protocol traffic beyond info. If natural response timing prevents cancellation, report the coverage gap instead of changing delays/power to manufacture it. Mock silence/error cases do not replace physical ones. Stop on reset/banner, incomplete/mismatched response, timeout, overflow, Busy, freeze, storage error, unexpected serial/power change or a privacy violation. Exit normally, retain sanitized results and request review before further attempts. Do not send stopscan, help, reboot or any scan/attack command to recover.

## Cleanup and privacy review

GUI worker owns protocol/storage/serial operations. Draw/input callbacks consume a published frame/queue only. On app teardown the input/draw callbacks are removed and viewport detached before stopping the session and freeing its context; serial cleanup precedes freeing the adapter. No frame mutex is held over I/O teardown. Supported HAL ordering is RX stop → serial deinit (documented to clear callback/context and disable hardware) → clear application ring/counters → release controller. Repeated release is a no-op. This ordering is reviewed and mock-tested, **not independently physically verified**. IRQ/driver-owned restoration semantics and the semi-blocking TX call's lack of deadline/error return remain physical/SDK limits.

After RX deinit, the application now clears its4096-byte ring and queue/error/transaction counters. That removes retained raw bytes from that application buffer; it is not a guarantee of erasing all SDK/peripheral/system memory. Logs use fixed uart_redacted records. History/state serialization contains local session IDs/times/counts/radio/result/settings, no device strings. Info-only diagnostic frames render fixed expected identity constants only after match; unvalidated UART text never reaches them. Detailed failures use fixed categories rather than echoing response content. Synthetic identifier tests verify frames, state slots and closed CSV; mock deinit clears callback context and controller release asserts it is gone.

## Updated build evidence

The updated, untested FAP is `dist/netsentinel_esp_info.fap`,37856 bytes, SHA256 `c8178384ef92eeb8d287b3b010447cab87b3e50d8dd933f32652953c9ce5d02e`. Clean matching Momentum SDK build compiles all12 C units and passes APPCHK/API87.1/target7; all62 actual imports resolve, zero weak imports and no other radio/power/USB-control imports. `NS_ESP_INFO_ONLY` still accepts exactly one info+LF per acquisition; help/stop/scan/other radio paths remain blocked. Default GUI/storage FAP and first-device baseline are unchanged.

All ten host suites, short-enum and ASan/UBSan variants, eight SDK-preflight/four publication cases and exact import/whitespace audits pass (local leak detection disabled). Evidence remains ignored/local under `logs/info-cleanup-*`, with clean compiler output in `logs/live-info-build.log`. No new deployment, firmware operation, radio scan or release occurred.

Next safe milestone: with separately approved exact-artifact deployment, execute the information-only cleanup/reacquisition, repeated launch/exit and cancellation cases above. Keep scan/NRF/CC enablement outside that milestone.

## USART reliability milestone

Owner now confirms launch, Connected after info, clean application exit, and Momentum's original Marauder app still functioning afterward on d3f89dfe/API87.1/target7, USART13/14/115200/8N1. This is operational restoration evidence for that sequence, not exhaustive pin/setting/heap restoration or repeated-session proof. Independent field checks and timing-sensitive cancellation remain pending.

Audit found queued UI requests could be bypassed by the public info API. Both queued and active transactions now block overlapping requests; pending cancellation still performs no acquisition. New exact-flag mock tests exercise eight mixed success/timeout/malformed/cancel transactions, correct history counts, command exclusion, reacquisition and actual GUI worker exit during an active query. No raw-response diagnostics or identifier persistence introduced. RX stop/deinit/buffer clearing/release and callback-context lifetimes remain unchanged and reviewed.

Ten host suites, short-enum/ASan-UBSan variants, eight preflight/four publication cases pass. Clean Momentum build/API87.1/target7/all62 imports pass. Updated undeployed FAP:37892 bytes, SHA25632e8eb76941d803e3b79f02880e50aadc27b3b94d68d0978889639a3b8c4952b. Default GUI/storage FAP, first ESP baseline and prior cleanup test preserved; prior cleanup artifact additionally stored in builds/preserved-67df4fc. All info-only gates remain; no expansion/power/firmware/deployment changes. Next safe milestone: approved repeated info/exit/cancellation verification using the structured checklist.

## ESP32 Diagnostics v0.2 (experimental, information-only)

Owner-reported physical verification of the preceding ESP Info artifact: **7 of 8 checks passed** on Momentum Dev d3f89dfe/API 87.1/target 7. Launch, dashboard rendering, three consecutive queries, Connected completion, normal exit, subsequent operation of Momentum Marauder, and inspected history/CSV privacy passed. **Back cancellation during an active request remains physically unverified.** This reports the supplied checklist results, not exhaustive resource restoration or independent verification of every response field. USART configuration remains TX13/RX14, 115200 baud, 8N1; no expansion settings changed.

The new diagnostics artifact has three views selected with Up/Down: validated firmware/profile, validated ESP-IDF/SD connection state, and timing. Identity values are fixed allowlisted constants after a complete matched reply. SD state is a typed snapshot from the last validated query. Last success is a session number for this application launch, not a wall-clock timestamp; zero means no success this launch. Elapsed time uses unsigned millisecond subtraction, with the configured timeout shown separately. These values are volatile and contain no identifiers. Connected means a completed query; the USART is already closed. Previous successful diagnostics remain marked as last success after a failed retry.

OK/Right retries only when idle. The visible retry action is disabled while queued or active. Back cancels either state and returns home; another Back exits. Mock tests cover queued cancellation, fragmented-response cancellation, retry rejection during reception, read rejection after teardown, repeated mixed outcomes and application exit. No extra cancellation command is sent; RX stop/deinit/release and buffer clearing remain enforced by existing mock HAL assertions. Hardware HAL TX of the five-byte info command remains semi-blocking; protocol deadlines cannot interrupt a hung HAL call.

Only one exact info+LF is accepted per USART acquisition. Scans, injection, reset, help and firmware-update commands are rejected by the exact allowlist. NRF24/CC1101 remain disabled. Raw serial text, MAC addresses and unique identifiers are never shown or persisted. Existing data and artifacts were preserved; the previous test FAP is additionally copied under ignored builds/preserved-00b103c. Default GUI/storage FAP and dist/netsentinel_esp_info.fap remain unchanged.

Build: separate **dist/netsentinel_esp_diagnostics.fap**, 38,944 bytes, SHA-256 **e4281f6bdc32090bfc16c5e52da3383233382b70f0324f0263c650a9754ecdfb**. Supported uFBT clean build against the permitted mntm-dev-d3f89dfe SDK passed compile/link/packaging/APPCHK. Actual FAP metadata: API **87.1**, target **7**. All **62** required imports match enabled exports; zero undefined weak imports. All ten host C suites, eight preflight tests, four publication tests, short-enum builds and ASan/UBSan passed (local sanitizer leak detection disabled). Runtime/build evidence stays in ignored logs. The experimental artifact is not deployed or physically tested and is not a release.

Next hardware verification requires separate approval of this exact artifact: check the three diagnostics views and retry states, repeated queries and ordinary exit, then opportunistic Back cancellation during an ordinary info request without deliberately stalling hardware. Recheck original Marauder operation and history/CSV privacy afterward. No scanning, firmware changes, power controls or additional radios are authorized.
