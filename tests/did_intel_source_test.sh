#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT"
test -s src/gui/DidIntelHelpers.cpp
test -s src/gui/DidIntelHelpers.h
grep -q 'USACallerLookup' src/gui/MainWindow.cpp
grep -q 'usacallerlookup.com/wp-json/ucl/v1/number/' src/gui/MainWindow.cpp
grep -q 'usaCallerLookupHtmlUrl' src/gui/MainWindow.cpp
grep -q 'parseUsaCallerLookupHtml' src/gui/DidIntelHelpers.cpp
grep -q 'spam.skipcalls.com/check/' src/gui/MainWindow.cpp
grep -q 'parseSkipCalls' src/gui/DidIntelHelpers.cpp
grep -q 'is_spam' src/gui/DidIntelHelpers.cpp
grep -q 'spamcalls.net/en/num/' src/gui/DidIntelHelpers.cpp
grep -q 'tellows.com/num/' src/gui/DidIntelHelpers.cpp
grep -q 'c-qui.fr/' src/gui/DidIntelHelpers.cpp
grep -q 'api.data247.com/v3.0' src/gui/MainWindow.cpp
grep -q 'SIPHER_DATA247_API_KEY' src/gui/MainWindow.cpp
grep -q 'parseData247Carrier' src/gui/DidIntelHelpers.cpp
grep -q 'api.veriphone.io/v3/verify' src/gui/MainWindow.cpp
grep -q 'SIPHER_VERIPHONE_API_KEY' src/gui/MainWindow.cpp
grep -q 'parseVeriphoneCarrier' src/gui/DidIntelHelpers.cpp
grep -q 'carrier-lookup-api.omkar.cloud/lookup' src/gui/MainWindow.cpp
grep -q 'SIPHER_OMKAR_API_KEY' src/gui/MainWindow.cpp
grep -q 'parseOmkarCarrier' src/gui/DidIntelHelpers.cpp
grep -q 'mode",QStringLiteral("static")' src/gui/MainWindow.cpp
grep -q 'setTransferTimeout(7000)' src/gui/MainWindow.cpp
grep -q 'neutrinoapi.net/hlr-lookup' src/gui/MainWindow.cpp
grep -q 'SIPHER_NEUTRINO_USER_ID' src/gui/MainWindow.cpp
grep -q 'SIPHER_NEUTRINO_API_KEY' src/gui/MainWindow.cpp
grep -q 'ENHANCED HLR / CURRENT CARRIER' src/gui/MainWindow.cpp
! grep -q 'OPEN SPAMCALLS PAGE' src/gui/MainWindow.cpp
! grep -Rqi --exclude-dir=.git 'IPQualityScore' src include CMakeLists.txt build.sh scripts
grep -q 'createLocalDemoTone' src/gui/MainWindow.cpp
grep -q 'playLocalDemoTone' src/gui/MainWindow.cpp
grep -q 'LOCAL AUDIO DEMO: IDLE — NO NETWORK OUTPUT' src/gui/MainWindow.cpp
grep -q 'return xdg / "sipher"' src/core/RuntimePaths.cpp
grep -q 'share/sipher/examples' CMakeLists.txt
grep -q 'share/doc/sipher' CMakeLists.txt
echo 'SIPHER 2.1 DID waterfall / local-audio / namespace contract passed'
