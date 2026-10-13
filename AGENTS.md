# NetSentinel Ultra development instructions

GitHub is the primary development repository: https://github.com/1B-don/NetSentinel-Ultra (origin). Continue toward the complete v1.0 application; do not create intermediate downloads or GitHub Releases.

## Boundaries

- Do not access, modify, build in, copy from or otherwise use the restricted Momentum firmware checkout. Do not bypass repository instructions. Use permitted public documentation, owner-provided permitted source and independently authored code/mocks.
- Keep application source here. Exact installed firmware target is Momentum d3f89dfe2ef6b01839201598e9be1590cba80322. Current upstream docs/shims are not evidence of compatible SDK exports or ABI.
- Receive-only radio work. No flashing/replacing firmware, erasing storage, changing USB permissions or RF transmissions. Obtain explicit owner approval before deploying or launching on the physical Flipper. Ask for physical selector changes when needed.
- ESP32 scan commands stay disabled until the installed firmware command/output and passive behavior are verified. BLE active discovery is not authorized.
- CC1101 prior operation is user-reported; simulated SPI/RSSI tests and formal physical RF verification are distinct. Lack of a controlled signal source does not block software work.
- Do not claim a working FAP before actual compilation and verification. No v1.0 release until all required physical tests pass.

## Automatic milestone workflow

Routine commits and non-forced pushes to origin are authorized when permitted by the execution environment. Do not request routine approval again.

1. Complete a concrete milestone and review source changes, GUI/input/lifecycle, radio ownership and cleanup.
2. Run all applicable automated tests. Preserve the existing seven suites; run host compile with warnings as errors and ASan/UBSan for relevant changes. Fix failures before publishing. Host API shims do not establish SDK compilation. Do not expand simulations instead of closing application integration gaps.
3. Review the entire proposed publication and unpublished commits for secrets, credentials, personal information, private configuration, logs and captured network data. Never publish these, even if ignored files were previously tracked. Synthetic fixtures must be labeled and contain invented data.
4. Update README and sanitized development documentation with implemented features, tests, exact blockers and limitations. Keep runtime/compiler logs and device captures local under ignored logs/build directories. Record publish failures in documentation without credentials/private output.
5. Stage only appropriate source, tests, build scripts and documentation; inspect the staged diff. Create a descriptive Git commit.
6. Push the tested milestone automatically to this repository with a normal explicit branch refspec. Never force-push or rewrite published history. Never publish GitHub Releases before verified v1.0 completion.
7. Verify remote branch SHA equals the milestone SHA with a separate remote query; optionally record remote CI separately from local results. Do not treat a failed/blocked push as published.
8. Report milestone name, commit SHA, local tests, push status and GitHub link. If environment restrictions prevent push, retain the tested commit and report the exact blocker. Continue unrelated permitted development.

If historical unpublished commits contain logs/private material, do not push them. Preserve local checkpoint history and prepare a sanitized source milestone on a new publication branch from the existing remote history; no force or published-history rewrite. Do not delete historical published material automatically. Document any pre-existing historical material separately.

## Current priority

Complete real Flipper integration: GUI/input/navigation/lifecycle, typed adapter boundaries, exact firmware-facing symbol inventory, SDK build readiness and mock API error/cancellation/cleanup coverage. External NRF/CC capabilities require a validated, explicitly external board port. Avoid guessed wiring or internal-radio fallback. Report instruction conflicts and continue unaffected work.

Publication may use the installed GitHub connector when shell Git networking is unavailable: create a source tree/commit and advance the remote ref without force, with an expected-head check, then verify by a separate read. Align the local primary branch to that exact verified commit, preserving any unpublished checkpoint history on a clearly local branch. Never push that checkpoint branch or its log-bearing ancestors.

Local `master` is the sanitized primary publication branch and tracks `origin/master`. `local/checkpoint-flipper-integration` preserves pre-migration local history and must never be pushed. Use `git push origin master:master` after audit/tests, then query the remote SHA. Commit author email is configured locally to the repository owner's public GitHub noreply address; do not replace it with private email. The owner published ab8e708 and its remote SHA was independently verified. Attempt normal milestone pushes and verify separately; if shell networking fails, preserve the commit and report the current failure. Prior connector tree creation returned HTTP 403; do not repeatedly retry missing connector permissions.

When GitHub DNS fails in this execution environment, retain the tested local commit, record its SHA/results and continue the next authorized milestone. Do not repeatedly retry the unavailable connection. List all commits requiring manual publication. Resume normal fast-forward publishing once connectivity is known to have changed; retain tests and privacy audits for the entire pending range.

The owner explicitly authorized independent official SDK 1.4.3 compilation. uFBT 0.2.6 at `~/flipper-build-env/bin/ufbt` and the permitted SDK-only cache may be used for build-only work after applicable instruction review. The minimal official-SDK FAP has compiled/linked/packaged and passed APPCHK; this does not establish installed Momentum compatibility or physical functionality. Keep NS_GUI_STORAGE_ONLY enabled, preserve target provenance and require approval before deployment/launch. Never read/use/flash cached firmware images as part of application work. Exact target SDK/ABI and physical tests remain separate gates.

Owner independently queried USB CLI and confirmed Momentum Dev d3f89dfe, August 18, 2026, application API 87.1, target 7; this supersedes the earlier 89.1 report and resolves the SDK mismatch stop. The supplied public ZIP checksum is 238db260ee7e8f6e9a79dd1c760e07a832351a7d92d896b4cb19082f8548b2c1. Its SDK table and bundled firmware ELF internally agree on 87.1. Permitted SDK instructions reviewed; matching clean build and all 49 import checks pass. Use isolated UFBT_HOME ~/.ufbt-netsentinel-momentum with authorized FBT_TOOLCHAIN_PATH ~/.ufbt; supported local update requires --hw-target f7. Keep official SDK cache and builds/preserved-official-api87 evidence intact. See docs/MOMENTUM_BUILD.md. Hardware adapters stay disabled and physical deployment/launch still require explicit approval. Never falsify metadata, bypass import checks or access restricted firmware source.
