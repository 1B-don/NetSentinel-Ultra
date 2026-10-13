# Remaining blockers for an installable FAP

**Current update:** USB CLI independently confirms installed API **87.1**, target 7, d3f89dfe; earlier 89.1/mismatch blockers below are superseded. Matching permitted Momentum SDK clean build, metadata and all 49 imports pass. Current FAP is 33,480 bytes, SHA256 `de38c1c7e08336a1ab5a3ebe6dcf2865ac8aa5ce9dedb97c8f8c17c024b859b5`. See [Momentum build result](MOMENTUM_BUILD.md). All radios remain disabled; physical installation/execution are pending explicit approval. For the device procedure use this artifact/hash; former fingerprint and metadata requests are historical.

Current status: official SDK 1.4.3 compilation/link/FAP packaging now passes with hardware disabled; installed-Momentum compatibility and physical tests remain pending. See BUILD_RESULT.md. The assessment below originally started at application commit `537b31e` and describes exact-target inputs still unverified. Installed target: Momentum `d3f89dfe2ef6b01839201598e9be1590cba80322`. All evidence comes from this application, its independent tests and permitted public documentation. No restricted checkout was accessed. No compiler/link error remains in the successful official-SDK build; exact Momentum SDK compilation has not run. “Unverified” does not mean a symbol is known to be absent.

## Blocker categories

| Category | Exact outstanding dependency | Default-shell FAP build | Full radio application / verification |
|---|---|---|---|
| Missing SDK headers or types | A permitted, target-compatible generated SDK include tree and transitive headers; declarations, enum values and layouts listed below; SDK C library/compiler support | Blocking: only independent mock headers exist here | Same headers plus radio-specific declarations |
| Missing firmware exports | Target generated export table/API version, provenance and loader resolver compatibility; 43 firmware function names are currently unverified; runtime/helper symbols only known after ARM compilation | Blocking: target table absent; no confirmed missing symbol yet | New board-port exports must also be checked |
| Application manifest or linker issues | Authorized build tool, ARM toolchain/sysroot, target compile flags, linker/relocation/packaging tools and supported external-source registration; actual manifest parsing, entry symbol and import resolution | Official SDK workflow/manifest/link/package now pass; installed-Momentum compatibility unknown | Enabling board port without its two hook implementations causes unresolved references; do not enable it prematurely |
| NRF24 hardware binding | Implement bind/unbind and acquire/release/SPI/CE/delay callbacks using verified Ultra wiring and supported external HAL | Does not block default shell: capability false, hook excluded | Blocking NRF scan: no concrete board port; timing, locking and pin restoration unverified |
| CC1101 external-radio binding | Explicit external device discovery/lifecycle, RX preset/frequency/calibration/settling/RSSI/idle interfaces, optional external raw-status transport | Does not block default shell: capability false | Blocking CC operation: no concrete external port; Momentum auto-selection is user-reported, not an application binding |
| ESP32 UART binding | Candidate USART adapter exists; verify target serial declarations/exports, USART routing/ownership, baud/framing and short TX timing | Serial HAL dependencies are now excluded by NS_GUI_STORAGE_ONLY; no UART capability in the initial build | Installed info labels/echo/prompt and passive scan sequence/output pending; scan flag false, BLE discovery unsupported |
| Physical hardware verification | Approved upload/launch and device GUI/storage/resource tests, selector-confirmed radio tests and evidence in VERIFICATION.md | Does not prevent build-only work | Required before installability/functionality claims; all NetSentinel physical tests pending |

## Missing SDK headers and types

Provide permitted generated headers for `furi.h`, `gui/gui.h`, `input/input.h`, `storage/storage.h`, `furi_hal_serial.h` and `furi_hal_serial_control.h`, including their transitive viewport/canvas, RTOS, HAL and target configuration headers. A handful of manually copied declarations is insufficient to compile a trustworthy target binary. Keep `tests/flipper_mock` out of include paths.

Types/constants to validate against those headers:

- Records/time: `RECORD_GUI`, `RECORD_STORAGE`, `UNUSED`, tick units/wrap and `furi_ms_to_ticks` contract.
- Synchronization: `FuriMutex`, `FuriMutexTypeNormal`, `FuriMessageQueue`, `FuriStatusOk`, `FuriWaitForever`; callback assignment/removal synchronization.
- GUI/input: `Gui`, `GuiLayerFullscreen`, `ViewPort`, `Canvas`, `FontSecondary`, `InputEvent`, six input keys and Short/Long/Repeat event constants. Confirm the draw/input callback signatures.
- Storage: `Storage`, `File`, `FS_Error`, `FSE_OK`, `FSE_EXIST`, `FSE_NOT_EXIST`, `FSAM_READ/WRITE`, `FSOM_OPEN_EXISTING/CREATE_ALWAYS/CREATE_NEW`; exact size/count/error returns and exclusive-create semantics.
- Serial: `FuriHalSerialHandle`, `FuriHalSerialIdUsart`, `FuriHalSerialRxEvent` data/frame/noise/overrun/parity flags; ISR callback signature and error-reporting support.
- C/compiler: `stdbool.h`, `stddef.h`, `stdint.h`, `stdio.h`, `stdlib.h`, `string.h`, `ctype.h`, `errno.h`, `stdatomic.h`; C11 support and the UART lock-free unsigned atomic static assertion. Validate boolean atomics and actual generated runtime dependencies on ARM.

## Firmware export names

The preflight discovers these 43 calls in `src/flipper/*.c`. Each needs an enabled export or a verified SDK inline/macro implementation; presence alone does not validate a signature or runtime ABI. No target table has been supplied. `make sdk-preflight` correctly blocks on missing input.

```text
canvas_clear
canvas_draw_line
canvas_draw_str
canvas_set_font
furi_get_tick
furi_hal_serial_async_rx
furi_hal_serial_async_rx_available
furi_hal_serial_async_rx_start
furi_hal_serial_async_rx_stop
furi_hal_serial_control_acquire
furi_hal_serial_control_release
furi_hal_serial_deinit
furi_hal_serial_init
furi_hal_serial_tx
furi_message_queue_alloc
furi_message_queue_free
furi_message_queue_get
furi_message_queue_put
furi_ms_to_ticks
furi_mutex_acquire
furi_mutex_alloc
furi_mutex_free
furi_mutex_release
furi_record_close
furi_record_open
gui_add_view_port
gui_remove_view_port
storage_common_mkdir
storage_file_alloc
storage_file_close
storage_file_free
storage_file_get_error
storage_file_open
storage_file_read
storage_file_size
storage_file_sync
storage_file_write
view_port_alloc
view_port_draw_callback_set
view_port_enabled_set
view_port_free
view_port_input_callback_set
view_port_update
```

The default source-name mode deliberately does not check C runtime imports: allocation, formatting, string operations, `strtol`, character classification, `errno` access and compiler-generated 64-bit division/atomic helpers need the actual target compiler/linker/import resolver. The optional actual-ELF undefined-symbol mode now checks required Function/Variable import names from an owner-provided ARM nm listing, including helper imports; it does not certify types or linkage. Review the generated ELF imports with the authorized SDK tooling and resolve each one through its documented mechanism. Do not assume desktop libc or a mock-linked executable proves those imports are available.

## Manifest and linker readiness

The manifest has an EXTERNAL app, `netsentinel_app` C entry point and 8 KiB stack. Local Python syntax and source expansion select 12 distinct C files: two root parser/analysis sources, seven core sources and three Flipper sources. Tests, legacy, mock headers and build outputs are excluded. Local includes remain relative within the application. The default manifest supplies `NS_GUI_STORAGE_ONLY`, excludes UART HAL declarations/calls, and supplies neither `NS_ENABLE_BOARD_PORT` nor a board implementation.

Current public [manifest documentation](https://developer.flipper.net/flipperzero/doxygen/app_manifests.html) describes the fields in use, source masks, external-app defines and string `fap_version`; it is not the installed SDK parser. Actual manifest parsing, target flags, stack/heap requirements, symbol exports, ELF relocation handling and FAP generation remain unverified. There is no confirmed manifest defect to fix speculatively.

For the board-enabled build, add the authorized concrete implementation to the application source list and define `NS_ENABLE_BOARD_PORT` through the SDK's supported manifest configuration. It must define `ns_flipper_board_bind` and `ns_flipper_board_unbind`. Host test definitions are never production implementations. Build and verify the default shell first to isolate baseline SDK mismatches, then compile the board-enabled variant; a shell-only artifact does not complete v1.0.

## Radio inputs needed after baseline compilation

NRF: verified Ultra V3 schematic/pin table (CE, CS, SCK, MOSI, MISO, selector and voltage/power behavior), permitted external SPI/GPIO handle and configuration declarations, ownership/locking semantics, finite transfer timing, CE timing and restoration behavior. Use bounded full-duplex transactions with CS deasserted on every error. The driver already enforces primary RX, disabled auto-ACK/retries and no payload/TX/carrier commands. RF settling/RPD accuracy still requires physical tests.

CC: permitted external-device APIs and required plugin/device packaging/lifecycle, exact external device identity/pin routing, device busy/presence behavior, RX-only preset/filter choices, frequency validity/calibration/settling, signed RSSI conversion and idle/restoration guarantees. Never select an internal fallback or trust a global auto-selection. Optional `cc_diagnose` needs supported bounded status SPI access. Both explicit device selection and resource cleanup must work on the installed build; status-only helper tests do not establish a radio binding.

ESP: verify the configured USART is the Ultra UART connection, 115200 8N1 and shared peripheral/expansion ownership. The current adapter refuses busy acquisition and leaves power/expansion unchanged, but these behaviors need target validation. Obtain owner-approved sanitized installed command/output evidence for `info`, `help`, start/stop and prompt/echo framing, and permitted implementation evidence that discovery is passive. Confirm incomplete/overflow/timeout cancellation and physical stop behavior. Do not enable scan commands using synthetic fixtures alone. Current tagged BLE discovery was active and is not enabled; a receive-only BLE path remains undocumented/unverified.

## Minimum authorized inputs for compilation

1. A usable SDK bundle or SDK-only build environment permitted by its applicable instructions, with target/revision provenance matching the installed firmware. Review instructions before use; equivalent restrictions stop that operation. This is not permission to copy from or clone around a restricted checkout.
2. Its generated public include tree, enabled API export table/API version and target configuration. The owner may place a permitted export-table copy at `builds/sdk-input/api_symbols.csv` for preflight; revision provenance is owner-confirmed, not inferred from CSV.
3. Compatible ARM toolchain/sysroot and the SDK's build/link/FAP packaging tools, with versions and documented build-only invocation/registration of this external application while source remains here. The ARM compiler and uFBT 0.2.6 are available via explicit paths; default PATH lacks them. No additional SDK download is attempted.
4. An allowed output location under this project's ignored `builds`/`logs`, plus source commit and SDK provenance recorded in the build evidence. No physical Flipper connection is needed for compilation. SDK-only uFBT is a permitted candidate workflow, but the exact target archive and package/toolchain networking remain unavailable.

These inputs suffice to attempt the default GUI/storage shell. NRF/CC wiring and a concrete board port are additional requirements for a complete three-radio application, not reasons to postpone a permitted baseline compile. ESP command capture is required to enable discovery, not to compile its gated adapter. A controlled Sub-GHz signal source is not a software/build prerequisite.

## Build validation when inputs arrive

Follow MANUAL_BUILD.md: review instructions/provenance, run tests and export preflight, compile through the supported build-only command, inspect compiler/linker and import-resolution results, fix application mismatches and rebuild. Keep full outputs local. Record toolchain/SDK revision and API version, application SHA, manifest/defines/source selection, artifact path/SHA256 and build status. Verify the artifact's target/API metadata through the SDK's supported tool before requesting device deployment approval. Successful compilation is build evidence; installation, runtime compatibility and RF functionality remain separate approved physical tests. No firmware flash, release or launch is part of this milestone.

Public SPI/external-device interfaces have now been reviewed successfully; see SDK_INTERFACES.md for candidate names and the remaining bounded-acquisition/plugin-lifecycle gaps. This is permitted current documentation, not target-export verification. No concrete radio port can yet be enabled safely using the available wiring and SDK evidence.

The minimal manifest now disables ESP UART as well as NRF/CC capabilities. Its 34 firmware calls are checked with `--gui-storage-only`; the 43-call inventory above describes the preserved nonminimal configuration. Restoring hardware requires removing that define after target API/command/binding review. See MANUAL_BUILD.md for the SDK-only workflow and smallest owner action.

A later untracked editor compile database revealed a permitted SDK-only installation and ARM toolchain. SDK state/components identify **official 1.4.3**, with no applicable instruction restrictions found; it does not match the required Momentum target. Its generated compile flags were used only for an independent ARM translation-unit check, with object/log outputs confined to this project's ignored build directory. All 12 minimal source files now compile after short-enum fixes. No firmware image was read/flashed, firmware checkout accessed, or FAP linked/packaged. A target-compatible SDK-only archive with provenance is still the smallest missing owner input; an ARM compiler itself is now available. The uFBT Python package/entrypoint was not found in the inspected environment.

## Confirmed API mismatch and public SDK candidate

Owner confirms exact Momentum revision d3f89dfe2ef6b01839201598e9be1590cba80322, August 18, 2026, API 89.1, target 7. Official FAP API 87.1 remains a successful official build, not a target-compatible build. Public distributor lists a matching-short-revision SDK ZIP; one download fails with DNS/curl exit 6. Archive contents/instructions/full revision/API are unverified, so no matching-SDK rebuild is claimed. [SDK source and smallest manual input](MOMENTUM_SDK.md). No firmware/device operations, metadata overrides or radio changes.

## Downloaded Momentum SDK validation milestone

Downloaded ZIP SHA256 matches owner value `238db260ee7e8f6e9a79dd1c760e07a832351a7d92d896b4cb19082f8548b2c1`; all 626 members pass CRC and path/symlink checks. Component version is `mntm-dev-d3f89dfe` / f7; export table explicitly reports **87.1**, inconsistent with owner-confirmed installed API **89.1**. SDK switch/target rebuild stopped; no API 89.1 FAP generated. Official artifact/evidence preserved separately and existing cache untouched. [Exact inspected evidence and minimum resolution input](MOMENTUM_SDK.md). No radio changes, deployment, firmware operations or restricted-checkout access. Host, short-enum, sanitizer and publication regression suites remain passing.
