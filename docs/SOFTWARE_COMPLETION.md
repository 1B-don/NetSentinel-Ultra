# v1.0 software completion audit

Audit started from dbecb97. Stable binaries and checksums preserved in ignored builds/preserved-dbecb97; no device data accessed. The root manifest stays GUI/storage-only. The unified candidate uses the existing strictly info-only USART gate. Physical adapter dependencies remain gates, not fake capabilities.

## Prioritized checklist

| Priority / issue | Action and verification | Status |
|---|---|---|
| 1: Incomplete/mismatched info recorded as OK | Require complete, correlated, matched status; record Invalid/Unsupported and release; previous valid snapshot remains explicitly historical | Fixed; protocol/transport tests |
| 1: Successful prompt could hide CSV sync/close failure | Propagate final teardown result to controller and history; no Connected on final I/O error | Fixed; sync and close fault tests |
| 1: Corrupt/non-ASCII serial bytes were repaired into printable text | Fail non-observation replies closed; discard malformed observation lines | Fixed; NUL/control/oversized/retry tests |
| 1: NRF/CC captures had no duration cap | Finish at 60 seconds before another measurement, across tick wrap; retain samples/history, close log, idle/release; report cleanup errors | Fixed; both radios and cleanup faults |
| 1: Back could leave stale queued OK actions | Drain at most 16 queued events at Back; latch remains independent of queue capacity | Fixed; shell/queue/lifecycle mocks |
| 2: Navigation could hide storage failure | Track state-file health independently, show unreadable/write-blocked status, explicit safe reload/retry | Fixed; failure/recovery navigation tests |
| 2: Blocked radio screens showed a start prompt | BLE/NRF/CC display meaningful unavailable states; no physical access | Fixed; all menu paths tested |
| 2: Wi-Fi analysis existed but its live gate was unclear in info builds | Add Wi-Fi readiness entry with installed-passive/SD requirements and link to ESP screen; it never bypasses scan denial | Fixed; ten-menu navigation tests |
| 2: History controls and radio codes were unclear | Name radios in history, display browse/filter/error controls; distinguish empty and filtered-empty records | Fixed; history/filter tests |
| 2: Repeated settings at bounds wrote unchanged snapshots | Skip identical settings writes, preserving NSP1 format and error recovery | Fixed; repeated held-key boundary tests |
| 2: Untrusted identity text could reach a dormant GUI branch | Display only validated fixed constants in all modes | Fixed; existing privacy/parser tests |
| 3: Consolidated candidate packaging/evidence | Separate named RC with exact info-only gate, icon, strict metadata/import checks and preserved prior outputs | Complete; manifest/bundle tests, SDK build and strict FAP checks |
| 2: Legacy app baud could block queries without a GUI remedy | Settings explicitly restores 115200, without hardware or firmware configuration changes | Fixed; restart/idempotent-save tests |
| External: Exact SPI routes, bounded external CC provider, installed Marauder capture/passive lifecycle | Keep physical adapters/scans disabled; see DEPENDENCY_REVIEW.md | Blocked on independently permitted evidence |

## Verification scope

Twelve C suites and twenty-three Python checks, short enums, ASan/UBSan, real Momentum API 87.1/target 7 clean compile/link/APPCHK and 63-import checks pass for the completion source. Tests include three restart cycles through all ten screens without radio acquisition, complete/incomplete/mismatched information, CRLF/LF and fragmented lines, embedded/repeated CR rejection, overflow-safe queued cancellation, overlap/resource failures, 420 torn writes, seeded parser/UART/PRX/UI stress and capture deadline cleanup. SDK binding is actual compilation; SPI behavior remains simulated. Sanitizer leak detection is disabled locally; mock live-allocation/record counts must return to zero.

No new physical test, firmware operation, expansion/power configuration change or deployment occurred. Existing information-only physical success remains owner-reported; Back cancellation and final candidate workflows remain untested. A 60-second controller limit cannot interrupt an opaque blocking HAL/driver call; physical providers must be bounded themselves. SD operations and the five-byte HAL TX have no SDK deadline result. Essential radio features cannot be declared operational or a full v1.0 release while their dependencies remain unresolved.
