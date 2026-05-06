#include "settings.h"
#include <QStandardPaths>

QString AppPaths::configDir() {
#ifdef Q_OS_WIN
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
           + "/AppGrader";
#else
    return QDir::homePath() + "/.config/appgrader";
#endif
}

QString AppPaths::presetsDir() {
    return configDir() + "/presets";
}

QString AppPaths::lutsDir() {
    return configDir() + "/luts";
}

QString AppPaths::activeIccPath(int slot) {
    return configDir() + (slot == 0 ? "/active0.icc" : "/active1.icc");
}

QString AppPaths::activeLutPath() {
    return configDir() + "/active.lut";
}

QString AppPaths::activeTargetPath() {
    return configDir() + "/active_target";
}

QString AppPaths::windowsListPath() {
    return configDir() + "/windows.list";
}

QString AppPaths::spatialParamsPath() {
    return configDir() + "/spatial.params";
}

void AppPaths::ensureDirsExist() {
    QDir().mkpath(configDir());
    QDir().mkpath(presetsDir());
    QDir().mkpath(lutsDir());
}
