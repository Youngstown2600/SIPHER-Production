#pragma once
#include <QString>

namespace SipherModernStyle {
struct Palette {
    QString background, sidebar, surface, raised, text, muted, accent, accentAlt, border, success, danger, warning;
    bool light{false};
};
QString normalizeThemeKey(const QString& key);
Palette paletteFor(const QString& key);
QString styleSheet(const QString& key);
}
