# Manual SDK compilation and device verification

The restricted Momentum checkout has not been accessed. An explicitly authorized official SDK 1.4.3 build has now succeeded; see BUILD_RESULT.md. Installed Momentum compatibility remains unverified. This workflow requires an SDK the owner is appropriately authorized to use; it does not bypass repository instructions. Do not use a current development SDK as assumed proof of compatibility with firmware `d3f89dfe2ef6b01839201598e9be1590cba80322`.

See [categorized build blockers and minimum authorized inputs](BUILD_BLOCKERS.md) before preparing a SDK build.

## Candidate app build

1. Read the authorized SDK's applicable instructions and confirm its revision/exported API matches the installed firmware. Preserve NetSentinel source in this repository; register the directory using the SDK's supported external-app source workflow. Do not include `legacy` or the host `tests/flipper_mock` headers in an SDK build.
2. Run `make test` and `make sanitize` here. Use the documented sandbox override only if leak detection is unavailable.
3. Build the existing candidate `application.fam`. It defines `netsentinel_app`, an 8 KiB stack, explicit core/Flipper source globs, and a v1.0 development target. The default manifest now defines `NS_GUI_STORAGE_ONLY`: GUI/navigation/storage are built while all hardware capabilities and UART acquisition are disabled. Relative local includes avoid guessing additional SDK include flags.
4. Follow the SDK's documented build-only command. A typical FBT target is `./fbt fap_netsentinel`, but verify it for the selected SDK. Do not use launch/upload/flash targets at this stage. Record all compiler/linker output under this repository's `logs` and return errors plus permitted API declarations for independent application fixes.
5. After successful compilation, retain the actual FAP under `builds` and record its hash, application commit, SDK/toolchain version, exported API version and compile log. Existing Downloads binaries and host shim executables are not FAP build evidence.

Public [manifest documentation](https://developer.flipper.net/flipperzero/doxygen/app_manifests.html) describes the candidate fields, but does not validate the restricted installed firmware ABI.

## Hardware integration before radio tests

An authorized board port must implement the optional `ns_flipper_board_bind/unbind` hooks, remove `NS_GUI_STORAGE_ONLY`, enable `NS_ENABLE_BOARD_PORT` in the candidate manifest, and add the port source to the build sources. It may fill only the isolated `NsBoardPort` radio callbacks and capability flags. Keep storage/context bindings intact and ESP scanning disabled. Verify Ultra CE/CS/SPI routing and supported external CC interfaces rather than guessing. The new read-only `NsCcBus` helper and optional `cc_diagnose` contract are described in `CC1101.md`; bind to an explicit external device, never an assumed global selection. All acquire/transfer/release work must be bounded, preserve original resources and remain receive-only. `adapters/README.md` is the port contract.

The optional hook is a preparation interface, not a completed driver. A default shell build cannot operate any radio. A candidate UART ingress/command adapter remains supplied in the nonminimal source configuration; installed labels/sequence, actual SDK exports and hardware timing remain unverified. See `MARAUDER.md`. Do not enable the imported legacy commands as a shortcut.

## Device stage, after explicit approval

Obtain owner approval before deployment or launching. Use supported app upload/launch or qFlipper file transfer; never alter USB permissions, flash firmware or erase storage. The first approved shell test should verify GUI, allocation/exit cleanup, actual SD settings/history and CSV handling. Radio tests require a verified board port and owner confirmation of the physical selector before each start. Stop before changing the selector.

Run every required scenario in `VERIFICATION.md`, capture device logs, and record expected/observed results. A successful build or launch does not establish RF behavior. No v1.0 release is produced until every required feature passes. Use receive-only observations/fixtures; do not operate a test transmitter without explicit approval.

## Export-name preflight

After reviewing SDK instructions and obtaining permission to use its exported API table, the owner may place an authorized copy of `api_symbols.csv` in `builds/sdk-input/`. Do not copy it from a restricted checkout without permission. Confirm the table was generated for the full target revision above, then run `make sdk-preflight`. The script never downloads, traverses or builds an SDK. It rejects external/symlinked input paths, wrong revision arguments, malformed tables and absent/disabled firmware functions. The input and output stay local under ignored build directories.

The checker discovers firmware calls in the actual Flipper C sources and records table API version/SHA256. Its CSV format follows the public upstream [API export table](https://raw.githubusercontent.com/flipperdevices/flipperzero-firmware/dev/targets/f7/api_symbols.csv); that current table is not used as the target SDK. The revision argument is an owner provenance assertion, not cryptographic proof that a table matches installed firmware. A passing name check does not validate C declarations, inline functions/macros, enum/struct layout, compiler-generated helpers, concrete board callbacks or ARM linkage. Actual compatible compilation and physical verification remain required. An absent input is a blocker, not a successful check.

## Check imports after an actual ARM link

When an authorized SDK build produces its final packaged-app import listing, prefer that SDK-generated POSIX `.impsyms` evidence and save a permitted local copy as `builds/sdk-input/undefined.txt`. Alternatively use the SDK's documented final ELF import tooling. An intermediate debug ELF can expose debug-only undefined symbols absent from the packaged app. Do not use a desktop test executable or pre-link object file. Record the actual ELF SHA256 and build provenance separately with the build evidence. Then run:

```sh
python3 scripts/sdk_preflight.py builds/sdk-input/api_symbols.csv --gui-storage-only \
  --firmware-revision d3f89dfe2ef6b01839201598e9be1590cba80322 \
  --undefined-symbols builds/sdk-input/undefined.txt
```

This mode checks required `U` imports against enabled Function and Variable entries, including runtime/helper imports outside the 43 source-call names. Undefined weak `w`/`v` entries are reported separately; this checker does not require their resolution, but the actual SDK loader/packager must confirm whether they are supported. Other formats, duplicate or empty evidence fail closed. An official SDK ARM ELF and final import listing now exist; exact Momentum target evidence remains unavailable. The checker hashes the listing and table but does not inspect ELF architecture, provenance, types, relocation compatibility or runtime loader behavior. A successful check remains diagnostic evidence, not a FAP build or installability claim.

## SDK-only path for the first build

Official [uFBT documentation](https://github.com/flipperdevices/flipperzero-ufbt/blob/dev/README.md) supports a fixed SDK archive URL or `ufbt update --local`, uses prebuilt firmware SDK components, manages the ARM toolchain and builds from this application's directory into `dist`. No firmware checkout is required. The [Momentum release listing](https://momentum-fw.dev/releases/) identifies `d3f89dfe`, but an exact SDK archive, its full-revision provenance/checksum and applicable usage instructions have not been obtained. The candidate third-party index `https://up.momentum-fw.dev/firmware/directory.json` was unreachable through both browsing and shell networking; its current schema/archive contents were not verified. Do not select an unpinned dev/release channel or accept uFBT's default official SDK as installed-firmware compatibility evidence.

Smallest owner action: obtain a permitted **SDK-only** archive for `d3f89dfe2ef6b01839201598e9be1590cba80322` from the distributor, confirm its provenance and applicable instructions, and place it under `builds/sdk-input/`. Do not obtain it from the restricted checkout. Supply those details so instruction review can precede use. This environment also cannot resolve PyPI/toolchain hosts; on the owner's network-connected laptop the build-only sequence is:

```sh
# Run from NetSentinel-Ultra after verifying SDK provenance/permission.
python3 -m venv .venv
.venv/bin/python -m pip install 'ufbt==0.2.6'
export UFBT_HOME="$PWD/builds/ufbt"
.venv/bin/ufbt update --local "$PWD/builds/sdk-input/authorized-sdk.zip"
.venv/bin/ufbt
```

The archive filename is a placeholder for the verified local archive. `0.2.6` is a published uFBT version, not an SDK compatibility assertion. Review applicable SDK instructions before execution; stop on restrictions. The update step may download its required toolchain. To build inside this execution environment instead, provide a permitted, populated uFBT SDK/toolchain environment plus the Python package, all applicable instructions and target provenance; a standalone archive cannot fix unavailable toolchain/package networking here.

Record the SDK status/API version and actual build output locally, then inspect compiler/linker/import failures. Do not run `launch`, `flash`, `flash_usb` or device targets. The current manifest compiles the 12 application C sources with `NS_GUI_STORAGE_ONLY`, no serial HAL header dependency and no active hardware paths. The corresponding preflight checks 34 firmware calls rather than the full UART configuration's 43. The new native build test only verifies this source configuration; it is not ARM/SDK evidence.

After an actual successful FAP build, record its hash/target/API metadata and application commit. Request deployment/launch approval, copy the FAP to the supported `apps/GPIO` location through the owner's supported USB file-transfer workflow, and test GUI navigation, settings save/relaunch, history load and repeated safe exit. These installation steps are prepared, not performed. This first artifact is a baseline for restoring verified adapters, not a v1.0 release.

### Subsequent available SDK assessment

The editor's untracked compile database revealed an SDK-only cache whose metadata is official 1.4.3 (release-channel official index). Its ARM compiler is available and all 12 minimal source files compile to ARM objects after short-enum fixes. No applicable AGENTS restrictions were found in the SDK/cache/toolchain, and no restricted checkout was used. This cache cannot establish installed Momentum compatibility and was not used to link/package an installable artifact. The exact target SDK-only archive/provenance remains required; the earlier network limitation still prevents installing the uFBT entrypoint here. The editor database was left untracked and unchanged.

`make short-enums` now runs the preserved suites with `-fshort-enums`, matching the compiler behavior that exposed invalid zero-bound checks. It is also in host CI. Actual SDK compile commands came from the existing database, with `NS_GUI_STORAGE_ONLY` added and object/dependency outputs redirected into `builds/arm-probe`; compiler logs remain local. The ARM objects are not FAP evidence.
