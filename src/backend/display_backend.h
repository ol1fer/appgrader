#pragma once
#include <string>
#include <vector>

struct MonitorInfo {
    std::string id;    /* e.g. "DP-2" */
    std::string name;  /* human-readable label */
    int width, height;
};

class DisplayBackend {
public:
    virtual ~DisplayBackend() = default;

    virtual std::vector<MonitorInfo> listMonitors() = 0;

    /* Apply an ICC profile file to a monitor by id. Pass empty string to reset. */
    virtual bool applyICC(const std::string &monitorId, const std::string &iccPath) = 0;

    /* Reset a monitor to no colour profile. */
    virtual bool resetMonitor(const std::string &monitorId) = 0;

    static DisplayBackend *create();  /* factory: returns platform implementation */
};
