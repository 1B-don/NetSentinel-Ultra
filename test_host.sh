#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
DIR="$(mktemp -d)"
trap 'rm -rf "$DIR"' EXIT
gcc -std=c11 -Wall -Wextra -Werror -pedantic ap_parser.c tests/test_parser.c -o "$DIR/test_parser"
"$DIR/test_parser"
gcc -std=c11 -Wall -Wextra -Werror -pedantic ns_analysis.c tests/test_analysis.c -o "$DIR/test_analysis"
"$DIR/test_analysis"
printf 'Host tests passed; FAP compilation and hardware verification are pending.\n'
