#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT"
grep -q 'ModernStyle.cpp' CMakeLists.txt
grep -q 'sipher_resources.qrc' CMakeLists.txt
grep -q 'AUTORCC ON' CMakeLists.txt
grep -q ':/sipher/logo.png' src/gui/MainWindow.cpp
grep -q 'Black Ice' src/gui/MainWindow.cpp
grep -q 'High Contrast' src/gui/MainWindow.cpp
grep -q 'black-ice' src/gui/ModernStyle.cpp
grep -q 'high-contrast' src/cli/CliDashboard.cpp
test -s src/gui/assets/sipher-logo.png
grep -q 'setObjectName(QStringLiteral("Sidebar"))' src/gui/MainWindow.cpp
grep -q 'Phone & Capture' src/gui/MainWindow.cpp
grep -q 'PBX Audit' src/gui/MainWindow.cpp
grep -q 'role","primary"' src/gui/MainWindow.cpp
grep -q 'QPushButton\[nav="true"\]' src/gui/ModernStyle.cpp
grep -q 'border-radius:14px' src/gui/ModernStyle.cpp
grep -q 'CONTROL DECK' src/cli/CliDashboard.cpp
grep -q 'u8"╭' src/cli/CliDashboard.cpp
grep -q 'u8"❯ ' src/cli/CliDashboard.cpp
grep -q '1.0.0-r16' include/trunkmonkey/Version.h
echo "r16 modern GUI/CLI source contract passed"
