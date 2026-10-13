# Marauder UART integration — installed-build verification pending

Owner observations: Ultra V3 runs Marauder v1.11.0, Marauder v4 profile, IDF v5.5.1-710-g8410210c9a; standalone Wi-Fi Recon displays AP names/channels. Standalone NRF Channel Scan receives activity. These observations confirm those applications' hardware path, not NetSentinel execution or an exact UART exchange.

## Evidence and missing capture

Reviewed the tagged [CLI implementation](https://raw.githubusercontent.com/justcallmekoko/ESP32Marauder/v1.11.0/esp32_marauder/CommandLine.cpp) and [radio implementation](https://raw.githubusercontent.com/justcallmekoko/ESP32Marauder/v1.11.0/esp32_marauder/WiFiScan.cpp). Commands are newline terminated, echoed with `#`, followed by a `> ` prompt without a newline. The beacon command uses a promiscuous receive path. Its start acknowledgment joins `Starting` directly to the scan name; beacon lines report RSSI, channel, MAC and ESSID. This is a candidate passive path, not proof of the installed binary's behavior. The working Wi-Fi Recon command sequence remains unknown.

Device information has a literal hardware label but version/IDF labels come from language tables. The tagged upstream language table now confirms `Version:` and `ESP-IDF:`. Owner now supplied sanitized installed info labels/status through CH340 USB-C; echo/prompt framing and help remain uncaptured; the alternate `ESP-IDF Version:` handler remains a provisional compatibility contract. A missing required identity label fails and incomplete status is not marked complete. Unknown fields are ignored; arbitrary UART text is never logged. Fixtures are synthetic source-format examples, not hardware recordings.

Needed from owner: remaining sanitized `help` and info echo/prompt framing, exact Wi-Fi Recon start/stop commands and a short raw output capture. Confirm channel hopping/default channel policy and passive behavior for that installed build before setting any verification capability. Do not issue `scanap` as a substitute without reviewing its installed implementation.

## Implemented source

`NsMarauder` sequences info → identity check → help → beacon-start acknowledgment → receive → stop acknowledgment. Every command phase requires its matching echo and prompt. Each phase has a fixed deadline; junk or incomplete lines do not extend it. Scanning uses the last valid AP observation deadline. Quiet RF may therefore terminate with Timeout; that is not proof of missing hardware.

The default application supports an information-query path and refuses scan commands. `esp_wifi_verified` defaults false and is not a user settings switch. Information labels/status are owner-observed; exact echo/prompt framing still needs confirmation. The candidate Wi-Fi state machine is exercised with verification explicitly supplied by tests only. BLE discovery stays disabled: the reviewed BLE scanner sets active scanning, which sends scan requests and does not satisfy receive-only testing.

`src/flipper/ns_uart.c` acquires the public USART controller at 115200 without changing expansion, OTG power or another owner's state. Busy ownership is reported. An interrupt callback enqueues bytes into a 4096-byte single-producer/consumer ring using lock-free atomics. The worker drains at most 1024 bytes per step. Ring loss poisons the response with Full; serial errors return I/O failure. Release stops RX, deinitializes and releases the controller. A future NRF/CC port must delegate ESP acquisition/release to these functions.

The UART allowlist contains only info, help, stopscan and verification-gated sniffbeacon. The SDK TX API has no deadline/error result and is semi-blocking; no unbounded wait-complete call is used. Its real timing/exported symbols and atomic support must be checked against an appropriately authorized compatible SDK. Failure cleanup sends best-effort stop once and releases; stopping on the ESP cannot be guaranteed after disconnection or TX failure. No automatic reset or power cycling occurs.

Cancellation waits for the matching stop echo/prompt or timeout before normal release. Back can request exit while that bounded shutdown completes. Fragmented CRLF/prompts are accepted; overlong/NUL lines are discarded to newline. Control-phase overflow fails closed. A later valid beacon recovers after a scan-line overflow. AP storage is bounded to 48 entries; a full table reports Full without writing outside it.

## Wi-Fi Explorer and logging

APs update by BSSID, with SSID/channel fallback only when BSSID is missing. Explorer shows names, channel, latest/minimum/maximum RSSI, per-channel AP counts and snapshot comparisons. Up/Down selects an AP, Left records a RAM baseline, Right toggles BSSID/security details (or requests info when no APs exist). Baselines survive scan restart in RAM, not app restart. Comparisons use all APs observed during the current session: Unseen means absent from this observation set, not proven offline. No airtime utilization or interference percentage is inferred from AP counts. SSIDs are bounded printable display strings; non-ASCII bytes are replaced, not decoded as UTF-8.

The observed beacon format does not report encryption. Security is explicitly unknown; indexed `info -a` detail requests remain unimplemented until indices/response framing are verified, rather than attaching security from a guessed index. The existing list parser accepts the selected suffix and honestly leaves BSSID unavailable. No list/index commands are automatically sent.

CSV now retains fixed redacted UART line telemetry only; MACs/SSIDs/identifiers and arbitrary serial bytes are never logged. Final 14-channel AP totals use the existing seven-column session schema. Settings, session ID reservation and completed history use existing durable storage. RSSI ranges/baselines are RAM-only; no independent normalized security export exists. SDK/SD/hardware validation remains required. A matching-SDK GUI/storage-only FAP compiled and its first basic GUI/settings test was performed by the owner; UART/radio physical testing remains pending.

## Current preparation milestone

See [Ultra V3 evidence and bounded-transport review](ESP32_PREPARATION.md). Line/prompt deadlines now reject late complete replies before parsing, with exact/wrapped-deadline tests; invalid UART reads clear output count. Wi-Fi UI distinguishes disabled/busy/waiting/timeout/info-received states. Candidate USART mapping is not a verified Ultra netlist. Hardware TX/ISR timing and installed identity formatting remain blockers; production hardware ports stay disabled.

## Owner-observed information parser/privacy milestone

See [latest USB-C verification](ESP32_PREPARATION.md). SD capacity and battery-unsupported status are parsed strictly; full completion is distinct from identity match and independent scan authorization. Fixtures contain no owner MACs. Raw UART CSV logging was removed globally in favor of fixed telemetry; privacy tests exercise unknown identifier-labelled lines without real identifiers. All mock/SDK tests pass, production remains GUI/storage-only.
