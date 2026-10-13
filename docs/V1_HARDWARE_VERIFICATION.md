# Final hardware verification procedure (not authorized to run)

Do not repeatedly install experimental FAPs. First finish the software gates in V1_READINESS.md and resolve the physical adapter/passivity dependencies. The current development FAP is info-only and cannot pass the full radio checklist. Any later installation/launch, selector change or hardware measurement requires separate owner authorization of the exact artifact, configuration and procedure. No firmware, expansion settings, USB permissions or device storage erase is part of this procedure.

## Preconditions

Record exact artifact SHA-256/build source and check all software tests, loader metadata/imports, target d3f89dfe/API 87.1/7, board identity and UART TX13/PB6 RX14/PB7 115200/8N1. Owner backs up existing app/data before an approved installation; retain stable rollback FAPs and preserve CSV/settings. Do not remove/reformat data or automatically rewrite corrupt state. For any enabled radio, review routing, initialization, receive/stop and cleanup effects before permission to operate it. No feature may claim passive behavior just from a label or a receive-named callback.

## GUI/storage and information checks

| Check | Current evidence | Final result |
|---|---|---|
| Launch/dashboard | Prior FAP owner pass | Pending final build |
| Every screen/back/keyboard path, width/readability | Host mocks and SDK string-width checks | Pending |
| Settings save/relaunch and invalid/unreadable storage recovery | Prior settings owner pass; fault mocks | Pending final build |
| History all/radio/error filters and numeric CSV export | Software pass | Pending |
| Diagnostics show matched version/profile/IDF/SD | Prior Connected transaction only; new screen mocks | Pending independent field checks |
| Three consecutive info queries and visible busy/retry exclusion | Prior three-query owner pass; mocks | Pending final build |
| Back cancel queued and receiving transaction | Deterministic fragmented/cancel/exit mocks | Physically pending |
| Normal exit and original Marauder app still works | Prior owner pass | Pending final build |
| GUI responsiveness, repeated session/launch/exit soak, heap/pin/setting restoration | Host cleanup/alloc mocks; limited operational owner evidence | Pending exhaustive checks |
| No identifiers/raw UART in CSV/history/UI | Owner inspected prior records; private mocks | Pending final build |

Perform cancellation opportunistically during a normal information query, without deliberately hanging hardware or issuing a different command. If the query finishes first, record cancellation as not exercised, not passed. Connected means the transaction finished and USART closed; subsequent independent app use is operational restoration evidence, not complete register/pin restoration proof.

## Radio checks after physical implementation and passive review

NRF24: obtain exact bus/CS/CE routes and selector truth table. Owner sets selector only when safely stopped. Confirm readback/detection, PRIM_RX, EN_AA=0, SETUP_RETR=0 and no TX/carrier/payload writes from init through teardown. Verify activity across channels against the known-working receive-only standalone scanner; exercise absent/busy/error cleanup. RPD is threshold activity, not RSSI/decoded packets.

CC1101: verify explicit external plugin/device identity and no internal fallback, bounded bus ownership, preset/calibration, receive state, RSSI conversion/offset and idle/release. Confirm initialization/auto-state configuration cannot transition to TX. Test signal plots/CSV and retuning only in supported board bands. A suitable controlled signal source is needed for formal RF reception verification; diagnostics alone cannot pass it. No transmitter operation is authorized here.

Wi-Fi/BLE: enable nothing until the exact installed-build command/init/receive/stop behavior is reviewed for TX and secondary storage effects. Public upstream BLE scanning uses active scan; it is excluded. A passive implementation unavailable in installed firmware is a feature blocker, not a reason to flash replacement firmware. Review Marauder-side packet capture defaults as well as NetSentinel privacy. Observe only after approval; compare channel/signal aggregation, baseline/filter behavior and stop/recovery with reviewed outputs. Never send discovery probes, association, injection or attack commands.

## Stop and completion conditions

Stop on unexpected transmission, raw identifier display/storage, reset/update behavior, stale Connected state presented as an open connection, unresponsive Back, serial ownership leak, wrong/internal radio, storage overwrite, crash or unexpected peripheral power/setting changes. Preserve only sanitized failure metadata; do not publish captures or personal data. Use preserved application rollback only with owner approval, never firmware rollback/flash. Mark each result PASS/FAIL/NOT EXERCISED with exact artifact and distinguish owner report, software simulation and physical measurement. Release requires all required gates and explicit approval, not simply an installable FAP.
