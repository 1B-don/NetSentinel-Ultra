# Settings and session persistence

Application-owned files are `/ext/apps_data/netsentinel/state_0.bin` and `state_1.bin`. CSVs use `session_0000000001.csv` with exclusive creation; earlier CSVs are never replaced or removed.

State schema `NSP1`, little-endian 32-bit words:

| Byte offset | Field |
|---|---|
| 0 | Four-byte magic NSP1 |
| 4 | Monotonic generation (1–UINT32_MAX) |
| 8 | Last reserved session ID |
| 12 | Completed-history count, 0–16 |
| 16–28 | Baud, UART timeout ms, NRF dwell us, CC frequency Hz |
| 32 onward | Oldest-first history entries: ID, start ms, end ms, observations, radio, result (24 bytes each) |
| End minus 4 | CRC32 of all earlier bytes, polynomial 0xedb88320 |

Exact file length is 36+24×history count (maximum 420 bytes). Padding, excessive length, unknown magic, invalid enum/range, duplicate/out-of-order IDs and checksum mismatches are rejected. Session times are app-relative elapsed milliseconds, not wall-clock dates; duration uses unsigned subtraction for wraparound.

On load, select the highest valid generation. If both files are absent, initialize defaults without writing until an explicit save/start. If neither is valid and any file was corrupt/unreadable, block writes; the application reports the error and Diagnostics can reload after the issue is resolved. No automated deletion or reset is provided.

On save, write only the inactive slot, sync/close, read back and compare. Publish the new generation/settings only after success. A torn write leaves the previously active slot. Generation/ID exhaustion returns Full rather than wrapping. Checksums detect corruption; they do not authenticate files. If a newer slot is corrupted or the SD is rolled back, ID state may also roll back: exclusive CSV creation prevents overwriting an existing session; a collision fails the new session and consumes the ID. No automatic CSV cleanup is performed.

A session start durably reserves an ID before radio acquisition. Acquisition failures consume IDs. A successful acquisition must open its CSV before polling radio measurements; tuning/logging failures stop and release the peripheral. Completed history is saved after cleanup. Failed history saves remain in RAM, block subsequent starts until persisted, and can be retried from Diagnostics. Shutdown retries them; data still unsaved at power loss is not recoverable. An ID reservation survives interruption, but interrupted sessions have no completed-history record.

Persistence and CSV callbacks use separate handles so history saves do not disrupt an open CSV. The Flipper adapter overwrites only its own inactive state file; it never formats/removes SD data. Real SD durability and firmware API timeout behavior remain unverified. CRC/alternating files do not guarantee survival of device/card failure affecting both slots.
