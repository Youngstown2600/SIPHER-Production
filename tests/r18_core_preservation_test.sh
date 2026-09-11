#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT"
tmp=$(mktemp "${TMPDIR:-/tmp}/sipher-r18-core.XXXXXX")
expected=$(mktemp "${TMPDIR:-/tmp}/sipher-r18-baseline.XXXXXX")
trap 'rm -f "$tmp" "$expected"' EXIT INT TERM
# r18 intentionally changes the SIP trace/wire monitor and PBX audit modules.
# Everything else in the r15 security-hardened core remains byte-identical.
grep -v -E '  (scripts/(build-pjsip\.sh|build-macos\.sh|build-termux\.sh)|include/sipher/(PbxAudit|SipTrace|SipWireMonitor)\.h|src/core/(PbxAudit|SipTrace|SipWireMonitor)\.cpp)$' \
  tests/core-r15-exploit-fix.sha256 > "$expected"
find src/core include/sipher scripts -type f ! -path '*/Version.h' \
  ! -path 'scripts/build-pjsip.sh' ! -path 'scripts/build-macos.sh' ! -path 'scripts/build-termux.sh' \
  ! -path 'include/sipher/PbxAudit.h' ! -path 'include/sipher/SipTrace.h' ! -path 'include/sipher/SipWireMonitor.h' \
  ! -path 'src/core/PbxAudit.cpp' ! -path 'src/core/SipTrace.cpp' ! -path 'src/core/SipWireMonitor.cpp' \
  -print0 | sort -z | xargs -0 sha256sum > "$tmp"
diff -u "$expected" "$tmp"
# Preserve the r17 exploit-fix source invariants even though the release name advanced.
grep -q 'verifyServer=true' src/core/SipEngine.cpp
grep -q 'nameserver.clear()' src/core/SipEngine.cpp
grep -q 'MaxAuditResponseBytes' src/core/PbxAudit.cpp
grep -q 'sanitizeTerminalText' src/cli/CliDashboard.cpp
grep -q 'mkdtemp' src/core/RuntimePaths.cpp
grep -q 'apply-pjsip-exploit-fixes.sh' scripts/build-pjsip.sh
grep -q 'uri_cnt >= PJ_ARRAY_SIZE(uri)' scripts/pjsip-2.17-exploit-fixes/0001-service-route-stack-overflow.patch
grep -q 'cr_attr_count >= PJ_ARRAY_SIZE(tags)' scripts/pjsip-2.17-exploit-fixes/0002-srtp-sdes-crypto-stack-overflow.patch
grep -q 'PJLIB_UTIL_ESTUNINVALIDID' scripts/pjsip-2.17-exploit-fixes/0003-simple-stun-response-authentication.patch
grep -q 'pj_strdup_with_null' scripts/pjsip-2.17-exploit-fixes/0004-tls-san-embedded-nul.patch
grep -q 'len = sizeof(out) - 1' scripts/pjsip-2.17-exploit-fixes/0005-gnutls-san-stack-overflow.patch
echo 'r18 core preservation/security gate passed'
