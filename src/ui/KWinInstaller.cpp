#include "KWinInstaller.h"
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>
#include <QThread>
#ifdef Q_OS_LINUX
#include <QDBusInterface>
#include <QDBusReply>
#endif

namespace KWinInstaller {

static const char *EFFECT_NAME = "appgrader_lut";

const char *statusLabel(Status s) {
    switch (s) {
        case Status::NotSupported:       return "Not supported by compositor";
        case Status::NotInstalled:       return "Not installed";
        case Status::InstalledNotLoaded: return "Installed (not loaded)";
        case Status::Loaded:             return "Loaded";
        case Status::Unknown:            return "Unknown (KWin not reachable)";
    }
    return "?";
}

/* Probe `qmake6 -query QT_INSTALL_PLUGINS` (then qmake) for the active
 * Qt6 plugin directory. Falls back to /usr/lib/qt6/plugins. */
static QString qtPluginDir() {
    for (const char *bin : {"qmake6", "qmake"}) {
        QProcess p;
        p.start(bin, {"-query", "QT_INSTALL_PLUGINS"});
        if (p.waitForStarted(500) && p.waitForFinished(1500)) {
            QString out = QString::fromUtf8(p.readAllStandardOutput()).trimmed();
            if (!out.isEmpty()) return out;
        }
    }
    return QStringLiteral("/usr/lib/qt6/plugins");
}

QString systemPluginPath() {
    return qtPluginDir() + "/kwin/effects/plugins/appgrader_lut.so";
}

QString findSourceLibrary() {
    const QString appDir = QCoreApplication::applicationDirPath();
    /* Build dir layout: <build>/appgrader and <build>/src/kwin_effect/appgrader_lut.so */
    QStringList candidates {
        appDir + "/src/kwin_effect/appgrader_lut.so",
        appDir + "/../src/kwin_effect/appgrader_lut.so",
        /* Installed alongside data: ${prefix}/lib/appgrader/ */
        appDir + "/../lib/appgrader/appgrader_lut.so",
        appDir + "/../lib64/appgrader/appgrader_lut.so",
        "/usr/lib/appgrader/appgrader_lut.so",
        "/usr/lib64/appgrader/appgrader_lut.so",
        "/usr/local/lib/appgrader/appgrader_lut.so",
    };
    for (const QString &p : candidates) {
        QFileInfo fi(p);
        if (fi.exists() && fi.isFile())
            return fi.absoluteFilePath();
    }
    return QString();
}

bool isInstalled() {
    return QFile::exists(systemPluginPath());
}

static QByteArray hashFile(const QString &path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return QByteArray();
    QCryptographicHash h(QCryptographicHash::Sha1);
    h.addData(&f);
    return h.result();
}

bool systemFileIsStale() {
    QString src = findSourceLibrary();
    QString sys = systemPluginPath();
    if (src.isEmpty() || !QFile::exists(sys)) return false;
    return hashFile(src) != hashFile(sys);
}

#ifdef Q_OS_LINUX
static QDBusInterface kwinIface() {
    return QDBusInterface("org.kde.KWin", "/Effects", "org.kde.kwin.Effects",
                          QDBusConnection::sessionBus());
}
#endif

bool isLoaded() {
#ifdef Q_OS_LINUX
    auto iface = kwinIface();
    if (!iface.isValid()) return false;
    QDBusReply<bool> r = iface.call("isEffectLoaded", EFFECT_NAME);
    return r.isValid() && r.value();
#else
    return false;
#endif
}

Status currentStatus() {
#ifdef Q_OS_LINUX
    auto iface = kwinIface();
    if (!iface.isValid()) return Status::Unknown;

    QDBusReply<bool> sup = iface.call("isEffectSupported", EFFECT_NAME);
    if (sup.isValid() && !sup.value()) return Status::NotSupported;

    QDBusReply<bool> loaded = iface.call("isEffectLoaded", EFFECT_NAME);
    if (loaded.isValid() && loaded.value()) return Status::Loaded;

    if (!isInstalled()) return Status::NotInstalled;
    return Status::InstalledNotLoaded;
#else
    return Status::Unknown;
#endif
}

void removeUserLocalCopy() {
    /* Earlier versions of the installer dropped the .so here. KWin doesn't
     * actually scan this path, so it just causes confusion. */
    const QString p = QDir::homePath()
        + "/.local/lib/qt6/plugins/kwin/effects/plugins/appgrader_lut.so";
    if (QFile::exists(p)) QFile::remove(p);
}

bool installToSystem(QString *err) {
    const QString src = findSourceLibrary();
    if (src.isEmpty()) {
        if (err) *err = "Could not locate appgrader_lut.so on disk. "
                        "Build the project or install the appgrader package first.";
        return false;
    }
    const QString dst = systemPluginPath();

    /* Don't replace a file we'd be reading from. */
    if (QFileInfo(src) == QFileInfo(dst)) {
        if (err) *err = QString();
        return true;
    }

    /* Unload first — KWin holds a dlopen handle on the loaded plugin and
     * replacing the file under it is the most likely crash trigger. */
    if (isLoaded()) {
        QString uerr;
        unload(&uerr);
        QThread::msleep(800);
    }

    /* `install` (from coreutils) is more robust than `cp` for replacing a
     * loaded library: it unlinks the existing inode then creates a fresh
     * one, which avoids overwriting a file the kernel still has mapped. */
    QStringList args { "install", "-Dm755", src, dst };
    int rc = QProcess::execute("pkexec", args);
    if (rc != 0) {
        if (err) {
            if (rc == 126 || rc == 127)
                *err = "Authorisation cancelled or pkexec unavailable.";
            else
                *err = QString("pkexec install failed (exit %1).").arg(rc);
        }
        return false;
    }

    removeUserLocalCopy();
    if (err) *err = QString();
    return true;
}

bool load(QString *err) {
#ifdef Q_OS_LINUX
    auto iface = kwinIface();
    if (!iface.isValid()) {
        if (err) *err = "Could not reach KWin on D-Bus.";
        return false;
    }
    QDBusReply<bool> r = iface.call("loadEffect", EFFECT_NAME);
    if (!r.isValid()) {
        if (err) *err = "loadEffect failed: " + r.error().message();
        return false;
    }
    if (err) *err = QString();
    return r.value();
#else
    if (err) *err = "Not supported on this platform.";
    return false;
#endif
}

bool unload(QString *err) {
#ifdef Q_OS_LINUX
    auto iface = kwinIface();
    if (!iface.isValid()) {
        if (err) *err = "Could not reach KWin on D-Bus.";
        return false;
    }
    QDBusReply<void> r = iface.call("unloadEffect", EFFECT_NAME);
    if (!r.isValid()) {
        if (err) *err = "unloadEffect failed: " + r.error().message();
        return false;
    }
    if (err) *err = QString();
    return true;
#else
    if (err) *err = "Not supported on this platform.";
    return false;
#endif
}

bool reload(QString *err) {
    QString uerr;
    unload(&uerr);
    QThread::msleep(800);
    return load(err);
}

}  // namespace KWinInstaller
