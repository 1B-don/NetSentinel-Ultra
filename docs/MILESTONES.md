# Development milestones and limitations

These are sanitized source/test summaries, not runtime logs or device captures. Raw compiler/runtime outputs stay local. Synthetic fixtures are invented protocol examples and are labeled separately.

- Initial owner-provided v0.2 import and portable radio core: parser/analysis tests, receive-only NRF register logic, CC callback boundary, UART/session mocks. Import licensing remains unresolved for distribution.
- Unified application/persistence: source GUI worker, fixed-path SD adapter, two-slot settings/history, ID reservations and cleanup. Mock checks cover 420 torn-write boundaries, restart and SD failures.
- Marauder source integration: gated UART state machine, bounded ring, provisional info labels, synthetic AP parsing, Wi-Fi Explorer/baseline/CSV. Installed raw command/output and passive semantics remain pending; scan gate false, active BLE unsupported.
- CC receive exploration: read-only SPI helper, RSSI trace/threshold summaries, receive-only frequency controls and CSV, simulated fault tests. Prior Ultra CC operation is user-reported; formal RF verification pending. A controlled signal source is not needed to continue software work.
- Flipper integration and GitHub workflow: isolated typed board binding, validated callbacks, unified hardware ownership, actual input-callback/queue mock path, Back latch, initialization/record/mutex failures, forced cleanup. Original seven suites preserved; board-enabled API contract suite added. Both Flipper variants compile against independent mocks. Local GCC warning-as-error and ASan/UBSan checks plus four publication-audit regression cases pass; leak detection unavailable in the traced local environment. These checks are not SDK ABI or hardware validation.

GitHub is the primary repository. Shell Git networking cannot resolve GitHub in this environment; the installed GitHub connector can read the repository. Publication uses a sanitized source snapshot based on the existing published master commit, excluding later local log-bearing commits. Local checkpoints are retained separately, and no published history is rewritten. Raw logs are removed from the current tracked tree but retained locally; pre-existing published historical logs are not erased or rewritten automatically.

Remaining build blockers: appropriately authorized revision-compatible exported SDK interfaces, concrete Ultra external NRF/CC bindings and wiring/modem validation, installed ESP command/output capture, actual compile/link, and approved physical GUI/SD/resource/radio tests. No working FAP or intermediate release is claimed. Remote CI outcomes are separate from local testing.

## NRF receive-only adapter enforcement

The owner-published `ab8e7089b2d41bf82fa1706dfe58a3c714aec89b` was independently verified as remote master. The new milestone restricts Flipper NRF SPI dispatch to the scanner's two-byte register operations, RX-only CONFIG values, disabled ACK/retries/pipe/carrier settings and channels 0–125. Invalid lengths/pointers fail before board dispatch. Tests exhaust all 65,536 command/value combinations for rejection outside the envelope, assert zero board transfers for rejected operations, then run the actual portable driver across all channels through the guarded boundary.

All eight host C suites, four publication-audit cases and ASan/UBSan pass (leak detection disabled in this environment). These are source/mock results, not SDK or physical verification. No restricted checkout was accessed and no hardware operation or release was performed. Compatible authorized SDK exports and verified Ultra external SPI/CE/CS and CC device interfaces are still required before compilation; ESP installed response capture remains required before enabling scan commands.

Shell remote queries still fail here with GitHub DNS resolution errors despite the owner's successful push. The milestone workflow attempts a normal push and preserves the local commit on failure; final reporting records the outcome. The connector's previously rejected write permission is not retried. Local checkpoint ancestry must never be pushed.

## SDK export preflight and GUI teardown

Continued from `7d8111d33aa3f6efd261d509072d002457b3b641` (tested, local; manual publication required after shell DNS failure). Added a build-only export-name checker over the actual Flipper sources, strict enabled-function CSV parsing, API version/table SHA256 reporting, owner-confirmed full target revision gate and project-local input confinement. Six synthetic checker regression cases cover disabled/missing exports, malformed/duplicate tables, source-call discovery, wrong revision and external/symlink input rejection. No SDK was downloaded, accessed or built; actual target exports have not been provided.

GUI cleanup now clears both callbacks and detaches before session/storage finalization, without holding the frame mutex across viewport methods. Existing lifecycle/allocation/error/cancellation tests require cleared callbacks on viewport free. Public upstream GUI interface review informed ordering; installed revision synchronization remains unverified.

Verification: eight host C suites, four publication-audit cases and six SDK-preflight cases pass with warnings-as-errors and ASan/UBSan (leak detection disabled). The actual preflight reports missing target input, as expected; no compatible compilation or FAP exists. Concrete external NRF/CC SDK/wiring bindings and installed ESP command/output verification remain pending. Restricted firmware checkout untouched; no physical operations or release. Automatic publishing remains configured; the known unavailable GitHub connection is not retried in this milestone, and tested local commits are retained for manual publication.
