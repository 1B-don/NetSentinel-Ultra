# SDK adapter contract

Implement `NsPlatform` in an appropriately authorized SDK outside the portable core. All callbacks execute on the application worker, not draw or interrupt callbacks. SPI/UART/storage must use bounded waits and propagate failures. An RX interrupt may only enqueue bytes; the worker calls `ns_session_feed`. Protect dashboard snapshots with the SDK mutex. Allocation failures must unwind in reverse order.

Acquire must fail if a peripheral is busy, avoid internal-radio fallback, track original power/expansion state, and restore it on release. Shared external SPI must be exclusively owned; users must confirm the physical selector before a session. NRF CE/CS routing is unresolved: do not guess pins. Probe success is register readback evidence, not proof of genuine Nordic silicon or RF function. CC callbacks must validate external presence, configure RX-only modem settings, calibrate and settle after frequency changes, validate RX state before RSSI, and idle/sleep on cleanup. Never use TX/auto-ACK/payload transmission APIs.

The installed ESP32 firmware version and passive command semantics are unknown. The default profile is NULL (unsupported). Do not enable a profile simply because a command name appears in `help`: verify its implementation and confirm it never invokes active Wi-Fi/BLE scans. Profile strings must be immutable application constants, never arbitrary UI input. BLE observations currently retain raw lines; device enumeration needs firmware-specific captured fixtures.

No concrete SDK adapter is verified or supplied yet. The legacy v0.2 SDK entry point is archived and excluded from builds. Portable host tests do not verify Flipper ABI, GPIO routing, power, radio reception, SD hardware or firmware cleanup.

## Current application and storage port

`src/flipper/netsentinel_app.c` supplies the candidate worker/GUI entry point. Input callbacks enqueue keys; draw reads a protected published `NsFrame`. I/O occurs outside the draw mutex. A tick-delta accumulator converts SDK ticks to elapsed milliseconds using `furi_ms_to_ticks(1000)`, preserving fractional ticks and normal unsigned tick wrap.

`src/flipper/ns_flipper.c` supplies fixed-path SD state and exclusive CSV operations. Public storage API calls are host-compiled against independent shims; real SDK declarations/exports and SD behavior must still be checked. `NsStore` has independent bounded snapshot handles; `NsPlatform` has one CSV handle. Close errors now return `NsResult`, and session history records failed sync/close or NRF/CC shutdown attempts.

The default radio acquire returns Unsupported and both capability flags are false. Optional `NS_ENABLE_BOARD_PORT` hooks are declared in `src/flipper/ns_flipper.h`. A bind implementation may install radio callbacks and board context/capabilities only, must acquire no pins/radio/power during bind, and must support partial-bind cleanup. Do not advertise capabilities without supported callbacks and verified wiring. Capability flags mean a port is installed, not successful hardware detection; presence probes still run after user selector confirmation. ESP32 scanning stays disabled in the controller regardless of these flags.

`release` remains a required final safety operation after best-effort hardware idle/power-down; it must restore bus/pins even if register commands fail. It currently has no return status, so a port unable to guarantee restoration must extend/report that contract before claiming verified cleanup. UART ingress, queue overflow reporting and original expansion/power restoration belong to a future authorized ESP port.
