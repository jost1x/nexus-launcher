#pragma once

#include <chrono>
#include <cstdint>

namespace nexus {

class MemoryMonitor {
public:
    MemoryMonitor();
    void poll();

private:
    std::chrono::steady_clock::time_point last_check_{};
    uint8_t reported_level_ = 0;
    bool reported_initial_usage_ = false;
};

}  // namespace nexus
