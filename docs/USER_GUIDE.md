# NetSentinel Ultra v1.0 software RC1 user guide

This guide describes implemented software, not a v1.0 hardware release. No additional installation is authorized. The unified RC1 FAP uses the verified info-only USART path; Wi-Fi/BLE observation and external-radio bindings stay blocked. A screen listing a module is not detection evidence.

## Navigation

New esp-stable/wifi-dev candidates open the unified dashboard described in [WIFI_DISCOVERY.md](WIFI_DISCOVERY.md). Up/Down selects ESP32 Diagnostics, Wi-Fi Discovery, Network Results, Session History, Settings, Hardware Readiness or More tools; OK opens. More tools retains the previous eleven-entry layout. Back stops local work and returns to the dashboard, then Back exits. Discovery is Unavailable and sends nothing. Network Results starts Not Tested; the Wi-Fi development variant's OK action runs only an offline parser check and does not create network observations. No raw SSID/BSSID is displayed or exported. The preserved owner-verified ESP Status binary retains its prior layout.

Up/Down selects dashboard entries; OK opens; Back cancels an active/queued operation and returns home, then Back exits. ESP32 diagnostics replaces the first dashboard Wi-Fi entry in the safe development build. The dashboard includes Wi-Fi readiness, which explains the live observation gate and links back to the ESP screen. Four detail views use Up/Down, showing matched firmware/profile, ESP-IDF/last validated SD state, last success session/elapsed/timeout, and the redacted last information outcome/result. OK or Right requests exactly one info query when idle. Retry is disabled while queued or active. Connected means completion with USART closed; last-success values remain historical until another validated reply. Connection state and latest query result are independent of unrelated UI errors. Elapsed time and session IDs are not wall-clock timestamps. No scan/reset/update commands are available.

NRF24 and CC1101 screens explain missing physical adapters. Their receive algorithms are tested with mocks and compiled, but these screens cannot activate radios in the current build. BLE remains explicitly unsupported. Failure messages distinguish timeout, busy, invalid response/state, storage/response capacity and I/O errors. Diagnostics OK retries dirty history or safely reloads SD state; queued/active operations block reload.

## Observation controls after physical adapter verification

NRF24: selector confirmation is required before receiving. OK starts/stops, Right toggles graph/statistics. Statistics rank only measured channels; RPD percentage is sampled activity above the chip threshold, never RSSI or decoded traffic. Sampling period includes GUI/SPI overhead, and dwell is bounded by settings. A zero sample count does not mean an empty channel.

CC1101: OK requests selector confirmation then receive-only sampling. Left/Right changes frequency by 100 kHz, Up cycles silicon-band presets, Down toggles raw status/trace. Physical board bandwidth and supported presets still need verification. RSSI and threshold percentages are measurements, not decoded signals. Retunes reset frequency-specific traces; active changes are temporary. Stored idle settings persist.

Wi-Fi software: Up/Down selects filtered observations; Right toggles details; Left records a RAM baseline. APs use local numeric labels; SSIDs/BSSIDs remain in volatile parser/comparison memory and are hidden from GUI/exports. Channel counts use all retained observations, while navigation uses filters. No matching observations shows an explicit empty state. Security is reported only as present/absent rather than untrusted raw text. Unknown or changed BSSID identity cannot be reconstructed from redacted CSV.

## Filters, history and settings

Filters screen: Up/Down selects channel/minimum RSSI/maximum age; Left/Right changes; OK clears. Channel 0 and age 0 mean all/no expiry. Filters are volatile and reset at launch; they do not alter acquisition or stored settings. Age uses wrapping application milliseconds. History Up/Down selects newest-first records; Left/Right cycles all/ESP/NRF/CC, OK toggles failures only. History names ESP32/NRF24/CC1101; CSV codes remain 1=ESP, 2=NRF, 3=CC. Empty history shows no completed sessions; filtered-empty history shows no matching sessions.

Settings retain the NSP1 dual-slot CRC schema: query timeout, NRF dwell, idle CC frequency and application baud. The fourth row displays stored baud; OK restores the verified 115200 value explicitly. Left/Right on that row has the same effect. No expansion setting, firmware or power configuration is changed. Changes save immediately when idle; identical values avoid additional SD writes. Errors preserve the previous in-memory setting and state-file failures remain visible on the dashboard and Diagnostics. USART remains fixed at TX13/PB6, RX14/PB7, 115200/8N1. Corrupted/unreadable state is never silently overwritten; Diagnostics reports retry failure.

## CSV exports and storage

Session records and numeric history exports use /ext/apps_data/netsentinel/session_XXXXXXXXXX.csv. Creation is exclusive and reserves a monotonically increasing ID before writing, so failed exports may leave gaps/partial files but cannot overwrite prior data. CSV exports screen OK exports the current history filter; it does not acquire a radio. History CSV encodes original session ID, radio, duration, observation count and result code (NsResult); no identifier/raw serial text. A failed write/sync/close is reported and the handle is closed. Last export ID updates only on success. No file deletion or automatic migration is performed.

Records share the existing seven-column CSV schema. UART lines contribute only fixed redacted telemetry. CC records carry frequencies, RSSI/status values and timing; NRF records contain channel/activity. Export timing is application-relative. The retained history is the latest 16 sessions; earlier CSV files remain on SD. Device storage inspection/backup and installation require owner action or separate approval.

Unified Diagnostics Up/Down selects storage, USART and radio-readiness views. Text wider than the screen ends with a visible `~`; detail screens keep long version fields separate. Pixel bounds are tested with mocks and checked by the actual SDK renderer API; visual presentation awaits final approved hardware validation.

Age filters continue expiring while the worker is idle. A full AP table or exhausted observation counter closes the session and reports Full rather than silently wrapping or retaining ownership. Numeric Wi-Fi observation CSV uses session-local AP labels, channel and RSSI. History result codes use the `enum` unit; earlier files keep their original unit labels and are not rewritten.

Source-verified diagnostics accept SD Not Connected with size zero and WSL disabled. The source recheck identifies NULL/promiscuous discovery rather than the previously attributed AP startup. Peer PCAP capture with SavePCAP defaulting on and installed passive/cleanup behavior remain unresolved. Do not enable discovery on that basis. NRF/CC simulated status checks do not establish RF passivity.

NRF24/CC1101 receive sessions are capped at 60 seconds in the controller. A complete capture stops before another sample, preserves statistics/history and releases ownership; cleanup failure is reported. Information timeouts remain configurable and separate. Incomplete/mismatched information replies record Invalid/Unsupported, never OK. CSV sync/close failures propagate to the UI and history. Back drains pre-cancellation queued input so stale OK events cannot restart a query. These are software-tested behaviors; physical radios remain disabled.
