#include "linux_backend.h"
#include <QProcess>
#include <QRegularExpression>
#include <cstdlib>

/* kscreen-doctor always emits ANSI colour codes even when not on a TTY. */
static QString stripAnsi(const QString &s) {
    static QRegularExpression ansi("\x1b\\[[0-9;]*m");
    QString r = s;
    return r.remove(ansi);
}

DisplayBackend *DisplayBackend::create() {
    return new LinuxBackend();
}

bool LinuxBackend::hasKscreen() const {
    return system("which kscreen-doctor > /dev/null 2>&1") == 0;
}

bool LinuxBackend::runKscreenDoctor(const std::string &arg) const {
    QProcess proc;
    proc.setProgram("kscreen-doctor");
    proc.setArguments(QStringList() << QString::fromStdString(arg));
    proc.start();
    if (!proc.waitForStarted(3000)) {
        qWarning("kscreen-doctor: could not start (not in PATH?)");
        return false;
    }
    proc.waitForFinished(5000);
    if (proc.exitCode() != 0) {
        qWarning("kscreen-doctor [%s] exit=%d stderr: %s",
                 arg.c_str(), proc.exitCode(),
                 proc.readAllStandardError().constData());
        return false;
    }
    QString out = proc.readAllStandardOutput();
    if (out.contains("failed", Qt::CaseInsensitive) ||
        out.contains("not usable", Qt::CaseInsensitive)) {
        qWarning("kscreen-doctor [%s] output: %s",
                 arg.c_str(), out.toLocal8Bit().constData());
        return false;
    }
    return true;
}

std::vector<MonitorInfo> LinuxBackend::listMonitors() {
    std::vector<MonitorInfo> monitors;

    QProcess proc;
    proc.start("kscreen-doctor", QStringList() << "-o");
    proc.waitForFinished(5000);
    QString out = proc.readAllStandardOutput();

    /* Parse lines like: "Output: 1 DP-2 <uuid>" */
    static QRegularExpression re_output(
        R"(Output:\s+\d+\s+(\S+)\s+\S+\n(?:.*\n)*?\s+Modes:.*\*\s+(\d+x\d+))",
        QRegularExpression::MultilineOption);

    /* Simpler line-by-line parse — strip ANSI codes first */
    QString current_id;
    for (const QString &rawLine : out.split('\n')) {
        const QString line = stripAnsi(rawLine).trimmed();
        static QRegularExpression re_id(R"(^Output:\s+\d+\s+(\S+))");
        static QRegularExpression re_geom(R"(Geometry:\s+\S+\s+(\d+)x(\d+))");

        QRegularExpressionMatch m;
        if ((m = re_id.match(line)).hasMatch()) {
            current_id = m.captured(1);
        } else if (!current_id.isEmpty() && (m = re_geom.match(line)).hasMatch()) {
            MonitorInfo mi;
            mi.id     = current_id.toStdString();
            mi.name   = current_id.toStdString();
            mi.width  = m.captured(1).toInt();
            mi.height = m.captured(2).toInt();
            monitors.push_back(mi);
            current_id.clear();
        }
    }

    /* Fallback: if kscreen-doctor not available, try xrandr */
    if (monitors.empty()) {
        QProcess xproc;
        xproc.start("xrandr", QStringList() << "--listmonitors");
        xproc.waitForFinished(3000);
        QString xout = xproc.readAllStandardOutput();
        static QRegularExpression re_xr(R"(\d+:\s+[+*]?(\S+)\s+\d+/\d+x\d+/\d+\+\d+\+\d+)");
        for (const QString &line : xout.split('\n')) {
            auto m2 = re_xr.match(line);
            if (m2.hasMatch()) {
                MonitorInfo mi;
                mi.id = mi.name = m2.captured(1).toStdString();
                mi.width = 0; mi.height = 0;
                monitors.push_back(mi);
            }
        }
    }

    return monitors;
}

bool LinuxBackend::applyICC(const std::string &monitorId, const std::string &iccPath) {
    /* QProcess passes args directly to the OS — no shell quoting needed or wanted.
     * kscreen-doctor output.DP-2.iccprofile./path/to/file.icc  */
    std::string arg = "output." + monitorId + ".iccprofile." + iccPath;
    bool ok = runKscreenDoctor(arg);
    if (!ok) {
        qWarning("applyICC: kscreen-doctor failed for monitor '%s' path '%s'",
                 monitorId.c_str(), iccPath.c_str());
    }
    return ok;
}

bool LinuxBackend::resetMonitor(const std::string &monitorId) {
    /* Pass empty string to clear the profile. */
    std::string arg = "output." + monitorId + ".iccprofile.";
    return runKscreenDoctor(arg);
}
