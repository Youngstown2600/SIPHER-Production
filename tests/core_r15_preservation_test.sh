#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT"
tmp=$(mktemp "${TMPDIR:-/tmp}/sipher-core.XXXXXX")
expected=$(mktemp "${TMPDIR:-/tmp}/platform-filtered-baseline.XXXXXX")
trap 'rm -f "$tmp" "$expected"' EXIT INT TERM
grep -v -E '  scripts/(build-pjsip\.sh|build-macos\.sh|build-termux\.sh)$' tests/core-r15-exploit-fix.sha256 > "$expected"
find src/core include/trunkmonkey scripts -type f ! -path '*/Version.h' \
  ! -path 'scripts/build-pjsip.sh' ! -path 'scripts/build-macos.sh' ! -path 'scripts/build-termux.sh' \
  -print0 | sort -z | xargs -0 sha256sum > "$tmp"
diff -u "$expected" "$tmp"
echo "r15 protocol/audio/audit core security-hardened baseline preserved; platform builders intentionally excluded"
