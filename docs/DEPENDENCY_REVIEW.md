# Receive-only dependency review

Read-only public-source research; no source checkout, firmware operations, hardware access or third-party implementation import. SDK declarations/export metadata are from the supplied permitted d3f89dfe distribution. Public driver behavior below is a reference, not proof of installed Momentum plugin behavior.

## Ultra V3 routes

The [manufacturer Ultra V3 guide](https://sacredlabstech.com/boards/ultra-v3/) and [component guide](https://sacredlabstech.com/guides/components/) describe modules, connector and selector. No authoritative SPI/CE/CSN/GDO net mapping was located in those guides or targeted public searches. Secondary generic pin tables do not establish this PCB's wiring and were not used. UART TX13/PB6, RX14/PB7 remains independently owner-verified plus checked against SDK direction APIs; that does not prove SPI routing.

Needed: manufacturer schematic/netlist for this board revision covering MOSI/MISO/SCK, NRF CE/CSN, CC CS/GDO0/GDO1/GDO2, selector power/bus routing, pullups/level circuitry and front-end controls. The smallest next action is to obtain that document or a manufacturer-confirmed signal table. Do not infer continuity from photographs. Power-off continuity measurement is an alternative only under separately approved owner hardware work, not part of this software milestone. No message to the manufacturer was sent.

## Marauder v1.11.0 lifecycle

Reviewed tag v1.11.0, public commit **2ff3716e2307f8a1c3d2503936728b8cbc18acd8**. References: [CLI](https://github.com/justcallmekoko/ESP32Marauder/blob/2ff3716e2307f8a1c3d2503936728b8cbc18acd8/esp32_marauder/CommandLine.cpp), [command names](https://github.com/justcallmekoko/ESP32Marauder/blob/2ff3716e2307f8a1c3d2503936728b8cbc18acd8/esp32_marauder/CommandLine.h), [WiFi lifecycle](https://github.com/justcallmekoko/ESP32Marauder/blob/2ff3716e2307f8a1c3d2503936728b8cbc18acd8/esp32_marauder/WiFiScan.cpp), [buffer storage](https://github.com/justcallmekoko/ESP32Marauder/blob/2ff3716e2307f8a1c3d2503936728b8cbc18acd8/esp32_marauder/Buffer.cpp), [settings defaults](https://github.com/justcallmekoko/ESP32Marauder/blob/2ff3716e2307f8a1c3d2503936728b8cbc18acd8/esp32_marauder/settings.cpp), [status labels](https://github.com/justcallmekoko/ESP32Marauder/blob/2ff3716e2307f8a1c3d2503936728b8cbc18acd8/esp32_marauder/lang_var.h).

| Phase | Verified upstream fact | Software consequence / unresolved evidence |
|---|---|---|
| Input/framing | CLI trims LF-terminated input, echoes with #, prints prompt after dispatch | Bounded correlated echo/prompt parser; no implicit command acceptance |
| Info | Reports identity/status and changes local display mode | Existing live info-only allowlist preserved; query completion is not open USART or proof of radio silence |
| Receive start | sniffbeacon dispatches WIFI_SCAN_AP to RunBeaconScan; initializes NULL-mode promiscuous receive | Candidate reception path, not blanket installed-binary passivity proof; prior connection/background state and ignored initialization error returns need review |
| Output | Beacon callback prints RSSI, channel, source MAC and ESSID | Strict AP parser; names/MAC stay only in RAM, numeric/redacted CSV/UI; no encryption field inferred |
| Capture storage | Beacon start opens PCAP handling; Buffer checks SavePCAP; settings factory default is true | Live observations blocked until installed state/storage effects confirmed. False disables buffer file/serial capture; omitting -serial alone does not prevent SD capture |
| Stop | Plain stop dispatches OFF and reports completion; shutdown depends on current/connected state | Host candidate requires echo, stop message and prompt. A prompt/port release alone is not RF-stop proof; lost-link abort cannot guarantee ESP stop |
| Other discovery | scanap uses another NULL-mode receiver callback in this tag; generic active-scan names are not proof either way | Not automatically classified as active just from its name; not enabled without exact installed lifecycle and output review |
| BLE | Bluetooth scan path enables active discovery | Excluded; receive-only support is not established |

Source labels explicitly include disabled WSL and no-SD with zero size; the information parser now accepts these exact variants with consistency checks. Connected card with zero size or disconnected card with positive size fails. Complete identity is required before the host-only observation state machine can advance. No upstream code was copied into implementation/fixtures. Synthetic fixtures use invented identifiers/status data.

Version/profile/IDF strings do not establish installed source/build options or persistent SavePCAP state. Needed: manufacturer build/source provenance corresponding to the installed binary, plus sanitized facts about capture configuration and absence of pre-existing association/background transmission. Do not publish complete settings dumps, credentials, identifiers or packets. No settings query/change or new command was sent. Existing hardware operation stays exactly one info+LF per acquisition.

## External CC1101 SDK and public reference

Matching SDK exports provide device registry/name lookup, begin/end, connection detection, preset/frequency selection, RX/idle and RSSI. See SDK_INTERFACES.md. A receive-only wrapper could eventually use a specifically reviewed external device with no internal fallback, but exporting these function names does not prove bounded or passive initialization.

Reviewed [official Flipper 1.4.3 external driver](https://github.com/flipperdevices/flipperzero-firmware/blob/1.4.3/applications/drivers/subghz/cc1101_ext/cc1101_ext.c), not Momentum source. It chooses the standard external SPI handle and PB2 as its GDO0-related pin. This is a generic driver's wiring expectation, not a verified Ultra pin mapping. Presence checking before allocation can initialize/reset and manipulate GDO pins; it is not a read-only probe. Frequency calibration polls indefinitely; SPI acquire has no deadline parameter. RX/idle transitions use fatal checks rather than recoverable result codes. The initialized presence branch rejects part number zero, whereas NetSentinel's status helper follows the datasheet and accepts part zero with a valid version. These reference behaviors must not be assumed identical on Momentum or wrapped with a fictitious timeout.

Responsibly implemented now: bounded status-read protocol, identity/state/RSSI checks, receive contract, tune/trace/activity CSV and a read-only MCSM1 guard. The guard rejects automatic RXOFF toward TX and conservatively also FSTXON, accepting only idle/stay-RX. FSTXON is synthesizer readiness, not transmission itself. Two consistent reads are required; all 256 register values and transfer failures are tested. This verifies one configuration condition, not all passive behavior. It does not initialize or bind a physical device and is not invoked on hardware. [TI datasheet](https://www.ti.com/lit/ds/symlink/cc1101.pdf) is the register/state reference.

Needed: independently permitted installed external-plugin implementation/identity, exact board routing and a bounded acquisition/init/calibration/receive/idle/cleanup provider with reviewed preset/automatic-state policy and RSSI calibration. A bounded independently authored provider is an authorized alternative after routes and configuration are supplied. An outer timer cannot interrupt a blocking SDK call safely. No plugin, registry or SPI imports are added to the current FAP.
