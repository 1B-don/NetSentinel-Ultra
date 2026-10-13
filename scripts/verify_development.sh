#!/usr/bin/env bash
# Local software/build checks only: no network, device access, SDK update or deployment.
set -euo pipefail
cd -- "$(dirname -- "$0")/.."
variant="${1:-development}"
case "$variant" in
    development) artifact=netsentinel_development.fap ;;
    lab-sim) artifact=netsentinel_lab_sim.fap ;;
    rc) artifact=netsentinel_ultra_v1_0_rc1.fap ;;
    esp-status) echo "Owner-verified ESP Status is frozen; use esp-stable or wifi-dev" >&2; exit 2 ;;
    esp-stable) artifact=netsentinel_esp_stable.fap ;;
    wifi-dev) artifact=netsentinel_wifi_dev.fap ;;
    *) echo "Usage: $0 [development|lab-sim|rc|esp-status|esp-stable|wifi-dev]" >&2; exit 2 ;;
esac
mkdir -p logs
make test > logs/v1-final-host.log 2>&1
make short-enums > logs/v1-final-short.log 2>&1
ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=0}" make sanitize > logs/v1-final-sanitize.log 2>&1
python3 scripts/build_esp_info.py --variant "$variant" > logs/v1-final-build.log 2>&1
python3 scripts/validate_fap.py "dist/$artifact" --require-icon > logs/v1-final-package.json
"$HOME/.ufbt/toolchain/x86_64-linux/bin/arm-none-eabi-nm" \
    --undefined-only --format=posix "dist/$artifact" > builds/sdk-input/v1-final-imports.txt
python3 scripts/sdk_preflight.py builds/sdk-input/api_symbols.csv \
    --firmware-revision d3f89dfe2ef6b01839201598e9be1590cba80322 \
    --undefined-symbols builds/sdk-input/v1-final-imports.txt > logs/v1-final-preflight.log
python3 scripts/publication_audit.py > logs/v1-final-publication.log
git diff --check
cat logs/v1-final-package.json
