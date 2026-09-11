#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT"
CXX=${CXX:-c++}
TMPBASE=${TMPDIR:-/tmp}/sipher-r19-tests-$$
trap 'rm -f "$TMPBASE"-*' EXIT INT TERM
./tests/r19_multi_account_source_test.sh
./tests/all_platform_builder_source_test.sh
./tests/freebsd_audio_compat_test.sh
compile_run(){ name=$1; shift; echo "==> $name"; "$CXX" -std=c++17 -Wall -Wextra -Wpedantic -Iinclude "$@" -o "$TMPBASE-$name"; "$TMPBASE-$name"; }
compile_run profile tests/profile_test.cpp src/core/Profile.cpp src/core/TextPool.cpp
compile_run sip-trace tests/sip_trace_test.cpp src/core/SipTrace.cpp
compile_run runtime-paths tests/runtime_paths_test.cpp src/core/RuntimePaths.cpp
compile_run cli-dashboard -Isrc/cli tests/cli_dashboard_test.cpp src/cli/CliDashboard.cpp src/core/Profile.cpp
compile_run capture-manager tests/capture_manager_test.cpp src/core/CaptureManager.cpp src/core/Logger.cpp -pthread
compile_run pbx-audit tests/pbx_audit_test.cpp src/core/PbxAudit.cpp src/core/RuntimePaths.cpp -pthread
echo 'SIPHER 2.0 r19 feature regression gate passed'
