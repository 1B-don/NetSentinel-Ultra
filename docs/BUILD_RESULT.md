# Genuine FAP build results

## Latest enum-validation rebuild

The requested type-limit comparisons were already corrected in local commit `1249df0`. This follow-up bounds result-name indexing by the actual table with a completeness assertion, retains unsigned input/result range checks, and validates full 32-bit stored radio/result words before narrowing to enums. Normal/short-enum/sanitizer tests reject negative/out-of-range inputs and CRC-valid malformed history, preserve older valid snapshot recovery, and prohibit invalid persistence writes. No warning suppression or build-flag change was made.

`~/flipper-build-env/bin/ufbt` returned exit 0: changed sources compile, link, packaging and APPCHK pass, hardware target 7 / API 87.1. No compiler/linker errors remain. SDK remains official 1.4.3; installed Momentum compatibility and physical operation remain unverified. Current artifact `dist/netsentinel.fap` is **33,480 bytes**, SHA256 `06cd1717590bd6658d2914a128432b8fcb9d0110ca463e3c77222ac4e5996b27`. Full output is local in `logs/enum-validation-build.log`. The earlier clean-build evidence below remains historical.

## Initial successful clean build

Application source commit: `47ce6b9e1830768f85a68ab9543dc240529bec58`. Documentation updates in the result milestone do not change compiled application source.

## Result

| Stage | Observed result |
|---|---|
| Manifest/configuration | EXTERNAL app; netsentinel_app; 8 KiB stack; NS_GUI_STORAGE_ONLY; 12 C sources; no tests/legacy |
| Compiler | ARM GCC 12.3.1; all 12 translation units compiled successfully in the clean build |
| SDK/build command | Independently permitted official SDK 1.4.3; uFBT 0.2.6 |
| Link and app metadata | PASS; debug ELF and packaged FAP generated |
| FAP packaging | PASS; ARM ELF32 relocatable, EABI5 |
| SDK APPCHK | PASS; hardware target 7, API 87.1 |
| Final import table | 49 required imports, all enabled in official API table; no serial HAL imports |
| Momentum d3f89dfe compatibility | UNVERIFIED |
| Physical launch/GUI/SD tests | NOT RUN |
| Hardware adapters | Disabled; no hardware operation performed |

Artifact: `dist/netsentinel.fap`, **33,476 bytes**.

SHA256:

```text
0d8dcabeeb0fbf0d57108e6bfeba48df036b69e5839cc2445135818c6dea6fbb
```

Debug ELF: `dist/debug/netsentinel_d.elf`. Logs: `logs/official-sdk-first-build.log`, `logs/official-sdk-app-clean.log`, `logs/official-sdk-full-rebuild.log`. Local structured evidence: `logs/official-sdk-build-evidence.json`; final import listing: `logs/official-sdk-final-imports.txt`. All remain ignored/local; no release package or GitHub Release was created.

## Commands and errors

Run from NetSentinel-Ultra:

```sh
~/flipper-build-env/bin/ufbt
# App build outputs only: clean and recompile to verify all translation units.
~/flipper-build-env/bin/ufbt -c
~/flipper-build-env/bin/ufbt
```

Initial build and clean rebuild returned exit 0. No source compiler, linker or missing-import errors occurred in this milestone; earlier ARM short-enum fixes were already present. An intervening attempt to force rebuilding with `-B` returned exit 2 and `SCons Error: no such option: -B`. This was an unsupported build option, not a source failure. It was replaced by the successful app clean/build sequence above; the SDK and firmware were not cleaned or flashed.

The SDK-generated final FAP import list passes the official export table. The debug ELF also exposes `uxTopUsedPriority`, which is absent from the final SDK import list; do not mistake the debug-only listing for the packaged app's runtime requirements. Prefer the SDK's final import/APPCHK evidence when evaluating load compatibility.

## Remaining compatibility and physical requirements

Obtain permitted installed-Momentum target/API/export metadata or its exact revision SDK-only archive, with provenance and usage instructions. Compare hardware target, API major/minor compatibility, the 49 final function/data imports, declaration/ABI and loader/relocation support. Official SDK APPCHK proves official export resolution, not installed-Momentum compatibility. Do not remove UART/radio guards to try loading an unverified driver.

After that review, obtain explicit approval for deployment/launch. Transfer the FAP through the supported USB file workflow into `apps/GPIO`, then verify launch/exit, navigation, settings save/relaunch, history/state recovery and actual SD behavior. Record device/app/SDK revisions, artifact hash, expected/observed results and logs. No firmware replacement, storage erase or USB permission changes are needed. Restore adapters only after their independent interface/protocol/wiring reviews and receive-only physical tests. Full v1.0 and any Release remain pending.

## Regression checks

Nine host C suites, eight SDK-preflight cases and four publication cases pass. Host short-enum variants and ASan/UBSan pass, with leak detection disabled in the local traced environment. Staged publication and whitespace audits pass; runtime/compiler logs and FAP binaries are excluded from publication.
