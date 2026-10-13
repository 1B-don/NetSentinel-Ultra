# Manual SDK compilation and device verification

The assistant has not accessed the restricted Momentum checkout or performed an SDK build. This workflow requires an SDK the owner is appropriately authorized to use; it does not bypass repository instructions. Do not use a current development SDK as assumed proof of compatibility with firmware `d3f89dfe2ef6b01839201598e9be1590cba80322`.

## Candidate app build

1. Read the authorized SDK's applicable instructions and confirm its revision/exported API matches the installed firmware. Preserve NetSentinel source in this repository; register the directory using the SDK's supported external-app source workflow. Do not include `legacy` or the host `tests/flipper_mock` headers in an SDK build.
2. Run `make test` and `make sanitize` here. Use the documented sandbox override only if leak detection is unavailable.
3. Build the existing candidate `application.fam`. It defines `netsentinel_app`, an 8 KiB stack, explicit core/Flipper source globs, and a v1.0 development target. The default shell implements GUI/storage and returns Unsupported for all radio acquisition. Relative local includes avoid guessing additional SDK include flags.
4. Follow the SDK's documented build-only command. A typical FBT target is `./fbt fap_netsentinel`, but verify it for the selected SDK. Do not use launch/upload/flash targets at this stage. Record all compiler/linker output under this repository's `logs` and return errors plus permitted API declarations for independent application fixes.
5. After successful compilation, retain the actual FAP under `builds` and record its hash, application commit, SDK/toolchain version, exported API version and compile log. Existing Downloads binaries and host shim executables are not FAP build evidence.

Public [manifest documentation](https://developer.flipper.net/flipperzero/doxygen/app_manifests.html) describes the candidate fields, but does not validate the restricted installed firmware ABI.

## Hardware integration before radio tests

An authorized board port must implement the optional `ns_flipper_board_bind/unbind` hooks, enable `NS_ENABLE_BOARD_PORT` in the candidate manifest, and add the port source to the build sources. It may fill only radio callbacks, capability flags and board context. Keep storage/context bindings intact and ESP scanning disabled. Verify Ultra CE/CS/SPI routing and supported external CC interfaces rather than guessing. All acquire/transfer/release work must be bounded, preserve original resources and remain receive-only. `adapters/README.md` is the port contract.

The optional hook is a preparation interface, not a completed driver. A default shell build cannot scan NRF/CC radios. UART ingress and firmware-specific passive commands/device parsing remain future work; do not enable the imported legacy commands as a shortcut.

## Device stage, after explicit approval

Obtain owner approval before deployment or launching. Use supported app upload/launch or qFlipper file transfer; never alter USB permissions, flash firmware or erase storage. The first approved shell test should verify GUI, allocation/exit cleanup, actual SD settings/history and CSV handling. Radio tests require a verified board port and owner confirmation of the physical selector before each start. Stop before changing the selector.

Run every required scenario in `VERIFICATION.md`, capture device logs, and record expected/observed results. A successful build or launch does not establish RF behavior. No v1.0 release is produced until every required feature passes. Use receive-only observations/fixtures; do not operate a test transmitter without explicit approval.
