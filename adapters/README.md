# SDK adapter contract

Implement `NsPlatform` in an appropriately authorized SDK outside the portable core. All callbacks execute on the application worker, not draw or interrupt callbacks. SPI/UART/storage must use bounded waits and propagate failures. An RX interrupt may only enqueue bytes; the worker calls `ns_session_feed`. Protect dashboard snapshots with the SDK mutex. Allocation failures must unwind in reverse order.

Acquire must fail if a peripheral is busy, avoid internal-radio fallback, track original power/expansion state, and restore it on release. Shared external SPI must be exclusively owned; users must confirm the physical selector before a session. NRF CE/CS routing is unresolved: do not guess pins. Probe success is register readback evidence, not proof of genuine Nordic silicon or RF function. CC callbacks must validate external presence, configure RX-only modem settings, calibrate and settle after frequency changes, validate RX state before RSSI, and idle/sleep on cleanup. Never use TX/auto-ACK/payload transmission APIs.

The installed ESP32 firmware version and passive command semantics are unknown. The default profile is NULL (unsupported). Do not enable a profile simply because a command name appears in `help`: verify its implementation and confirm it never invokes active Wi-Fi/BLE scans. Profile strings must be immutable application constants, never arbitrary UI input. BLE observations currently retain raw lines; device enumeration needs firmware-specific captured fixtures.

No concrete SDK adapter is verified or supplied yet. The legacy v0.2 SDK entry point is archived and excluded from builds. Portable host tests do not verify Flipper ABI, GPIO routing, power, radio reception, SD hardware or firmware cleanup.
