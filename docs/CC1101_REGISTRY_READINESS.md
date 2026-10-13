# External CC1101 registry integration readiness

The narrower [metadata-only discovery review](CC1101_DISCOVERY_READINESS.md) identifies registry lifetime and unverified load effects as the immediate blocker even when no hardware/provider method is called. No standalone discovery FAP is justified by the current permitted contracts.

Source baseline: `86c9771ce89b4e6c70da1437143fa313f826a40b`. Reviewed target: permitted Momentum d3f89dfe SDK, API 87.1, hardware target 7. Outcome: named external discovery is an available API pattern; a safe bounded physical receive adapter is not yet justified. All physical CC1101 gates remain disabled. No experimental FAP was installed or launched.

## Discovery without the external entrypoint import

The permitted `lib/subghz/devices/devices.h` declares `subghz_devices_init/deinit`, `get_by_name`, `get_name`, `begin/end`, `is_connect`, `is_frequency_valid`, `load_preset`, `set_frequency`, `set_rx`, `get_rssi`, `idle/sleep` and receive methods. The checksum-verified target table enables those functions. The external interconnect header defines `SUBGHZ_DEVICE_CC1101_EXT_NAME` as `cc1101_ext`; the direct `subghz_device_cc1101_ext_ep` import is disabled.

An application can use the enabled `subghz_devices_get_by_name("cc1101_ext")` interface without importing that entrypoint. Whether it returns a device on the installed target depends on a valid registry lifetime and an available registered provider. Export availability is not evidence that either condition holds. Do not call lookup on a guessed/uninitialized registry or use an internal device as fallback.

The [public upstream registry implementation](https://raw.githubusercontent.com/flipperdevices/flipperzero-firmware/dev/lib/subghz/devices/registry.c) demonstrates dynamic plugin-manager population and exact-name lookup. The [upstream device wrapper](https://raw.githubusercontent.com/flipperdevices/flipperzero-firmware/dev/lib/subghz/devices/devices.c) checks registry validity before lookup and initialization/deinitialization. This explains the supported pattern independently of a direct entrypoint import. It is mutable upstream reference evidence, not the installed Momentum implementation; do not copy its lifecycle assumptions, plugin path or function signatures into this target. The permitted SDK ABI is authoritative for compilation.

The target exposes no `subghz_device_registry_is_valid` export, registry reservation/refcount API or registry lock accessor in the supplied table. Therefore a FAP cannot establish concurrent-safe initialization/teardown by checking an exported validity flag. A private application mutex would not coordinate with other firmware users. Blind init/deinit is not a safe diagnostic; provider loading and callbacks may execute code or touch hardware. Pointer validity must last through final end/release and every outstanding callback; deinit before those finish can invalidate provider code/data.

## Evidence stages

| Stage | Required evidence | What it does not establish |
|---|---|---|
| Registry available | Reviewed installed-target registry initialization, ownership/lifetime and lookup result | Physical chip presence, successful begin or RF data |
| External registered | Non-null descriptor named exactly `cc1101_ext`, required callback presence and permitted provider provenance | Correct Ultra wiring or bounded/passive callbacks; names are not authentication |
| Provider initialized | Successful reviewed begin under exclusive ownership, partial-init failure unwind | Connected chip, settled RX, RSSI calibration or reception |
| Connected | Reviewed provider-specific chip communication check | is-connect need not be read-only; success alone does not establish RX |
| RX configured | Supported board frequency/preset, passive state-transition policy, completed calibration/settling and independent RX-state confirmation | Decoded data, calibrated RSSI, known-signal reception or transmit-free full lifecycle |
| RSSI sampled | Finite external read, finite numeric value, valid RX state and explicit calibration semantics | A packet or known RF signal; fluctuations can represent noise |
| Data received | Independently verified observation of a controlled source, with privacy-safe results | Prior stages cannot be inferred merely from a UI label |
| Restored | Verified idle/end, CS/pin/bus restoration, no outstanding callbacks and released registry lifetime | Cannot be inferred from a returned bool or a closed screen |

Owner evidence remains: Ultra V3 selector CC1101, Momentum automatic External selection, Read RAW at 433.92 MHz and slight RSSI fluctuations. No known signal was decoded. These are observations of the existing app, not NetSentinel registry, SPI, RX or restoration evidence.

## Bounds and receive-only assessment

| API operation | Target declaration | Missing guarantee |
|---|---|---|
| Registry init/deinit | void, no deadline | Allocation/plugin loading errors, concurrent lifetime, finite completion and teardown |
| Lookup/name | descriptor/string return, no deadline | Registry lifetime and provider provenance; descriptor alone does not reserve hardware |
| Begin / is-connect | bool, no deadline | Exclusive acquisition, finite chip-ready wait, cancellation, partial-init unwind and passive effects |
| Preset / frequency | void preset, frequency result, no deadline | Finite calibration/settling, supported board configuration, errors and TX-free automatic transitions |
| Set RX | void, no deadline | Confirmed RX state, finite transition and failure status |
| RSSI | float, no deadline | Finite wait, confirmed external RX, finite value/error discrimination and calibration |
| Idle / sleep / end | void, no deadline | Finite cleanup, restoration errors, provider lifetime and cancellation completion |
| Async RX start/stop | void, no deadline | Bounded startup/stop, callback quiescence and safe buffer/context lifetime |

`SubGhzDeviceInterconnect` is a callback ABI, version 1, not a lease/arbiter service. A wrapper cannot manufacture these guarantees from callback presence. Direct interconnect calls still require reviewed registry lifetime and do not improve underlying bounds. Matching return frequency is useful validation but does not prove tuning completed within a deadline. RSSI zero is a possible value, not an error signal; future integration must reject non-finite/out-of-contract values and verify RX separately. No registry raw status method in the reviewed surface provides MARCSTATE; a reviewed provider must establish how to validate state.

The existing core requires 100 ms probe, 500 ms tune, 200 ms diagnostic and 20 ms primitive idle/RSSI budgets. These are future provider requirements. None of the registry calls accepts those bounds. SPI acquisition remains blocking; transfer timeouts cannot bound acquisition. A worker can leave the UI responsive but cannot make a blocked provider safely cancellable or unloadable. Thread termination, foreign unlock, GPIO substitution and private structure casts are not alternatives.

For a future adapter, require explicit named external lookup and matching identity; reject null, wrong-name and incomplete descriptors before begin. Do not query the internal name, reset/power-cycle, modify Momentum settings or copy the public CLI's internal fallback/power behavior. Audit begin, is-connect, preset, frequency, RX, idle and end as a complete lifecycle, including automatic transitions and partial failures. An initialization failure must restore anything it acquired; a failed discovery must not issue idle/end on an uninitialized or different device. Cleanup failure must be reported while preserving actual ownership state, rather than declared successful. Do not enable a radio merely because the gate macros can be defined.

## Implemented software and exact remaining gates

No physical registry adapter was added because the available contracts do not justify it. Runtime/UI/settings/data, ESP Info, NRF probe and CC callback ABI are preserved. Added deterministic future-provider mocks to the existing CC suite: invalid registry, missing external device, wrong-name internal descriptor rejected before begin/RX, begin failure, disconnected external device with teardown, timeout, 12 fluctuating-RSSI receive/cancel/repeated-stop cycles, tune failure and idle timeout. Existing contention and typed-board tests remain. These invented finite mocks test the portable core's response/cleanup requirements, not installed registry implementation or production external-selection behavior; there is no live selection path in the FAP.

Before a candidate can be ready for a hardware test, supply a permitted installed-target registry/provider implementation or supported contract establishing synchronized registry lifetime, external identity, bounded acquisition/callbacks, partial failure cleanup and passive initialization/calibration/RX/teardown. Also obtain Ultra V3 revision-specific CS/GDO/SPI/selector/voltage routing and board-supported receive preset. No generic pin map or provider name substitutes for those inputs.

After those gates close, implement a separately gated adapter, compile/package/test it, preserve stable artifacts and request approval of its exact checksum/configuration/procedure. Start with bounded external communication/status checks; RX/RSSI comes after passive configuration review. Ambient RSSI can verify monitoring only; formal reception/calibration requires a suitable known source and separate approved procedure. No hardware/module/settings change or deployment is authorized here. Build checksum and complete validation results are recorded in MILESTONES.md.
