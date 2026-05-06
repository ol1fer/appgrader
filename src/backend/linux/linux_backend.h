#pragma once
#include "backend/display_backend.h"

class LinuxBackend : public DisplayBackend {
public:
    std::vector<MonitorInfo> listMonitors() override;
    bool applyICC(const std::string &monitorId, const std::string &iccPath) override;
    bool resetMonitor(const std::string &monitorId) override;

private:
    bool hasKscreen() const;
    bool runKscreenDoctor(const std::string &arg) const;
};
