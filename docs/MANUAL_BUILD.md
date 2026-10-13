# Manual SDK compilation and device verification

This document is a staged workflow for the owner. It does not authorize the assistant to access the restricted Momentum checkout, clone its contents elsewhere to bypass instructions, or operate the Flipper. No SDK compilation has been attempted in this independent project.

## Prerequisites

1. Obtain an SDK you are appropriately authorized to use. Read its applicable instructions yourself. For firmware `d3f89dfe`, compatibility evidence must cover full commit `d3f89dfe2ef6b01839201598e9be1590cba80322`, API symbol versions and external-radio interfaces. A current dev SDK is not sufficient evidence.
2. Supply permitted API header declarations/export metadata and board pin mapping for implementing the adapter, or implement the adapter manually. Do not supply restricted repository code for the assistant to work around its instructions.
3. Implement `NsPlatform`, a worker and GUI entry point in this application repository. Satisfy every requirement in `adapters/README.md`. UART IRQs enqueue bytes; a worker parses them and performs storage/radio operations. Implement settings read/write and persistent history, with bounded files, exclusive filenames and failed-write handling.
4. Confirm ESP32 firmware/passive commands. Until then leave the profile NULL. Never compile the archived v0.2 entry point as a shortcut.

## Once integration exists

Create an actual SDK manifest using the authorized SDK's documented external-app syntax, with entry point `netsentinel_app`, appropriate stack/heap requirements, root parser/analysis sources, portable `src/core` files and the concrete adapter/UI sources. The current `application.fam` is explicitly a nonbuildable placeholder to prevent accidentally building the legacy active scanning application.

Run `make test` and `make sanitize`. Have a human place or register the app with the SDK using its supported external-source workflow, preserving application source ownership in this repository. A typical FBT target is `./fbt fap_netsentinel`; verify this target with the selected SDK's own documentation. Do not run firmware flashing targets. Save full compiler/linker output in this repository's `logs` directory. Return failures, SDK revision and permitted API declarations for fixes in independent application code.

After successful compilation, copy only the newly produced FAP to `builds`, calculate `sha256sum`, and record artifact hash, application Git commit, SDK/toolchain versions, exported API version and compile log. Existing Downloads binaries are not build evidence.

## Device stage, only after explicit approval

Use the selected SDK's documented app upload/launch workflow or qFlipper file transfer. Do not change USB permissions, flash firmware, erase storage or transmit RF. Obtain approval for deployment and launching before invoking either. Obtain owner confirmation of NRF24/CC1101 selector position before acquiring the selected module. Changing selectors during an active session is unsupported: stop, release, change position, then restart.

Run the scenarios in `VERIFICATION.md` and capture device logs. A successful launch alone does not prove Wi-Fi/BLE/RF behavior. Do not label an artifact v1.0 until all required feature tests pass, including settings/history persistence and actual SD CSV reads. RF absence and reception tests require owner-approved receive-only fixtures or naturally occurring signals; no test transmitter is operated by this project.
