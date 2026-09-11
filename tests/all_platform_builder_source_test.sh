#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT"
grep -q -- '--os linux|freebsd|macos|termux' build.sh
grep -q 'Darwin' build.sh
test -x scripts/build-macos.sh
test -x scripts/build-termux.sh
grep -q 'macdeployqt' scripts/build-macos.sh
grep -q 'hdiutil' scripts/build-macos.sh
grep -q 'Xcode' scripts/build-macos.sh
grep -q 'brew' scripts/build-macos.sh
grep -q 'x11-repo' scripts/build-termux.sh
grep -q 'TERMUX_VERSION' scripts/build-termux.sh
grep -q 'PJSIP' scripts/build-termux.sh
grep -q 'Darwin' scripts/build-pjsip.sh
grep -q 'TERMUX_SYS_PREFIX' scripts/build-pjsip.sh
grep -q 'MACOSX_BUNDLE TRUE' CMakeLists.txt
grep -q 'BUNDLE DESTINATION' CMakeLists.txt
sh -n build.sh
sh -n scripts/build-macos.sh
sh -n scripts/build-pjsip.sh
bash -n scripts/build-termux.sh
printf '%s\n' 'SIPHER 2.0 all-platform builder source contract passed'
