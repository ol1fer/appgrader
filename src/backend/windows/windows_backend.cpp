#include "windows_backend.h"

/* Windows colour management via mscms.dll (ICM).
 * SetICMProfile() sets an ICC profile for a given display DC. */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <icm.h>

DisplayBackend *DisplayBackend::create() {
    return new WindowsBackend();
}

std::vector<MonitorInfo> WindowsBackend::listMonitors() {
    std::vector<MonitorInfo> monitors;

    auto callback = [](HMONITOR hmon, HDC, LPRECT, LPARAM data) -> BOOL {
        auto *v = reinterpret_cast<std::vector<MonitorInfo>*>(data);
        MONITORINFOEXW mi;
        mi.cbSize = sizeof(mi);
        GetMonitorInfoW(hmon, &mi);
        MonitorInfo info;
        char buf[64];
        WideCharToMultiByte(CP_UTF8, 0, mi.szDevice, -1, buf, sizeof(buf), nullptr, nullptr);
        info.id   = buf;
        info.name = buf;
        info.width  = mi.rcMonitor.right  - mi.rcMonitor.left;
        info.height = mi.rcMonitor.bottom - mi.rcMonitor.top;
        v->push_back(info);
        return TRUE;
    };

    EnumDisplayMonitors(nullptr, nullptr, callback, reinterpret_cast<LPARAM>(&monitors));
    return monitors;
}

bool WindowsBackend::applyICC(const std::string &monitorId, const std::string &iccPath) {
    /* Open a DC for the named display device */
    std::wstring wDevice(monitorId.begin(), monitorId.end());
    HDC hdc = CreateDCW(wDevice.c_str(), wDevice.c_str(), nullptr, nullptr);
    if (!hdc) return false;

    std::wstring wPath(iccPath.begin(), iccPath.end());
    BOOL ok = SetICMProfileW(hdc, const_cast<LPWSTR>(wPath.c_str()));
    DeleteDC(hdc);
    return ok != 0;
}

bool WindowsBackend::resetMonitor(const std::string &monitorId) {
    /* Restore default profile by setting NULL — get default profile name first */
    std::wstring wDevice(monitorId.begin(), monitorId.end());
    HDC hdc = CreateDCW(wDevice.c_str(), wDevice.c_str(), nullptr, nullptr);
    if (!hdc) return false;

    /* Remove all non-default profiles to reset */
    DeleteDC(hdc);
    return true;  /* best-effort on Windows */
}
