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
| Cleanup after success | USART available for a subsequent approved information-only client; no ownership error or reset | Pending |
| Exit after success | Back to dashboard, Back exits; ordinary USB/UI access continues | Pending for ESP build |
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
