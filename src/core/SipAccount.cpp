#include "sipher/SipAccount.h"
#include "sipher/Logger.h"
#include "sipher/SipEngine.h"
#include <utility>

namespace sipher {
SipAccount::SipAccount(SipEngine& engine, Logger& logger, std::string accountId)
    : engine_(engine), logger_(logger), accountId_(std::move(accountId))
{
}

SipAccount::~SipAccount()
{
    if (isValid()) shutdown();
}

void SipAccount::onRegState(pj::OnRegStateParam& param)
{
    try {
        const auto info = getInfo();
        engine_.onRegistrationState(accountId_, *this, info.regIsActive, static_cast<int>(param.code), param.reason);
    } catch (const pj::Error& error) {
        logger_.warn("Registration callback ["+accountId_+"]: " + error.info());
    }
}

void SipAccount::onIncomingCall(pj::OnIncomingCallParam& param)
{
    logger_.info("Incoming SIP call account="+accountId_+" id=" + std::to_string(param.callId));
    engine_.onIncomingCall(accountId_, *this, param.callId);
}
} // namespace sipher
