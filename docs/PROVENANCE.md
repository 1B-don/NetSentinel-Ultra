# Imported source

Imported NetSentinel Ultra v0.2 from the owner-provided Downloads directory. Original manifest attributes Brandon Midgette. No explicit license file was supplied; redistribution licensing remains unresolved. No Momentum source, SDK, toolchain, NRF24 reference, build scripts, workflow, or prebuilt FAP was copied. The imported README is historical, not current verification evidence.

## Independently authored integration sources

`src/core`, `include/netsentinel` and the mock tests were independently written for this project; no standalone NRF24 code was imported because the available local reference is in the restricted checkout. The owner must resolve distribution licensing for imported v0.2 code before publishing a release.

Hardware/interface references (documentation consulted, no code copied):

- Sacred Labs Ultra V3 guide: https://sacredlabstech.com/boards/ultra-v3/ — physical radio selector; no verified CE/CS pin mapping obtained.
- Nordic receive timing discussion: https://devzone.nordicsemi.com/f/nordic-q-a/36802/carriers-scan-procedure-with-nrf24l01/142008 — RPD latching and receive settling. Driver uses a minimum 170 microseconds, default 200; RF timing remains unverified.
- Nordic register specification: https://devzone.nordicsemi.com/cfs-file/__key/communityserver-discussions-components-files/4/content.pdf — RX control and automatic acknowledgment registers; genuine L01+ identification remains pending.
- TI CC1101 datasheet: https://www.ti.com/lit/ds/symlink/cc1101.pdf — supported frequency bands. Driver delegates modem setup/calibration/RSSI conversion to the authorized external SDK adapter.
- ESP32 Marauder CLI reference: https://github.com/justcallmekoko/ESP32Marauder/wiki/CLI — command capabilities are firmware-dependent; no passive profile derived or enabled.
- Flipper external-app documentation: https://developer.flipper.net/flipperzero/doxygen/apps_on_sd_card.html — manual packaging requires concrete SDK integration and API compatibility.

## Application shell and persistence work

The candidate Flipper shell follows the owner-provided legacy GUI lifecycle, with independently authored controller/storage integration. API names and callback usage were checked against public Flipper [storage](https://developer.flipper.net/flipperzero/doxygen/storage_8h.html), [viewport](https://developer.flipper.net/flipperzero/doxygen/view__port_8h.html) and [manifest](https://developer.flipper.net/flipperzero/doxygen/app_manifests.html) documentation. No SDK headers or repository code were copied. `tests/flipper_mock` contains independently authored, intentionally incomplete contract shims. They cannot prove real SDK compilation or ABI compatibility.

## Marauder protocol work

The independent UART state machine and shims use observable protocol facts from the tagged v1.11.0 [CommandLine.cpp](https://raw.githubusercontent.com/justcallmekoko/ESP32Marauder/v1.11.0/esp32_marauder/CommandLine.cpp) and [WiFiScan.cpp](https://raw.githubusercontent.com/justcallmekoko/ESP32Marauder/v1.11.0/esp32_marauder/WiFiScan.cpp), and the public [serial HAL documentation](https://developer.flipper.net/flipperzero/doxygen/furi__hal__serial_8h.html). No upstream implementation/header files were copied or vendored. Serial-controller calls also appear in the owner-provided permitted legacy source. Fixtures are independently written synthetic examples; identity labels are provisional. Source inspection does not prove the installed binary or Momentum ABI.

## CC1101 status diagnostics

The independently authored status-only SPI helper and simulated transport are based on the public TI CC1101 [datasheet](https://www.ti.com/lit/ds/symlink/cc1101.pdf), sections on SPI status access, chip identity, main state and RSSI. No firmware, external-radio plugin source, SDK headers or hardware pin mapping was imported. User-reported prior CC1101 operation does not establish NetSentinel reception.
