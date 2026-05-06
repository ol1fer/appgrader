#pragma once
#include <QString>
#include <QDir>

class AppPaths {
public:
    static QString configDir();
    static QString presetsDir();
    static QString lutsDir();
    static QString activeIccPath(int slot = 0);   /* slot 0 or 1, alternated each apply */
    static QString activeLutPath();               /* active.lut read by KWin effect */
    static QString activeTargetPath();            /* active_target: "output:<name>" or "window:<uuid>" */
    static QString windowsListPath();             /* windows.list: written by effect, read by app */
    static QString spatialParamsPath();           /* spatial.params: vignette/noise/sharpen/clarity */

    static void ensureDirsExist();
};
