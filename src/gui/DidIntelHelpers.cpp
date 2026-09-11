#include "DidIntelHelpers.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QStringList>
#include <QTextDocument>
#include <initializer_list>

namespace sipher::didintel {
namespace {
QString digitsOnly(const QString& value)
{
    QString out;out.reserve(value.size());for(const QChar c:value)if(c.isDigit())out.append(c);return out;
}

QString compactText(const QByteArray& payload)
{
    QTextDocument doc;doc.setHtml(QString::fromUtf8(payload));
    QString text=doc.toPlainText();text.replace(QChar(0x00a0),' ');
    return text.simplified();
}

QString match1(const QString& text,const QString& pattern,QRegularExpression::PatternOptions options=QRegularExpression::CaseInsensitiveOption)
{
    QRegularExpression re(pattern,options);const auto m=re.match(text);return m.hasMatch()?m.captured(1).trimmed():QStringLiteral("N/A");
}

QString firstString(const QJsonObject&obj,std::initializer_list<const char*>keys)
{
    for(const char*key:keys){const QJsonValue v=obj.value(QLatin1String(key));if(v.isString()&&!v.toString().trimmed().isEmpty())return v.toString().trimmed();}
    return QStringLiteral("N/A");
}

int firstInt(const QJsonObject&obj,std::initializer_list<const char*>keys,int fallback=-1)
{
    for(const char*key:keys){const QJsonValue v=obj.value(QLatin1String(key));if(v.isDouble())return v.toInt();if(v.isString()){bool ok=false;const int n=v.toString().toInt(&ok);if(ok)return n;}}
    return fallback;
}

QString boolText(const QJsonValue&v)
{
    if(v.isBool())return v.toBool()?QStringLiteral("YES"):QStringLiteral("NO");
    if(v.isDouble())return v.toInt()!=0?QStringLiteral("YES"):QStringLiteral("NO");
    return QStringLiteral("N/A");
}

QString stringList(const QJsonValue&v)
{
    if(v.isArray()){
        QStringList out;for(const auto&item:v.toArray()){
            if(item.isString())out<<item.toString();
            else if(item.isObject()){
                const auto io=item.toObject();QString label=io.value("subject").toString();if(label.isEmpty())label=io.value("name").toString();if(label.isEmpty())label=io.value("type").toString();if(!label.isEmpty())out<<label;
            }
        }
        return out.isEmpty()?QStringLiteral("N/A"):out.join(", ");
    }
    return v.isString()?v.toString():QStringLiteral("N/A");
}

QString countryCallingCode(const QString& country)
{
    if(country=="US"||country=="CA")return "1";
    if(country=="GB")return "44";
    if(country=="AU")return "61";
    if(country=="DE")return "49";
    if(country=="FR")return "33";
    if(country=="JP")return "81";
    return {};
}
}

NormalizedNumber normalizeNumber(const QString& entered,const QString& countryHint)
{
    NormalizedNumber out;out.country=countryHint.trimmed().toUpper();if(out.country.isEmpty())out.country="US";
    QString digits=digitsOnly(entered);if(digits.isEmpty()){out.error="The number does not contain any digits.";return out;}
    const bool explicitInternational=entered.trimmed().startsWith('+');
    const QString cc=countryCallingCode(out.country);
    if(explicitInternational){out.e164Digits=digits;}
    else if(out.country=="US"||out.country=="CA"){
        if(digits.size()==11&&digits.startsWith('1'))digits.remove(0,1);
        if(digits.size()!=10){out.error="US/Canada numbers must contain 10 national digits (a leading +1 or 1 is accepted).";return out;}
        out.e164Digits="1"+digits;
    }else{
        if(cc.isEmpty()){out.error="Unsupported country hint. Enter the number in full E.164 format beginning with +.";return out;}
        while(digits.startsWith('0'))digits.remove(0,1);
        if(digits.isEmpty()){out.error="The national number is empty after normalization.";return out;}
        out.e164Digits=cc+digits;
    }
    if(out.country=="US"||out.country=="CA")out.nationalDigits=out.e164Digits.startsWith('1')?out.e164Digits.mid(1):out.e164Digits;
    else if(!cc.isEmpty()&&out.e164Digits.startsWith(cc))out.nationalDigits=out.e164Digits.mid(cc.size());
    else out.nationalDigits=out.e164Digits;
    out.valid=out.e164Digits.size()>=7&&out.e164Digits.size()<=15;
    if(!out.valid)out.error="Normalized number is outside the E.164 length range.";
    return out;
}

QString spamCallsUrl(const NormalizedNumber& number){return QStringLiteral("https://spamcalls.net/en/num/")+number.e164Digits;}
QString tellowsUrl(const NormalizedNumber& number){return QStringLiteral("https://www.tellows.com/num/+")+number.e164Digits;}
QString cQuiUrl(const NormalizedNumber& number)
{
    if(number.country!="FR")return {};
    QString local=number.nationalDigits;if(!local.startsWith('0'))local.prepend('0');return QStringLiteral("https://www.c-qui.fr/")+local;
}

QString parseUsaCallerLookup(const QByteArray& payload,const QString& entered,const NormalizedNumber& number)
{
    QJsonParseError parseError{};const auto doc=QJsonDocument::fromJson(payload,&parseError);
    if(parseError.error!=QJsonParseError::NoError||!doc.isObject())return QString("Provider returned unreadable JSON: %1").arg(parseError.errorString());
    const QJsonObject o=doc.object(),location=o.value("location").toObject(),prefix=o.value("prefix").toObject(),numbering=o.value("numbering").toObject(),attribution=o.value("attribution").toObject();
    QJsonObject complaints=o.value("complaints").toObject();if(complaints.isEmpty())complaints=o.value("ftc_complaints").toObject();
    QString carrier=firstString(o,{"carrier","carrier_name"}),lineType=firstString(o,{"line_type","type"});
    if(o.value("carrier").isObject()){const auto co=o.value("carrier").toObject();carrier=firstString(co,{"name","carrier","company"});if(lineType=="N/A")lineType=firstString(co,{"line_type","type"});}
    if(carrier=="N/A")carrier=firstString(prefix,{"carrier","carrier_name","company"});if(carrier=="N/A")carrier=firstString(numbering,{"carrier","carrier_name","company"});
    if(lineType=="N/A")lineType=firstString(prefix,{"line_type","type"});if(lineType=="N/A")lineType=firstString(numbering,{"line_type","type"});
    QString city=firstString(location,{"city","rate_center"});if(city=="N/A")city=firstString(prefix,{"city","rate_center"});if(city=="N/A")city=firstString(numbering,{"city","rate_center"});if(city=="N/A")city=firstString(o,{"city","rate_center"});
    QString state=firstString(location,{"state","region"});if(state=="N/A")state=firstString(prefix,{"state","region"});if(state=="N/A")state=firstString(numbering,{"state","region"});if(state=="N/A")state=firstString(o,{"state","region"});
    QString timezone=firstString(location,{"timezone","time_zone"});if(timezone=="N/A")timezone=firstString(prefix,{"timezone","time_zone"});if(timezone=="N/A")timezone=firstString(numbering,{"timezone","time_zone"});if(timezone=="N/A")timezone=firstString(o,{"timezone","time_zone"});
    const int total=firstInt(complaints,{"total","count","complaint_count"},firstInt(o,{"complaint_count","complaints_total"},0));
    const int robocallPct=firstInt(complaints,{"robocall_percent","robocall_percentage","robocall_pct"},firstInt(o,{"robocall_percent","robocall_percentage"},-1));
    const int communityCount=firstInt(o,{"community_report_count","community_reports_count"},-1);
    const QString firstReported=firstString(complaints,{"first_reported","first_date","first_complaint_date"}),lastReported=firstString(complaints,{"last_reported","last_date","last_complaint_date"});
    QString subjects=stringList(complaints.value("top_subjects"));if(subjects=="N/A")subjects=stringList(complaints.value("subjects"));
    const QString states=stringList(complaints.value("states")),sourceUrl=firstString(attribution,{"url","source_url","number_url"});const bool tollFree=o.value("toll_free").toBool(false);
    QString reputation;if(total<=0)reputation="No FTC complaint records were returned at lookup time.";else if(total<5)reputation=QString("Complaint history present (%1 report%2); treat this as a signal, not a verdict.").arg(total).arg(total==1?"":"s");else if(total<20)reputation=QString("Elevated complaint history (%1 reports).").arg(total);else reputation=QString("Heavy complaint history (%1 reports); caller-ID spoofing can still implicate an innocent subscriber.").arg(total);
    QString out;out+=QString("Query:              %1\nNormalized:         +%2\n").arg(entered,number.e164Digits);out+=QString("Carrier:            %1\nLine type:          %2\nRate center/city:   %3\nState:              %4\nTime zone:          %5\nToll free:          %6\n").arg(carrier,lineType,city,state,timezone,tollFree?"YES":"NO");
    out+=QString("FTC complaints:     %1\n").arg(total);if(robocallPct>=0)out+=QString("Reported robocall:   %1%\n").arg(robocallPct);else out+=QString("Robocall flag:      %1\n").arg(boolText(complaints.value("robocall")));
    out+=QString("First reported:     %1\nLast reported:      %2\nReporting states:   %3\nTop subjects:       %4\n").arg(firstReported,lastReported,states,subjects);if(communityCount>=0)out+=QString("Community reports:  %1\n").arg(communityCount);else if(o.value("community_reports").isArray())out+=QString("Community reports:  %1\n").arg(o.value("community_reports").toArray().size());
    out+=QString("Reputation:         %1\n").arg(reputation);if(sourceUrl!="N/A")out+=QString("Provider page:      %1\n").arg(sourceUrl);out+="Carrier source is numbering-registry data and may not reflect the serving carrier after porting.";return out;
}

QString parseSpamCalls(const QByteArray& payload,const NormalizedNumber& number)
{
    const QString text=compactText(payload);if(text.isEmpty())return "No readable page content returned.";
    QString risk=match1(text,QStringLiteral("Spam-Risk\\s+([^()]+?)\\s*\\([0-9][0-9.,]*\\s+User Reports?\\)"));
    QString reports=match1(text,QStringLiteral("Spam-Risk\\s+[^()]+?\\s*\\(([0-9][0-9.,]*)\\s+User Reports?\\)"));
    if(reports=="N/A")reports=match1(text,QStringLiteral("([0-9][0-9.,]*)\\s+User Reports?"));
    QString latest=match1(text,QStringLiteral("Latest User Report\\s+(.{1,90}?)(?=\\s+(?:people|Harassment\\?|Answer the phone\\?|How would you|thumb_|phone_))"));
    QString harassment=match1(text,QStringLiteral("Harassment\\?\\s+([0-9]{1,3}%\\s+say\\s+Yes)"));
    QString answer=match1(text,QStringLiteral("Answer the phone\\?\\s+([0-9]{1,3}%\\s+say\\s+No)"));
    QString location=match1(text,QStringLiteral("(?:United States|France|Germany|United Kingdom|Australia|Canada|Japan)\\s+place\\s*([^0-9]{2,80}?)\\s+people"));
    QString out=QString("Number:             +%1\nSpam risk:          %2\nUser reports:       %3\nLatest report:      %4\nHarassment:         %5\nAnswer-phone signal:%6\n").arg(number.e164Digits,risk,reports,latest,harassment,answer);
    if(location!="N/A")out+=QString("Reported location:  %1\n").arg(location);out+=QString("Source page:        %1").arg(spamCallsUrl(number));return out;
}

QString parseTellows(const QByteArray& payload,const NormalizedNumber& number)
{
    const QString html=QString::fromUtf8(payload),text=compactText(payload);if(text.isEmpty())return "No readable page content returned.";
    QString score=match1(html,QStringLiteral("(?:Phone number score|Score)[:\\s]+([1-9])"));if(score=="N/A")score=match1(text,QStringLiteral("(?:Phone number score|tellows score)[:\\s]+([1-9])"));
    QString type=match1(text,QStringLiteral("Types? of call:\\s*(.{1,120}?)(?=\\s+(?:Caller Name|Owner|Details|City|Telephone number|Comments|Ratings|Where does|Your phone|$))"));
    QString ratings=match1(text,QStringLiteral("([0-9][0-9.,]*)\\s+Ratings? for"));
    QString out=QString("Number:             +%1\nTellows score:      %2 / 9\nType of call:       %3\nRatings:            %4\nSource page:        %5").arg(number.e164Digits,score,type,ratings,tellowsUrl(number));return out;
}

QString parseCQui(const QByteArray& payload,const NormalizedNumber& number)
{
    const QString text=compactText(payload);if(text.isEmpty())return "No readable page content returned.";
    const QString carrier=match1(text,QStringLiteral("attribu(?:é|e) initialement par l['’]op(?:é|e)rateur\\s+(.{1,80}?)(?=\\s+Ce num(?:é|e)ro|\\s+Ce numéro|$)"));
    const QString requests=match1(text,QStringLiteral("demand(?:é|e)\\s+([0-9][0-9 ]*)\\s+fois"));
    const QString mnemonic=match1(text,QStringLiteral("code mn(?:é|e)mo est\\s+([A-Z0-9_-]+)"));
    const QString allocation=match1(text,QStringLiteral("attribu(?:é|e) (?:à|a) la date du\\s+([0-9/]+)"));
    const QString range=match1(text,QStringLiteral("appartient (?:à|a) la tranche\\s+([0-9 ]+[–-][0-9 ]+)"));
    return QString("French original carrier: %1\nDirectory requests: %2\nOperator mnemonic:  %3\nAllocated:          %4\nNumber block:       %5\nPortability note: c-qui.fr reports the original assignment; the number may have been ported.\nSource page:        %6").arg(carrier,requests,mnemonic,allocation,range,cQuiUrl(number));
}

QString parseNeutrinoHlr(const QByteArray& payload,const NormalizedNumber& number)
{
    QJsonParseError err{};const auto doc=QJsonDocument::fromJson(payload,&err);if(err.error!=QJsonParseError::NoError||!doc.isObject())return QString("Neutrino returned unreadable JSON: %1").arg(err.errorString());
    const auto o=doc.object();if(o.contains("api-error"))return QString("Neutrino API error: %1").arg(o.value("api-error-msg").toString("Unknown API error"));
    auto s=[&](const char*k){const auto v=o.value(QLatin1String(k));if(v.isString()&&!v.toString().isEmpty())return v.toString();return QStringLiteral("N/A");};
    QString out;
    out+=QString("Number:             +%1\n").arg(number.e164Digits);
    out+=QString("Number valid:       %1\n").arg(boolText(o.value("number-valid")));
    out+=QString("HLR valid:          %1\n").arg(boolText(o.value("hlr-valid")));
    out+=QString("HLR status:         %1\n").arg(s("hlr-status"));
    out+=QString("Number type:        %1\n").arg(s("number-type"));
    out+=QString("Origin network:     %1\n").arg(s("origin-network"));
    out+=QString("Current network:    %1\n").arg(s("current-network"));
    out+=QString("Ported network:     %1\n").arg(s("ported-network"));
    out+=QString("Ported:             %1\n").arg(boolText(o.value("is-ported")));
    out+=QString("Roaming:            %1\n").arg(boolText(o.value("is-roaming")));
    out+=QString("Location:           %1\n").arg(s("location"));
    out+=QString("Country:            %1\n").arg(s("country"));
    out+=QString("MCC/MNC:            %1 / %2\n").arg(s("mcc"),s("mnc"));
    out+=QString("Network tags:       %1").arg(s("network-tags"));
    return out;
}

}
