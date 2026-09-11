#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT"
grep -q 'TRUNKMONKEY_VERSION "1.0.0-r18-DID-Route-Audit"' include/trunkmonkey/Version.h
grep -q 'DID / NUMBER INTELLIGENCE' src/gui/MainWindow.cpp
grep -q 'IPQS-KEY' src/gui/MainWindow.cpp
grep -q 'SIPHER_IPQS_API_KEY' src/gui/MainWindow.cpp
grep -q 'CARRIER HANDOFF / NEXT-OUT' src/gui/MainWindow.cpp
grep -q 'Actual INVITE peer' src/gui/MainWindow.cpp
grep -q 'dst_name' src/core/SipWireMonitor.cpp
grep -q 'peerAddress' include/trunkmonkey/SipTrace.h
grep -q 'UDP/TCP TRANSPORT PARITY' src/gui/MainWindow.cpp
grep -q 'TOPOLOGY / INFORMATION EXPOSURE' src/gui/MainWindow.cpp
grep -q '&Legacy' src/gui/MainWindow.cpp
grep -q 'Blue Tone / Blue Box' src/gui/MainWindow.cpp
grep -q 'Red Box' src/gui/MainWindow.cpp
grep -q 'NO AUDIO / NO NETWORK OUTPUT' src/gui/MainWindow.cpp
# Legacy panels must remain local visual/history simulators, not live signaling/audio controls.
legacy=$(sed -n '/void MainWindow::showBlueBoxLegacy()/,/static AuditTransport guiAuditTransport/p' src/gui/MainWindow.cpp)
printf '%s' "$legacy" | grep -q 'visualization only'
if printf '%s' "$legacy" | grep -Eq 'sendDtmf|playTone|dial\(|makeCall|pjsua_call|pjmedia_tonegen'; then
  echo 'legacy simulator unexpectedly contains live call/tone control' >&2
  exit 1
fi
grep -q 'COMPONENTS Core Widgets Network' CMakeLists.txt
echo 'r18 DID/route/audit source contract passed'
