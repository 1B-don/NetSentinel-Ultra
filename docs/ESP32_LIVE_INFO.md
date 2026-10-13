# First USART information-only test build — not deployed

## New owner evidence and permitted SDK direction checks

Owner confirms the attached Ultra V3 communicates through Momentum's existing Marauder app with USART Default pins13/14,115200,8N1, on d3f89dfe/API87.1/target7. USB-C/CH340 independently confirms info identity. This is operational evidence for the intended Flipper interface, not a newly obtained PCB netlist or NetSentinel UART test.

The reviewed permitted SDK headers specify USART1 TX PB6 and RX PB7, expose typed `FuriHalSerialDirectionTx/Rx` and `furi_hal_serial_get_gpio_pin`, and document `furi_hal_resources_get_ext_pin_number`. Read-only bundled-ELF decoding confirms USART table TX pointer `gpio_usart_tx` maps to connector13 and RX pointer `gpio_usart_rx` maps to14. Thus direction is determined from SDK peripheral definitions/table/API, not inferred from two GPIO numbers. Flipper TX13 is output toward ESP RX; Flipper RX14 is input from ESP TX. Exact ESP module pins and intervening circuitry are still unknown. No netlist or voltage measurement is claimed.

The transport rechecks typed TX/RX connector mapping and baud support before initializing; mismatch releases ownership and fails without opening RX or sending bytes. It explicitly configures115200,8 data bits,no parity,1 stop bit with exported SDK APIs. No direct register/pin writes, software flow-control pins, alternate-pin fallback, power/reset/BOOT/selector operations, USB bridge changes or expansion/logging configuration changes are implemented.

## Feature boundary and lifecycle

Default `application.fam` stays `NS_GUI_STORAGE_ONLY`, preserving the radio-disabled build. Separate generated test manifest uses appid `netsentinel_esp_info`, display name `NetSentinel ESP Info`, and **NS_ESP_INFO_ONLY**. Combining this flag with GUI-only or board-port flags is a compile-time error. ESP info-only capability is source-selected, not a persistent user toggle; NRF24/CC1101/BLE/Wi-Fi scan capabilities remain disabled.

The USART transport accepts **exactly five bytes `info` + LF, once per acquisition**. Help, stopscan, scan commands, arguments, reboot and concatenated commands are rejected even if a caller incorrectly sets a Wi-Fi verification flag. Marauder scan requests are independently rejected at compile-time-selected protocol entry. No command is sent at app launch or page entry.

On the Wi-Fi/ESP page, OK or Right schedules the information query, showing Connecting before acquisition. Worker opens exclusive USART, starts RX, sends info and parses its response. Back cancels queued/active work and normal navigation/exit remains. States are Disconnected (local USART closed), Connecting, Waiting, Connected (last complete matching query passed), Timeout, and Error. **Connected is a historical query result; USART has already been closed**, not a promise of continued electrical connectivity. Busy/timeout are not chip-absence detection; plain UART cannot reliably detect cable removal. Full identity/status requires the correlated echo, required fields and final prompt. Expected echo/prompt remain source-derived; absent/different framing fails closed rather than loosening verification.

Resource safeguards: acquire never forcibly takes another owner's controller. Mapping rejection releases the uninitialized handle. Normal completion, cancellation, timeout, overflow and serial faults stop RX, deinitialize serial and release once. ISR processing has32-byte invocation budget; excess pending bytes poison the transaction.4096-byte ring and4096-byte total-response budgets apply even when the worker drains between bursts. Worker drains at most1024 bytes per step. Protocol deadline uses persisted timeout (default5000ms, validated1000–60000ms) and unsigned tick-wrap arithmetic. No allocation, parsing, storage or drawing occurs in the IRQ callback. No automatic retry/reset/power cycle is performed. Arbitrary UART lines/addresses are never persisted; fixed redacted line telemetry and session outcomes only.

**Remaining timing risk:** SDK TX is documented semi-blocking and has no deadline/error result. Only one short5-byte command is submitted; no `tx_wait_complete` import/call is used. Protocol timeout cannot interrupt a hypothetical stuck HAL call. Real interrupt-stop timing, HAL-owned restoration, allocation/SD behavior and contention still need physical testing. Host tests do not prove these timing/resource outcomes. Electrical circuit details remain unmeasured despite the owner-confirmed working interface.

## Reproducible separate build

Run `python3 scripts/build_esp_info.py` from the project. The helper validates the supplied archive checksum and installed SDK components/options/export metadata, copies only project sources/headers into ignored `builds/esp-info-app`, substitutes an info-only manifest there, and invokes supported uFBT app clean/build with the permitted Momentum SDK home and existing authorized toolchain. It does not edit the root manifest, update firmware/SDK or invoke launch. It checks the default FAP stayed unchanged and copies only the separately named FAP to root dist. Do not repurpose this helper to enable other radio ports.

Clean build: all12 C translation units compile with warnings-as-errors; link, metadata, FAP/FASTFAP and APPCHK pass with API87.1/target7. Direct packaged metadata/import inspection passes: **62 required imports**, zero weak imports, every function/data entry enabled in supplied Momentum API table.13 HAL imports cover serial acquire/release/init/framing/baud/RX/TX and resource-pin lookup. No SPI, Sub-GHz, GPIO-write, power-control, USB-control or TX-wait-complete imports.

| Artifact | Value |
|---|---|
| Test FAP | dist/netsentinel_esp_info.fap |
| Bytes |37384 |
| SHA256 |c27cefddc3379404b4bd689a3196574f858d31a91af262d0a71700a39f45db67 |
| Target/API/stack |7 /87.1 /8192 bytes |
| Physical launch/info exchange |Not performed |

Preserved default FAP: `dist/netsentinel.fap`,34888 bytes, SHA25627a73a79f5e3959d3d367a2cd2f12030f3fe048e6277ce8689f51893e2d38bf6. First-device baseline remains `builds/preserved-first-device-baseline/netsentinel.fap`,33480 bytes, SHA256de38c1c7e08336a1ab5a3ebe6dcf2865ac8aa5ce9dedb97c8f8c17c024b859b5. These local copies/checksums are preserved; the owner's physical observation did not independently supply a transferred binary checksum. Official SDK evidence is also untouched.

Local evidence: `logs/live-info-build.log`, `logs/live-info-clean.log`, `logs/live-info-evidence.json`, `logs/live-info-import-audit.log`, `logs/live-info-host.log`, `logs/live-info-short.log`, `logs/live-info-sanitize.log`. Complete import list is local in `builds/sdk-input/live-info-imports.txt`. Logs/binaries/staged SDK files are ignored and not published.

## Tests and exact approval boundary

Ten host C suites (all prior nine preserved), eight preflight cases and four publication cases pass. All ten short-enum/ASan-UBSan variants pass; local leak detection is disabled. The new exact-flag suite verifies wrong-map rejection,8N1 configuration, command/scan exclusion despite caller flags, one info submission, connecting/waiting/complete states, cancellation before/after acquisition, quiet timeout, busy-owner preservation, response-budget overflow, and cleanup with no live mock resources. Existing serial error/fragment/malformed/privacy/storage/UI tests remain passing.

Before any physical action, obtain approval for **this exact hash and one info exchange**, including local app-data/history/log changes. Back up existing baseline FAP and `apps_data/netsentinel` through the supported file workflow: this separate appid still shares the existing NetSentinel settings/history directory, so the query consumes a durable session ID and creates redacted CSV/history. No erasure or automatic restore is authorized. Keep the existing working attachment, power and selector unchanged; close the Marauder app/USB bridge/other serial owners normally. If USART is Busy, stop and report; do not disable expansion/logging or change permissions to force it.

After approval for transfer and launch, copy only this test FAP to apps/GPIO under its separate name. Launch and confirm no UART action until OK/Right on the ESP page. Press once; expect Connecting → Waiting → Connected, with version/profile displayed and USART released. On timeout, partial/wrong identity, overflow, reset/banner or error, use Back to exit, preserve sanitized result and stop without fallback commands or automatic retries. Confirm ordinary Flipper/serial access after exit and inspect only redacted session records. No help, Wi-Fi/BLE scan, stopscan, RF transmission, firmware update or power/reset action belongs to this test. Deployment, execution and rollback file operations are still unapproved. No GitHub Release is produced.
