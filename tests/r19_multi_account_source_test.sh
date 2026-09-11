#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT"

grep -q 'TRUNKMONKEY_VERSION "2.0"' include/trunkmonkey/Version.h
grep -q 'void start(unsigned maxCalls=50)' include/trunkmonkey/SipEngine.h
grep -q 'std::map<std::string,std::unique_ptr<ManagedAccount>> accounts_' include/trunkmonkey/SipEngine.h
grep -q 'addAccount(const SipProfile' include/trunkmonkey/SipEngine.h
grep -q 'removeAccount(const std::string& accountId)' include/trunkmonkey/SipEngine.h
grep -q 'setActiveAccount(const std::string& accountId)' include/trunkmonkey/SipEngine.h
grep -q 'No SIP accounts configured' src/core/SipEngine.cpp
grep -q 'natUpdateStunServers' src/core/SipEngine.cpp
grep -q 'accountId_' include/trunkmonkey/SipAccount.h
grep -q 'engine_.onRegistrationState(accountId_' src/core/SipAccount.cpp
grep -q 'engine_.onIncomingCall(accountId_' src/core/SipAccount.cpp
grep -q 'std::string accountId;' include/trunkmonkey/CallSnapshot.h

grep -q 'SIP &Accounts...' src/gui/MainWindow.cpp
grep -q 'MANAGE SIP ACCOUNTS' src/gui/MainWindow.cpp
grep -q 'No SIP accounts configured' src/gui/MainWindow.cpp
grep -q 'USE FOR OUTBOUND' src/gui/MainWindow.cpp
grep -q 'accountSelector_' src/gui/MainWindow.cpp
grep -q 'accountsDir' src/gui/main.cpp
! grep -q 'First-Run SIP Setup' src/gui/ProfileDialog.cpp
! grep -q 'createDefaultIfMissing(profilePath.string())' src/gui/main.cpp

grep -q 'DID / NUMBER INTELLIGENCE' src/gui/MainWindow.cpp
grep -q 'USACallerLookup' src/gui/MainWindow.cpp
grep -q 'OPEN SPAMCALLS REPUTATION' src/gui/MainWindow.cpp
! grep -q 'IPQS-KEY' src/gui/MainWindow.cpp
grep -q 'CARRIER HANDOFF / NEXT-OUT' src/gui/MainWindow.cpp
grep -q 'UDP/TCP TRANSPORT PARITY' src/gui/MainWindow.cpp
grep -q 'TOPOLOGY / INFORMATION EXPOSURE' src/gui/MainWindow.cpp
grep -q '&Legacy' src/gui/MainWindow.cpp
legacy=$(sed -n '/void MainWindow::showBlueBoxLegacy()/,/static AuditTransport guiAuditTransport/p' src/gui/MainWindow.cpp)
printf '%s' "$legacy" | grep -q 'visualization only'
if printf '%s' "$legacy" | grep -Eq 'sendDtmf|playTone|dial\(|makeCall|pjsua_call|pjmedia_tonegen'; then
  echo 'legacy simulator unexpectedly contains live call/tone control' >&2
  exit 1
fi

grep -q 'verifyServer=true' src/core/SipEngine.cpp
grep -q 'nameserver.clear()' src/core/SipEngine.cpp
grep -q 'MaxAuditResponseBytes' src/core/PbxAudit.cpp
grep -q 'sanitizeTerminalText' src/cli/CliDashboard.cpp
grep -q 'mkdtemp' src/core/RuntimePaths.cpp
grep -q 'apply-pjsip-exploit-fixes.sh' scripts/build-pjsip.sh

echo 'r19 multi-account / zero-account feature contract passed under SIPHER 2.0'
