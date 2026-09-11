#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT"

grep -q 'TRUNKMONKEY_VERSION "2.0"' include/trunkmonkey/Version.h
grep -q 'project(SIPHERClient VERSION 2.0.0' CMakeLists.txt
grep -q 'OUTPUT_NAME "sipher"' CMakeLists.txt
grep -q 'SIPHER_UNIFIED_ENTRY=1' CMakeLists.txt
! grep -q 'OUTPUT_NAME "sipher-gui"' CMakeLists.txt
! grep -q 'OUTPUT_NAME "sipher-cli"' CMakeLists.txt

grep -q 'terminalAttached()' src/app/main.cpp
grep -q 'guiSessionAvailable()' src/app/main.cpp
grep -q 'SIPHER_UI' src/app/main.cpp
grep -q 'arg == "--cli"' src/app/main.cpp
grep -q 'arg == "--gui"' src/app/main.cpp
grep -q 'return sipherRunCli' src/app/main.cpp
grep -q 'return sipherRunGui' src/app/main.cpp
grep -q 'int sipherRunCli' src/cli/main.cpp
grep -q 'int sipherRunGui' src/gui/main.cpp

grep -q 'BINARY_NAME="sipher"' scripts/build-macos.sh
grep -q 'BINARY_NAME="sipher"' scripts/build-termux.sh
! grep -q 'GUI_NAME="sipher-gui"' scripts/build-macos.sh
! grep -q 'GUI_NAME="sipher-gui"' scripts/build-termux.sh

# Current packaging/runtime paths must expose one public binary name.
! grep -q 'cp .*sipher-gui' windows/build-portable.sh
! grep -q 'sipher-gui.exe' windows/build-portable.sh

echo 'SIPHER 2.0 unified single-binary source contract passed'
