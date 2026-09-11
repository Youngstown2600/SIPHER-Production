#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT"
grep -q 'SIPHER_VERSION "2.1"' include/sipher/Version.h
grep -q 'DID / NUMBER INTELLIGENCE' src/gui/MainWindow.cpp
grep -q 'USACallerLookup' src/gui/MainWindow.cpp
grep -q 'www.usacallerlookup.com/wp-json/ucl/v1/number/' src/gui/MainWindow.cpp
! grep -q 'OPEN SPAMCALLS PAGE' src/gui/MainWindow.cpp
grep -q 'spamcalls.net/en/num/' src/gui/DidIntelHelpers.cpp
! grep -q 'IPQS-KEY' src/gui/MainWindow.cpp
! grep -q 'SIPHER_IPQS_API_KEY' src/gui/MainWindow.cpp
grep -q 'CARRIER HANDOFF / NEXT-OUT' src/gui/MainWindow.cpp
grep -q 'Actual INVITE peer' src/gui/MainWindow.cpp
grep -q 'dst_name' src/core/SipWireMonitor.cpp
grep -q 'pkt_info.src_name' src/core/SipWireMonitor.cpp
grep -q 'pkt_info.src_port' src/core/SipWireMonitor.cpp
! grep -q 'pkt_info.addr' src/core/SipWireMonitor.cpp
! grep -q 'auto emit=' src/gui/MainWindow.cpp
grep -q 'appendHeaderList' src/gui/MainWindow.cpp
grep -q 'peerAddress' include/sipher/SipTrace.h
grep -q 'UDP/TCP TRANSPORT PARITY' src/gui/MainWindow.cpp
grep -q 'TOPOLOGY / INFORMATION EXPOSURE' src/gui/MainWindow.cpp
grep -q '&Legacy' src/gui/MainWindow.cpp
grep -q 'Blue Tone / Blue Box' src/gui/MainWindow.cpp
grep -q 'Red Box' src/gui/MainWindow.cpp
grep -q 'LOCAL AUDIO DEMO: IDLE — NO NETWORK OUTPUT' src/gui/MainWindow.cpp
# Legacy panels must remain local visual/history simulators, not live signaling/audio controls.
legacy=$(sed -n '/void MainWindow::showBlueBoxLegacy()/,/static AuditTransport guiAuditTransport/p' src/gui/MainWindow.cpp)
printf '%s' "$legacy" | grep -q 'LOCAL AUDIO DEMO'
if printf '%s' "$legacy" | grep -Eq 'sendDtmf|playTone|dial\(|makeCall|pjsua_call|pjmedia_tonegen'; then
  echo 'legacy simulator unexpectedly contains live call/tone control' >&2
  exit 1
fi
grep -q 'COMPONENTS Core Widgets Network' CMakeLists.txt
echo 'r18 DID/route/audit features + Linux/PJSIP/Qt hotfix contract passed under SIPHER 2.1'
