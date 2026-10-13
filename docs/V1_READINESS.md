# NetSentinel Ultra v1.0 software RC1 readiness

Target: permitted Momentum Dev d3f89dfe SDK, API 87.1, hardware target 7. No new deployment or GitHub Release authorized. RC1 consolidates all presently supported functions; it is not an operational three-radio release. Historical evidence remains in MILESTONES.md and USART_HARDWARE_CHECKLIST.md.

## Completion checklist

- [x] Audit dbecb97 source, lifecycle and manifest against v1.0 scope; preserve stable artifacts and icon.
- [x] Fix actionable query outcomes, serial corruption, CSV close-error propagation, cancellation queue handling, bounded capture sessions and storage recovery presentation.
- [x] Complete dashboard, ten-menu navigation, diagnostics, settings, filters, history, session lifecycle and numeric CSV workflows.
- [x] Explicit legacy app-baud recovery to 115200 without firmware or expansion setting changes.
- [x] Single unified RC1 manifest with info-only transport gate and approved embedded icon.
- [x] Host/mock, malformed input, persistence recovery, repeated launch/navigation, fuzz/stress, short-enum, ASan/UBSan, publication tests.
- [x] Actual matching SDK compilation/link/APPCHK and strict metadata/entry/import validation.
- [x] User guide, reproducible build procedure, limitations and physical checklist.
- [ ] Resolve board SPI/CE/CSN/CS/GDO routing and bounded physical providers.
- [ ] Resolve installed Marauder passive lifecycle and peer-side capture policy before any observation authorization.
- [ ] Verify final artifact physically and obtain explicit release approval.

## Feature classification

| Feature | Software-tested / implemented | Simulated | SDK-validated | Hardware evidence / dependency |
|---|---|---|---|---|
| Dashboard, all menus, controls, width, Back/exit | Complete supported workflows and unavailable states | Three cycles through all ten screens; queue/allocation/error tests, 10,000 UI events | Pass | Prior dashboard/exit pass; final full GUI/visual/soak pending |
| USART info exchange, retries, timeout/cancellation | Fixed TX13/PB6 RX14/PB7, 115200 8N1, one info command, no overlaps, safe teardown | Fragmentation, errors, interrupted/repeated queries, wrap, limits and resource contention | Pass | Prior three Connected queries, exit and original app interoperation; final cancellation/restoration pending |
| Version/profile/IDF/SD diagnostics | Complete matched reply required; typed historical snapshot, duration/session/timeout/error views | Complete/missing/mismatched/no-SD/corrupt replies | Pass | Prior successful transaction does not independently verify every field; final screen checks pending |
| Settings/history/recovery | NSP1 CRC dual slots/readback, monotonic IDs, 16 retained sessions, idempotent saves and explicit baud repair | 420 torn writes, read/write failures, enum validation, restart/recovery | Pass | Prior settings persistence; final controls and fault behavior pending |
| History and observation filters | Complete volatile channel/RSSI/age and radio/error filters | Empty/filtered-empty, invalid values, wrap and idle expiry | Pass | Final physical navigation pending; filters intentionally reset at launch |
| CSV exports/privacy | Numeric/redacted, exclusive create, pre-reserved IDs, sync/close errors, no identifiers/raw UART | Storage faults and privacy tests | Pass | Prior records inspected by owner; final exports and SD failure handling pending |
| Wi-Fi AP/channel/RSSI/baseline cores | Implemented, no invented encryption output | Sanitized protocol/observation fixtures, parser mutations, overflow/recovery | Compiled; physical scan gate off | Blocked: installed build/options, passivity across init/stop/background state, SavePCAP side effects |
| BLE discovery | Explicitly unsupported | Command denial | Disabled | Reviewed upstream path uses active discovery; no verified receive-only alternative |
| NRF24 receive sampling, capability checks, plot/statistics/CSV | PRIM_RX, auto-ACK/retry off, readback, bounded 60-second sessions | SPI faults, counter/deadline cleanup, 25,200 PRX samples | Core compiled; no SPI imports | Blocked: Ultra SPI/CE/CSN routes, ownership/bounded provider and chip verification; standalone scanner owner success is not this adapter's evidence |
| CC1101 receive diagnostics, tuning, RSSI/trace/CSV | Status reads, RXOFF guard, checked receive contract, cadence, bounded sessions | All 256 RXOFF values, identity/state/fault/RSSI/trace/deadline tests | Core compiled; no external registry/SPI imports | Blocked: exact routes/provider/preset, bounded calibration/init, RSSI calibration and formal RF reception |
| Packaging/approved icon | One named RC1 with exact icon bytes, API/target/stack/name/entry/import checks | Manifest/icon mutation, all variant/source bundle tests | Pass; 63 imports, zero weak | This exact RC1 is undeployed and untested on device |

## Exact remaining inputs

[Dependency review](DEPENDENCY_REVIEW.md) contains primary source references. Obtain an independently permitted manufacturer signal/net table for the actual Ultra V3 revision (SPI, NRF CE/CSN, CC CS/GDO, selector and voltage circuitry). A separately approved owner power-off continuity check is an alternative; none performed here. For CC, obtain a permitted installed external plugin/provider implementation and reviewed receive-only preset with recoverable bounded init/calibration/ownership behavior; generic SDK exports or official-driver pin expectations are insufficient. For Wi-Fi, obtain installed-build/source correspondence and sanitized capture/association/background-state facts; do not provide credentials, complete settings dumps or packets.

Nothing in software can establish unknown PCB routing, guarantee an opaque blocking driver deadline, prove installed RF silence or exercise physical cancellation without those inputs and separate approval. No guessed bindings, internal-radio fallback, firmware replacement, attack commands or synthetic live measurements were added. See SOFTWARE_COMPLETION.md for implemented audit fixes and V1_LIMITATIONS.md for release gates.
