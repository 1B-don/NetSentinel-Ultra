# First clean Momentum SDK build — physical tests pending

## Compatibility evidence

The owner independently queried the running Flipper firmware through USB CLI: Momentum Dev, revision d3f89dfe, August 18, 2026, hardware target 7, application API **87.1**. The previous 89.1 report is superseded. Owner identifies the full revision as `d3f89dfe2ef6b01839201598e9be1590cba80322`; embedded firmware metadata and CLI output expose only the short revision, not independently the full hash.

The permitted [public SDK distribution](https://up.momentum-fw.dev/builds/firmware/dev/flipper-z-f7-sdk-mntm-dev-d3f89dfe.zip) was supplied locally. SHA256 `238db260ee7e8f6e9a79dd1c760e07a832351a7d92d896b4cb19082f8548b2c1` matches the owner checksum. All 626 members pass CRC and safe-path/symlink inspection; no bundled AGENTS instructions found. Component metadata is f7 / mntm-dev-d3f89dfe. Export table, bundled firmware loader interface and API-reporting function agree on 87.1. SDK compiler/linker settings and build scripts were reviewed; firmware-flashing README operations were not used. No restricted source checkout was accessed or SDK code vendored into application source.

## Build workflow

uFBT 0.2.6 installed the unmodified local ZIP through its supported update command. Separate home `~/.ufbt-netsentinel-momentum` retains the working official `~/.ufbt` SDK. The existing authorized ARM GCC 12.3.1/toolchain is reused through `FBT_TOOLCHAIN_PATH`. Run from the application directory:

```sh
UFBT_HOME="$HOME/.ufbt-netsentinel-momentum" FBT_TOOLCHAIN_PATH="$HOME/.ufbt" \
  "$HOME/flipper-build-env/bin/ufbt" update --hw-target f7 \
  --local "$PWD/builds/sdk-input/flipper-z-f7-sdk-mntm-dev-d3f89dfe.zip"
UFBT_HOME="$HOME/.ufbt-netsentinel-momentum" FBT_TOOLCHAIN_PATH="$HOME/.ufbt" \
  "$HOME/flipper-build-env/bin/ufbt" -c
UFBT_HOME="$HOME/.ufbt-netsentinel-momentum" FBT_TOOLCHAIN_PATH="$HOME/.ufbt" \
  "$HOME/flipper-build-env/bin/ufbt"
```

Setup issues resolved without SDK edits: local installation first required explicit `--hw-target f7`; an SDK home nested under the app caused SCons `Tried to lookup Dir .../src as a File` before compilation. Reinstalling through the supported command into the separate home above resolved this collision. The unsuccessful nested cache is local/ignored and unused. Neither export tables nor FAP metadata were patched. The SDK's local state reports version `unknown`; actual component metadata, archive hash and API table provide recorded provenance rather than relying on that generic state field.

Clean and build returned exit 0. All **12** application C files compile with SDK flags including `-Werror`. Link, APPMETA, FAP/FASTFAP, output installation to local dist and APPCHK all pass. No compiler/linker errors remain. Installation in the build output here means local file copying, not device deployment.

## Artifact

| Field | Actual value |
|---|---|
| FAP | dist/netsentinel.fap |
| Bytes | 33,480 |
| SHA256 | de38c1c7e08336a1ab5a3ebe6dcf2865ac8aa5ce9dedb97c8f8c17c024b859b5 |
| Format | ELF32 little-endian ARM relocatable, EABI5 |
| Metadata | magic 0x52474448, manifest 1, API 87.1, target 7 |
| Source | 4951aabcde2e2851c72b830ad2dbfd6205ab2145; subsequent milestone changes documentation only |
| Imports | 49 required, zero undefined weak; every function/data name enabled in Momentum table |
| Radio configuration | NS_GUI_STORAGE_ONLY on all 12 compiler commands; NS_ENABLE_BOARD_PORT absent |
| Physical launch/GUI/SD | Not performed |

Debug ELF: `dist/debug/netsentinel_d.elf`. Original official FAP/debug/evidence remain separately in ignored `builds/preserved-official-api87/`; original FAP SHA256 `af74bfb1cdcc8fc3bf319d97b8fe056b563474cc39da223a1f87cd005ee37d30`. Official SDK cache was not changed.

No packaged UART/SPI/GPIO/Sub-GHz HAL imports exist. All ESP32/NRF24/CC1101 capabilities remain disabled. Packaged imports were obtained directly with ARM nm (not the debug-only symbol list), checked against the matching export table, and supplied to the existing strict preflight with owner full-revision provenance. Preflight alone validates names; actual SDK compile/link/APPCHK and direct metadata inspection provide additional build evidence.

Full logs/evidence remain local: `logs/momentum-sdk-install-external-cache.log`, `logs/momentum-clean-external-cache.log`, `logs/momentum-first-build.log`, `logs/momentum-build-evidence.json`, `logs/momentum-final-imports.txt`, `logs/momentum-import-preflight.log`. No binaries, logs or device captures are published.

## Tests and remaining risks

Nine host C suites, eight preflight cases and four publication cases pass. All nine short-enum and ASan/UBSan suites pass (local leak detection disabled). Strict actual-import preflight and publication/whitespace audits pass. Physical correctness is still unverified: allocation, input/lifecycle, loader/relocation behavior, SD save/readback and cleanup need approved on-device tests. Same API/build identity is stronger compatibility evidence than the official build, but does not prove those runtime outcomes.

The [first device test procedure](FIRST_DEVICE_TEST.md) remains approval-gated. Use this exact Momentum artifact/hash for the first test and back up existing app/data first. No firmware update, permission changes, RF testing, adapter enablement or GitHub Release is authorized.
