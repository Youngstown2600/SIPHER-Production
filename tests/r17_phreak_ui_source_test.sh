#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT"
grep -q 'TRUNKMONKEY_VERSION "1.0.0-r17-Exploit-Fix"' include/trunkmonkey/Version.h
grep -q 'PHREAK LAB' src/gui/MainWindow.cpp
grep -q 'PhreakRail' src/gui/MainWindow.cpp
grep -q 'WireHeader' src/gui/MainWindow.cpp
grep -q 'CARRIER ACCESS' src/gui/MainWindow.cpp
grep -q 'phreak> ' src/cli/CliDashboard.cpp
grep -q 'PHREAK DECK' src/cli/CliDashboard.cpp
grep -q '╔' src/cli/CliDashboard.cpp
grep -q 'black-ice' src/cli/CliDashboard.cpp
grep -q 'terminal-gold' src/cli/CliDashboard.cpp
grep -q 'Black Ice' src/gui/MainWindow.cpp
grep -q 'Terminal Gold' src/gui/MainWindow.cpp
test -s src/gui/assets/sipher-logo.png
! grep -q 'CARRIER OPERATIONS CONSOLE' src/gui/MainWindow.cpp
grep -q 'LIVE WIRE // CALL TAP' src/cli/CliDashboard.cpp
grep -q 'activeCallStatsLines' src/cli/CliDashboard.cpp
grep -q 'lastLiveCallRefresh' src/cli/main.cpp
grep -q 'lastMainHadLiveCall' src/cli/main.cpp
grep -q 'dashboard.render(liveState,frame,false)' src/cli/main.cpp
grep -q '\\033\[?25l\\033\[H' src/cli/CliDashboard.cpp
printf '%s\n' 'r17 underground phreak GUI/CLI source contract passed'
