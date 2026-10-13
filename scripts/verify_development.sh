#!/usr/bin/env bash
# Local software/build checks only: no network, device access, SDK update or deployment.
set -euo pipefail
cd -- "$(dirname -- "$0")/.."
mkdir -p logs
make test > logs/v1-final-host.log 2>&1
make short-enums > logs/v1-final-short.log 2>&1
ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=0}" make sanitize > logs/v1-final-sanitize.log 2>&1
python3 scripts/build_esp_info.py --development > logs/v1-final-build.log 2>&1
python3 scripts/validate_fap.py dist/netsentinel_development.fap > logs/v1-final-package.json
"$HOME/.ufbt/toolchain/x86_64-linux/bin/arm-none-eabi-nm" \
    --undefined-only --format=posix dist/netsentinel_development.fap > builds/sdk-input/v1-final-imports.txt
python3 scripts/sdk_preflight.py builds/sdk-input/api_symbols.csv \
    --firmware-revision d3f89dfe2ef6b01839201598e9be1590cba80322 \
    --undefined-symbols builds/sdk-input/v1-final-imports.txt > logs/v1-final-preflight.log
python3 scripts/publication_audit.py > logs/v1-final-publication.log
git diff --check
cat logs/v1-final-package.json
