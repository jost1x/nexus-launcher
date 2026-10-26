#include "rpc/rpc_client.h"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <pwd.h>
#include <unistd.h>

namespace nexus {
namespace {

using namespace std::chrono_literals;

}  // namespace

std::string default_socket_path() {
    if (const char* configured = std::getenv("NEXUSD_SOCKET"); configured != nullptr && *configured != '\0') {
        return configured;
    }
    if (const char* cache = std::getenv("XDG_CACHE_HOME"); cache != nullptr && *cache != '\0' &&
        std::filesystem::path(cache).is_absolute()) {
        return (std::filesystem::path(cache) / "nexusd" / "nexusd.sock").string();
    }
    if (const char* home = std::getenv("HOME"); home != nullptr && *home != '\0') {
        return (std::filesystem::path(home) / ".cache" / "nexusd" / "nexusd.sock").string();
    }
    if (const auto* user = getpwuid(getuid()); user != nullptr && user->pw_dir != nullptr && *user->pw_dir != '\0') {
        return (std::filesystem::path(user->pw_dir) / ".cache" / "nexusd" / "nexusd.sock").string();
    }
    return {};
}

RpcClient::RpcClient(std::string socket_path)
    : socket_path_(std::move(socket_path)),
      channel_(grpc::CreateChannel("unix:" + socket_path_, grpc::InsecureChannelCredentials())),
      wifi_stub_(nexus::v1::WifiService::NewStub(channel_)),
      hardware_stub_(nexus::v1::HardwareService::NewStub(channel_)),
      system_stub_(nexus::v1::SystemService::NewStub(channel_)),
      worker_(&RpcClient::run, this) {}

RpcClient::~RpcClient() {
    {
        std::lock_guard lock(mutex_);
        stopping_ = true;
        if (active_context_) active_context_->TryCancel();
        watch_cancelled_ = true;
        if (watch_context_) watch_context_->TryCancel();
        wifi_watch_cancelled_ = true;
        if (wifi_watch_context_) wifi_watch_context_->TryCancel();
        for (auto& task : queue_) task.context->TryCancel();
    }
    ready_.notify_all();
    if (worker_.joinable()) worker_.join();
    if (watch_thread_.joinable()) watch_thread_.join();
    if (wifi_watch_thread_.joinable()) wifi_watch_thread_.join();
}

uint64_t RpcClient::request_wifi_status() { return submit(RpcOperation::WifiStatus); }
uint64_t RpcClient::watch_wifi_status() {
    std::lock_guard lock(mutex_);
    if (stopping_) return 0;
    if (wifi_watch_id_ != 0) return wifi_watch_id_;
    wifi_watch_id_ = next_id_++;
    wifi_watch_thread_ = std::thread(&RpcClient::run_wifi_watch, this, wifi_watch_id_);
    return wifi_watch_id_;
}
uint64_t RpcClient::scan_wifi(bool refresh) { return submit(RpcOperation::WifiScan, 0, refresh); }
uint64_t RpcClient::request_hardware_status() { return submit(RpcOperation::HardwareStatus); }
uint64_t RpcClient::watch_hardware_status() {
    std::lock_guard lock(mutex_);
    if (stopping_) return 0;
    if (watch_id_ != 0) return watch_id_;
    watch_id_ = next_id_++;
    watch_thread_ = std::thread(&RpcClient::run_hardware_watch, this, watch_id_);
    return watch_id_;
}
uint64_t RpcClient::request_system_info() { return submit(RpcOperation::SystemInfo); }
uint64_t RpcClient::set_brightness(int percent) { return submit(RpcOperation::SetBrightness, percent); }
uint64_t RpcClient::set_volume(int percent) { return submit(RpcOperation::SetVolume, percent); }

uint64_t RpcClient::submit(RpcOperation operation, int percent, bool refresh) {
    auto context = std::make_shared<grpc::ClientContext>();
    const auto duration = operation == RpcOperation::WifiScan ? 20s : 6s;
    context->set_deadline(std::chrono::system_clock::now() + duration);
    std::lock_guard lock(mutex_);
    if (stopping_) return 0;
    const auto id = next_id_++;
    queue_.push_back(Task{id, operation, percent, refresh, std::move(context)});
    ready_.notify_all();
    return id;
}

void RpcClient::cancel(uint64_t id) {
    std::lock_guard lock(mutex_);
    if (watch_id_ == id) {
        watch_cancelled_ = true;
        if (watch_context_) watch_context_->TryCancel();
        ready_.notify_all();
    }
    if (wifi_watch_id_ == id) {
        wifi_watch_cancelled_ = true;
        if (wifi_watch_context_) wifi_watch_context_->TryCancel();
        ready_.notify_all();
    }
    if (active_id_ == id && active_context_) active_context_->TryCancel();
    for (auto& task : queue_) {
        if (task.id == id) task.context->TryCancel();
    }
}

std::vector<RpcResult> RpcClient::take_results() {
    std::lock_guard lock(mutex_);
    std::vector<RpcResult> results;
    results.swap(completed_);
    return results;
}

void RpcClient::run() {
    while (true) {
        Task task{};
        {
            std::unique_lock lock(mutex_);
            ready_.wait(lock, [this] { return stopping_ || !queue_.empty(); });
            if (stopping_ && queue_.empty()) return;
            task = std::move(queue_.front());
            queue_.pop_front();
            active_context_ = task.context;
            active_id_ = task.id;
        }
        auto result = invoke(task);
        {
            std::lock_guard lock(mutex_);
            completed_.push_back(std::move(result));
            active_context_.reset();
            active_id_ = 0;
        }
    }
}

}  // namespace nexus
