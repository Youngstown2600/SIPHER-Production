#include "ModernStyle.h"

namespace SipherModernStyle {
QString normalizeThemeKey(const QString& key){
    QString k=key.trimmed().toCaseFolded();k.replace('_','-');
    if(k.isEmpty()||k=="classic"||k=="classic-light")k="system";
    if(k=="hacker"||k=="matrix"||k=="crt-green")k="night-vision";
    else if(k=="ice")k="arctic";
    else if(k=="nord"||k=="stealth")k="slate";
    else if(k=="retro-blue")k="blue-box";
    else if(k=="beige-box"||k=="copper")k="terminal-gold";
    else if(k=="vaporwave"||k=="synthwave"||k=="ultraviolet")k="cyberpunk";
    else if(k=="signal-red")k="red-box";
    else if(k=="deep-space"||k=="dracula")k="black-ice";
    return k;
}

Palette paletteFor(const QString& raw){
    const QString k=normalizeThemeKey(raw);
    // The first ten palettes are intentionally identical to SIPHER r22.
    Palette p{"#0d1117","#161b22","#21262d","#2a3038","#e6edf3","#8b949e","#58a6ff","#79c0ff","#30363d","#3fb950","#f85149","#d29922",false};
    if(k=="midnight")p={"#07111f","#0c2137","#112d49","#173859","#e5efff","#8298b8","#5aa7ff","#7bd6ff","#234b74","#68d391","#ff7182","#f6c453",false};
    else if(k=="slate")p={"#101214","#1a1d20","#252a2f","#30363c","#d9dde2","#929aa3","#7fa7c9","#a8c2d8","#3f4851","#71b58a","#d66b73","#c4a45f",false};
    else if(k=="ocean")p={"#041019","#08202e","#0c2a3c","#10384d","#d8f2ff","#77a8bf","#45c8ff","#8ee6ff","#194e68","#59e6b5","#ff7182","#ffd166",false};
    else if(k=="arctic")p={"#071219","#122833","#183844","#204a58","#effcff","#91b6c2","#80e4ff","#d4fbff","#326171","#63dfbd","#ff7284","#ffe082",false};
    else if(k=="solarized")p={"#002b36","#073642","#104752","#155663","#93a1a1","#73878b","#2aa198","#268bd2","#355d64","#859900","#dc322f","#b58900",false};
    else if(k=="monochrome")p={"#0b0b0b","#171717","#222222","#2d2d2d","#ededed","#9b9b9b","#d0d0d0","#ffffff","#3a3a3a","#c8f7c5","#ff7b7b","#ddddaa",false};
    else if(k=="cobalt")p={"#07152b","#0d2446","#173d75","#1e4c8e","#e5efff","#8aa4c5","#668fd0","#8cc7ff","#456fb5","#6ed59b","#ff7182","#e9bd66",false};
    else if(k=="amber")p={"#120c03","#211606","#2b1d08","#38260b","#ffe0a0","#b78c4d","#ffb52f","#ffd274","#6d4b18","#a8d45b","#ff6f62","#ffd274",false};
    else if(k=="high-contrast")p={"#000000","#101010","#191919","#222222","#ffffff","#c9c9c9","#00d9ff","#ffffff","#5e5e5e","#32ff7e","#ff405c","#ffe600",false};
    // SIPHER-exclusive phone-phreak / underground palettes.
    else if(k=="black-ice")p={"#010509","#031019","#061a24","#092633","#dff8ff","#78a8b8","#16d9ff","#8af1ff","#16485a","#55e6ae","#ff6675","#ffd166",false};
    else if(k=="night-vision")p={"#010501","#030a03","#061206","#0a1d0a","#caffca","#69a969","#4dff4d","#a2ff7a","#1d5c1d","#7cff63","#ff5d69","#e5dc62",false};
    else if(k=="blue-box")p={"#020914","#041326","#071f38","#0b2d4f","#d8f2ff","#71a4bf","#28b8ff","#74ddff","#14557b","#55e6ae","#ff6675","#ffd166",false};
    else if(k=="red-box")p={"#0d0203","#160406","#220609","#300a0e","#ffe7e7","#b89191","#ff344f","#ff875f","#68151e","#6ee7a8","#ff183b","#ffd36a",false};
    else if(k=="2600")p={"#020402","#041006","#07180a","#0b2410","#d7ffd7","#70a879","#39ff14","#49f5ff","#17651e","#60ff88","#ff4d77","#fff000",false};
    else if(k=="wargames")p={"#010401","#031003","#061806","#092409","#a8ffa8","#679b67","#33ff33","#9bff9b","#155f1b","#65ff65","#ff5c5c","#e6d450",false};
    else if(k=="phosphor")p={"#061006","#081708","#0b1d0b","#102710","#b9ffb9","#68a568","#68ff68","#a1ffa1","#245c24","#68ff68","#ff7474","#d9d05d",false};
    else if(k=="cyberpunk")p={"#090614","#10091d","#160d29","#21143a","#f4efff","#a48ab7","#25e6ff","#ff4ed8","#4a2c66","#61ffa6","#ff557f","#fff06a",false};
    else if(k=="blood-moon")p={"#100205","#190408","#22060b","#310a12","#ffe4e6","#b98a91","#ff4d68","#ff8a9c","#6f1927","#68d391","#ff334f","#ffb454",false};
    else if(k=="terminal-gold")p={"#100d03","#181305","#221b07","#30270b","#fff3b0","#b6a55d","#ffd23f","#ffe98a","#65551c","#b6dc69","#ff735f","#ffe279",false};
    return p;
}

QString styleSheet(const QString& key){
    const Palette p=paletteFor(key);
    return QStringLiteral(R"QSS(
*{outline:none;} QWidget{color:%5;font-family:"DejaVu Sans Mono","Liberation Mono",monospace;font-size:10pt;} QMainWindow,QDialog,QWidget#PhreakRoot{background:%1;}
QToolTip{background:%4;color:%5;border:1px solid %7;padding:5px;}
QFrame#PhreakRail{background:%2;border-right:2px solid %7;} QFrame#WireHeader{background:%2;border:1px solid %9;border-left:3px solid %7;}
QLabel#BrandTitle{font-size:17pt;font-weight:900;letter-spacing:2px;} QLabel#BrandVersion{color:%8;font-weight:800;letter-spacing:1px;} QLabel#Muted,QLabel#PageSubtitle{color:%6;} QLabel#PageTitle{font-size:18pt;font-weight:900;color:%8;letter-spacing:1px;}
QLabel#StatusPill{background:%1;color:%10;border:1px solid %10;padding:5px 10px;font-weight:900;} QLabel#WarningPill{background:%1;color:%12;border:1px solid %12;padding:5px 10px;font-weight:900;}
QPushButton{background:%3;color:%5;border:1px solid %9;padding:7px 10px;min-height:18px;} QPushButton:hover{border-color:%7;color:%8;background:%4;} QPushButton:pressed{background:%1;} QPushButton:disabled{color:%6;}
QPushButton[role="primary"]{background:%1;color:%7;border:1px solid %7;font-weight:900;} QPushButton[role="primary"]:hover{background:%7;color:%1;} QPushButton[role="danger"]{color:%11;border-color:%11;}
QPushButton[nav="true"]{text-align:left;background:%1;border:1px solid transparent;border-left:2px solid %9;padding:9px 10px;font-weight:700;} QPushButton[nav="true"]:hover{border-left-color:%7;color:%8;background:%3;} QPushButton[nav="true"]:checked{background:%4;border:1px solid %7;border-left:4px solid %7;color:%8;}
QLineEdit,QPlainTextEdit,QTextEdit,QListWidget,QTreeWidget,QTableWidget,QComboBox,QSpinBox{background:%1;color:%5;border:1px solid %9;padding:6px 8px;selection-background-color:%7;selection-color:%1;} QLineEdit:focus,QPlainTextEdit:focus,QTextEdit:focus,QTableWidget:focus,QComboBox:focus,QSpinBox:focus{border-color:%7;}
QHeaderView::section{background:%2;color:%8;border:none;border-right:1px solid %9;border-bottom:1px solid %7;padding:7px;font-weight:800;} QTableWidget{gridline-color:%9;}
QGroupBox{background:%2;border:1px solid %9;border-left:2px solid %7;margin-top:12px;padding-top:10px;font-weight:800;} QGroupBox::title{subcontrol-origin:margin;left:10px;padding:0 5px;color:%8;}
QTabWidget::pane{background:%2;border:1px solid %9;} QTabBar::tab{background:%1;color:%6;border:1px solid %9;padding:8px 12px;} QTabBar::tab:selected{color:%8;border-color:%7;background:%3;font-weight:800;}
QMenuBar,QStatusBar{background:%1;color:%6;border-color:%9;} QMenu{background:%2;color:%5;border:1px solid %7;} QMenu::item:selected{background:%7;color:%1;}
QScrollBar:vertical{background:%1;width:9px;} QScrollBar:horizontal{background:%1;height:9px;} QScrollBar::handle{background:%9;min-height:25px;min-width:25px;} QScrollBar::handle:hover{background:%7;} QScrollBar::add-line,QScrollBar::sub-line{width:0;height:0;}
)QSS")
      .arg(p.background).arg(p.sidebar).arg(p.surface).arg(p.raised).arg(p.text).arg(p.muted)
      .arg(p.accent).arg(p.accentAlt).arg(p.border).arg(p.success).arg(p.danger).arg(p.warning);
}
}
