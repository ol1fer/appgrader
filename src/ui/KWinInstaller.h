#pragma once
#include <QString>

/* Helpers for installing/loading/unloading the appgrader_lut KWin effect.
 * All ops use D-Bus where possible; install copies the bundled .so into
 * the user-local plugin path, so no sudo is needed. */
namespace KWinInstaller {

enum class Status {
    NotSupported,        /* compositor reports the effect type is unsupported */
    NotInstalled,        /* no plugin .so found at any known path */
    InstalledNotLoaded,  /* plugin file present, KWin says not loaded */
    Loaded,              /* plugin loaded and active */
    Unknown              /* D-Bus query failed (KWin not reachable) */
};

const char *statusLabel(Status s);

/* Where the source .so we'd copy from lives. Searches: alongside binary
 * (build dir), <prefix>/lib/appgrader/, /usr/lib/appgrader/. Returns empty
 * if none found. */
QString findSourceLibrary();

/* System plugin path that KWin actually scans: <Qt6 plugin dir>/kwin/effects/plugins/.
 * Probed at runtime; falls back to /usr/lib/qt6/plugins/... if probing fails. */
QString systemPluginPath();

/* True if a .so already lives at systemPluginPath() but its bytes differ
 * from findSourceLibrary() (typical signal that the user needs to update). */
bool systemFileIsStale();

/* Best guess at whether the .so is installed somewhere KWin will scan. */
bool isInstalled();

/* Live D-Bus queries. */
bool isLoaded();
Status currentStatus();

/* Actions. Each returns true on apparent success and writes a human
 * message to *err on failure. */

/* Copies the source .so to the system plugin path via pkexec (polkit).
 * The user will see the system password prompt. Unloads first if loaded. */
bool installToSystem(QString *err);

/* Removes a stale user-local .so to avoid confusion (best-effort). */
void removeUserLocalCopy();

bool unload(QString *err);
bool load(QString *err);

/* Convenience: unload, sleep briefly, load. The brief gap lets KWin
 * actually drop the dlopen handle before we re-load. */
bool reload(QString *err);

}  // namespace KWinInstaller
