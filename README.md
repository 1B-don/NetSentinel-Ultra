# NetSentinel Ultra v0.2 (Momentum)

A **developer preview** of a Flipper Zero GPIO tool designed for **Sacred Labs FlipMods Ultra V3** with an ESP32 running **ESP32 Marauder**. Source-only unless the matching Momentum firmware SDK is available. This project was host-tested but **has not been built into a genuine .fap or tested with real hardware**.

## Real capabilities

| Menu | Implemented | What it does |
| --- | --- | --- |
| Wi-Fi Explorer | Yes, UART/Marauder dependent | Sends `scanap`, `stopscan`, `list -a`, parses known AP observation layouts. Press Right for SSID/BSSID/channel/RSSI. |
| Channel Analyzer | Yes | Counts detected APs by channel 1–14. Not a spectrum analyzer, signal occupancy or interference meter. Scan with OK. |
| Client Observations | Limited, UART/Marauder dependent | Sends `scansta`, `stopscan`, `list -c` and displays raw response. It **does not** claim to identify or classify every station. |
| Bluetooth Monitor | Limited, UART/Marauder dependent | Sends `sniffbt`, `stopscan`, displays received raw lines. Does not claim to decode or enumerate BLE devices. |
| Network Baseline | Yes | Stores up to 48 AP observations in RAM. Reports new/missing BSSIDs and channel changes if BSSID provided. Saved only during current app run. |
| Export Wi-Fi CSV | Yes, Flipper SD required | Saves observed APs into `/ext/apps_data/netsentinel/scan_001.csv` (increments file number). Does not save BLE/client raw lines. |
| ESP32 Information | Yes, UART/Marauder dependent | Sends `info`, shows recent response text. |
| UART Diagnostics | Yes | Displays recent raw serial lines, dropped bytes and overlong lines. |
| Ultra Hardware | Informational only | Documents which Ultra module to select and explains driver limitations. **No NRF24 or CC1101 hardware scanning yet.** |

This version only transmits the allowlisted, non-injection Marauder commands `scanap`, `scansta`, `sniffbt`, `stopscan`, `list -a`, `list -c`, and `info`. No deauthentication, spoofing, credential capture, packet injection, or attack automation is included.

## Hardware restrictions

- Ultra V3 ESP32-WROOM-32UE works at **2.4GHz** Wi-Fi; it cannot see 5/6GHz networks. The ESP32 remains powered with either mechanical switch position.
- **NRF24 / CC1101** are **not** operated by this version. Sacred Labs documents the physical switch for selecting between them. Use existing compatible Momentum applications for those modules until direct drivers, safety boundaries, and GPIO mapping are verified.
- The app assumes **Flipper USART GPIO 13/14 at 115200 baud**. On alternate boards/versions, UART routing may differ.
- The Marauder version on the ESP32 determines which commands are supported and how output is formatted. Run `help` in its established companion app to check your unit. Only the supported known AP output formats populate Wi-Fi Explorer; review UART Diagnostics if APs do not appear.
- Passive radio reception and scan results can contain identifiers belonging to nearby networks and devices. Use where lawful and with appropriate permission.

## Build a .fap on Kubuntu

1. Ensure your Momentum firmware version is known. The `.fap` must be ABI-compatible with the firmware installed on the Flipper.
2. Install requirements and clone matching Momentum source:

```sh
sudo apt update
sudo apt install -y git python3 build-essential
cd ~/Downloads
git clone --recursive https://github.com/Next-Flip/Momentum-Firmware.git
```

If your installed Momentum release is older/newer than the cloned branch, select a corresponding tag/commit and initialize its submodules before building.

3. Extract the NetSentinel ZIP and run:

```sh
cd ~/Downloads/NetSentinel-Ultra-v0.2
./build_momentum.sh ~/Downloads/Momentum-Firmware
```

On success, this script places `netsentinel.fap` **in the NetSentinel-Ultra-v0.2 folder**. The build system downloads its ARM toolchain automatically on first use; Internet is required.

4. In qFlipper file manager, put `netsentinel.fap` in `/ext/apps/GPIO/` (displayed as `SD Card/apps/GPIO/`). Open **Apps → GPIO → NetSentinel Ultra**. This installation does not update Momentum itself.

## Optional: build a downloadable .fap with GitHub Actions

The project also contains `.github/workflows/build-fap.yml`. If you upload the **contents** of this project to a GitHub repository, open **Actions → Build NetSentinel Ultra FAP → Run workflow**, enter a `momentum_ref` matching your installed Momentum build, and download the Actions artifact after the build completes. The artifact contains `netsentinel.fap`. GitHub builds this using Momentum's actual SDK, not a locally fabricated file. **The workflow has not been executed or hardware-validated here.**

Do not use `dev` if your Flipper runs a particular older stable tag; prefer its matching release tag/commit.

## Navigation

- Home: **Up/Down** choose, **OK** enter, **Back** quit.
- Wi-Fi: **OK** start/stop; **Up/Down** select access point; **Right** details; **Back** home.
- Channel analyzer: **Up/Down** choose 1–14; **OK** start/stop a Wi-Fi scan.
- Client/Bluetooth monitor: **OK** start/stop; **Back** stops a scan and exits.
- Network baseline: **OK** copies current Wi-Fi scan to RAM. To compare, return to Wi-Fi Explorer and capture a second full scan, then view baseline page again. Missing results may simply mean out of range, not an attack.
- CSV export: stop Wi-Fi scan, open Export, press **OK**. Inspect `scan_###.csv` under `SD Card/apps_data/netsentinel`.
- Device info: **OK** requests latest Marauder `info` output.
- Ultra hardware: **Up/Down** ESP32 / NRF24 / CC1101 information.

## Limitations & diagnostics

1. If `UART BUSY`, exit other UART/Marauder applications. This FAP temporarily disables Expansion service while it acquires USART; it restores Expansion when quitting.
2. If `NO REPLY`, check that the board is correctly seated and powered, firmware is configured, and the ESP32 UART pins are routed as expected. `RX OK` means only that some bytes arrived.
3. If RX lines arrive but AP count is zero, use **UART Diagnostics** to see whether your Marauder version emits a format the parser doesn't support.
4. A snapshot is limited to **48 APs**; additional networks are not stored. Scans are not continuous monitoring and cannot prove access-point disappearance or malicious behavior.
5. Client/Bluetooth output is raw and may differ substantially by ESP32 Marauder version.
6. App does not log precise location, decrypt Wi-Fi packets, or identify individual people.
7. The companion Marauder hardware/firmware is not changed by the app; normal scan state is stopped on exit.

## Source tests

```sh
./test_host.sh
```

Tests validate the parser, channel counts, baseline matching/channel changes and CSV quoting; source-level host tests are **not** proof of Momentum ABI compatibility or RF behavior.

## Future work

- A safer, device-specific CC1101 **receive-only** scanner using verified external CC1101 driver and board switch states.
- NRF24 passive channel observations with a hardware-verified SPI implementation, separate from Wi-Fi AP counts.
- Reliable Bluetooth device-list parser per Marauder firmware format.
- Persistent baselines, opt-in export filters, and on-device comparisons.
- Hardware integration test with the owner's Ultra board, then a firmware-specific `.fap` release.

## References

- Sacred Labs Ultra V3: https://sacredlabstech.com/boards/ultra-v3/
- ESP32 Marauder wiki CLI: https://github.com/justcallmekoko/ESP32Marauder/wiki/CLI
- Momentum Firmware: https://github.com/Next-Flip/Momentum-Firmware
- Flipper FAP build: https://developer.flipper.net/flipperzero/doxygen/apps_on_sd_card.html
