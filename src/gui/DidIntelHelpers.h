#pragma once
#include <QByteArray>
#include <QString>

namespace sipher::didintel {

struct NormalizedNumber {
    QString country;
    QString nationalDigits;
    QString e164Digits;
    bool valid{false};
    QString error;
};

NormalizedNumber normalizeNumber(const QString& entered,const QString& countryHint);
QString parseUsaCallerLookup(const QByteArray& payload,const QString& entered,const NormalizedNumber& number);
bool usaCallerLookupNeedsCarrierFallback(const QByteArray& payload);
QString parseSpamCalls(const QByteArray& payload,const NormalizedNumber& number);
QString parseTellows(const QByteArray& payload,const NormalizedNumber& number);
QString parseCQui(const QByteArray& payload,const NormalizedNumber& number);
QString parseNeutrinoHlr(const QByteArray& payload,const NormalizedNumber& number);
QString parseData247Carrier(const QByteArray& payload,const NormalizedNumber& number);
QString spamCallsUrl(const NormalizedNumber& number);
QString tellowsUrl(const NormalizedNumber& number);
QString cQuiUrl(const NormalizedNumber& number);

}
