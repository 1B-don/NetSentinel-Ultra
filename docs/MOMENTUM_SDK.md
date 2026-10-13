# Exact Momentum SDK investigation

## Confirmed installed target (owner device information)

| Field | Confirmed value |
|---|---|
| Firmware | Momentum Dev |
| Build date | August 18, 2026 |
| Revision | d3f89dfe2ef6b01839201598e9be1590cba80322 |
| Firmware API | 89.1 |
| Hardware target | 7 |

The current successful official SDK 1.4.3 FAP targets API **87.1**, hardware target 7. The API major differs from the installed firmware. It is not a validated target build and must not be installed/launched on the basis of the official compilation. No API metadata editing, import-check bypass or firmware change is permitted.

## Public SDK-only candidate found

The distributor's [development build directory](https://up.momentum-fw.dev/builds/firmware/dev/) lists `flipper-z-f7-sdk-mntm-dev-d3f89dfe.zip`, 29.8 MiB, August 18, 2026 at 20:51. Direct candidate:

[Download SDK ZIP](https://up.momentum-fw.dev/builds/firmware/dev/flipper-z-f7-sdk-mntm-dev-d3f89dfe.zip)

This is a published SDK distribution, separate from the restricted checkout. The filename/date match the abbreviated installed revision; **archive contents, full commit identity, API 89.1, checksum/provenance and usage restrictions have not yet been inspected**. Do not claim exact SDK compatibility from its filename. The directory listing does not provide an inspected publisher checksum/signature. A locally calculated SHA256 records downloaded bytes but is not independent authenticity proof.

One HTTPS download attempt failed with curl exit 6: `Could not resolve host: up.momentum-fw.dev`. No archive was obtained, no matching-SDK build attempted and no SDK cache changed. The restricted repository was not accessed. Existing official FAP/build environment and GUI/storage-only hardware guards are preserved.

## Smallest manual input

Download only that SDK ZIP on the development laptop and save it as `builds/sdk-input/flipper-z-f7-sdk-mntm-dev-d3f89dfe.zip` inside this project. Preserve its public source URL and any publisher checksum/instructions. Do not download a firmware update, DFU or replacement checkout. No device action is necessary.

Once present, inspect ZIP member paths safely before extracting, rejecting traversal/absolute paths and symlinks. Review bundled AGENTS/instructions/license before use; if equivalent restrictions apply, stop the affected SDK operation. Inspect only permitted SDK/build metadata and headers; do not use or flash bundled firmware images. Confirm SDK metadata full revision, f7/target 7 and API exports Version 89.1. If full revision is absent, obtain distributor provenance linking the archive to the complete target revision rather than assuming a prefix match is sufficient.

Only after authorization/provenance checks, initialize a separate project-local uFBT SDK home using the documented `ufbt update --local` SDK workflow, leaving the working official cache intact. Toolchain acquisition may also need working networking. Build with unchanged `NS_GUI_STORAGE_ONLY`; do not enable UART/NRF/CC ports. Validate compiler/linker errors normally, APPCHK, resulting `.fapmeta` target/API and every actual packaged function/data import against the matching export table. Record SDK/archive/artifact hashes and source commit locally. Matching exported names alone do not replace declaration/layout/calling ABI checks.

## Remaining verification

No API 89.1 FAP has been produced. The only successful build remains official SDK 1.4.3 / API 87.1; see BUILD_RESULT.md for the artifact. After an exact-target successful build, loader, GUI/input/lifecycle and real SD persistence still need explicit installation/execution approval and the FIRST_DEVICE_TEST.md procedure. Radios remain disabled throughout the first test. No firmware changes or releases are part of this workflow.

## Downloaded archive validation — stopped on API mismatch

The owner supplied the public ZIP locally with SHA256 `238db260ee7e8f6e9a79dd1c760e07a832351a7d92d896b4cb19082f8548b2c1`; the independently calculated checksum matches. All 626 ZIP members pass CRC, with no absolute/traversal paths or symlinks detected. No bundled AGENTS.md/SKILL.md was found. SDK/compiler/linker settings and scripts are present. The bundled scripts README concerns firmware flashing/resources, not an instruction to flash for app compilation; none of those operations was performed. No new distribution-wide permission restriction was found in inspected instructions. The archive also contains firmware/update images; these were not used for metadata extraction, building or flashing.

Exact inspected metadata:

```json
{"hw_target":"f7","flavor":"C","version":"mntm-dev-d3f89dfe"}
```

Exact export table header and version from `sdk_headers/f7_sdk/targets/f7/api_symbols.csv`:

```csv
entry,status,name,type,params
Version,+,87.1,,
```

This **does not match confirmed device API 89.1**. The package identifies the abbreviated revision; inspected component metadata does not establish the full commit hash. A matching filename/date/checksum demonstrates which bytes were supplied, not compatibility with API 89.1. The discrepancy is unresolved; it may reflect the distributed SDK or device API information, and no cause is assumed.

As requested, SDK installation and matching-target rebuild stop at this discrepancy. The supported `ufbt update --local` workflow was not invoked. No generated API 89.1 FAP exists, no SDK metadata was edited, and no import validation was bypassed. Existing official SDK cache and `dist/netsentinel.fap` remain unchanged. Separate local preservation: `builds/preserved-official-api87/` contains the official FAP, debug ELF, package evidence and build log. Preserved/current FAP SHA256: `af74bfb1cdcc8fc3bf319d97b8fe056b563474cc39da223a1f87cd005ee37d30`. Local archive evidence is `logs/momentum-archive-validation.json`; all binaries/logs remain ignored and unpublished.

Minimum next input: distributor-confirmed SDK-only metadata/archive with API 89.1 attributable to the full installed revision, or authoritative clarification of why the downloaded revision-labelled SDK reports 87.1 while the installed device reports 89.1. Do not change the device firmware to resolve this. Once resolved, use an isolated SDK home, supported local ZIP update, clean app rebuild and packaged import/API checks with radios disabled.

All nine host suites, short-enum variants and ASan/UBSan suites, eight SDK-preflight cases and four publication cases pass for this documentation/validation milestone. No application source changes or device tests occurred.
