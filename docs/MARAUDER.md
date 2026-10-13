# Marauder integration status

The owner verified USART information queries on Momentum d3f89dfe/API 87.1/7, Ultra V3, Marauder 1.11.0/v4, IDF v5.5.1-710-g8410210c9a, TX13/PB6 RX14/PB7 at 115200/8N1. Three consecutive queries, clean exit, original Marauder app interoperability and inspected-record privacy passed on the preceding ESP Info artifact. Active-request Back cancellation and new diagnostic fields are not physically verified.

Current development FAP uses NS_ESP_INFO_ONLY: one exact info+LF per acquisition; no scans, help, stop, reset, settings or update commands are permitted by the physical transport. Default GUI/storage FAP has no UART access. NRF/CC/BLE remain disabled. Connected means a completed validated information query after USART release, not a held connection or proof that unrelated ESP firmware background tasks are silent.

## Implemented information path

The [ESP status milestone](ESP32_STATUS_DIAGNOSTICS.md) isolates typed connection/result state from unrelated UI actions, adds a fourth redacted outcome view, discards pre-dispatch buffered bytes and rejects duplicate echoes. Whole delayed responses have no nonce and cannot be authenticated as fresh. Prior stable FAPs remain preserved; the separate status candidate is software/SDK tested and undeployed.

USART ownership is exclusive, TX/RX direction and supported baud are checked before initialization, framing is explicit 8N1. RX ISR uses a bounded lock-free ring and poisons responses on overflow/serial error; worker parsing has fixed deadlines across tick wrap. Repeated/queued requests are excluded. Cancellation sends no additional command. Teardown stops callback reception, disables UART, clears staging/ring data and releases ownership before context destruction. HAL's five-byte TX call is semi-blocking with no error/deadline return; hardware timing/restoration remains a separate gate.

Parser requires correlated echo/prompt, bounded nonempty fields and no duplicates. Complete status and matched identity are distinct flags. It recognizes the owner-observed info fields and upstream-documented WSL disabled / SD Not Connected / size zero variants. SD state and capacity must agree. Unknown identifier-labelled fields are ignored, never retained/logged. Public UI uses fixed matched constants and typed metadata, not untrusted serial strings. SD/last-success snapshots are volatile and labelled historical; no raw data or identity is persisted.

## Dormant observation implementation

See [dependency review](DEPENDENCY_REVIEW.md) and [unified discovery milestone](WIFI_DISCOVERY.md) for exact source/protocol boundaries. The pinned-source recheck corrects the earlier AP-start attribution: initWiFi loads settings/changes channel; discovery uses NULL/promiscuous mode. AP setup occurs in a separate routine. Peer PCAP opens and SavePCAP defaults true; installed passivity/privacy/stop are not verified. All shipped transports still accept only info. The new finite discovery/list/detail model has a host-test-only protocol gate and no physical command binding; its outputs are not live measurements.

AP parser accepts reviewed beacon/list formats, validates channel/RSSI, uses BSSID matching with SSID/channel fallback and caps entries at 48. Full tables/counters stop and release safely. RSSI extrema and baseline comparisons are in RAM. UI hides names/MACs; channel/RSSI/age filters and local AP labels are bounded. No security detail format is invented; encryption remains unavailable in reviewed beacon output. Numeric Wi-Fi CSV records and channel totals use the seven-column schema, while arbitrary UART lines produce fixed redacted telemetry. Peer-side PCAP capture is a separate dependency and cannot be made private merely by redacting Flipper logs.

All related unit/integration/seeded fuzz/short-enum/sanitizer tests and matching SDK compile pass. A new default-build regression test asserts that even matching identity cannot cause a Wi-Fi start command. These tests do not establish physical observation, passive BLE support or runtime performance. Stable artifacts/data are preserved; no deployment is authorized.
