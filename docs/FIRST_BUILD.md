# First NetSentinel build on Kubuntu

**Current update:** USB CLI independently confirms installed API **87.1**, target 7, d3f89dfe; earlier 89.1/mismatch blockers below are superseded. Matching permitted Momentum SDK clean build, metadata and all 49 imports pass. Current FAP is 33,480 bytes, SHA256 `de38c1c7e08336a1ab5a3ebe6dcf2865ac8aa5ce9dedb97c8f8c17c024b859b5`. See [Momentum build result](MOMENTUM_BUILD.md). All radios remain disabled; physical installation/execution are pending explicit approval. For the device procedure use this artifact/hash; former fingerprint and metadata requests are historical.

The current manifest prepares GUI, navigation, settings/history and SD storage. All ESP32/NRF24/CC1101 capabilities are disabled with `NS_GUI_STORAGE_ONLY`. A successful first shell build is an integration checkpoint, not the completed three-radio v1.0 application.

Current result: the owner authorized official-SDK compilation independently of Momentum compatibility. A clean uFBT build now produces `dist/netsentinel.fap`; see [BUILD_RESULT.md](BUILD_RESULT.md). No device deployment or physical test has occurred.

## What is verified locally

- An ARM GCC 12.3.1 toolchain is present in the existing SDK-only cache. The available cache identifies **official firmware 1.4.3**, not the installed Momentum revision.
- uFBT **0.2.6** is available at `~/flipper-build-env/bin/ufbt`. Use this explicit executable because it is not on the default session PATH.
- The actual official 1.4.3 SDK manifest parser accepts `application.fam`: EXTERNAL type, `netsentinel_app`, version `(1, 0)`, 8 KiB stack, `NS_GUI_STORAGE_ONLY`, 12 source files. Tests/legacy are excluded. Empty `requires` and default targets are accepted. These fields do not supply missing link exports; actual target parser/linker validation remains pending.
- All 12 sources now compile/link/package a real FAP against the official SDK, with APPCHK target 7 / API 87.1. Installed-Momentum compatibility and physical NetSentinel operation are not verified.

## Build using the installed environment

```sh
cd ~/Projects/NetSentinel-Ultra
~/flipper-build-env/bin/ufbt
```

This builds only; it does not deploy. Keep all hardware adapters disabled. The current environment targets official SDK 1.4.3.

## Optional isolated setup on another machine

Run in your network-connected Kubuntu terminal:

```sh
cd ~/Projects/NetSentinel-Ultra
python3 -m venv .venv
.venv/bin/python -m pip install 'ufbt==0.2.6'
.venv/bin/python -m pip show ufbt
```

If Python reports a venv/ensurepip setup error, retain the exact message before changing system packages. This keeps uFBT isolated from system Python. Official [uFBT instructions](https://github.com/flipperdevices/flipperzero-ufbt/blob/dev/README.md) document SDK-only builds and local/fixed-URL SDK inputs; normal first-run defaults use the official release SDK, which is not installed-Momentum compatibility evidence.

## Obtain the one missing target input

Ask the Momentum distributor for a permitted **f7 SDK-only archive** generated for commit `d3f89dfe2ef6b01839201598e9be1590cba80322`, with its revision provenance, checksum and usage instructions. The public [release listing](https://momentum-fw.dev/releases/) lists the short revision, but a specific compatible SDK download has not been validated here. The candidate third-party SDK index could not be reached from this environment. Do not copy from or use the restricted firmware checkout, clone around its restrictions, or substitute an unpinned development SDK.

Place that permitted archive in `builds/sdk-input/` and provide its filename/provenance/instructions. Instructions must be reviewed before use. This is the smallest missing input to resume target build preparation; an exported API table alone cannot package an application.

## Build after that input is verified

The commands below use a placeholder archive name; run them only after its target and permitted use are confirmed:

```sh
export UFBT_HOME="$PWD/builds/ufbt"
.venv/bin/ufbt update --local "$PWD/builds/sdk-input/authorized-sdk.zip"
.venv/bin/ufbt
```

uFBT manages its required ARM/Python tools and puts the FAP in `dist`. The update can require network downloads. Do not run flash, flash_usb, launch or upload commands. Keep the full compiler/linker output local under ignored `logs`; report exact errors so fixes can be made in application source. `MANUAL_BUILD.md` covers export/import checks, build provenance and artifact hashes. No exact Momentum SDK is available here, so the target-specific archive steps have not been performed. The separately authorized official-SDK build succeeded.

## First device test after a successful build

Record application commit, SDK revision/API version, build log and FAP SHA256, and validate target/API metadata using the SDK's supported tooling. Obtain explicit approval before USB deployment or launch. Copy through the supported file-transfer workflow into `apps/GPIO`, then verify opening the app, navigation, saving/reloading a setting, history load and repeated exit without crashes/resource leaks. Record observed results and device logs locally. There is no radio testing in the initial configuration and no firmware update, erase or USB permission change is required.
