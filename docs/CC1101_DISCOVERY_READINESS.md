# Read-only external provider discovery decision

Baseline: `e44588f55613067e7c91d1e68b54ac31ed81b39a`. Target: permitted Momentum d3f89dfe SDK API 87.1 / target 7. Decision: **not ready**. No discovery variant or standalone diagnostic FAP is generated. No registry function is called by the application. Existing physical radio gates and artifacts remain preserved.

## Narrow scope and exact blocker

This milestone asks only whether a provider descriptor is registered, not whether the chip connects, receives, or produces RSSI. It would need only an owned, stable registry and exact external-name lookup; it must not invoke begin, is-connect, reset, presets, frequency, RX, RSSI, GPIO, SPI or expansion power operations. Unknown Ultra GPIO routing and SPI acquisition are not by themselves prerequisites for a truly metadata-only lookup. The immediate blocker is the unsupported registry initialization/ownership/lifetime contract and unverified plugin-load effects.

Rechecked the permitted SDK `lib/subghz/devices/devices.h`, `types.h`, external interconnect declaration, plugin-manager declaration and complete target export table. `subghz_devices_init/deinit/get_by_name/get_name` are enabled. `subghz_device_cc1101_ext_ep` remains disabled as a direct import, but named discovery need not import it. The SDK headers declare init/deinit as void with no status, timeout, reservation, reference count or cancellation parameter. No exported device-registry validity or shared-lifetime acquisition/release mechanism is present. These declarations do not specify idempotent initialization, borrowing an already initialized registry, ownership across concurrent clients, partial-allocation recovery or shutdown safety.

The SDK's `lib/subghz/registry.h` concerns the **protocol decoder registry** (`SubGhzProtocolRegistry`); its count/index/name functions are not an alternate radio-device discovery/lifetime interface. Do not confuse that exported protocol table with a registered external CC provider.

The [public upstream device wrapper](https://raw.githubusercontent.com/flipperdevices/flipperzero-firmware/dev/lib/subghz/devices/devices.c) requires a valid device registry for lookup/deinit and an invalid one for init. The [upstream registry implementation](https://raw.githubusercontent.com/flipperdevices/flipperzero-firmware/dev/lib/subghz/devices/registry.c) allocates a global registry and populates it through a plugin manager before name lookup; teardown frees provider and registry storage. This illustrates why lookup and pointer lifetime need explicit ownership. It does not prove the installed Momentum implementation. No restricted firmware source was read, and no source was copied.

The lookup is not a receiver initialization call at the declared interface level. Nevertheless, the full init/lookup/deinit sequence cannot be certified as free of GPIO/power/radio effects from these declarations. The permitted SDK contains no installed provider implementation establishing the effects of descriptor loading/entrypoint execution. Generic plugin loading is code execution, not a metadata-only file inspection guarantee. The report does not assert that the installed initialization touches hardware; its effects are unverified.

## Proposed outcome semantics, not measured results

| Outcome | Evidence needed before reporting it | Current status |
|---|---|---|
| External provider registered | Safe registry lifetime, non-null exact `cc1101_ext` descriptor/name | Not measured; does not imply connected or functional |
| Provider absent | Successful safe registry setup and exact lookup returning null | Not measured; cannot infer absence from a missing permission/unsafe lifecycle |
| Registry unavailable | Supported lifetime acquisition reports unavailability | No such status interface established; unavailable state cannot be queried safely |
| Initialization failure | Recoverable, defined init failure result and partial cleanup | Public init returns void; no dependable failure distinction established |
| Unsupported operation | Required safe discovery lifecycle is not established by permitted contracts | Current readiness conclusion, not a device/provider absence result |

Do not collapse unknown registry availability or initialization failure into provider absence. Do not query an internal name, use the internal descriptor when external lookup fails, or call is-connect merely to validate registration. Matching name establishes registration only, not authenticity.

## Cancellation, ownership and cleanup

Without a lifetime lease, a pointer from another app's registry can become invalid on teardown. Foreground app state, an app-local mutex or a check-then-init pattern cannot establish synchronized ownership. Blind init can conflict with an existing registry, and blind deinit can destroy another client's state. No undocumented validity function, global pointer, private cast or callback replacement is permitted.

A worker and GUI cancellation flag do not bound registry initialization/plugin loading. Cancellation must not unload the app or free callback context while the worker remains active; joining an unbounded worker does not solve the requirement. A future supported lease must be released only if owned, exactly once after pointer use ends, including cancellation and failures. Metadata-only cleanup must not call radio idle/end/sleep. The current application acquires no registry resources and therefore has nothing to cancel or release for discovery.

## Implemented verification and shortest next input

Kept the previous deterministic CC future-provider tests for provider presence/absence, wrong names, initialization failure and repeated sessions. They are explicitly invented finite mocks and are not a supported discovery diagnostic. No further fake registry lifecycle was introduced.

Improved names-only SDK preflight to inventory `subghz_` and `plugin_manager_` calls alongside existing firmware prefixes. A synthetic source regression verifies init/name lookup/name read/deinit/plugin-load detection, excludes comment/string pseudo-calls, and confirms current runtime has no such calls. Actual FAP import validation separately rejects registry imports. All existing suites are retained; full pipeline results and artifact checksum are recorded in MILESTONES.md.

The shortest next input is a permitted installed-target implementation or explicit supported contract documenting registry initialization prerequisites, synchronization/ownership, plugin-load/descriptor effects, finite error/cancellation behavior and teardown. Evidence might establish a safe existing lifecycle without new firmware; this review does not require firmware changes or authorize them. If that contract is supplied, an isolated experimental build can be considered using exact external lookup and no hardware/provider callbacks. Until then, no standalone FAP, diagnostic import list or runtime permissions are proposed. Any future deployment/launch still needs exact-artifact owner approval. ESP32, NRF24, CC mocks, UI/settings/data and known-good binaries are unchanged.
