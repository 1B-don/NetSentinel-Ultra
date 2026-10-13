# NRF24 bounded SPI acquisition review

Scope: permitted Momentum d3f89dfe SDK-only headers and export table, API 87.1, target 7. No firmware implementation, restricted checkout, device or reference binary was accessed. This review establishes available interface contracts, not hidden implementation behavior.

## Finding

The later [public CC provider/SPI implementation review](CC1101_PROVIDER_IMPLEMENTATION.md) also found upstream 1.4.3 polled transfer/drain loops without enforced deadlines. A transfer timeout parameter in API 87.1 is a declared interface, not installed-target proof of a finite whole-call bound. This does not establish Momentum implementation behavior and does not change the disabled probe gate.

No safe bounded acquisition strategy is established by these exported APIs. Physical NRF24 remains disabled; no Flipper transport is added for `ns_nrf_probe_readonly()`.

The inspected SDK files are `targets/furi_hal_include/furi_hal_spi.h`, `targets/f7/furi_hal/furi_hal_spi_types.h`, `targets/f7/furi_hal/furi_hal_spi_config.h`, `furi/core/mutex.h`, `lib/momentum/settings.h`, and the checksum-verified API table described in MOMENTUM_BUILD.md. All SPI acquisition-related declarations and table entries were examined.

`furi_hal_spi_acquire(const FuriHalSpiBusHandle*)` is enabled but void and explicitly blocking; its documentation also warns of a crash on programming error. There is no exported SPI try-acquire, timed acquire, cancellable acquire or shared-bus mutex accessor. Timed `bus_tx/rx/trx/trx_dma` calls operate after acquisition and do not bound preceding ownership waits. Release and handle initialization/deinitialization also return void without deadline or error status.

`FuriHalSpiBus` exposes only `spi`, `callback`, and `current_handle`. Its event callback returns void and takes only a bus and event. Lock/unlock and activation/deactivation events are declared, but no timeout, success result, cancellation or atomic reservation contract is supplied. `FuriHalSpiBusHandle` exposes the bus, callback and GPIO pointers, not the underlying shared lock. The declarations do not reveal the lock implementation, type, recursion policy or callback execution time; none is assumed.

## Ownership and alternatives

| Alternative | Assessment |
|---|---|
| Timed `furi_mutex_acquire()` or zero-wait mutex attempt | Exported, but requires the actual mutex pointer. No SPI bus mutex accessor/field is provided. A separately allocated application mutex cannot serialize firmware NFC/Sub-GHz clients. |
| Check `current_handle` then acquire | A snapshot is not a reservation. Another client can acquire between check and call; an idle snapshot cannot guarantee a finite wait. Reading or writing the field cannot replace synchronized ownership. |
| Invoke bus Lock callback directly | No finite-wait argument/result; bypasses the public acquire lifecycle and has undocumented implementation semantics. It cannot establish a bounded lock. |
| Invoke activation callbacks, replace callbacks, mutate handle/bus state | Does not acquire the existing shared lock; risks concurrent peripheral reconfiguration and restoration failure. Rejected. |
| Worker thread plus GUI deadline | Keeps the GUI schedulable but leaves acquisition pending. The worker can later acquire after cancellation; freeing its context or unloading the app can then be unsafe. Joining may wait indefinitely. A timed response is not bounded cleanup. |
| Terminate waiting thread or release from another thread | No documented SPI abort/ownership-transfer contract; cannot prove lock or callback unwind. Rejected. |
| Scheduler/interrupt suppression, direct peripheral access, bit-banging or private structure casts | Bypasses synchronization or relies on unverified layout and routing. Rejected. |
| New bus instance, bus deinit/reinit or different generic handle | Does not provide independent ownership of shared hardware. Both external handles are on bus R, shared with NFC/Sub-GHz; bus D serves display/SD. Reinitializing shared buses is not an application ownership mechanism. |
| Assume exclusive use because no other radio screen is open | No exported reservation/exclusion contract establishes that firmware clients cannot acquire. Device absence or foreground app state is insufficient. |

Cancellation can be checked before and after a bounded callback, but cannot interrupt this HAL acquire contract. NetSentinel must not report completion and dispose of resources while a blocking acquisition worker remains alive. Successful ownership must be released by the owning execution context through the supported lifecycle before context destruction. Failed acquisition must not release another client's lock. A future provider must also establish prompt release and handle restoration, not just timed transfers.

## Routing and remaining requirements

`SpiDefault` means generic CS PA4; `SpiExtra` means generic CS PC3. Both external SDK handles use PA6 MISO, PA7 MOSI and PB3 SCK on bus R. This distinction is preserved and is not Ultra V3 routing evidence. No setting is read or changed, no GPIO initialized, and CE remains untouched.

Closing this blocker requires a permitted installed-target ownership API/provider with an enforceable finite acquisition result, cancellation policy and cleanup contract, or reviewed evidence of an existing synchronized reservation mechanism providing that guarantee. This review identifies none; no firmware modification is proposed. Separately obtain revision-matched Ultra V3 SPI/CSN/CE/selector/voltage routing. Any future enabled artifact requires explicit checksum/procedure approval before deployment or launch. A read-only register probe proves only plausible communication, not chip identity or RF reception.

## Software verification

The existing mock suite covers contention, acquisition timeout, transfer timeout, tick wrap, repeated sessions and cleanup. Added acquisition I/O/cancellation failures, retention of another client's ownership, acquisition consuming the complete budget, cancellation arriving during successful acquisition, and successful reuse after failure/cancellation. These test the injected provider contract; they do not simulate or prove a finite SDK acquire. Runtime source, UI, ESP32, CC1101 and all physical gates are unchanged. The complete development pipeline validates host/short-enum/sanitizer suites and matching SDK packaging; exact evidence is recorded in MILESTONES.md.
