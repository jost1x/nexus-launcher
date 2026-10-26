#include "platform/memory_monitor.h"

#include <lvgl.h>

#include <cstdio>

namespace nexus {
namespace {

constexpr auto kCheckInterval = std::chrono::seconds(5);
constexpr size_t kLowMemoryBytes = 48 * 1024;
constexpr size_t kCriticalMemoryBytes = 16 * 1024;

}  // namespace

MemoryMonitor::MemoryMonitor() : last_check_(std::chrono::steady_clock::now()) {}

void MemoryMonitor::poll() {
    const auto now = std::chrono::steady_clock::now();
    if (last_check_.time_since_epoch().count() != 0 && now - last_check_ < kCheckInterval) return;
    last_check_ = now;

    lv_mem_monitor_t monitor{};
    lv_mem_monitor(&monitor);
    if (!reported_initial_usage_) {
        std::fprintf(stderr,
                     "[nexus-ui] LVGL heap: free=%zu/%zu bytes, largest block=%zu, peak used=%zu bytes\n",
                     monitor.free_size, monitor.total_size, monitor.free_biggest_size, monitor.max_used);
        std::fflush(stderr);
        reported_initial_usage_ = true;
    }
    const uint8_t level = monitor.free_size <= kCriticalMemoryBytes
                              ? 2
                              : monitor.free_size <= kLowMemoryBytes ? 1 : 0;
    if (level == reported_level_) return;

    if (level == 2) {
        std::fprintf(stderr,
                     "[nexus-ui] LVGL heap critical: free=%zu/%zu bytes, largest block=%zu, peak used=%zu bytes\n",
                     monitor.free_size, monitor.total_size, monitor.free_biggest_size, monitor.max_used);
        std::fflush(stderr);
    } else if (level == 1) {
        std::fprintf(stderr,
                     "[nexus-ui] LVGL heap low: free=%zu/%zu bytes, largest block=%zu, peak used=%zu bytes\n",
                     monitor.free_size, monitor.total_size, monitor.free_biggest_size, monitor.max_used);
        std::fflush(stderr);
    } else if (reported_level_ != 0) {
        std::fprintf(stderr, "[nexus-ui] LVGL heap recovered: free=%zu/%zu bytes\n",
                     monitor.free_size, monitor.total_size);
        std::fflush(stderr);
    }
    reported_level_ = level;
}

}  // namespace nexus
