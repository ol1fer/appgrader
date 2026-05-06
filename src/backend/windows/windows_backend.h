#pragma once
#include "backend/display_backend.h"

class WindowsBackend : public DisplayBackend {
public:
    std::vector<MonitorInfo> listMonitors() override;
    bool applyICC(const std::string &monitorId, const std::string &iccPath) override;
    bool resetMonitor(const std::string &monitorId) override;
};
