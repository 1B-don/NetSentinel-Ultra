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
