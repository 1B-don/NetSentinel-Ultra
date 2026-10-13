# External CC1101 implementation decision

Baseline: `55ccbbe0710e5f7d8ef50e1f77a7c5b6af9b698d`. Installed target: Momentum d3f89dfe, API 87.1, target 7. Review inputs: permitted SDK-only headers/table and public Flipper upstream 1.4.3 source. Upstream is a reference implementation, not installed Momentum provenance. No restricted checkout, device or plugin execution was used; no upstream code was copied.

## Recommendation

Do not wrap the current generic registry calls as a finite receive adapter. Preserve the typed CC contract and physical gates. First obtain permitted installed-provider/source correspondence and the registry/SPI ownership implementation. Use the existing registry only if that review establishes a supported shared lifetime, external-only selection and enforceable finite provider operations. If it matches the reviewed upstream's unbounded waits, a wrapper cannot satisfy the contract.

An independently authored minimal adapter is an alternative only after a verified shared reservation/transaction API and Ultra route are supplied. It can avoid generic provider reset/GDO self-test and asynchronous capture, but cannot replace the firmware lock with a private mutex. No concrete finite hardware ownership mechanism is currently available through the reviewed exports. A firmware arbiter would be a possible separate project, not a solution implemented or authorized here.

No additional mock radio functionality or source changes are included in this milestone. It produces a decision document, not an experimental FAP.

## Public provider lifecycle reviewed

Reference: [`applications/drivers/subghz/cc1101_ext/cc1101_ext.c`, tag 1.4.3](https://raw.githubusercontent.com/flipperdevices/flipperzero-firmware/1.4.3/applications/drivers/subghz/cc1101_ext/cc1101_ext.c).

| Phase / function | Reference behavior / consequence |
|---|---|
| alloc/check_init | Singleton allocation; external SPI handle and PB2 GDO0; handle init, reset, GDO self-test/register writes, shutdown. A 100 ms timer begins after blocking acquisition. |
| is_connect | Uninitialized path allocates/tests/frees; initialized path reads part number. Not a read-only discovery method. |
| load_custom_preset | Reset, register writes and PA-table loading. Requires full preset review. |
| set_frequency | Frequency registers, calibration, unbounded IDLE polling under acquired SPI. |
| rx/idle | SPI acquire, transition, fatal check of status-wait result, GDO2 write. |
| get_rssi | SPI read and fixed 74 dB offset conversion; does not independently validate RX. |
| sleep/free | Sleep idles, changes GDO0 and shuts down; free deinitializes handle and frees singleton. Free alone is not receive shutdown. |
| async RX | TIM17/GDO interrupts and context; stop idles before removing capture resources. More ownership requirements than RSSI polling. |

No OTG toggle appears in this provider source; that is not a power-restoration guarantee for its caller or installed provider. Its PB2 expectation is not Ultra routing evidence. Its time-limited self-test/status checks do not bound preceding acquisition or indefinite calibration.

## Plugin, low-level and synchronization review

[`cc1101_ext_interconnect.c`, tag 1.4.3](https://raw.githubusercontent.com/flipperdevices/flipperzero-firmware/1.4.3/applications/drivers/subghz/cc1101_ext/cc1101_ext_interconnect.c) maps begin/end to alloc/free and exposes a descriptor named `cc1101_ext` through a descriptor-returning entrypoint. It maps RX/RSSI and also TX methods; the invalid-frequency wrapper can crash. NetSentinel would bind only reviewed receive operations and reject invalid input before provider calls. The source's entrypoint returns metadata; this establishes that specific reference's behavior, not the installed plugin binary's identity or loader effects.

[`lib/subghz/devices/registry.c`, tag 1.4.3](https://raw.githubusercontent.com/flipperdevices/flipperzero-firmware/1.4.3/lib/subghz/devices/registry.c) builds a global registry using plugin-manager discovery, includes the internal device and releases plugin storage during teardown. [`devices.c`](https://raw.githubusercontent.com/flipperdevices/flipperzero-firmware/1.4.3/lib/subghz/devices/devices.c) checks registry validity and delegates through callbacks. Exact external lookup does not require a direct entrypoint import; it also does not reserve hardware or keep a provider alive against concurrent teardown.

[`lib/drivers/cc1101.c`, tag 1.4.3](https://github.com/flipperdevices/flipperzero-firmware/blob/1.4.3/lib/drivers/cc1101.c) contains chip-ready polling, transactions, status waits, strobes and register access. Its helpers assert chip-ready success instead of providing complete recoverable errors. A status-wait timer cannot preempt a transaction already in progress. This is another layer that must be reviewed before relying on a provider's apparent timeout.

[`targets/f7/furi_hal/furi_hal_spi_config.c`, tag 1.4.3](https://raw.githubusercontent.com/flipperdevices/flipperzero-firmware/1.4.3/targets/f7/furi_hal/furi_hal_spi_config.c) uses a normal bus-R mutex with `FuriWaitForever`; handle activation configures pins/CS, deactivation restores idle configuration. The public mutex variable is not exported by the permitted target table. Calling callbacks directly or acquiring that mutex through guessed/private access is unsupported; prelocking a normal mutex and then calling public acquire could deadlock.

[`furi_hal_spi.c`, tag 1.4.3](https://raw.githubusercontent.com/flipperdevices/flipperzero-firmware/1.4.3/targets/f7/furi_hal/furi_hal_spi.c) sequences bus lock/activation/current-handle/handle activation and reverses these on release. Its polled transfer loops have no deadline checks; the final FIFO/busy drain explicitly ignores the timeout argument. DMA acquisition also includes an indefinite semaphore wait. This is **not** proof of installed Momentum implementation. It proves why a timeout parameter or boolean return in the matching SDK header is insufficient evidence of a total finite operation. Previous reports' “timed transfer” descriptions mean the declared parameter only; no physical deadline was verified.

Power belongs to the complete application lifecycle, not merely the chip driver. The previously reviewed [public current upstream Sub-GHz CLI](https://raw.githubusercontent.com/flipperdevices/flipperzero-firmware/dev/applications/main/subghz/subghz_cli.c) enables external power in selection and can fall back to internal. Do not reuse either behavior. No Momentum expansion/power configuration is read or changed here.

## API 87.1 availability versus implementation details

Read the permitted `lib/subghz/devices/devices.h`, `types.h`, external interconnect header, plugin manager, SPI API/types/config and the complete checksummed export table.

| Interface | Target availability | Consequence |
|---|---|---|
| subghz_devices_init/deinit/get_by_name/get_name | Enabled | Public discovery/delegation surface; no lifetime reservation, finite init result or refcount guarantee |
| begin/end/is_connect/load_preset/set_frequency/set_rx/get_rssi/idle/sleep | Enabled | No operation deadline/cancellation argument; several void results; implementation review required |
| plugin_manager_alloc/load_single/get/get_ep/free | Enabled | An app-owned loader is possible; ABI/app-ID checks do not arbitrate the physical bus or establish finite loading |
| External descriptor entrypoint | `subghz_device_cc1101_ext_ep` disabled | Cannot directly import; registry or supported loader may discover a plugin |
| cc1101_* low-level helpers, provider alloc/free/RX helpers | No enabled functions or cc1101.h/provider implementation header in supplied SDK | Cannot import these internal implementations; independent code would need separately reviewed contracts |
| Device-registry validity/global ownership | No exported validity, lifetime lock or lease API | Cannot borrow/reserve by private globals or blindly initialize/deinitialize |
| SPI handle init/deinit/acquire/release/trx, external handles | Enabled | Acquire is documented blocking; transfer parameter does not establish whole-call bounds |
| Bus-R mutex | No export/accessor or field in public bus structure | Timed furi_mutex APIs cannot access/reserve the shared lock safely |
| GPIO, bus enable/disable, timer/interrupt, OTG functions | Some enabled exports | Export does not imply resource ownership, board routing or permission to use them; unused |

`SubGhzBegin` in this target takes `SubGhzDeviceConf*`; source correspondence must account for exact ABI, not copy upstream function-pointer layouts. Retain configuration defaults and never enable bypass-region, power or amplifier behavior based on guessed semantics. Generic internal Sub-GHz HAL functions are not an external-device fallback.

## Required implementation inputs and prospective project files

Obtain permitted installed-target counterparts of the following reference layers, with exact revision/package identity: external provider `.c/.h` and interconnect `.c/.h`; device `devices.c/.h`, `registry.c/.h`, `types.h`; plugin loader/descriptor implementation and its actual manifest/packaging metadata; `cc1101.c/.h` and register definitions; `furi_hal_spi.c`, SPI configuration/types/API and shared resource ownership contracts. The public provider/interconnect, registry/wrapper, low-level chip and SPI files above were reviewed. A complete plugin-manager implementation/actual build manifest was not retrieved in this review, so loader bounds and package identity remain unverified. The SDK-only archive is not a substitute for those implementations; no restricted-source access is requested.

Hardware input: Ultra V3 PCB revision and authoritative nets for CC SCK/MOSI/MISO/CS, chip-ready/MISO behavior, GDO0 and any used GDO2, selector path, supplies and signal levels. Generic SDK external PA6/PA7/PB3 plus PA4 CS are not a netlist. Preserve `SpiDefault`/PA4 versus `SpiExtra`/PC3; neither proves this CC route. RSSI-only polling can avoid asynchronous capture/TIM17, but a chosen provider may still require GDO0 for initialization. Do not assume an unused external signal is disconnected or safe to manipulate.

If prerequisites close, proposed independent files would be `src/flipper/ns_cc_registry.c` and a typed `ns_cc_registry.h`, with a separately named/gated discovery or status manifest generated by the build helper. They would hold an explicitly acquired registry/provider lifetime and map only supported external operations to the existing `NsPlatform` CC callbacks. No such files or flags are added now. Direct SPI alternative would need its own bounded synchronized transport beneath the existing `NsCcBus` status helper, then a reviewed receive preset/state controller; it is not justified by GPIO exports alone.

## Finite ownership and cancellation acceptance contract

A feasible implementation must expose a supported shared lease with finite acquisition, Busy/Timeout results, cancellation between finite operations and cleanup by the owner. Record acquired resources only after successful acquisition; unwind partial initialization; never release another client's registry/bus. Enforce total bounds over chip-ready, transfer, FIFO drain, calibration and state settling; do not reset a deadline on each poll. All errors must return rather than crash. Cleanup must quiesce callbacks, idle a started receiver, deassert CS and restore the actual previous pin/resource state before releasing lifetime. Failed cleanup must remain visible, and app exit must not dispose of a live worker/context.

Current budgets remain probe 100 ms, tune 500 ms, diagnostic 200 ms, primitive idle/RSSI 20 ms. The reviewed APIs do not implement this acceptance contract. Worker timers, application-private locks, direct callback calls, thread termination, private mutex imports or a second plugin instance do not satisfy it. SPI transaction exclusion alone also does not serialize a complete calibration/RX session or conflicting GPIO/handle lifetimes.

## Shortest justified test path

1. Obtain permitted installed provider correspondence plus a supported finite ownership/transaction/lifetime contract. If the installed chain has the reviewed unbounded loops, no finite adapter can be implemented on top of it under current restrictions. Keep it disabled; identify the missing firmware/API capability explicitly.
2. An app-owned **single-plugin metadata lookup** is worth evaluating once exact installed plugin/loader provenance, no-hardware entrypoint/load effects and finite cleanup are reviewed. It could avoid the global registry lifetime conflict, but does not establish hardware exclusivity. No plugin is loaded here. This path does not require GPIO routing to establish registration only.
3. For hardware communication, also obtain exact Ultra nets, passive initialization/preset/state policies and restoration evidence. Build a separately gated status-only test through the validated ownership interface, with no reset/GDO self-test/RX unless independently reviewed and approved. Report plausible communication only.
4. Run full source/SDK/import checks, preserve stable binaries and seek separate owner approval of the exact artifact/configuration/procedure before deployment/launch. Receive/RSSI follows only after passive RX review; known-signal verification remains separate. No selector change, RF operation, firmware modification or experimental installation is part of this milestone.

Outcome: there is a supported API pattern, but no verified reliable installed-target implementation path yet. Required evidence is concrete above; no extra radio simulation can close it.
