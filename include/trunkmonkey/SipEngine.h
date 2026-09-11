#pragma once
#include "trunkmonkey/CallSnapshot.h"
#include "trunkmonkey/Profile.h"
#include "trunkmonkey/SipTrace.h"
#include <pjsua2.hpp>
#include <atomic>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
namespace trunkmonkey {
class CallSession; class CaptureManager; class Logger; class SipAccount; class SipWireMonitor;
enum class CaptureKind;
struct AudioDeviceInfo { int id{-1}; std::string driver; std::string name; unsigned inputCount{0}; unsigned outputCount{0}; };
struct AudioStatusInfo {
    int captureId{-1};
    int playbackId{-1};
    bool soundActive{false};
    bool hotplugWatchAvailable{false};
    bool autoSwitchEnabled{true};
    std::string hotplugBackend;
    std::string captureDevice;
    std::string playbackDevice;
    std::string systemRoute;
};
struct SipAccountStatus {
    std::string id;
    std::string name;
    std::string username;
    std::string sipDomain;
    Transport transport{Transport::Udp};
    std::uint16_t localSipPort{5060};
    bool registered{false};
    bool activeOutbound{false};
    std::string registrationText{"Not registered"};
};
class SipEngine {
public:
    explicit SipEngine(Logger& logger); ~SipEngine();
    SipEngine(const SipEngine&)=delete; SipEngine& operator=(const SipEngine&)=delete;
    // r19: the endpoint can run with zero SIP accounts. Accounts may then be
    // added/removed independently while the rest of SIPHER stays usable.
    void start(unsigned maxCalls=50);
    void start(const SipProfile& profile,unsigned maxCalls=50); // compatibility helper
    void stop();
    bool started()const; bool registered()const; std::string registrationText()const;
    const SipProfile& profile()const;
    std::string addAccount(const SipProfile& profile,const std::string& requestedId={});
    void removeAccount(const std::string& accountId);
    void setActiveAccount(const std::string& accountId);
    std::string activeAccountId()const;
    bool hasAccounts()const;
    std::vector<SipAccountStatus> accounts()const;
    SipProfile accountProfile(const std::string& accountId)const;
    // Runtime dial prefix belongs to the currently selected outbound account.
    void setDialPrefix(const std::string& prefix);
    std::string dialPrefix()const;
    int makeCall(const std::string& destination,const std::string& callerId={},bool makeForeground=true,CallPurpose purpose=CallPurpose::Phone,bool applyDialPrefix=true);
    void answer(int id); void reject(int id,int code=603); void hangup(int id); void hangupAll();
    void hold(int id); void resume(int id); void sendDtmf(int id,const std::string& digits,unsigned durationMs=0);
    void setMicrophoneMuted(int id,bool muted);
    std::vector<AudioDeviceInfo> audioDevices()const;
    int activeCaptureDevice()const; int activePlaybackDevice()const;
    void selectAudioDevices(int captureId,int playbackId);
    void selectPlaybackDevice(int playbackId);
    void refreshAudioDevices();
    void reopenAudioDevices();
    AudioStatusInfo audioStatus()const;
    bool pollSystemAudioRoute();
    void setAudioAutoSwitch(bool enabled);
    bool audioAutoSwitchEnabled() const;
    void setCallAudioFile(int id,const std::string& path);
    void setForeground(int id); void clearForeground();
    std::vector<CallSnapshot> calls()const;
    CallSnapshot callSnapshot(int id)const;
    std::string mediaDump(int id)const;
    std::string sipLadder(int id)const;
    std::string callReport(int id)const;
    void exportCallReport(int id,const std::string& path)const;

    std::vector<SipTraceEntry> sipTrace(int id)const;
    void startSipTraceFile(int id,const std::string& path);
    void stopSipTraceFile(int id);
    bool sipTraceRecording(int id)const;
    std::string sipTracePath(int id)const;
    void startSipPcap(const std::string& path,const std::string& interfaceName="any");
    void startSipPcap(int id,const std::string& path,const std::string& interfaceName="any");
    void startRtpPcap(int id,const std::string& path,const std::string& interfaceName="any");
    void startCallPcap(const std::string& path,const std::string& interfaceName="any");
    void startCallPcap(int id,const std::string& path,const std::string& interfaceName="any");
    void stopCapture(CaptureKind kind);
    void stopCaptures();
    std::string captureStatus()const;
    void openPcapInWireshark(int id,const std::string& path)const;
    void openSipPcapInWireshark(const std::string& path)const;
    void openVoipPcapInWireshark(const std::string& path)const;

    std::string normalizeDestination(const std::string& value,bool applyDialPrefix=true)const;
    std::string callerIdentityUri(const std::string& value)const;
    void onIncomingCall(const std::string& accountId,SipAccount& account,int id);
    void onRegistrationState(const std::string& accountId,SipAccount& account,bool active,int code,const std::string& reason);
    void onSipMessage(SipTraceEntry entry);
private:
    struct ArchivedCall {
        CallSnapshot snapshot;
        std::vector<SipTraceEntry> sipTrace;
        std::string sipTracePath;
    };
    struct ManagedAccount;
    std::shared_ptr<CallSession> findCall(int id)const;
    const ArchivedCall* findArchivedCallLocked(int id)const;
    std::shared_ptr<CallSession> requirePhoneCall(int id)const;
    bool addCall(const std::shared_ptr<CallSession>& call);
    void archiveDisconnectedCall(const std::shared_ptr<CallSession>& call,const CallSnapshot& state);
    void onCallUpdated(int id);
    void configureIdentity(pj::CallOpParam& prm,const std::string& callerId)const;
    void initializeEndpoint(unsigned maxCalls);
    pj::TransportId ensureTransport(const SipProfile& profile);
    void refreshAggregateRegistrationLocked();
    Logger& logger_;
    mutable std::mutex mutex_;
    mutable std::mutex callCreateMutex_;
    mutable std::mutex audioMutex_;
    mutable std::mutex audioRouteMutex_;
    mutable std::mutex dialPrefixMutex_;
    mutable std::mutex accountMutex_;
    SipProfile profile_;
    std::string dialPrefix_;
    std::string activeAccountId_;
    std::unique_ptr<pj::Endpoint> endpoint_;
    std::map<std::string,std::unique_ptr<ManagedAccount>> accounts_;
    std::vector<std::unique_ptr<ManagedAccount>> retiredAccounts_;
    std::map<std::string,pj::TransportId> transports_;
    std::unique_ptr<SipWireMonitor> sipMonitor_;
    std::unique_ptr<CaptureManager> captures_;
    std::map<int,std::shared_ptr<CallSession>> calls_;
    std::map<int,ArchivedCall> archivedCalls_;
    std::map<std::string,int> callIdIndex_;
    std::map<std::string,std::vector<SipTraceEntry>> pendingSip_;
    int foregroundId_{-1};
    std::string lastSystemAudioRoute_;
    std::uint64_t lastSystemAudioPollMs_{0};
    bool systemAudioWatchUnavailableLogged_{false};
    std::atomic<bool> audioAutoSwitch_{true};
    std::atomic<bool> started_{false},registered_{false},stopping_{false};
    mutable std::mutex regMutex_;
    std::string registrationText_{"Stopped"};
    std::vector<std::string> registrationHistory_;
public:
    std::vector<std::string> registrationHistory()const;
};
}
