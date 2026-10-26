#include "rpc/rpc_client.h"

#include <algorithm>
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
uint64_t RpcClient::list_saved_wifi_networks() { return submit_wifi(RpcOperation::WifiSavedNetworks); }
uint64_t RpcClient::set_wifi_enabled(bool enabled) {
    return submit_wifi(RpcOperation::WifiSetEnabled, {}, {}, {}, enabled);
}
uint64_t RpcClient::connect_wifi(std::string ssid, std::string password, std::string security) {
    return submit_wifi(RpcOperation::WifiConnect, std::move(ssid), std::move(password), std::move(security));
}
uint64_t RpcClient::connect_saved_wifi_network(std::string id) {
    return submit_wifi(RpcOperation::WifiConnectSaved, std::move(id));
}
uint64_t RpcClient::set_preferred_wifi_network(std::string id) {
    return submit_wifi(RpcOperation::WifiSetPreferred, std::move(id));
}
uint64_t RpcClient::forget_wifi_network(std::string id) {
    return submit_wifi(RpcOperation::WifiForget, std::move(id));
}
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
    Task task{};
    task.id = id;
    task.operation = operation;
    task.percent = percent;
    task.refresh = refresh;
    task.context = std::move(context);
    queue_.push_back(std::move(task));
    ready_.notify_all();
    return id;
}

uint64_t RpcClient::submit_wifi(RpcOperation operation, std::string text, std::string secret,
                               std::string security, bool enabled) {
    auto context = std::make_shared<grpc::ClientContext>();
    context->set_deadline(std::chrono::system_clock::now() + 15s);
    std::lock_guard lock(mutex_);
    if (stopping_) return 0;
    Task task{};
    task.id = next_id_++;
    task.operation = operation;
    task.text = std::move(text);
    task.secret = std::move(secret);
    task.security = std::move(security);
    task.enabled = enabled;
    task.context = std::move(context);
    const auto id = task.id;
    queue_.push_back(std::move(task));
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
        std::fill(task.secret.begin(), task.secret.end(), '\0');
        task.secret.clear();
        {
            std::lock_guard lock(mutex_);
            completed_.push_back(std::move(result));
            active_context_.reset();
            active_id_ = 0;
        }
    }
}

}  // namespace nexus
