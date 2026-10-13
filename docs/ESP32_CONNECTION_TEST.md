# ESP32 information-only connection test — not authorized to execute

**Owner update:** USB-C/CH340 communication at115200 succeeds with Marauder v1.11.0/v4 and IDF v5.5.1-710-g8410210c9a; see ESP32_PREPARATION.md. This satisfies direct USB status evidence only. Exact header wiring and bridge test approval remain unresolved. The agent did not perform this connection. Raw identity-address fields must not be retained in future logs or fixtures; use sanitized field/framing observations only.

## Evidence and unresolved wiring

Owner front/back photographs visually confirm the Ultra V3 identity, ESP32-WROOM-32UE, NRF24, CC1101, USB-C, microSD, RST/BOOT buttons, selector and18-pin header. This is owner-confirmed component/layout evidence; photographs were not supplied as files in this workspace and the agent does not claim personal trace inspection. Components/layout do not prove hidden copper connectivity, level shifting or the exact board revision netlist.

Manufacturer sources reviewed: [Ultra V3 guide](https://sacredlabstech.com/boards/ultra-v3/), [component guide](https://sacredlabstech.com/guides/components/), [update guide](https://sacredlabstech.com/guides/marauder-update/), and linked shop/store pages. No inspected authoritative Ultra V3 UART schematic, net-assignment table or circuit diagram was found. Store access was limited; an image download failed on DNS. Search photo descriptions and other-generation Ultra V1/Combo boards are not wiring evidence. Manufacturer update advice is not authorization to flash.

| Required fact | Verified now | Before physical test |
|---|---|---|
| Board/modules/header/controls | Owner photographs; manufacturer guide agrees | Match exact physical revision |
| Flipper TX output → ESP RX input net | Not established | Manufacturer/board-specific schematic or explicit verified net mapping |
| ESP TX output → Flipper RX input net | Not established | Same, including connector numbering and ESP endpoint |
| UART logic voltage |3.3V platform/module electrical baseline; actual Ultra circuit unknown | Confirm actual signal levels and intervening circuitry |
| Power/ground/regulator/backfeed path | Unknown Ultra circuit | Confirm intended attached-board power arrangement without changing power |
| Switch coupling to UART | Manufacturer says ESP powered regardless of NRF/CC selector; data-line isolation unknown | Confirm selector does not switch/reset UART or require a position |
| Level shifter, resistors, mux, USB-UART driver sharing | Unknown | Verify devices/nets, direction and output contention |
| RST/BOOT/DTR/RTS coupling | Controls visually identified, nets unknown | Establish bridge extra outputs cannot actuate them |

Generic USART13/14, LPUART15/16 and ESP32 UART0 assignments are **not assigned to the Ultra nets in this document**. [Upstream Flipper integration guide](https://github.com/justcallmekoko/ESP32Marauder/wiki/Flipper-Zero) confirms control over UART through compatible ESP32 boards, but covers other targets and supplies no Ultra V3 netlist. Do not reflash the owner's v4-profile firmware to follow its generic `_flipper.bin` instructions. [Flipper electrical documentation](https://docs.flipper.net/zero/gpio-and-modules) describes3.3V GPIO; the [Espressif module datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-wroom-32e_esp32-wroom-32ue_datasheet_en.pdf) describes module supply/DC limits. Neither proves board-specific shifting. No5V UART input, loose jumpers, new power feed or rewiring is proposed.

## Minimal status exchange

Pinned upstream evidence: [v1.11.0 CLI](https://raw.githubusercontent.com/justcallmekoko/ESP32Marauder/v1.11.0/esp32_marauder/CommandLine.cpp), [command definitions](https://raw.githubusercontent.com/justcallmekoko/ESP32Marauder/v1.11.0/esp32_marauder/CommandLine.h), [language table](https://raw.githubusercontent.com/justcallmekoko/ESP32Marauder/v1.11.0/esp32_marauder/lang_var.h), and [info implementation](https://raw.githubusercontent.com/justcallmekoko/ESP32Marauder/v1.11.0/esp32_marauder/WiFiScan.cpp). Startup uses115200 baud; input ends with LF. Expected command echo is `#info` or `#help` on a line, then response lines, then `> ` without a terminating newline. Tagged labels are `Version:`, `Hardware:` and `ESP-IDF:`. Help lists command descriptions, including disallowed commands; their presence must never cause execution.

Synthetic expected identity, not a hardware capture:

```text
#info
Version: v1.11.0
Hardware: Marauder v4
ESP-IDF: v5.5.1-710-g8410210c9a
>[space]
```

`[space]` denotes the literal trailing space in the unterminated prompt. Actual info may include headings, memory, MAC and card/battery fields between these lines. Installed status fields are now owner-confirmed; exact echo/prompt framing/help still need confirmation. MAC/identifier lines must be omitted entirely from retained captures. Bare `info` reads identity but changes Marauder's internal display/scan-mode state; it is not side-effect-free. Do not run it while another operation is active. No arguments, no `info -a`, Wi-Fi/BLE scan, stopscan, settings, reset/update or attack command is part of this test. Minimal first exchange is exactly5 UART bytes `info` + LF. Only after its complete response may an optional second5-byte `help` + LF be sent under the same approval. These are serial control bytes, not RF transmission commands. Existing NetSentinel info mode does not implement an independent help transaction; this optional manual bridge test is distinct.

## Bridge suitability — conditional, not established safe for this board

[Flipper documentation](https://docs.flipper.net/zero/gpio-and-modules) provides GPIO → USB-UART Bridge. It can forward terminal bytes through the existing header without a new FAP. It **enables electrical GPIO/UART access**, so current permission does not authorize opening/using it.

Read-only review of the permitted [official1.4.3 bridge source](https://github.com/flipperdevices/flipperzero-firmware/blob/1.4.3/applications/main/gpio/usb_uart_bridge.c) identifies UART selection, USB CDC changes, optional RTS/DTR GPIO outputs and a separate software DE/RE output. Flow control can drive pin pairs2/3,6/7 or16/15; DE/RE uses PA4 and cleanup changes its mode even when optional control outputs are disabled. The source acquires/releases serial and changes USB configuration. It is not proof of installed Momentum bridge implementation, defaults, power effects or complete pin-state restoration. No source was copied or patched.

Therefore bridge is a **candidate** once exact Ultra nets and installed bridge settings/extra-pin effects are reviewed, not a workaround for unknown wiring. A bridge timeout/error recovery is not assumed bounded by the NetSentinel mocks. If those facts cannot be verified, do not use it. Direct board USB-C is not proposed as an alternative because auto-reset, USB-UART sharing/backfeed and control-line coupling are unverified.

## Preconditions — all required before approval request

1. Obtain board-specific TX/RX endpoints/directions, logic levels, ground/power arrangement, switching and control-line information listed above. No GPIO or powered probing is authorized now. Manufacturer confirmation or documentation is the smallest preferred input; unpowered continuity work would be a separately authorized alternative.
2. Confirm the installed bridge's UART selection matches that route. Confirm a way to keep flow/control lines disabled and software DE/RE off from entry onward, including default/cleanup extra-pin effects. If it cannot be confirmed before entry, stop rather than open it to discover defaults on the attached board.
3. Board is already in its intended, stable attachment/power arrangement, with ESP idle and no other app/terminal controlling it. Do not energize, attach/detach, toggle selector, press RST/BOOT or connect board USB-C. Any necessary selector/power/connection change requires a separate owner instruction.
4. Record existing USB/bridge/serial settings and a plan to exit/restore them without touching unrelated state. Close competing USB clients voluntarily; no permission changes or forced ownership takeover.
5. Prepare terminal at115200,8N1,no hardware/software flow control, LF-only, no automatic commands/uploads/DTR/RTS-reset actions. Confirm installed ESP framing rather than treating8N1 as already device-verified. Keep capture local, bounded to4096 bytes per response and five seconds per command; abort on exceeding either. Avoid terminal macros or automatic retries.
6. Request explicit owner approval for this precise bridge-only hardware query and its accepted temporary USB/UART changes. This document and prior GUI-test results are not approval. No NetSentinel hardware-enable/FAP deployment is included.

## Procedure after those preconditions and explicit approval

Open the verified bridge configuration; locate the resulting USB serial endpoint without guessing a fixed device name. With approved UART/control options, send only `info` + LF once. Require the matching echo, complete identity fields and final prompt within the five-second/4096-byte bound. Record only sanitized firmware/profile/IDF values and framing observations in project documentation. A nonempty reply proves bytes traversed the chosen interface, not exact PCB net identity or passive RF behavior.

If approved and the first exchange succeeds, send `help` + LF once and require its echo/list/prompt within the same bound. Never invoke listed commands. Close terminal and exit bridge; restore recorded settings if that restoration was included in approval, verify ordinary USB access and clean exit. Failure to restore is a failed test, not permission to reset/flash. No session scans or RF captures are expected. Passing does not verify NetSentinel UART, AP discovery, BLE or any radio driver.

Stop immediately on unexpected reset/banner, active scan/packet output, garbled/NUL/oversized response, missing echo/fields/prompt, wrong version/profile, timeout, disconnect, busy ownership, unexpected power/USB behavior or inability to keep extra GPIO controls off. Close terminal and exit via ordinary UI only; do not send stopscan/reboot, retry automatically, switch power/selector, change baud repeatedly or flash to recover. Report the bounded failure and preserve local evidence.
