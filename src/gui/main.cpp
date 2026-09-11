#include "MainWindow.h"
#include "ProfileDialog.h"
#include "trunkmonkey/Logger.h"
#include "trunkmonkey/MultiCallManager.h"
#include "trunkmonkey/Profile.h"
#include "trunkmonkey/RuntimePaths.h"
#include "trunkmonkey/SipEngine.h"
#include "trunkmonkey/Version.h"
#include <QApplication>
#include <QColor>
#include <QFont>
#include <QFontDatabase>
#include <QIcon>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QPen>
#include <filesystem>
using namespace trunkmonkey;
static QIcon makeSipherIcon(){
    QPixmap pix(64,64);pix.fill(Qt::transparent);
    QPainter p(&pix);p.setRenderHint(QPainter::Antialiasing,false);
    p.fillRect(QRect(2,2,60,60),QColor(10,12,10));
    QPen border(QColor(110,255,150));border.setWidth(2);p.setPen(border);p.drawRect(3,3,58,58);
    QFont f=QFontDatabase::systemFont(QFontDatabase::FixedFont);f.setBold(true);f.setPixelSize(34);p.setFont(f);
    p.setPen(QColor(125,255,160));p.drawText(QRect(4,3,56,42),Qt::AlignCenter,QStringLiteral("S"));
    QFont small=QFontDatabase::systemFont(QFontDatabase::FixedFont);small.setBold(true);small.setPixelSize(9);p.setFont(small);
    p.drawText(QRect(4,42,56,16),Qt::AlignCenter,QStringLiteral("IPHER"));
    p.setPen(QPen(QColor(75,170,95),1));for(int y=8;y<60;y+=6)p.drawLine(7,y,57,y);
    p.end();return QIcon(pix);
}
int sipherRunGui(int argc,char**argv){
    runtime::configurePortableEnvironment();
    QApplication app(argc,argv);app.setApplicationName("SIPHER 2.0");app.setApplicationVersion(TRUNKMONKEY_VERSION);
    app.setWindowIcon(makeSipherIcon());
    try{runtime::ensureUserDirectories();}catch(const std::exception&error){QMessageBox::critical(nullptr,"SIPHER",QString::fromStdString(error.what()));return 2;}
    const std::filesystem::path executable=argc>0?std::filesystem::path(argv[0]):std::filesystem::path{};
    const std::filesystem::path profilePath=argc>=2?std::filesystem::path(argv[1]):runtime::defaultProfilePath(executable);
    Logger log(runtime::logPath().string());log.setConsoleEnabled(false);SipEngine engine(log);MultiCallManager multi(engine,log);
    try{
        engine.start(50);
        const auto accountsDir=profilePath.parent_path()/"accounts";
        std::filesystem::create_directories(accountsDir);

        // One-time compatibility migration: if an older single-profile install
        // has a configured profile and the new account directory is empty,
        // copy it into r19's multi-account store. The legacy file is left alone.
        bool haveAccountFiles=false;
        for(const auto& entry:std::filesystem::directory_iterator(accountsDir)){
            if(entry.is_regular_file() && entry.path().extension()==".conf"){haveAccountFiles=true;break;}
        }
        if(!haveAccountFiles && std::filesystem::exists(profilePath)){
            try{
                const auto legacy=ProfileStore::loadDraft(profilePath.string());
                if(ProfileStore::isConfigured(legacy)){
                    ProfileStore::save(ProfileStore::load(profilePath.string()),(accountsDir/"legacy.conf").string());
                    log.info("Migrated legacy SIP profile into r19 multi-account store as legacy.conf");
                }
            }catch(const std::exception& e){log.warn(std::string("Legacy SIP profile migration skipped: ")+e.what());}
        }

        for(const auto& entry:std::filesystem::directory_iterator(accountsDir)){
            if(!entry.is_regular_file() || entry.path().extension()!=".conf")continue;
            try{
                const auto draft=ProfileStore::loadDraft(entry.path().string());
                if(!ProfileStore::isConfigured(draft)){log.warn("Skipping unconfigured SIP account file: "+entry.path().string());continue;}
                engine.addAccount(ProfileStore::load(entry.path().string()),entry.path().stem().string());
            }catch(const pj::Error& e){log.warn("Unable to start SIP account "+entry.path().filename().string()+": "+e.info());}
            catch(const std::exception& e){log.warn("Unable to load SIP account "+entry.path().filename().string()+": "+e.what());}
        }
    }catch(const pj::Error&error){QMessageBox::critical(nullptr,"SIP startup failed",QString::fromStdString(error.info()));return 1;}catch(const std::exception&error){QMessageBox::critical(nullptr,"Startup failed",error.what());return 1;}
    MainWindow window(engine,multi,log,profilePath.string());window.show();const int rc=app.exec();multi.cancelLaunching();engine.stop();return rc;
}

#ifndef SIPHER_UNIFIED_ENTRY
int main(int argc, char** argv)
{
    return sipherRunGui(argc, argv);
}
#endif
