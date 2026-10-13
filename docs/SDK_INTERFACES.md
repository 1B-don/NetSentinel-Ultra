# Firmware-facing build requirements

Target: installed Momentum d3f89dfe2ef6b01839201598e9be1590cba80322. No restricted SDK/source was accessed. This is an inventory of names used by the independent application, not confirmation that the target exports them. Current public [viewport](https://developer.flipper.net/flipperzero/doxygen/view__port_8h.html) and [serial](https://developer.flipper.net/flipperzero/doxygen/furi__hal__serial_8h.html) docs support their general contracts but do not verify that revision's ABI.

## Existing compile dependencies

| Boundary | Header | Required names |
|---|---|---|
| Records/time | furi.h | furi_record_open, furi_record_close, furi_get_tick, furi_ms_to_ticks; RECORD_GUI, RECORD_STORAGE |
| Mutex | furi.h | furi_mutex_alloc/acquire/release/free; FuriMutex, FuriMutexTypeNormal, FuriStatusOk, FuriWaitForever |
| Queue | furi.h | furi_message_queue_alloc/put/get/free; FuriMessageQueue |
| Input | input/input.h | InputEvent, InputKeyUp/Down/Left/Right/Ok/Back, InputTypeShort/Repeat/Long |
| GUI | gui/gui.h | Gui, GuiLayerFullscreen, gui_add_view_port, gui_remove_view_port |
| Viewport | gui/gui.h transitive viewport header | ViewPort, view_port_alloc/free, view_port_draw_callback_set, view_port_input_callback_set, view_port_enabled_set, view_port_update |
| Canvas | GUI headers | Canvas, FontSecondary, canvas_clear/set_font/draw_str/draw_line |
| Storage | storage/storage.h | Storage, File, FS_Error, FSE_OK/EXIST/NOT_EXIST; FSAM_READ/WRITE; FSOM_OPEN_EXISTING/CREATE_ALWAYS/CREATE_NEW; storage_common_mkdir; storage_file_alloc/free/open/close/read/write/size/sync/get_error |
| Serial controller | furi_hal_serial_control.h | furi_hal_serial_control_acquire/release; FuriHalSerialIdUsart |
| Serial HAL | furi_hal_serial.h | FuriHalSerialHandle, FuriHalSerialRxEventData/FrameError/NoiseError/OverrunError/ParityError; furi_hal_serial_init/deinit/tx; furi_hal_serial_async_rx_start/stop/available and furi_hal_serial_async_rx |
| C runtime/compiler | stdlib, stdio, string, ctype, errno, stdatomic | allocation, string/number formatting/parsing, lock-free unsigned atomics and bool atomics; 64-bit integer division used in time/plots |

Names grouped with slash notation expand to their literal common prefix. Use exported API declarations from an appropriately authorized compatible SDK to validate argument/return types, enum values, struct layout and link exports. Independent mocks are intentionally incomplete and may model recoverable errors where an SDK actually asserts or aborts on allocation failure. Verify whether callback removal/GUI unregister synchronizes in-flight callbacks before freeing context. UART ISR ring assumes lock-free atomics; its TX helper lacks a HAL deadline/error result. No real device concurrency/heap/stack check has run.

The candidate manifest uses external App type, netsentinel_app entry, 8 KiB stack, core/Flipper source globs and development metadata. `tests`, `legacy`, SDK mocks and logs must never enter its compile sources. The current default and board-enabled variants are host-compiled separately with warnings as errors.

## Board adapter boundary

Optional NS_ENABLE_BOARD_PORT enables `bool ns_flipper_board_bind(NsBoardPort*)` and `void ns_flipper_board_unbind(NsBoardPort*)`. Bind may populate only that object and must perform no hardware acquisition. Partial bind failure must be safe to unbind. Advertised NRF requires acquire/release/spi/ce/delay_us. Advertised CC requires acquire/release/cc_probe/cc_receive/cc_rssi/cc_idle; cc_diagnose is optional. Incomplete callbacks fail initialization before GUI launch. Callback context belongs to the board; parent GUI/storage/UART contexts remain untouched.

The parent adapter dispatches acquisition/release and checks a single owner across UART, NRF and CC. Acquisition failures must unwind any resources acquired within the callback. release must make the device/bus/pins safe even after failed register operations. Wrong-owner SPI/CC calls fail; wrong-owner release cannot release another peripheral. Deinit releases ownership before unbinding. Board tests supply mocks only, not a deployable port.

**NRF unresolved:** exact permitted Ultra CE/CS routing, explicitly external shared SPI handle/locking, chip-ready/transfer timeout semantics and supported GPIO configuration/restoration declarations. These firmware-facing names are intentionally not invented. Portable radio operations already expose only primary RX, no auto-ACK/retries or payload TX.

**CC unresolved:** authorized external-device enumeration/selection, plugin/device lifecycle, receive-only preset/filter configuration, frequency/calibration/settling, validated RSSI, idle and raw status transport. No global auto-selection or internal fallback is assumed. Public SPI and external-device documentation requests failed in this session; no guessed exports were added. Owner-reported prior external operation is separate from these unresolved application bindings.

**ESP unresolved:** installed localized info labels, Wi-Fi Recon raw sequence/output, passive/channel policy, physical UART timing and target serial exports. Verification capability stays false; no active BLE scanning or attack commands are exposed.

## Next permitted compilation step

Obtain the compatible SDK's permitted exported headers/API table and supported external-app build instructions. Review its instructions before using it. Compile with the candidate manifest through its supported build-only workflow, retain compiler/linker output locally, and fix concrete mismatches here. No deployment/launch without explicit approval. No FAP, hardware verification or release is claimed from the host builds.

The Flipper NRF dispatch validates each two-byte transfer before forwarding to the board: only scanner register reads/writes, primary RX configuration and channel values 0–125 are accepted. The guard adds no firmware symbols. It complements external-port locking, CS handling and cleanup; it does not supply the missing external SPI/GPIO binding.
