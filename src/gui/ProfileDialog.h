#pragma once
#include "sipher/Profile.h"
#include <QString>
class QWidget;

bool editSipProfileDialog(QWidget* parent, sipher::SipProfile& profile,
                          const QString& profilePath, bool firstRun=false);
