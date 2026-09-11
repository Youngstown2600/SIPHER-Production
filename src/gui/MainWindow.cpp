#include "MainWindow.h"
#include "ProfileDialog.h"
#include "ModernStyle.h"
#include "trunkmonkey/CallSnapshot.h"
#include "trunkmonkey/CaptureManager.h"
#include "trunkmonkey/Logger.h"
#include "trunkmonkey/MultiCallManager.h"
#include "trunkmonkey/PbxAudit.h"
#include "trunkmonkey/RuntimePaths.h"
#include "trunkmonkey/SipEngine.h"
#include "trunkmonkey/SipTrace.h"
#include "trunkmonkey/TextPool.h"
#include <QAbstractItemView>
#include <algorithm>
#include <QAction>
#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QComboBox>
#include <QCheckBox>
#include <QDateTime>
#include <QDialog>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFileDialog>
#include <QFontDatabase>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QKeySequence>
#include <QLineEdit>
#include <QMessageBox>
#include <QMenuBar>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QHostInfo>
#include <QDnsLookup>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>
#include <QUrlQuery>
#include <QMenu>
#include <QPlainTextEdit>
#include <QRegularExpression>
#include <QPixmap>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QStatusBar>
#include <QTabWidget>
#include <QTabBar>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <sstream>
#include <QStringList>
#include <vector>
#include <utility>
using namespace trunkmonkey;
static std::vector<std::string> loadList(const QString&p){if(p.isEmpty())return{};TextPool t;t.load(p.toStdString());return t.values();}
static QString showAddr(const std::string&s){return s.empty()?QStringLiteral("--"):QString::fromStdString(s);}

namespace {
struct ParsedSipTarget { QString host; quint16 port{0}; bool explicitPort{false}; };

QString jsonTriState(const QJsonValue& value)
{
    if(value.isBool())return value.toBool()?QStringLiteral("YES"):QStringLiteral("NO");
    return QStringLiteral("N/A");
}

ParsedSipTarget parseSipTarget(QString value,Transport transport)
{
    ParsedSipTarget out;value=value.trimmed();
    const int lt=value.indexOf('<'),gt=value.indexOf('>');if(lt>=0&&gt>lt)value=value.mid(lt+1,gt-lt-1).trimmed();
    if(value.startsWith("sip:",Qt::CaseInsensitive))value=value.mid(4);else if(value.startsWith("sips:",Qt::CaseInsensitive))value=value.mid(5);
    if(value.startsWith("//"))value=value.mid(2);
    const int semi=value.indexOf(';');if(semi>=0)value=value.left(semi);const int query=value.indexOf('?');if(query>=0)value=value.left(query);
    const int at=value.lastIndexOf('@');if(at>=0)value=value.mid(at+1);
    value=value.trimmed();
    if(value.startsWith('[')){
        const int end=value.indexOf(']');if(end>0){out.host=value.mid(1,end-1);if(end+1<value.size()&&value.at(end+1)==':'){bool ok=false;const int p=value.mid(end+2).toInt(&ok);if(ok&&p>0&&p<=65535){out.port=(quint16)p;out.explicitPort=true;}}}
    }else{
        const int first=value.indexOf(':'),last=value.lastIndexOf(':');
        if(first>0&&first==last){bool ok=false;const int p=value.mid(last+1).toInt(&ok);if(ok&&p>0&&p<=65535){out.host=value.left(last);out.port=(quint16)p;out.explicitPort=true;}else out.host=value;}else out.host=value;
    }
    if(out.host.isEmpty())out.host=value;
    if(!out.port)out.port=(transport==Transport::Tls)?5061:5060;
    return out;
}

QStringList sipHeaderValues(const std::string& raw,const QString& wanted)
{
    QStringList out;const auto text=QString::fromStdString(raw);const auto lines=text.split(QRegularExpression("\r?\n"));
    const QString prefix=wanted+":";
    for(const auto&line:lines){if(line.startsWith(prefix,Qt::CaseInsensitive)){const auto v=line.mid(prefix.size()).trimmed();if(!v.isEmpty()&&!out.contains(v))out.push_back(v);}}
    return out;
}

void appendUnique(QStringList&dst,const QStringList&src){for(const auto&v:src)if(!dst.contains(v))dst.push_back(v);}
}

MainWindow::MainWindow(SipEngine&e,MultiCallManager&m,Logger&l,std::string profilePath,QWidget*p):QMainWindow(p),engine_(e),multi_(m),logger_(l),profilePath_(std::move(profilePath)){
    network_=new QNetworkAccessManager(this);buildUi();setWindowTitle("S.I.P.H.E.R. r18 — DID Intelligence / Route Audit");setMinimumSize(900,620);resize(1280,800);refreshTimer_=new QTimer(this);connect(refreshTimer_,&QTimer::timeout,this,&MainWindow::refresh);refreshTimer_->start(250);refresh();
}
void MainWindow::buildUi(){
    auto* fileMenu=menuBar()->addMenu(QStringLiteral("&File"));
    auto* exitAction=fileMenu->addAction(QStringLiteral("E&xit"));
    exitAction->setShortcut(QKeySequence(QKeySequence::Quit));
    connect(exitAction,&QAction::triggered,qApp,&QApplication::quit);

    auto* settingsMenu=menuBar()->addMenu(QStringLiteral("&Settings"));
    auto* profileAction=settingsMenu->addAction(QStringLiteral("SIP &Profile..."));
    connect(profileAction,&QAction::triggered,this,&MainWindow::editProfile);
    auto* audioAction=settingsMenu->addAction(QStringLiteral("&Audio Devices..."));
    connect(audioAction,&QAction::triggered,this,&MainWindow::showAudioDevices);
    auto* audioStatusAction=settingsMenu->addAction(QStringLiteral("Audio &Status..."));
    connect(audioStatusAction,&QAction::triggered,this,&MainWindow::showAudioStatus);
#ifndef _WIN32
    auto* audioAutoAction=settingsMenu->addAction(QStringLiteral("Automatically Follow &Headset / System Audio"));
    audioAutoAction->setCheckable(true);
    audioAutoAction->setChecked(engine_.audioAutoSwitchEnabled());
    audioAutoAction->setToolTip(QStringLiteral("Automatically reopen and reattach PJSIP audio when Linux PipeWire/PulseAudio or FreeBSD OSS/snd_hda routing changes"));
    connect(audioAutoAction,&QAction::toggled,this,[this](bool enabled){
        engine_.setAudioAutoSwitch(enabled);
        statusBar()->showMessage(enabled?QStringLiteral("Automatic headset/device switching enabled"):QStringLiteral("Automatic headset/device switching disabled"),5000);
    });
#endif
    auto* audioReopenAction=settingsMenu->addAction(QStringLiteral("&Reopen / Refresh Audio"));
    audioReopenAction->setToolTip(QStringLiteral("Close and reopen the PJSIP sound device, refresh device IDs, and reattach the active call"));
    connect(audioReopenAction,&QAction::triggered,this,&MainWindow::reopenAudio);
#ifndef _WIN32
    auto* audioOutputAction=settingsMenu->addAction(QStringLiteral("Audio &Output..."));
    audioOutputAction->setToolTip(QStringLiteral("Choose the Unix/Linux playback device without changing the microphone; the sound device is reopened immediately"));
    connect(audioOutputAction,&QAction::triggered,this,&MainWindow::showAudioOutput);
#endif
    auto* regHistoryAction=settingsMenu->addAction(QStringLiteral("&Registration History..."));
    connect(regHistoryAction,&QAction::triggered,this,&MainWindow::showRegistrationHistory);

    auto* legacyMenu=menuBar()->addMenu(QStringLiteral("&Legacy"));
    auto* blueBoxAction=legacyMenu->addAction(QStringLiteral("&Blue Tone / Blue Box — Historical Lab..."));
    blueBoxAction->setToolTip(QStringLiteral("Historical signaling reference/simulator panel. Live network-control tone generation is intentionally disabled."));
    connect(blueBoxAction,&QAction::triggered,this,&MainWindow::showBlueBoxLegacy);
    auto* redBoxAction=legacyMenu->addAction(QStringLiteral("&Red Box — Historical Lab..."));
    redBoxAction->setToolTip(QStringLiteral("Historical payphone-signaling reference/simulator panel. Live coin-control tone generation is intentionally disabled."));
    connect(redBoxAction,&QAction::triggered,this,&MainWindow::showRedBoxLegacy);

    auto*c=new QWidget;
    c->setObjectName(QStringLiteral("PhreakRoot"));
    auto*shell=new QHBoxLayout(c);shell->setContentsMargins(0,0,0,0);shell->setSpacing(0);

    auto*sidebar=new QFrame(c);sidebar->setObjectName(QStringLiteral("PhreakRail"));sidebar->setFixedWidth(228);
    auto*side=new QVBoxLayout(sidebar);side->setContentsMargins(18,22,18,18);side->setSpacing(8);
    auto*brand=new QLabel(sidebar);brand->setObjectName(QStringLiteral("BrandLogo"));brand->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);brand->setMinimumHeight(50);brand->setToolTip(QStringLiteral("S.I.P.H.E.R."));
    const QPixmap brandPixmap(QStringLiteral(":/sipher/logo.png"));
    if(!brandPixmap.isNull()) brand->setPixmap(brandPixmap.scaledToWidth(192,Qt::SmoothTransformation));
    else { brand->setText(QStringLiteral("S.I.P.H.E.R.")); brand->setObjectName(QStringLiteral("BrandTitle")); }
    side->addWidget(brand);
    auto*edition=new QLabel(QStringLiteral("r18 // DID INTEL / CARRIER ACCESS"),sidebar);edition->setObjectName(QStringLiteral("BrandVersion"));side->addWidget(edition);
    auto*tagline=new QLabel(QStringLiteral("DID INTEL / SIGNAL TAP / SWITCH AUDIT"),sidebar);tagline->setObjectName(QStringLiteral("Muted"));tagline->setWordWrap(true);side->addWidget(tagline);
    side->addSpacing(14);
    auto*navHost=new QWidget(sidebar);auto*nav=new QVBoxLayout(navHost);nav->setContentsMargins(0,0,0,0);nav->setSpacing(4);side->addWidget(navHost);
    side->addStretch(1);
    auto*themeLabel=new QLabel(QStringLiteral("// DISPLAY PROFILE"),sidebar);themeLabel->setObjectName(QStringLiteral("Muted"));side->addWidget(themeLabel);
    theme_=new QComboBox(sidebar);
    theme_->addItem("System","system");theme_->addItem("Midnight","midnight");theme_->addItem("Slate","slate");theme_->addItem("Ocean","ocean");theme_->addItem("Arctic","arctic");theme_->addItem("Solarized","solarized");theme_->addItem("Monochrome","monochrome");theme_->addItem("Cobalt","cobalt");theme_->addItem("Amber","amber");theme_->addItem("High Contrast","high-contrast");
    theme_->insertSeparator(theme_->count());
    theme_->addItem("Black Ice","black-ice");theme_->addItem("Night Vision","night-vision");theme_->addItem("Blue Box","blue-box");theme_->addItem("Red Box","red-box");theme_->addItem("2600","2600");theme_->addItem("WarGames","wargames");theme_->addItem("Phosphor","phosphor");theme_->addItem("Cyberpunk","cyberpunk");theme_->addItem("Blood Moon","blood-moon");theme_->addItem("Terminal Gold","terminal-gold");
    const QString settingsPath=QString::fromStdString(trunkmonkey::runtime::settingsPath().string());QSettings themeSettings(settingsPath,QSettings::IniFormat);const QString savedTheme=themeSettings.value(QStringLiteral("ui/theme"),QStringLiteral("system")).toString().toCaseFolded();int themeIndex=theme_->findData(savedTheme);if(themeIndex<0)themeIndex=0;theme_->setCurrentIndex(themeIndex);
    connect(theme_,QOverload<int>::of(&QComboBox::currentIndexChanged),this,[this](int){applyTheme(theme_->currentData().toString());});
    side->addWidget(theme_);
    auto*sideProfile=new QPushButton(QStringLiteral("EDIT IDENTITY / SIP"),sidebar);connect(sideProfile,&QPushButton::clicked,this,&MainWindow::editProfile);side->addWidget(sideProfile);
    shell->addWidget(sidebar);

    auto*content=new QWidget(c);auto*outer=new QVBoxLayout(content);outer->setContentsMargins(24,18,24,18);outer->setSpacing(12);
    auto*topBar=new QFrame(content);topBar->setObjectName(QStringLiteral("WireHeader"));auto*top=new QHBoxLayout(topBar);top->setContentsMargins(0,0,0,0);top->setSpacing(12);
    auto*headings=new QVBoxLayout;headings->setSpacing(2);auto*pageTitle=new QLabel(QStringLiteral("// LINE ACCESS"),topBar);pageTitle->setObjectName(QStringLiteral("PageTitle"));auto*pageSubtitle=new QLabel(QStringLiteral("DIAL / TAP / CAPTURE  ::  SIP + RTP carrier session workbench"),topBar);pageSubtitle->setObjectName(QStringLiteral("PageSubtitle"));pageSubtitle->setWordWrap(true);headings->addWidget(pageTitle);headings->addWidget(pageSubtitle);top->addLayout(headings,1);
    registration_=new QLabel(QStringLiteral("STARTING"),topBar);registration_->setObjectName(QStringLiteral("StatusPill"));registration_->setAlignment(Qt::AlignCenter);top->addWidget(registration_);outer->addWidget(topBar);

    tabs_=new QTabWidget(content);tabs_->setDocumentMode(true);tabs_->tabBar()->hide();

    // MAIN: phone controls, selected media and packet capture in one compact workspace.
    auto*mainPage=new QWidget;auto*ml=new QVBoxLayout(mainPage);ml->setContentsMargins(7,7,7,7);ml->setSpacing(6);
    auto*f=new QFormLayout;dialEdit_=new QLineEdit;dialEdit_->setPlaceholderText("3305551212 or sip:user@example.net");callerIdEdit_=new QLineEdit;callerIdEdit_->setPlaceholderText("Optional caller identity");dialPrefixEdit_=new QLineEdit(QString::fromStdString(engine_.dialPrefix()));dialPrefixEdit_->setPlaceholderText("Optional per-PBX prefix, e.g. 9 or 4071");dialPrefixEdit_->setToolTip("Session routing prefix. Change it here whenever you move to another PBX. Blank means no prefix. Explicit sip:/sips: URIs and user@domain destinations are never modified.");connect(dialPrefixEdit_,&QLineEdit::editingFinished,this,[this](){try{engine_.setDialPrefix(dialPrefixEdit_->text().trimmed().toStdString());statusBar()->showMessage(QString("Dial prefix: %1").arg(dialPrefixEdit_->text().trimmed().isEmpty()?QStringLiteral("<none>"):dialPrefixEdit_->text().trimmed()),3000);}catch(const std::exception&e){QMessageBox::warning(this,"Dial prefix",e.what());dialPrefixEdit_->setText(QString::fromStdString(engine_.dialPrefix()));}});f->addRow("Destination",dialEdit_);f->addRow("Dial prefix",dialPrefixEdit_);f->addRow("Caller ID",callerIdEdit_);ml->addLayout(f);
    auto*r=new QGridLayout;auto*b=new QPushButton("CALL");b->setProperty("role","primary");connect(b,&QPushButton::clicked,this,&MainWindow::dial);r->addWidget(b,0,0);b=new QPushButton("ANSWER");connect(b,&QPushButton::clicked,this,&MainWindow::answerSelected);r->addWidget(b,0,1);b=new QPushButton("HANGUP");b->setProperty("role","danger");connect(b,&QPushButton::clicked,this,&MainWindow::hangupSelected);r->addWidget(b,0,2);b=new QPushButton("DTMF PAD...");connect(b,&QPushButton::clicked,this,&MainWindow::showDtmfPad);r->addWidget(b,0,3);muteButton_=new QPushButton("MUTE MIC");connect(muteButton_,&QPushButton::clicked,this,&MainWindow::toggleMuteSelected);r->addWidget(muteButton_,0,4);b=new QPushButton("HANGUP ALL");connect(b,&QPushButton::clicked,this,&MainWindow::hangupAll);r->addWidget(b,0,5);b=new QPushButton("EXIT");connect(b,&QPushButton::clicked,qApp,&QApplication::quit);r->addWidget(b,0,6);ml->addLayout(r);

    auto*mediaBox=new QGroupBox("Selected Call Media");auto*media=new QGridLayout(mediaBox);callIdLabel_=new QLabel("--");mediaTarget_=new QLabel("--");mediaSource_=new QLabel("--");mediaLocal_=new QLabel("--");mediaCodec_=new QLabel("--");mediaQuality_=new QLabel("--");mediaQuality_->setTextInteractionFlags(Qt::TextSelectableByMouse);media->addWidget(new QLabel("SIP Call-ID:"),0,0);media->addWidget(callIdLabel_,0,1,1,3);media->addWidget(new QLabel("RTP target:"),1,0);media->addWidget(mediaTarget_,1,1);media->addWidget(new QLabel("RTP source:"),1,2);media->addWidget(mediaSource_,1,3);media->addWidget(new QLabel("Local RTP:"),2,0);media->addWidget(mediaLocal_,2,1);media->addWidget(new QLabel("Codec:"),2,2);media->addWidget(mediaCodec_,2,3);media->addWidget(new QLabel("Quality:"),3,0);media->addWidget(mediaQuality_,3,1,1,3);media->setColumnStretch(1,1);media->setColumnStretch(3,1);ml->addWidget(mediaBox);auto*diagButtons=new QGridLayout;b=new QPushButton("SIP LADDER...");connect(b,&QPushButton::clicked,this,&MainWindow::showSipLadder);diagButtons->addWidget(b,0,0);b=new QPushButton("EXPORT CALL REPORT...");connect(b,&QPushButton::clicked,this,&MainWindow::exportCallReport);diagButtons->addWidget(b,0,1);ml->addLayout(diagButtons);

    auto*capBox=new QGroupBox("Packet Capture");auto*cap=new QGridLayout(capBox);captureInterface_=new QComboBox;captureInterface_->setEditable(true);for(const auto& iface:CaptureManager::availableInterfaces())captureInterface_->addItem(QString::fromStdString(iface));if(captureInterface_->count()==0)captureInterface_->addItem("any");int anyIndex=captureInterface_->findText("any");if(anyIndex>=0)captureInterface_->setCurrentIndex(anyIndex);captureInterface_->setToolTip(QString::fromStdString(CaptureManager::permissionHint()));cap->addWidget(new QLabel("Interface:"),0,0);cap->addWidget(captureInterface_,0,1);
    sipPcapStart_=new QPushButton("START SIP PCAP (PRE-DIAL)...");connect(sipPcapStart_,&QPushButton::clicked,this,&MainWindow::startSipPcap);cap->addWidget(sipPcapStart_,0,2);rtpPcapStart_=new QPushButton("START RTP PCAP...");connect(rtpPcapStart_,&QPushButton::clicked,this,&MainWindow::startRtpPcap);cap->addWidget(rtpPcapStart_,0,3);callPcapStart_=new QPushButton("START FULL VOIP PCAP (PRE-DIAL)...");callPcapStart_->setProperty("role","primary");callPcapStart_->setToolTip("Recommended for Wireshark Telephony > VoIP Calls. Starts before dialing and keeps SIP/SDP/RTP/RTCP in one chronological capture.");connect(callPcapStart_,&QPushButton::clicked,this,&MainWindow::startCallPcap);cap->addWidget(callPcapStart_,0,4);pcapStop_=new QPushButton("STOP PCAPS");connect(pcapStop_,&QPushButton::clicked,this,&MainWindow::stopPcaps);cap->addWidget(pcapStop_,0,5);captureStatus_=new QLabel("SIP PCAP: stopped | RTP PCAP: stopped | FULL VOIP PCAP: stopped");captureStatus_->setWordWrap(true);cap->addWidget(captureStatus_,1,0,1,4);b=new QPushButton("OPEN LAST PCAP IN WIRESHARK");b->setToolTip("Full VoIP captures force SIP decoding and enable RTP heuristic fallback. SIP-only and RTP-only captures remain available for focused troubleshooting.");connect(b,&QPushButton::clicked,this,&MainWindow::openLastPcap);cap->addWidget(b,1,4,1,2);auto*perm=new QLabel(QString::fromStdString(CaptureManager::permissionHint()));perm->setWordWrap(true);perm->setStyleSheet(QStringLiteral("font-size: 10px;"));cap->addWidget(perm,2,0,1,6);ml->addWidget(capBox);
    auto*mainNote=new QLabel("Recommended: START FULL VOIP PCAP before dialing. It keeps the initial INVITE, SDP, responses, RTP/RTCP, BYE, and final response in one file so Wireshark Telephony > VoIP Calls can correlate signaling and media. SIP-only and RTP-only captures remain available for focused troubleshooting.");mainNote->setWordWrap(true);ml->addWidget(mainNote);ml->addStretch();tabs_->addTab(mainPage,"Main");

    // ACTIVE CALLS
    auto*active=new QWidget;auto*al=new QVBoxLayout(active);calls_=new QTableWidget(0,12);calls_->setHorizontalHeaderLabels({"ID","Mode","Dir","FG","State","SIP","Remote","Caller ID","RTP Target","RTP Source","Codec","Reason"});calls_->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);calls_->horizontalHeader()->setStretchLastSection(true);calls_->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);calls_->setColumnWidth(0,48);calls_->setColumnWidth(1,72);calls_->setColumnWidth(2,48);calls_->setColumnWidth(3,38);calls_->setColumnWidth(4,110);calls_->setColumnWidth(5,58);calls_->setColumnWidth(6,190);calls_->setColumnWidth(7,120);calls_->setSelectionBehavior(QAbstractItemView::SelectRows);calls_->setSelectionMode(QAbstractItemView::SingleSelection);connect(calls_,&QTableWidget::itemSelectionChanged,this,&MainWindow::refreshDiagnostics);al->addWidget(calls_,1);
    r=new QGridLayout;b=new QPushButton("FOREGROUND");connect(b,&QPushButton::clicked,this,&MainWindow::foregroundSelected);r->addWidget(b,0,0);b=new QPushButton("HOLD");connect(b,&QPushButton::clicked,this,&MainWindow::holdSelected);r->addWidget(b,0,1);b=new QPushButton("RESUME");connect(b,&QPushButton::clicked,this,&MainWindow::resumeSelected);r->addWidget(b,0,2);b=new QPushButton("MUTE / UNMUTE MIC");connect(b,&QPushButton::clicked,this,&MainWindow::toggleMuteSelected);r->addWidget(b,0,3);b=new QPushButton("DTMF PAD...");connect(b,&QPushButton::clicked,this,&MainWindow::showDtmfPad);r->addWidget(b,0,4);al->addLayout(r);tabs_->addTab(active,"Active Calls");

    // SIP LOG
    auto*sipPage=new QWidget;auto*sl=new QVBoxLayout(sipPage);diagnosticNote_=new QLabel("Select a normal Phone call on Active Calls to inspect its SIP dialog.");diagnosticNote_->setWordWrap(true);sl->addWidget(diagnosticNote_);
    auto*sipButtons=new QGridLayout;sipTraceStart_=new QPushButton("START RAW SIP TRACE...");connect(sipTraceStart_,&QPushButton::clicked,this,&MainWindow::startSipTrace);sipButtons->addWidget(sipTraceStart_,0,0);sipTraceStop_=new QPushButton("STOP RAW SIP TRACE");connect(sipTraceStop_,&QPushButton::clicked,this,&MainWindow::stopSipTrace);sipButtons->addWidget(sipTraceStop_,0,1);sl->addLayout(sipButtons);
    sipLog_=new QTableWidget(0,7);sipLog_->setHorizontalHeaderLabels({"Time","Flow","Peer","Signal","CSeq","Code","Reason"});sipLog_->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);sipLog_->horizontalHeader()->setStretchLastSection(true);sipLog_->setSelectionBehavior(QAbstractItemView::SelectRows);sipLog_->setSelectionMode(QAbstractItemView::SingleSelection);connect(sipLog_,&QTableWidget::itemSelectionChanged,this,&MainWindow::showRawSip);sl->addWidget(sipLog_,2);rawSipFlow_=new QLabel("Select a SIP signal above. SENT means S.I.P.H.E.R. transmitted it to the PBX; RECEIVED means it came from the PBX.");rawSipFlow_->setWordWrap(true);rawSipFlow_->setStyleSheet(QStringLiteral("font-weight:700;"));sl->addWidget(rawSipFlow_);rawSip_=new QPlainTextEdit;rawSip_->setReadOnly(true);rawSip_->setPlaceholderText("Select a SIP transaction above to inspect the full message.");rawSip_->setMaximumBlockCount(10000);sl->addWidget(rawSip_,2);tabs_->addTab(sipPage,"SIP Log");

    // DID INTELLIGENCE / CARRIER HANDOFF
    auto*didPage=new QWidget;auto*didLayout=new QVBoxLayout(didPage);
    auto*didGroup=new QGroupBox("DID / NUMBER INTELLIGENCE",didPage);auto*didBox=new QVBoxLayout(didGroup);auto*didForm=new QFormLayout;
    didNumber_=new QLineEdit;didNumber_->setPlaceholderText("DID / telephone number, preferably E.164 (for example +13305551212)");
    didCountry_=new QComboBox;didCountry_->setEditable(true);didCountry_->addItems({"US","CA","GB","AU","DE","FR","JP"});didCountry_->setCurrentText("US");didCountry_->setToolTip("Country hint used by the reputation provider when the number is ambiguous.");
    didApiKey_=new QLineEdit;didApiKey_->setEchoMode(QLineEdit::Password);didApiKey_->setPlaceholderText("IPQualityScore API key (or set SIPHER_IPQS_API_KEY)");
    didForm->addRow("DID / Number",didNumber_);didForm->addRow("Country hint",didCountry_);didForm->addRow("Reputation API key",didApiKey_);didBox->addLayout(didForm);
    auto*didNote=new QLabel("Lookup returns carrier/line type plus provider reputation signals such as spam reports, recent abuse, risk and fraud score. These are reputation indicators, not proof that a caller committed fraud. API keys are kept in memory for the current run and sent in the IPQS-KEY request header.");didNote->setWordWrap(true);didBox->addWidget(didNote);
    auto*didButtons=new QHBoxLayout;auto*lookupButton=new QPushButton("DIP / LOOKUP DID");lookupButton->setProperty("role","primary");connect(lookupButton,&QPushButton::clicked,this,&MainWindow::lookupDid);didButtons->addWidget(lookupButton);auto*copyRoute=new QPushButton("USE NUMBER FOR NEXT-OUT");connect(copyRoute,&QPushButton::clicked,this,[this](){if(routeDestination_)routeDestination_->setText(didNumber_?didNumber_->text():QString{});});didButtons->addWidget(copyRoute);didButtons->addStretch();didBox->addLayout(didButtons);
    didOutput_=new QPlainTextEdit;didOutput_->setReadOnly(true);didOutput_->setPlaceholderText("DID carrier and reputation results appear here.");didOutput_->setMaximumBlockCount(2000);didBox->addWidget(didOutput_,1);didLayout->addWidget(didGroup,1);

    auto*routeGroup=new QGroupBox("CARRIER HANDOFF / NEXT-OUT",didPage);auto*routeBox=new QVBoxLayout(routeGroup);auto*routeForm=new QFormLayout;routeDestination_=new QLineEdit;routeDestination_->setPlaceholderText("Number or SIP URI to analyze without placing a call");routeForm->addRow("Destination",routeDestination_);routeBox->addLayout(routeForm);
    auto*routeNote=new QLabel("Shows the expected first carrier signaling hop from the active SIP profile, resolved IP candidates, normalized Request-URI and any Route / Record-Route / Via / Contact information already observed on the selected call. Carrier-internal routing may be hidden by an SBC.");routeNote->setWordWrap(true);routeBox->addWidget(routeNote);
    auto*routeButton=new QPushButton("ANALYZE NEXT-OUT / HANDOFF");connect(routeButton,&QPushButton::clicked,this,&MainWindow::analyzeNextOut);routeBox->addWidget(routeButton);routeOutput_=new QPlainTextEdit;routeOutput_->setReadOnly(true);routeOutput_->setPlaceholderText("Expected and observed SIP handoff information appears here.");routeOutput_->setMaximumBlockCount(3000);routeBox->addWidget(routeOutput_,1);didLayout->addWidget(routeGroup,1);
    tabs_->addTab(didPage,"DID Intelligence");

    // QUEUE TEST
    auto*q=new QWidget;auto*ql=new QVBoxLayout(q);f=new QFormLayout;batchCount_=new QSpinBox;batchCount_->setRange(1,50);batchCount_->setValue(5);launchInterval_=new QSpinBox;launchInterval_->setRange(50,60000);launchInterval_->setValue(250);launchInterval_->setSuffix(" ms");batchDestination_=new QLineEdit;batchDestination_->setPlaceholderText("Single queue/DID target");fixedCallerId_=new QLineEdit;fixedCallerId_->setPlaceholderText("Fixed CID (ignored when list is loaded)");f->addRow("Calls",batchCount_);f->addRow("Launch interval",launchInterval_);f->addRow("Single destination",batchDestination_);f->addRow("Fixed caller ID",fixedCallerId_);ql->addLayout(f);destinationFileLabel_=new QLabel("No destination list loaded");callerIdFileLabel_=new QLabel("No caller-ID list loaded");queueAudioFileLabel_=new QLabel("Live/no injected audio");r=new QGridLayout;b=new QPushButton("LOAD DESTINATIONS.TXT");connect(b,&QPushButton::clicked,this,&MainWindow::loadDestinations);r->addWidget(b,0,0);r->addWidget(destinationFileLabel_,0,1);b=new QPushButton("LOAD CALLERIDS.TXT");connect(b,&QPushButton::clicked,this,&MainWindow::loadCallerIds);r->addWidget(b,1,0);r->addWidget(callerIdFileLabel_,1,1);b=new QPushButton("LOAD AUDIO FILE...");connect(b,&QPushButton::clicked,this,&MainWindow::loadQueueAudio);r->addWidget(b,2,0);r->addWidget(queueAudioFileLabel_,2,1);ql->addLayout(r);auto*note=new QLabel("Each launched call is an independent SIP dialog and RTP session. Optional WAV/MP3 audio is normalized by ffmpeg and injected into every queue-test call. Batch calls are not conferenced and are not automatically routed to the local headset.");note->setWordWrap(true);ql->addWidget(note);b=new QPushButton("START QUEUE TEST");b->setProperty("role","primary");connect(b,&QPushButton::clicked,this,&MainWindow::launchBatch);ql->addWidget(b);ql->addStretch();tabs_->addTab(q,"Queue Test");

    // PBX AUDIT — active, bounded probes for systems the operator is authorized to test.
    auto*auditPage=new QWidget;auto*aul=new QVBoxLayout(auditPage);auto*warning=new QLabel(QString::fromUtf8(PbxAudit::warningText()));warning->setWordWrap(true);warning->setStyleSheet(QStringLiteral("font-weight:700; color:#ff5a5a;"));aul->addWidget(warning);
    auto*auf=new QFormLayout;auditHost_=new QLineEdit;auditHost_->setPlaceholderText("PBX/SBC hostname or IP (CIDR is for Discover only)");auditUser_=new QLineEdit;auditUser_->setPlaceholderText("Known authorized test extension/account; blank skips account differential");auditPort_=new QSpinBox;auditPort_->setRange(1,65535);auditPort_->setValue(5060);auditTransport_=new QComboBox;auditTransport_->addItem("UDP","udp");auditTransport_->addItem("TCP","tcp");auditExtFirst_=new QSpinBox;auditExtFirst_->setRange(1,999999);auditExtFirst_->setValue(100);auditExtLast_=new QSpinBox;auditExtLast_->setRange(1,999999);auditExtLast_->setValue(120);auf->addRow("Target",auditHost_);auf->addRow("SIP port",auditPort_);auf->addRow("Transport",auditTransport_);auf->addRow("Authorized test user",auditUser_);auf->addRow("Extension range start",auditExtFirst_);auf->addRow("Extension range end",auditExtLast_);aul->addLayout(auf);
    auto*autoOptions=new QGridLayout;auditIncludeVulns_=new QCheckBox("Public CVE correlation");auditIncludeVulns_->setChecked(true);auditIncludeVulns_->setToolTip("Metadata lookup only; no exploit code is executed.");auditIncludeParser_=new QCheckBox("Parser normalization");auditIncludeParser_->setChecked(true);auditIncludeResilience_=new QCheckBox("Bounded rate resilience");auditIncludeResilience_->setChecked(true);auditIncludeTls_=new QCheckBox("SIP TLS posture");auditIncludeTls_->setChecked(true);auditIncludeExtensions_=new QCheckBox("Include extension range");auditIncludeExtensions_->setChecked(false);auditIncludeExtensions_->setToolTip("Opt-in scope-expanding differential audit; maximum 100 extensions.");autoOptions->addWidget(auditIncludeVulns_,0,0);autoOptions->addWidget(auditIncludeParser_,0,1);autoOptions->addWidget(auditIncludeResilience_,0,2);autoOptions->addWidget(auditIncludeTls_,1,0);autoOptions->addWidget(auditIncludeExtensions_,1,1);aul->addLayout(autoOptions);
    auto*runAuto=new QPushButton("RUN AUTOMATED CHAINED AUDIT");runAuto->setProperty("role","primary");runAuto->setMinimumHeight(36);runAuto->setToolTip("Recommended: chains each audit stage into a single prioritized report.");connect(runAuto,&QPushButton::clicked,this,&MainWindow::runAuditAuto);aul->addWidget(runAuto);auditProgress_=new QLabel("Ready — automated audit output feeds each applicable stage into the next.");auditProgress_->setWordWrap(true);aul->addWidget(auditProgress_);
    auto*aub=new QGridLayout;b=new QPushButton("PBX FINGERPRINT");connect(b,&QPushButton::clicked,this,&MainWindow::runAuditFingerprint);aub->addWidget(b,0,0);b=new QPushButton("CVE / EXPLOIT-DB LOOKUP");connect(b,&QPushButton::clicked,this,&MainWindow::runAuditVulns);aub->addWidget(b,0,1);b=new QPushButton("SAVE REPORT...");connect(b,&QPushButton::clicked,this,&MainWindow::saveAuditReport);aub->addWidget(b,0,2);
    b=new QPushButton("SERVICE PROBE");connect(b,&QPushButton::clicked,this,&MainWindow::runAuditProbe);aub->addWidget(b,1,0);b=new QPushButton("DISCOVER CIDR");connect(b,&QPushButton::clicked,this,&MainWindow::runAuditDiscover);aub->addWidget(b,1,1);b=new QPushButton("METHOD POLICY");connect(b,&QPushButton::clicked,this,&MainWindow::runAuditMethods);aub->addWidget(b,1,2);b=new QPushButton("AUTH POLICY");connect(b,&QPushButton::clicked,this,&MainWindow::runAuditAuth);aub->addWidget(b,2,0);b=new QPushButton("EXTENSION AUDIT");connect(b,&QPushButton::clicked,this,&MainWindow::runAuditExtensions);aub->addWidget(b,2,1);b=new QPushButton("COMPLIANCE");connect(b,&QPushButton::clicked,this,&MainWindow::runAuditCompliance);aub->addWidget(b,2,2);b=new QPushButton("PARSER ABUSE");connect(b,&QPushButton::clicked,this,&MainWindow::runAuditParser);aub->addWidget(b,3,0);b=new QPushButton("RATE RESILIENCE");connect(b,&QPushButton::clicked,this,&MainWindow::runAuditResilience);aub->addWidget(b,3,1);b=new QPushButton("ATTACK SCENARIO");connect(b,&QPushButton::clicked,this,&MainWindow::runAuditScenario);aub->addWidget(b,3,2);b=new QPushButton("TLS 5061");connect(b,&QPushButton::clicked,this,&MainWindow::runAuditTls);aub->addWidget(b,4,0);b=new QPushButton("TRANSPORT PARITY");connect(b,&QPushButton::clicked,this,&MainWindow::runAuditTransportParity);aub->addWidget(b,4,1);b=new QPushButton("TOPOLOGY EXPOSURE");connect(b,&QPushButton::clicked,this,&MainWindow::runAuditTopologyExposure);aub->addWidget(b,4,2);b=new QPushButton("AUTOMATED FULL (DEFAULTS)");connect(b,&QPushButton::clicked,this,&MainWindow::runAuditFull);aub->addWidget(b,5,0,1,3);aul->addLayout(aub);auditOutput_=new QPlainTextEdit;auditOutput_->setReadOnly(true);auditOutput_->setPlaceholderText("Audit results appear here. The automated audit is recommended; individual tools remain available for focused troubleshooting.");aul->addWidget(auditOutput_,1);tabs_->addTab(auditPage,"PBX Audit");

    // PROFILE / CONFIG
    auto*profilePage=new QWidget;auto*pfl=new QVBoxLayout(profilePage);profileSummary_=new QLabel;profileSummary_->setTextInteractionFlags(Qt::TextSelectableByMouse);profileSummary_->setAlignment(Qt::AlignTop|Qt::AlignLeft);profileSummary_->setWordWrap(true);pfl->addWidget(profileSummary_);b=new QPushButton("EDIT SIP PROFILE...");connect(b,&QPushButton::clicked,this,&MainWindow::editProfile);pfl->addWidget(b);pfl->addStretch();tabs_->addTab(profilePage,"Profile");

    // ACTIVITY
    auto*activityPage=new QWidget;auto*actl=new QVBoxLayout(activityPage);activityLog_=new QPlainTextEdit;activityLog_->setReadOnly(true);activityLog_->setMaximumBlockCount(2000);actl->addWidget(activityLog_);tabs_->addTab(activityPage,"Activity");

    const QStringList navNames={QStringLiteral("// LINE ACCESS"),QStringLiteral("// ACTIVE LINES"),QStringLiteral("// SIGNAL TAP"),QStringLiteral("// NUMBER INTEL"),QStringLiteral("// BLAST DECK"),QStringLiteral("// SWITCH AUDIT+"),QStringLiteral("// IDENTITY"),QStringLiteral("// WIRE LOG")};
    const QStringList navText={QStringLiteral("[01] LINE ACCESS"),QStringLiteral("[02] ACTIVE LINES"),QStringLiteral("[03] SIGNAL TAP"),QStringLiteral("[04] NUMBER INTEL"),QStringLiteral("[05] BLAST DECK"),QStringLiteral("[06] SWITCH AUDIT+"),QStringLiteral("[07] IDENTITY"),QStringLiteral("[08] WIRE LOG")};
    const QStringList subtitles={QStringLiteral("Dial, tap and capture carrier sessions."),QStringLiteral("Control live dialogs, media and DTMF."),QStringLiteral("Read raw SIP traffic and transaction flow."),QStringLiteral("DID carrier/reputation dip and carrier handoff analysis."),QStringLiteral("Queue and call-blast lab traffic."),QStringLiteral("Authorized PBX / SBC recon, exposure and transport audit bench."),QStringLiteral("SIP account, route and audio identity."),QStringLiteral("Local operator and engine activity trail.")};
    std::vector<QPushButton*> navButtons;
    for(int i=0;i<navText.size();++i){auto*button=new QPushButton(navText[i],navHost);button->setCheckable(true);button->setProperty("nav",true);button->setChecked(i==0);nav->addWidget(button);navButtons.push_back(button);connect(button,&QPushButton::clicked,this,[this,i](){tabs_->setCurrentIndex(i);});}
    connect(tabs_,&QTabWidget::currentChanged,this,[pageTitle,pageSubtitle,navButtons,navNames,subtitles](int index){if(index>=0&&index<navNames.size()){pageTitle->setText(navNames[index]);pageSubtitle->setText(subtitles[index]);}for(int i=0;i<(int)navButtons.size();++i)navButtons[(std::size_t)i]->setChecked(i==index);});
    outer->addWidget(tabs_,1);shell->addWidget(content,1);setCentralWidget(c);applyTheme(theme_->currentData().toString());statusBar()->showMessage("S.I.P.H.E.R. r18 // DID INTEL // SIP + RTP + NEXT-OUT + SWITCH AUDIT+");setDiagnosticsEnabled(false);
}

void MainWindow::refresh(){
    try{
        if(engine_.pollSystemAudioRoute()) statusBar()->showMessage("Audio route changed — PJSIP sound device reopened and active call reattached",5000);
    }catch(const std::exception& e){
        statusBar()->showMessage(QString("Audio hot-plug recovery failed: %1").arg(e.what()),7000);
    }
    registration_->setText(QString::fromStdString(engine_.registrationText()));
    if(profileSummary_){const auto&p=engine_.profile();profileSummary_->setText(QString("<b>%1</b><br>File: %2<br>SIP URI: sip:%3@%4<br>Registrar: %5<br>Profile default prefix: %6<br>Current dial prefix: %7<br>Transport: %8<br>ICE: %9 &nbsp; SRTP: %10")
        .arg(QString::fromStdString(p.name)).arg(QString::fromStdString(profilePath_)).arg(QString::fromStdString(p.username)).arg(QString::fromStdString(p.sipDomain)).arg(QString::fromStdString(p.registrar)).arg(p.dialPrefix.empty()?QStringLiteral("<none>"):QString::fromStdString(p.dialPrefix)).arg(engine_.dialPrefix().empty()?QStringLiteral("<none>"):QString::fromStdString(engine_.dialPrefix())).arg(QString::fromStdString(toString(p.transport)).toUpper()).arg(p.useIce?"enabled":"disabled").arg(p.enableSrtp?"enabled":"disabled"));}
    if(activityLog_){QString summary=QString("Registration: %1\nCalls known: %2\n%3\nLog file: %4")
        .arg(QString::fromStdString(engine_.registrationText())).arg((int)engine_.calls().size()).arg(QString::fromStdString(engine_.captureStatus())).arg(QString::fromStdString(trunkmonkey::runtime::logPath().string()));if(activityLog_->toPlainText()!=summary)activityLog_->setPlainText(summary);}
    int keep=pendingSelectId_>=0?pendingSelectId_:selectedCallId();pendingSelectId_=-1;
    auto v=engine_.calls();calls_->blockSignals(true);calls_->setRowCount((int)v.size());int row=0,selectRow=-1;for(auto&x:v){
        QString codec=x.codecName.empty()?"--":QString::fromStdString(x.codecName)+(x.codecClockRate?QString("/%1").arg(x.codecClockRate):QString{});
        QStringList s={QString::number(x.id),x.purpose==CallPurpose::Phone?"PHONE":"QUEUE",x.direction==CallDirection::Incoming?"IN":"OUT",x.foreground?"*":"",QString::fromStdString(x.state),QString::number(x.lastStatusCode),QString::fromStdString(x.remoteUri),QString::fromStdString(x.callerId),showAddr(x.remoteRtpAddress),showAddr(x.sourceRtpAddress),codec,QString::fromStdString(x.lastReason)};
        for(int col=0;col<s.size();++col)calls_->setItem(row,col,new QTableWidgetItem(s[col]));if(x.id==keep)selectRow=row;++row;
    }
    calls_->blockSignals(false);if(selectRow>=0)calls_->selectRow(selectRow);else if(v.size()==1&&v.front().purpose==CallPurpose::Phone)calls_->selectRow(0);refreshDiagnostics();
}
int MainWindow::selectedCallId()const{auto rows=calls_->selectionModel()->selectedRows();if(rows.isEmpty())return-1;auto*item=calls_->item(rows.first().row(),0);return item?item->text().toInt():-1;}
void MainWindow::selectCallId(int id){for(int r=0;r<calls_->rowCount();++r){auto*i=calls_->item(r,0);if(i&&i->text().toInt()==id){calls_->selectRow(r);return;}}pendingSelectId_=id;}
void MainWindow::setDiagnosticsEnabled(bool e)
{
    // SIP packet capture is account/transport-level and intentionally remains
    // available with no selected call so it can be armed before the INVITE.
    if(sipPcapStart_) sipPcapStart_->setEnabled(engine_.started());
    if(callPcapStart_) callPcapStart_->setEnabled(engine_.started());
    if(captureInterface_) captureInterface_->setEnabled(engine_.started());
    for(auto*w:{sipTraceStart_,sipTraceStop_,rtpPcapStart_}) if(w) w->setEnabled(e);
}
void MainWindow::refreshDiagnostics(){
    int id=selectedCallId();captureStatus_->setText(QString::fromStdString(engine_.captureStatus()));if(id<0){setDiagnosticsEnabled(false);if(muteButton_){muteButton_->setEnabled(false);muteButton_->setText("MUTE MIC");}callIdLabel_->setText("--");mediaTarget_->setText("--");mediaSource_->setText("--");mediaLocal_->setText("--");mediaCodec_->setText("--");if(mediaQuality_)mediaQuality_->setText("--");diagnosticNote_->setText("Select a normal Phone call to view its SIP dialog and media endpoints.");sipLog_->setRowCount(0);rawSip_->clear();if(rawSipFlow_)rawSipFlow_->setText("Select a SIP signal above. SENT means S.I.P.H.E.R. transmitted it to the PBX; RECEIVED means it came from the PBX.");displayedTraceCallId_=-1;displayedTraceCount_=0;return;}
    try{
        auto c=engine_.callSnapshot(id);callIdLabel_->setText(showAddr(c.callIdString));mediaTarget_->setText(showAddr(c.remoteRtpAddress));mediaSource_->setText(showAddr(c.sourceRtpAddress));mediaLocal_->setText(showAddr(c.localRtpAddress));
        mediaCodec_->setText(c.codecName.empty()?"--":QString::fromStdString(c.codecName)+(c.codecClockRate?QString(" / %1 Hz").arg(c.codecClockRate):QString{}));
        if(mediaQuality_){const double den=(double)(c.rtpRxPackets+c.rtpRxLoss);const double loss=den>0.0?100.0*c.rtpRxLoss/den:0.0;mediaQuality_->setText(QString("RX %1 pkts | loss %2% | jitter %3 ms | RTT %4 ms | R %5 | MOS %6").arg((qulonglong)c.rtpRxPackets).arg(loss,0,'f',2).arg(c.rxJitterMs,0,'f',1).arg(c.rttMs,0,'f',1).arg(c.estimatedRFactor,0,'f',1).arg(c.estimatedMos,0,'f',2));}
        if(muteButton_){muteButton_->setEnabled(c.purpose==CallPurpose::Phone&&!c.disconnected);muteButton_->setText(c.microphoneMuted?"UNMUTE MIC":"MUTE MIC");}
        if(c.purpose!=CallPurpose::Phone){setDiagnosticsEnabled(false);diagnosticNote_->setText("Queue-test call selected. 2.0 keeps detailed SIP/RTP trace controls on normal single Phone calls only.");sipLog_->setRowCount(0);rawSip_->clear();if(rawSipFlow_)rawSipFlow_->setText("Select a SIP signal above. SENT means S.I.P.H.E.R. transmitted it to the PBX; RECEIVED means it came from the PBX.");displayedTraceCallId_=-1;displayedTraceCount_=0;return;}
        setDiagnosticsEnabled(true);auto rec=engine_.sipTraceRecording(id);sipTraceStart_->setEnabled(!rec);sipTraceStop_->setEnabled(rec);diagnosticNote_->setText(rec?QString("Raw SIP trace recording: %1").arg(QString::fromStdString(engine_.sipTracePath(id))):"Live SIP dialog logging active. Later INVITE transactions are labeled RE-INVITE.");
        auto t=engine_.sipTrace(id);if(displayedTraceCallId_!=id||displayedTraceCount_!=t.size()){
            sipLog_->blockSignals(true);sipLog_->setRowCount((int)t.size());for(int r=0;r<(int)t.size();++r){auto&e=t[(std::size_t)r];QString peer=e.peerAddress.empty()?QStringLiteral("--"):QString::fromStdString(e.peerAddress)+(e.peerPort?QString(":%1").arg(e.peerPort):QString{});QStringList s={QDateTime::fromMSecsSinceEpoch((qint64)e.timestampMs).toString("HH:mm:ss.zzz"),e.direction==SipDirection::Sent?"SENT →":"← RECEIVED",peer,QString::fromStdString(e.label),QString::number(e.cseq),e.statusCode?QString::number(e.statusCode):QString{},QString::fromStdString(e.reason)};for(int col=0;col<s.size();++col){auto*i=new QTableWidgetItem(s[col]);if(col==0){i->setData(Qt::UserRole,QString::fromStdString(e.rawMessage));i->setData(Qt::UserRole+1,e.direction==SipDirection::Sent?QStringLiteral("SENT → PBX (TX)"):QStringLiteral("← RECEIVED FROM PBX (RX)"));}sipLog_->setItem(r,col,i);}}
            sipLog_->blockSignals(false);displayedTraceCallId_=id;displayedTraceCount_=t.size();if(!t.empty())sipLog_->scrollToBottom();
        }
    }catch(const std::exception&e){diagnosticNote_->setText(QString("Diagnostics unavailable: %1").arg(e.what()));setDiagnosticsEnabled(false);}
}
void MainWindow::showRawSip(){auto rows=sipLog_->selectionModel()->selectedRows();if(rows.isEmpty())return;auto*i=sipLog_->item(rows.first().row(),0);if(i){rawSip_->setPlainText(i->data(Qt::UserRole).toString());if(rawSipFlow_)rawSipFlow_->setText(i->data(Qt::UserRole+1).toString());}}
void MainWindow::dial(){try{auto d=dialEdit_->text().trimmed();if(d.isEmpty())return;engine_.setDialPrefix(dialPrefixEdit_?dialPrefixEdit_->text().trimmed().toStdString():std::string{});const auto effective=engine_.normalizeDestination(d.toStdString(),true);auto id=engine_.makeCall(d.toStdString(),callerIdEdit_->text().trimmed().toStdString(),true,CallPurpose::Phone,true);pendingSelectId_=id;statusBar()->showMessage(QString("Dialing %1").arg(QString::fromStdString(effective)),5000);}catch(const pj::Error&e){QMessageBox::critical(this,"Dial failed",QString::fromStdString(e.info()));}catch(const std::exception&e){QMessageBox::critical(this,"Dial failed",e.what());}}
void MainWindow::answerSelected(){try{int id=selectedCallId();if(id>=0)engine_.answer(id);}catch(const pj::Error&e){QMessageBox::warning(this,"Answer",QString::fromStdString(e.info()));}catch(const std::exception&e){QMessageBox::warning(this,"Answer",e.what());}}
void MainWindow::hangupSelected(){try{int id=selectedCallId();if(id>=0)engine_.hangup(id);}catch(const pj::Error&e){QMessageBox::warning(this,"Hangup",QString::fromStdString(e.info()));}catch(const std::exception&e){QMessageBox::warning(this,"Hangup",e.what());}}
void MainWindow::foregroundSelected(){try{int id=selectedCallId();if(id>=0)engine_.setForeground(id);}catch(const pj::Error&e){QMessageBox::warning(this,"Foreground",QString::fromStdString(e.info()));}catch(const std::exception&e){QMessageBox::warning(this,"Foreground",e.what());}}
void MainWindow::holdSelected(){try{int id=selectedCallId();if(id>=0)engine_.hold(id);}catch(const pj::Error&e){QMessageBox::warning(this,"Hold",QString::fromStdString(e.info()));}catch(const std::exception&e){QMessageBox::warning(this,"Hold",e.what());}}
void MainWindow::resumeSelected(){try{int id=selectedCallId();if(id>=0)engine_.resume(id);}catch(const pj::Error&e){QMessageBox::warning(this,"Resume",QString::fromStdString(e.info()));}catch(const std::exception&e){QMessageBox::warning(this,"Resume",e.what());}}
void MainWindow::toggleMuteSelected(){try{int id=selectedCallId();if(id<0)return;auto c=engine_.callSnapshot(id);if(c.purpose!=CallPurpose::Phone)return;engine_.setMicrophoneMuted(id,!c.microphoneMuted);refreshDiagnostics();}catch(const pj::Error&e){QMessageBox::warning(this,"Microphone",QString::fromStdString(e.info()));}catch(const std::exception&e){QMessageBox::warning(this,"Microphone",e.what());}}
void MainWindow::showDtmfPad(){
    const int id=selectedCallId();
    if(id<0){QMessageBox::information(this,"DTMF","Select an active Phone call first.");return;}
    try{if(engine_.callSnapshot(id).purpose!=CallPurpose::Phone){QMessageBox::information(this,"DTMF","DTMF pad is available for normal Phone calls.");return;}}catch(const std::exception&e){QMessageBox::warning(this,"DTMF",e.what());return;}

    QDialog pad(this);pad.setWindowTitle(QString("DTMF Pad — Call %1").arg(id));pad.setModal(false);pad.setMinimumWidth(280);
    auto*layout=new QVBoxLayout(&pad);auto*hint=new QLabel("Press and hold a key. The RFC4733 event duration follows how long you hold the mouse button; the event is sent on release.");hint->setWordWrap(true);layout->addWidget(hint);
    auto*grid=new QGridLayout;layout->addLayout(grid);auto*status=new QLabel("Ready");status->setAlignment(Qt::AlignCenter);layout->addWidget(status);
    QElapsedTimer held;QString activeDigit;
    const QStringList digits={"1","2","3","4","5","6","7","8","9","*","0","#"};
    for(int i=0;i<digits.size();++i){
        auto*key=new QPushButton(digits[i]);key->setMinimumSize(70,48);key->setAutoRepeat(false);grid->addWidget(key,i/3,i%3);
        connect(key,&QPushButton::pressed,&pad,[&,key](){activeDigit=key->text();held.restart();status->setText(QString("Holding %1...").arg(activeDigit));});
        connect(key,&QPushButton::released,&pad,[&,key](){
            if(activeDigit!=key->text() || !held.isValid())return;
            const auto elapsed=held.elapsed();
            const unsigned duration=static_cast<unsigned>(std::clamp<qint64>(elapsed,80,5000));
            try{engine_.sendDtmf(id,key->text().toStdString(),duration);status->setText(QString("Sent %1 — %2 ms").arg(key->text()).arg(duration));}
            catch(const pj::Error&e){QMessageBox::warning(&pad,"DTMF",QString::fromStdString(e.info()));}
            catch(const std::exception&e){QMessageBox::warning(&pad,"DTMF",e.what());}
            activeDigit.clear();held.invalidate();
        });
    }
    auto*close=new QPushButton("CLOSE");connect(close,&QPushButton::clicked,&pad,&QDialog::accept);layout->addWidget(close);
    pad.exec();
}
void MainWindow::startSipTrace(){int id=selectedCallId();if(id<0)return;auto path=QFileDialog::getSaveFileName(this,"Save raw SIP trace",QString("sipher-call-%1-sip.log").arg(id),"Log files (*.log *.txt);;All files (*)");if(path.isEmpty())return;try{engine_.startSipTraceFile(id,path.toStdString());refreshDiagnostics();}catch(const std::exception&e){QMessageBox::warning(this,"SIP trace",e.what());}}
void MainWindow::stopSipTrace(){int id=selectedCallId();if(id<0)return;try{engine_.stopSipTraceFile(id);refreshDiagnostics();}catch(const std::exception&e){QMessageBox::warning(this,"SIP trace",e.what());}}
void MainWindow::startSipPcap()
{
    const int id=selectedCallId();
    const QString suggested=id>=0?QString("sipher-call-%1-sip.pcapng").arg(id):QString("sipher-predial-sip-%1.pcapng").arg(QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss"));
    auto path=QFileDialog::getSaveFileName(this,"Save SIP packet capture (start before dialing)",suggested,"PCAP files (*.pcap *.pcapng);;All files (*)");
    if(path.isEmpty())return;
    try{
        engine_.startSipPcap(path.toStdString(),captureInterface_->currentText().trimmed().toStdString());
        lastPcapPath_=path;
        lastPcapCallId_=id;
        lastPcapKind_=LastPcapKind::Sip;
        statusBar()->showMessage("SIP PCAP armed. Dial now; the initial INVITE, prefixed Request-URI, and any 403/401/407 response will be captured.",8000);
        refreshDiagnostics();
    }catch(const std::exception&e){QMessageBox::warning(this,"SIP PCAP",e.what());}
}
void MainWindow::startRtpPcap(){int id=selectedCallId();if(id<0)return;auto path=QFileDialog::getSaveFileName(this,"Save RTP packet capture",QString("sipher-call-%1-rtp.pcapng").arg(id),"PCAP files (*.pcap *.pcapng);;All files (*)");if(path.isEmpty())return;try{engine_.startRtpPcap(id,path.toStdString(),captureInterface_->currentText().trimmed().toStdString());lastPcapPath_=path;lastPcapCallId_=id;lastPcapKind_=LastPcapKind::Rtp;refreshDiagnostics();}catch(const std::exception&e){QMessageBox::warning(this,"RTP PCAP",e.what());}}
void MainWindow::startCallPcap(){auto path=QFileDialog::getSaveFileName(this,"Save full VoIP packet capture (start before dialing)",QString("sipher-full-voip-%1.pcapng").arg(QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss")),"PCAP files (*.pcap *.pcapng);;All files (*)");if(path.isEmpty())return;try{engine_.startCallPcap(path.toStdString(),captureInterface_->currentText().trimmed().toStdString());lastPcapPath_=path;lastPcapCallId_=-1;lastPcapKind_=LastPcapKind::Voip;refreshDiagnostics();statusBar()->showMessage("FULL VOIP PCAP armed BEFORE DIAL. Place the call now; keep capture running through hangup for Wireshark VoIP Calls.",9000);}catch(const std::exception&e){QMessageBox::warning(this,"Full VoIP PCAP",e.what());}}
void MainWindow::stopPcaps(){engine_.stopCaptures();refreshDiagnostics();}
void MainWindow::openLastPcap(){
    if(lastPcapPath_.isEmpty()){QMessageBox::information(this,"Wireshark","Start a SIP, RTP, or full VoIP PCAP first.");return;}
    try{
        if(lastPcapKind_==LastPcapKind::Sip){engine_.openSipPcapInWireshark(lastPcapPath_.toStdString());statusBar()->showMessage("Opened SIP PCAP in Wireshark with forced SIP Decode As",5000);}
        else if(lastPcapKind_==LastPcapKind::Voip){engine_.openVoipPcapInWireshark(lastPcapPath_.toStdString());statusBar()->showMessage("Opened full VoIP PCAP. Use Telephony > VoIP Calls; SIP/SDP and RTP are in the same capture.",8000);}
        else if(lastPcapKind_==LastPcapKind::Rtp){if(lastPcapCallId_<0)throw std::runtime_error("The selected RTP PCAP no longer has a call context for automatic decode.");engine_.openPcapInWireshark(lastPcapCallId_,lastPcapPath_.toStdString());statusBar()->showMessage("Opened RTP PCAP in Wireshark with automatic RTP/RTCP Decode As",5000);}
        else throw std::runtime_error("No PCAP has been started in this session.");
    }catch(const std::exception&e){QMessageBox::warning(this,"Wireshark",e.what());}
}
void MainWindow::loadDestinations(){destinationFile_=QFileDialog::getOpenFileName(this,"Destination list",{},"Text files (*.txt);;All files (*)");destinationFileLabel_->setText(destinationFile_.isEmpty()?"No destination list loaded":destinationFile_);}
void MainWindow::loadCallerIds(){callerIdFile_=QFileDialog::getOpenFileName(this,"Caller-ID list",{},"Text files (*.txt);;All files (*)");callerIdFileLabel_->setText(callerIdFile_.isEmpty()?"No caller-ID list loaded":callerIdFile_);}
void MainWindow::loadQueueAudio(){queueAudioFile_=QFileDialog::getOpenFileName(this,"Queue-test audio",{},"Audio files (*.wav *.mp3 *.flac *.ogg *.m4a);;All files (*)");if(queueAudioFileLabel_)queueAudioFileLabel_->setText(queueAudioFile_.isEmpty()?"Live/no injected audio":queueAudioFile_);}
void MainWindow::launchBatch(){try{MultiCallPlan p;p.callCount=(std::size_t)batchCount_->value();p.launchIntervalMs=(unsigned)launchInterval_->value();p.singleDestination=batchDestination_->text().trimmed().toStdString();p.fixedCallerId=fixedCallerId_->text().trimmed().toStdString();p.audioFile=queueAudioFile_.toStdString();if(!destinationFile_.isEmpty())p.destinations=loadList(destinationFile_);if(!callerIdFile_.isEmpty())p.callerIds=loadList(callerIdFile_);multi_.start(p);}catch(const std::exception&e){QMessageBox::critical(this,"Queue test",e.what());}}
void MainWindow::hangupAll(){engine_.hangupAll();}
void MainWindow::showSipLadder(){int id=selectedCallId();if(id<0)return;try{QMessageBox box(this);box.setWindowTitle(QString("SIP Ladder — Call %1").arg(id));box.setTextFormat(Qt::PlainText);box.setText(QString::fromStdString(engine_.sipLadder(id)));box.setStandardButtons(QMessageBox::Ok);box.exec();}catch(const std::exception&e){QMessageBox::warning(this,"SIP ladder",e.what());}}
void MainWindow::exportCallReport(){int id=selectedCallId();if(id<0)return;auto path=QFileDialog::getSaveFileName(this,"Export call diagnostic report",QString("sipher-call-%1-report.txt").arg(id),"Text reports (*.txt);;All files (*)");if(path.isEmpty())return;try{engine_.exportCallReport(id,path.toStdString());statusBar()->showMessage("Call report exported",5000);}catch(const std::exception&e){QMessageBox::warning(this,"Call report",e.what());}}

void MainWindow::showAudioDevices(){try{const auto devices=engine_.audioDevices();if(devices.empty()){QMessageBox::information(this,"Audio Devices","No PJSIP audio devices are available.");return;}QDialog d(this);d.setWindowTitle("Audio Devices");auto*layout=new QVBoxLayout(&d);auto*form=new QFormLayout;auto*capture=new QComboBox;auto*playback=new QComboBox;for(const auto&dev:devices){const auto label=QString("[%1] %2 / %3").arg(dev.id).arg(QString::fromStdString(dev.driver)).arg(QString::fromStdString(dev.name));if(dev.inputCount>0){capture->addItem(label,dev.id);if(dev.id==engine_.activeCaptureDevice())capture->setCurrentIndex(capture->count()-1);}if(dev.outputCount>0){playback->addItem(label,dev.id);if(dev.id==engine_.activePlaybackDevice())playback->setCurrentIndex(playback->count()-1);}}form->addRow("Microphone",capture);form->addRow("Playback",playback);layout->addLayout(form);auto*hint=new QLabel("Capture and playback are independent. Applying a change performs a real PJSIP close/refresh/reopen and reattaches the foreground call. Linux follows PipeWire/PulseAudio. FreeBSD follows PulseAudio when present, otherwise native OSS/snd_hda default-unit, PCM and recording-source changes.");hint->setWordWrap(true);layout->addWidget(hint);auto*buttons=new QGridLayout;auto*apply=new QPushButton("APPLY");auto*cancel=new QPushButton("CANCEL");buttons->addWidget(apply,0,0);buttons->addWidget(cancel,0,1);layout->addLayout(buttons);connect(cancel,&QPushButton::clicked,&d,&QDialog::reject);connect(apply,&QPushButton::clicked,&d,[&](){engine_.selectAudioDevices(capture->currentData().toInt(),playback->currentData().toInt());d.accept();});d.exec();}catch(const pj::Error&e){QMessageBox::warning(this,"Audio Devices",QString::fromStdString(e.info()));}catch(const std::exception&e){QMessageBox::warning(this,"Audio Devices",e.what());}}

void MainWindow::showAudioOutput()
{
    try{
        const auto devices=engine_.audioDevices();
        QDialog d(this);
        d.setWindowTitle("Audio Output");
        auto* layout=new QVBoxLayout(&d);
        auto* form=new QFormLayout;
        auto* playback=new QComboBox;
        for(const auto& dev:devices){
            if(dev.outputCount==0) continue;
            const auto label=QString("[%1] %2 / %3  (%4 output channel%5)")
                .arg(dev.id)
                .arg(QString::fromStdString(dev.driver))
                .arg(QString::fromStdString(dev.name))
                .arg(dev.outputCount)
                .arg(dev.outputCount==1 ? "" : "s");
            playback->addItem(label,dev.id);
            if(dev.id==engine_.activePlaybackDevice()) playback->setCurrentIndex(playback->count()-1);
        }
        if(playback->count()==0){
            QMessageBox::information(this,"Audio Output","No playback-capable PJSIP audio devices are available.");
            return;
        }
        form->addRow("Output device",playback);
        layout->addLayout(form);
        auto* hint=new QLabel("Changes only playback/output; the microphone is left unchanged. S.I.P.H.E.R. now closes and reopens PJSIP audio after selection so the change affects an already-open call stream. On Linux, ALSA / pipewire is the preferred desktop route.");
        hint->setWordWrap(true);
        layout->addWidget(hint);
        auto* buttons=new QGridLayout;
        auto* apply=new QPushButton("USE OUTPUT");
        auto* cancel=new QPushButton("CANCEL");
        buttons->addWidget(apply,0,0);buttons->addWidget(cancel,0,1);
        layout->addLayout(buttons);
        connect(cancel,&QPushButton::clicked,&d,&QDialog::reject);
        connect(apply,&QPushButton::clicked,&d,[&](){
            const int id=playback->currentData().toInt();
            engine_.selectPlaybackDevice(id);
            statusBar()->showMessage(QString("Audio output changed to device %1 and PJSIP sound device reopened").arg(id),5000);
            d.accept();
        });
        d.exec();
    }catch(const pj::Error&e){
        QMessageBox::warning(this,"Audio Output",QString::fromStdString(e.info()));
    }catch(const std::exception&e){
        QMessageBox::warning(this,"Audio Output",e.what());
    }
}
void MainWindow::reopenAudio()
{
    try{
        engine_.refreshAudioDevices();
        const auto st=engine_.audioStatus();
        statusBar()->showMessage(QString("Audio reopened: capture %1, playback %2, sound %3")
            .arg(st.captureId).arg(st.playbackId).arg(st.soundActive?"ACTIVE":"INACTIVE"),6000);
        if(!st.soundActive) QMessageBox::warning(this,"Audio Reopen","PJSIP completed the reopen but did not report an active sound device. Check the Engine Log for the exact media error.");
    }catch(const pj::Error& e){QMessageBox::warning(this,"Audio Reopen",QString::fromStdString(e.info()));}
    catch(const std::exception& e){QMessageBox::warning(this,"Audio Reopen",e.what());}
}

void MainWindow::showAudioStatus()
{
    try{
        const auto st=engine_.audioStatus();
        QString text=QString("Capture: %1\nPlayback: %2\nPJSIP sound active: %3\nAutomatic switching: %4\nHot-plug watch: %5")
            .arg(QString::fromStdString(st.captureDevice))
            .arg(QString::fromStdString(st.playbackDevice))
            .arg(st.soundActive?"YES":"NO")
            .arg(st.autoSwitchEnabled?"ON":"OFF")
            .arg(st.hotplugWatchAvailable?"AVAILABLE":"UNAVAILABLE");
        if(!st.hotplugBackend.empty())text+=QString("\nWatcher backend: %1").arg(QString::fromStdString(st.hotplugBackend));
        if(!st.systemRoute.empty())text+=QString("\nSystem route: %1").arg(QString::fromStdString(st.systemRoute));
        QMessageBox::information(this,"Audio Status",text);
    }catch(const pj::Error& e){QMessageBox::warning(this,"Audio Status",QString::fromStdString(e.info()));}
    catch(const std::exception& e){QMessageBox::warning(this,"Audio Status",e.what());}
}

void MainWindow::showRegistrationHistory(){std::ostringstream out;for(const auto&line:engine_.registrationHistory())out<<line<<"\n";QMessageBox box(this);box.setWindowTitle("Registration History");box.setTextFormat(Qt::PlainText);box.setText(QString::fromStdString(out.str().empty()?std::string("No registration state changes recorded yet."):out.str()));box.exec();}

void MainWindow::lookupDid()
{
    if(!didNumber_||!didOutput_||!network_)return;
    const QString number=didNumber_->text().trimmed();
    if(number.isEmpty()){QMessageBox::information(this,"DID Intelligence","Enter a DID / telephone number first.");return;}
    QString key=didApiKey_?didApiKey_->text().trimmed():QString{};
    if(key.isEmpty())key=QString::fromUtf8(qgetenv("SIPHER_IPQS_API_KEY")).trimmed();
    if(key.isEmpty()){
        QMessageBox::information(this,"DID Intelligence","An IPQualityScore API key is required for live carrier/reputation data. Enter it in the DID Intelligence panel or set SIPHER_IPQS_API_KEY before starting S.I.P.H.E.R.");
        return;
    }

    QUrl url(QStringLiteral("https://ipqualityscore.com/api/json/phone"));
    QUrlQuery query;query.addQueryItem(QStringLiteral("phone"),number);query.addQueryItem(QStringLiteral("strictness"),QStringLiteral("1"));
    const QString country=didCountry_?didCountry_->currentText().trimmed().toUpper():QString{};if(!country.isEmpty())query.addQueryItem(QStringLiteral("country[]"),country);
    url.setQuery(query);
    QNetworkRequest request(url);request.setRawHeader("IPQS-KEY",key.toUtf8());request.setHeader(QNetworkRequest::UserAgentHeader,QStringLiteral("S.I.P.H.E.R./1.0.0-r18"));
    didOutput_->setPlainText(QString("Looking up %1...\n\nProvider: IPQualityScore Phone Number Validation API\nThe API key is being sent in the IPQS-KEY header, not embedded in the URL.").arg(number));
    auto*reply=network_->get(request);
    connect(reply,&QNetworkReply::finished,this,[this,reply,number](){
        const QByteArray payload=reply->readAll();const auto networkError=reply->error();const QString networkErrorText=reply->errorString();reply->deleteLater();
        if(networkError!=QNetworkReply::NoError){didOutput_->setPlainText(QString("DID lookup failed for %1\n\nNetwork/API error: %2").arg(number,networkErrorText));return;}
        QJsonParseError parseError{};const auto doc=QJsonDocument::fromJson(payload,&parseError);if(parseError.error!=QJsonParseError::NoError||!doc.isObject()){didOutput_->setPlainText(QString("DID lookup returned an unreadable response for %1\n\n%2").arg(number,parseError.errorString()));return;}
        const auto o=doc.object();const bool success=o.value("success").toBool(false);const int score=o.value("fraud_score").toInt(-1);const bool spammer=o.value("spammer").isBool()&&o.value("spammer").toBool();const bool recent=o.value("recent_abuse").isBool()&&o.value("recent_abuse").toBool();const bool risky=o.value("risky").isBool()&&o.value("risky").toBool();
        QString verdict;
        if(spammer)verdict="FLAGGED — provider reports recent spam/harassing-call or text reports.";
        else if(recent)verdict="HIGH RISK — provider reports recent/ongoing abuse activity.";
        else if(risky||score>=85)verdict="HIGH RISK — provider reputation/risk signals are elevated.";
        else if(score>=75)verdict="SUSPICIOUS — elevated reputation score; investigate before treating as abusive.";
        else verdict="No major spam/abuse flag returned by this provider at lookup time.";
        auto textValue=[&](const char*key){const auto v=o.value(key);return v.isString()?v.toString():QStringLiteral("N/A");};
        QString out;
        out+="S.I.P.H.E.R. r18 — DID / NUMBER INTELLIGENCE\n";
        out+="================================================\n";
        out+=QString("Query:              %1\n").arg(number);
        out+=QString("Provider message:   %1\n").arg(textValue("message"));
        out+=QString("Lookup success:     %1\n").arg(success?"YES":"NO");
        out+=QString("Formatted:          %1\n").arg(textValue("formatted"));
        out+=QString("Valid:              %1\n").arg(jsonTriState(o.value("valid")));
        out+=QString("Active:             %1\n").arg(jsonTriState(o.value("active")));
        out+=QString("Carrier:            %1\n").arg(textValue("carrier"));
        out+=QString("Line type:          %1\n").arg(textValue("line_type"));
        out+=QString("Country:            %1\n").arg(textValue("country"));
        out+=QString("Region:             %1\n").arg(textValue("region"));
        out+=QString("VOIP:               %1\n").arg(jsonTriState(o.value("VOIP")));
        out+=QString("Prepaid:            %1\n").arg(jsonTriState(o.value("prepaid")));
        out+=QString("Active status:      %1\n").arg(textValue("active_status"));
        out+="\nREPUTATION / ABUSE\n------------------\n";
        out+=QString("Fraud score:        %1\n").arg(score>=0?QString::number(score)+" / 100":QStringLiteral("N/A"));
        out+=QString("Risky:              %1\n").arg(jsonTriState(o.value("risky")));
        out+=QString("Recent abuse:       %1\n").arg(jsonTriState(o.value("recent_abuse")));
        out+=QString("Spammer flag:       %1\n").arg(jsonTriState(o.value("spammer")));
        out+=QString("Do Not Call:        %1\n").arg(jsonTriState(o.value("do_not_call")));
        out+=QString("Leaked/compromised:%1\n").arg(QString(" %1").arg(jsonTriState(o.value("leaked"))));
        out+=QString("\nVERDICT\n-------\n%1\n").arg(verdict);
        out+="\nInterpretation: reputation scores and flags are indicators from a third-party data set, not proof of criminal activity or caller identity. S.I.P.H.E.R. intentionally does not display reverse-owner identity enrichment or associated email/address data.\n";
        didOutput_->setPlainText(out);statusBar()->showMessage("DID intelligence lookup complete",5000);
    });
}

void MainWindow::analyzeNextOut()
{
    if(!routeOutput_)return;
    try{
        const auto&p=engine_.profile();
        QString destination=routeDestination_?routeDestination_->text().trimmed():QString{};if(destination.isEmpty()&&didNumber_)destination=didNumber_->text().trimmed();if(destination.isEmpty()&&dialEdit_)destination=dialEdit_->text().trimmed();
        QString requestUri=destination.isEmpty()?QStringLiteral("<destination not supplied>"):QString::fromStdString(engine_.normalizeDestination(destination.toStdString(),true));
        QString configured;QString source;
        if(!p.outboundProxy.empty()){configured=QString::fromStdString(p.outboundProxy);source="Outbound proxy";}
        else if(!p.registrar.empty()){configured=QString::fromStdString(p.registrar);source="Registrar (no outbound proxy configured)";}
        else{configured=QString::fromStdString(p.sipDomain);source="SIP domain (direct/RFC3263-style resolution)";}
        const auto target=parseSipTarget(configured,p.transport);
        QString out="S.I.P.H.E.R. r18 — CARRIER HANDOFF / NEXT-OUT\n================================================\n";
        out+=QString("Destination input:       %1\n").arg(destination.isEmpty()?QStringLiteral("<none>"):destination);
        out+=QString("Normalized Request-URI:  %1\n").arg(requestUri);
        out+=QString("Profile transport:       %1\n").arg(QString::fromStdString(toString(p.transport)).toUpper());
        out+=QString("Handoff source:          %1\n").arg(source);
        out+=QString("Configured handoff URI:  %1\n").arg(configured.isEmpty()?QStringLiteral("<none>"):configured);
        out+=QString("Expected next-hop host:  %1\n").arg(target.host.isEmpty()?QStringLiteral("<unresolved>"):target.host);
        out+=QString("Expected next-hop port:  %1%2\n").arg(target.port).arg(target.explicitPort?QStringLiteral(" (explicit)"):QStringLiteral(" (default/candidate)"));

        if(!target.host.isEmpty()){
            const auto info=QHostInfo::fromName(target.host);out+="\nDNS A/AAAA CANDIDATES\n--------------------\n";
            if(info.error()==QHostInfo::NoError&&!info.addresses().isEmpty()){QStringList ips;for(const auto&a:info.addresses()){const auto ip=a.toString();if(!ips.contains(ip))ips.push_back(ip);}for(const auto&ip:ips)out+=QString(" - %1:%2\n").arg(ip).arg(target.port);}else out+=QString(" - Resolution unavailable: %1\n").arg(info.errorString());

            if(p.outboundProxy.empty()&&!target.explicitPort){
                const QString service=(p.transport==Transport::Tls?QStringLiteral("_sips._tcp."):(p.transport==Transport::Tcp?QStringLiteral("_sip._tcp."):QStringLiteral("_sip._udp.")))+target.host;
                QDnsLookup dns(QDnsLookup::SRV,service);QEventLoop loop;QTimer timer;timer.setSingleShot(true);connect(&dns,&QDnsLookup::finished,&loop,&QEventLoop::quit);connect(&timer,&QTimer::timeout,&loop,[&](){dns.abort();loop.quit();});timer.start(2200);dns.lookup();loop.exec();
                out+=QString("\nSIP SRV CANDIDATES (%1)\n--------------------\n").arg(service);
                if(dns.error()==QDnsLookup::NoError&&!dns.serviceRecords().isEmpty()){for(const auto&r:dns.serviceRecords())out+=QString(" - priority %1 weight %2  %3:%4\n").arg(r.priority()).arg(r.weight()).arg(r.target()).arg(r.port());}else out+=QString(" - No SRV result observed%1\n").arg(dns.error()==QDnsLookup::NoError?QString{}:QString(": ")+dns.errorString());
            }
        }

        out+="\nOBSERVED SIP PATH DISCLOSURES\n-----------------------------\n";
        const int id=selectedCallId();
        if(id<0)out+="No call is selected. Select a Phone call under ACTIVE LINES and rerun this analysis to add observed SIP headers.\n";
        else{
            try{
                const auto trace=engine_.sipTrace(id);QString firstInvite,observedPeer;QStringList routes,recordRoutes,vias,contacts;
                for(const auto&e:trace){if(firstInvite.isEmpty()&&e.direction==SipDirection::Sent&&e.method=="INVITE"){const auto raw=QString::fromStdString(e.rawMessage);firstInvite=raw.split(QRegularExpression("\\r?\\n")).value(0).trimmed();if(!e.peerAddress.empty())observedPeer=QString::fromStdString(e.peerAddress)+(e.peerPort?QString(":%1").arg(e.peerPort):QString{});}appendUnique(routes,sipHeaderValues(e.rawMessage,"Route"));appendUnique(recordRoutes,sipHeaderValues(e.rawMessage,"Record-Route"));appendUnique(vias,sipHeaderValues(e.rawMessage,"Via"));appendUnique(contacts,sipHeaderValues(e.rawMessage,"Contact"));}
                out+=QString("Selected Call ID:        %1\n").arg(id);if(!firstInvite.isEmpty())out+=QString("Observed INVITE line:    %1\n").arg(firstInvite);out+=QString("Actual INVITE peer:      %1\n").arg(observedPeer.isEmpty()?QStringLiteral("<not captured>"):observedPeer);
                auto emit=[&](const QString&label,const QStringList&values){out+=label+"\n";if(values.isEmpty())out+=" - <none disclosed>\n";else for(const auto&v:values)out+=" - "+v+"\n";};
                emit("Route headers:",routes);emit("Record-Route headers:",recordRoutes);emit("Via headers:",vias);emit("Contact headers:",contacts);
            }catch(const std::exception&e){out+=QString("Selected call trace unavailable: %1\n").arg(e.what());}
        }
        out+="\nROUTE NOTE\n----------\nThe configured/resolved peer above is the expected SIP handoff from S.I.P.H.E.R. Once the carrier SBC accepts the INVITE, additional carrier-internal proxies, tandems or terminating-network hops may be intentionally hidden by topology-hiding. IP traceroute to the SBC is a network path, not a PSTN/SIP call-route trace.\n";
        routeOutput_->setPlainText(out);statusBar()->showMessage("Next-out / carrier handoff analysis complete",5000);
    }catch(const std::exception&e){QMessageBox::warning(this,"Carrier handoff analysis",e.what());}
}

void MainWindow::showBlueBoxLegacy()
{
    QDialog d(this);d.setWindowTitle("Legacy — Blue Tone / Blue Box Historical Lab");d.setMinimumWidth(560);auto*l=new QVBoxLayout(&d);auto*title=new QLabel("BLUE TONE / BLUE BOX // HISTORICAL SIGNALING LAB");title->setStyleSheet("font-weight:700; font-size:16px;");l->addWidget(title);auto*text=new QLabel("Blue boxes are historically associated with in-band telephone network-control signaling on older long-distance systems. This S.I.P.H.E.R. panel is an offline visual/history simulator only: it does not generate network-control audio, inject signaling into calls, or provide live carrier manipulation functions.");text->setWordWrap(true);l->addWidget(text);auto*status=new QLabel("SIMULATION: IDLE — NO AUDIO / NO NETWORK OUTPUT");status->setWordWrap(true);l->addWidget(status);auto*g=new QGridLayout;for(int i=0;i<12;++i){auto*b=new QPushButton(QString("LEGACY KEY %1").arg(i+1));connect(b,&QPushButton::clicked,&d,[status,i](){status->setText(QString("SIMULATION: legacy control %1 selected — visualization only; nothing transmitted.").arg(i+1));});g->addWidget(b,i/3,i%3);}l->addLayout(g);auto*close=new QPushButton("CLOSE");connect(close,&QPushButton::clicked,&d,&QDialog::accept);l->addWidget(close);d.exec();
}

void MainWindow::showRedBoxLegacy()
{
    QDialog d(this);d.setWindowTitle("Legacy — Red Box Historical Lab");d.setMinimumWidth(560);auto*l=new QVBoxLayout(&d);auto*title=new QLabel("RED BOX // HISTORICAL PAYPHONE LAB");title->setStyleSheet("font-weight:700; font-size:16px;");l->addWidget(title);auto*text=new QLabel("Red boxes are historically associated with imitating legacy payphone coin signaling. This panel preserves the history/aesthetic as an offline simulator only. It cannot produce live coin-control tones, interact with a payphone, alter billing, or transmit control signaling into a call.");text->setWordWrap(true);l->addWidget(text);auto*status=new QLabel("SIMULATION: IDLE — NO AUDIO / NO NETWORK OUTPUT");status->setWordWrap(true);l->addWidget(status);auto*g=new QHBoxLayout;for(int i=0;i<3;++i){auto*b=new QPushButton(QString("COIN SIGNAL %1").arg(QChar('A'+i)));connect(b,&QPushButton::clicked,&d,[status,i](){status->setText(QString("SIMULATION: coin event %1 selected — visualization only; nothing transmitted.").arg(QChar('A'+i)));});g->addWidget(b);}l->addLayout(g);auto*close=new QPushButton("CLOSE");connect(close,&QPushButton::clicked,&d,&QDialog::accept);l->addWidget(close);d.exec();
}

static AuditTransport guiAuditTransport(QComboBox* box){return PbxAudit::transportFromString(box?box->currentData().toString().toStdString():"udp");}
void MainWindow::runAuditAuto(){
    const auto host=auditHost_->text().trimmed().toStdString();if(host.empty()){QMessageBox::information(this,"Automated PBX audit","Enter a PBX/SBC target first.");return;}
    if(QMessageBox::question(this,"Confirm authorized scope","Run the chained active SIP audit against this target? Only continue if you own the system or have explicit authorization to assess it.",QMessageBox::Yes|QMessageBox::No,QMessageBox::No)!=QMessageBox::Yes)return;
    try{
        AutomatedAuditOptions opt;opt.host=host;opt.port=(std::uint16_t)auditPort_->value();opt.transport=guiAuditTransport(auditTransport_);opt.username=auditUser_->text().trimmed().toStdString();if(opt.username.empty())opt.username=engine_.profile().username;
        opt.includeVulnerabilityLookup=auditIncludeVulns_&&auditIncludeVulns_->isChecked();opt.includeParserAudit=auditIncludeParser_&&auditIncludeParser_->isChecked();opt.includeResilienceAudit=auditIncludeResilience_&&auditIncludeResilience_->isChecked();opt.includeTlsAudit=auditIncludeTls_&&auditIncludeTls_->isChecked();opt.includeExtensionAudit=auditIncludeExtensions_&&auditIncludeExtensions_->isChecked();opt.extensionFirst=(unsigned)auditExtFirst_->value();opt.extensionLast=(unsigned)auditExtLast_->value();
        QApplication::setOverrideCursor(Qt::WaitCursor);auditOutput_->setPlainText("Starting chained audit...\n");
        auto result=PbxAudit::automatedAudit(opt,[this](unsigned phase,unsigned total,const std::string&label){const auto text=QString("Audit phase %1/%2 — %3").arg(phase).arg(total).arg(QString::fromStdString(label));auditProgress_->setText(text);statusBar()->showMessage(text);QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);});
        QApplication::restoreOverrideCursor();lastAuditReport_=result.toText();auditOutput_->setPlainText(QString::fromStdString(lastAuditReport_));auditProgress_->setText(QString("Complete — %1 HIGH, %2 WARN, %3 PASS, %4 INFO").arg(result.highCount).arg(result.warnCount).arg(result.passCount).arg(result.infoCount));statusBar()->showMessage("Automated PBX/SIP audit complete",7000);
    }catch(const std::exception&e){QApplication::restoreOverrideCursor();auditProgress_->setText("Automated audit stopped with an error.");QMessageBox::warning(this,"Automated PBX audit",e.what());}
}
void MainWindow::runAuditFingerprint(){try{const auto host=auditHost_->text().trimmed().toStdString();if(host.empty()){QMessageBox::information(this,"PBX fingerprint","Enter a target first.");return;}auto fp=PbxAudit::fingerprint(host,(std::uint16_t)auditPort_->value(),PbxAudit::transportFromString(auditTransport_->currentData().toString().toStdString()));lastAuditReport_=fp.toText();auditOutput_->setPlainText(QString::fromStdString(lastAuditReport_));}catch(const std::exception&e){QMessageBox::warning(this,"PBX fingerprint",e.what());}}
void MainWindow::runAuditVulns(){try{const auto host=auditHost_->text().trimmed().toStdString();if(host.empty()){QMessageBox::information(this,"Vulnerability lookup","Enter a target first.");return;}auto fp=PbxAudit::fingerprint(host,(std::uint16_t)auditPort_->value(),PbxAudit::transportFromString(auditTransport_->currentData().toString().toStdString()));lastAuditReport_=fp.toText()+"\n"+PbxAudit::vulnerabilityLookupReport(fp);auditOutput_->setPlainText(QString::fromStdString(lastAuditReport_));}catch(const std::exception&e){QMessageBox::warning(this,"Vulnerability lookup",e.what());}}
void MainWindow::runAuditProbe(){try{const auto host=auditHost_->text().trimmed().toStdString();if(host.empty())throw std::runtime_error("Target is required");auto r=PbxAudit::serviceProbe(host,(std::uint16_t)auditPort_->value(),guiAuditTransport(auditTransport_));lastAuditReport_=PbxAudit::report("PBX SERVICE PROBE",{r});auditOutput_->setPlainText(QString::fromStdString(lastAuditReport_));}catch(const std::exception&e){QMessageBox::warning(this,"PBX audit",e.what());}}
void MainWindow::runAuditDiscover(){try{const auto cidr=auditHost_->text().trimmed().toStdString();if(cidr.empty())throw std::runtime_error("IPv4 CIDR is required");auto entries=PbxAudit::discoverIpv4Cidr(cidr,(std::uint16_t)auditPort_->value(),guiAuditTransport(auditTransport_));lastAuditReport_=PbxAudit::report("BOUNDED SIP DISCOVERY",{}, {}, {},entries);auditOutput_->setPlainText(QString::fromStdString(lastAuditReport_));}catch(const std::exception&e){QMessageBox::warning(this,"PBX discovery",e.what());}}
void MainWindow::runAuditMethods(){try{const auto host=auditHost_->text().trimmed().toStdString();if(host.empty())throw std::runtime_error("Target is required");auto rs=PbxAudit::methodAudit(host,(std::uint16_t)auditPort_->value(),guiAuditTransport(auditTransport_));lastAuditReport_=PbxAudit::report("PBX METHOD POLICY AUDIT",rs);auditOutput_->setPlainText(QString::fromStdString(lastAuditReport_));}catch(const std::exception&e){QMessageBox::warning(this,"PBX methods",e.what());}}
void MainWindow::runAuditAuth(){try{const auto host=auditHost_->text().trimmed().toStdString();const auto user=auditUser_->text().trimmed().toStdString();if(host.empty()||user.empty())throw std::runtime_error("Target and a known test user are required");auto r=PbxAudit::authenticationAudit(host,user,(std::uint16_t)auditPort_->value(),guiAuditTransport(auditTransport_));lastAuditReport_=PbxAudit::report("PBX AUTHENTICATION AUDIT",{r});auditOutput_->setPlainText(QString::fromStdString(lastAuditReport_));}catch(const std::exception&e){QMessageBox::warning(this,"PBX audit",e.what());}}
void MainWindow::runAuditExtensions(){try{const auto host=auditHost_->text().trimmed().toStdString();if(host.empty())throw std::runtime_error("Target is required");auto entries=PbxAudit::extensionAudit(host,(unsigned)auditExtFirst_->value(),(unsigned)auditExtLast_->value(),(std::uint16_t)auditPort_->value(),guiAuditTransport(auditTransport_));lastAuditReport_=PbxAudit::report("PBX EXTENSION DIFFERENTIAL AUDIT",{},entries);auditOutput_->setPlainText(QString::fromStdString(lastAuditReport_));}catch(const std::exception&e){QMessageBox::warning(this,"PBX audit",e.what());}}
void MainWindow::runAuditCompliance(){try{const auto host=auditHost_->text().trimmed().toStdString();if(host.empty())throw std::runtime_error("Target is required");auto rs=PbxAudit::complianceAudit(host,(std::uint16_t)auditPort_->value(),guiAuditTransport(auditTransport_));lastAuditReport_=PbxAudit::report("PBX BOUNDED COMPLIANCE AUDIT",rs);auditOutput_->setPlainText(QString::fromStdString(lastAuditReport_));}catch(const std::exception&e){QMessageBox::warning(this,"PBX audit",e.what());}}
void MainWindow::runAuditParser(){try{const auto host=auditHost_->text().trimmed().toStdString();if(host.empty())throw std::runtime_error("Target is required");auto rs=PbxAudit::parserAbuseAudit(host,(std::uint16_t)auditPort_->value(),guiAuditTransport(auditTransport_));lastAuditReport_=PbxAudit::report("PBX PARSER-ABUSE SIMULATION",rs);auditOutput_->setPlainText(QString::fromStdString(lastAuditReport_));}catch(const std::exception&e){QMessageBox::warning(this,"PBX parser audit",e.what());}}
void MainWindow::runAuditResilience(){try{const auto host=auditHost_->text().trimmed().toStdString();if(host.empty())throw std::runtime_error("Target is required");auto rs=PbxAudit::resilienceAudit(host,(std::uint16_t)auditPort_->value(),guiAuditTransport(auditTransport_));lastAuditReport_=PbxAudit::report("PBX BOUNDED RATE-RESILIENCE SIMULATION",rs);auditOutput_->setPlainText(QString::fromStdString(lastAuditReport_));}catch(const std::exception&e){QMessageBox::warning(this,"PBX resilience audit",e.what());}}
void MainWindow::runAuditScenario(){try{const auto host=auditHost_->text().trimmed().toStdString();if(host.empty())throw std::runtime_error("Target is required");auto user=auditUser_->text().trimmed().toStdString();if(user.empty())user=engine_.profile().username;auto rs=PbxAudit::attackScenarioAudit(host,user,(std::uint16_t)auditPort_->value(),guiAuditTransport(auditTransport_));std::string tls;try{tls=PbxAudit::tlsAudit(host,5061,3500);}catch(const std::exception&e){tls=std::string("TLS probe unavailable: ")+e.what();}lastAuditReport_=PbxAudit::report("REAL-WORLD PBX ATTACK-SCENARIO SIMULATION",rs,{},tls);auditOutput_->setPlainText(QString::fromStdString(lastAuditReport_));}catch(const std::exception&e){QMessageBox::warning(this,"PBX attack scenario",e.what());}}
void MainWindow::runAuditTls(){try{const auto host=auditHost_->text().trimmed().toStdString();if(host.empty())throw std::runtime_error("Target is required");const auto tls=PbxAudit::tlsAudit(host,5061);lastAuditReport_=PbxAudit::report("PBX TLS AUDIT",{}, {},tls);auditOutput_->setPlainText(QString::fromStdString(lastAuditReport_));}catch(const std::exception&e){QMessageBox::warning(this,"PBX TLS audit",e.what());}}
void MainWindow::runAuditTransportParity(){try{const auto host=auditHost_->text().trimmed().toStdString();if(host.empty())throw std::runtime_error("Target is required");auto rs=PbxAudit::transportParityAudit(host,(std::uint16_t)auditPort_->value());lastAuditReport_=PbxAudit::report("PBX UDP/TCP TRANSPORT PARITY",rs);auditOutput_->setPlainText(QString::fromStdString(lastAuditReport_));}catch(const std::exception&e){QMessageBox::warning(this,"PBX transport parity",e.what());}}
void MainWindow::runAuditTopologyExposure(){try{const auto host=auditHost_->text().trimmed().toStdString();if(host.empty())throw std::runtime_error("Target is required");auto r=PbxAudit::topologyExposureAudit(host,(std::uint16_t)auditPort_->value(),guiAuditTransport(auditTransport_));lastAuditReport_=PbxAudit::report("PBX TOPOLOGY / INFORMATION EXPOSURE",{r});auditOutput_->setPlainText(QString::fromStdString(lastAuditReport_));}catch(const std::exception&e){QMessageBox::warning(this,"PBX topology exposure",e.what());}}
void MainWindow::runAuditFull(){try{const auto host=auditHost_->text().trimmed().toStdString();if(host.empty())throw std::runtime_error("Target is required");AutomatedAuditOptions opt;opt.host=host;opt.port=(std::uint16_t)auditPort_->value();opt.transport=guiAuditTransport(auditTransport_);opt.username=auditUser_->text().trimmed().toStdString();if(opt.username.empty())opt.username=engine_.profile().username;auto result=PbxAudit::automatedAudit(opt);lastAuditReport_=result.toText();auditOutput_->setPlainText(QString::fromStdString(lastAuditReport_));if(auditProgress_)auditProgress_->setText(QString("Complete — %1 HIGH, %2 WARN, %3 PASS, %4 INFO").arg(result.highCount).arg(result.warnCount).arg(result.passCount).arg(result.infoCount));}catch(const std::exception&e){QMessageBox::warning(this,"PBX audit",e.what());}}
void MainWindow::saveAuditReport(){if(lastAuditReport_.empty()){QMessageBox::information(this,"PBX audit","Run an audit first.");return;}auto path=QFileDialog::getSaveFileName(this,"Save PBX audit report","sipher-pbx-audit.txt","Text reports (*.txt);;All files (*)");if(path.isEmpty())return;try{PbxAudit::saveReport(path.toStdString(),lastAuditReport_);statusBar()->showMessage("PBX audit report saved",5000);}catch(const std::exception&e){QMessageBox::warning(this,"PBX audit",e.what());}}

void MainWindow::editProfile(){
    try{
        for(const auto& c:engine_.calls()){
            if(!c.disconnected){QMessageBox::warning(this,"SIP Profile","Hang up active calls before changing the SIP profile.");return;}
        }
        multi_.cancelLaunching();
        SipProfile oldProfile=engine_.profile();
        SipProfile edited=ProfileStore::loadDraft(profilePath_);
        if(!editSipProfileDialog(this,edited,QString::fromStdString(profilePath_),false))return;
        ProfileStore::save(edited,profilePath_);
        try{
            engine_.stop();
            engine_.start(ProfileStore::load(profilePath_),50);
            if(dialPrefixEdit_) dialPrefixEdit_->setText(QString::fromStdString(engine_.dialPrefix()));
            statusBar()->showMessage("SIP profile saved and reloaded; dial prefix reset to profile default",5000);
        }catch(...){
            ProfileStore::save(oldProfile,profilePath_);
            try{engine_.stop();engine_.start(oldProfile,50);}catch(...){}
            throw;
        }
    }catch(const pj::Error&e){QMessageBox::critical(this,"SIP profile reload failed",QString::fromStdString(e.info()));}
     catch(const std::exception&e){QMessageBox::critical(this,"SIP profile reload failed",e.what());}
}
void MainWindow::applyTheme(const QString&t){
    const QString themeId=SipherModernStyle::normalizeThemeKey(t);
    const QString settingsPath=QString::fromStdString(trunkmonkey::runtime::settingsPath().string());
    QSettings settings(settingsPath,QSettings::IniFormat);
    settings.setValue(QStringLiteral("ui/theme"),themeId);
    qApp->setStyleSheet(SipherModernStyle::styleSheet(themeId));
}
