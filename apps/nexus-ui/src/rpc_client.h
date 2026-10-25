#pragma once

#include <grpcpp/client_context.h>
#include <grpcpp/grpcpp.h>

#include <condition_variable>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <variant>
#include <vector>

#include "nexus/v1/nexus.grpc.pb.h"

namespace nexus {

enum class RpcOperation { WifiStatus, WifiScan, HardwareStatus, SystemInfo, SetBrightness, SetVolume };

using RpcPayload = std::variant<nexus::v1::WifiServiceGetStatusResponse,
                                nexus::v1::ScanResponse,
                                nexus::v1::HardwareServiceGetStatusResponse,
                                nexus::v1::GetInfoResponse,
                                nexus::v1::SetBrightnessResponse,
                                nexus::v1::SetVolumeResponse>;

struct RpcResult {
    uint64_t id = 0;
    RpcOperation operation = RpcOperation::WifiStatus;
    grpc::Status status;
    RpcPayload payload;
};

class RpcClient {
public:
    explicit RpcClient(std::string socket_path);
    ~RpcClient();
    RpcClient(const RpcClient&) = delete;
    RpcClient& operator=(const RpcClient&) = delete;

    uint64_t request_wifi_status();
    uint64_t scan_wifi(bool refresh = true);
    uint64_t request_hardware_status();
    uint64_t request_system_info();
    uint64_t set_brightness(int percent);
    uint64_t set_volume(int percent);
    void cancel(uint64_t id);
    std::vector<RpcResult> take_results();

private:
    struct Task {
        uint64_t id;
        RpcOperation operation;
        int percent;
        bool refresh;
        std::shared_ptr<grpc::ClientContext> context;
    };

    uint64_t submit(RpcOperation operation, int percent = 0, bool refresh = false);
    void run();
    RpcResult invoke(const Task& task);

    std::string socket_path_;
    std::shared_ptr<grpc::Channel> channel_;
    std::unique_ptr<nexus::v1::WifiService::Stub> wifi_stub_;
    std::unique_ptr<nexus::v1::HardwareService::Stub> hardware_stub_;
    std::unique_ptr<nexus::v1::SystemService::Stub> system_stub_;
    std::mutex mutex_;
    std::condition_variable ready_;
    std::deque<Task> queue_;
    std::vector<RpcResult> completed_;
    std::shared_ptr<grpc::ClientContext> active_context_;
    uint64_t active_id_ = 0;
    uint64_t next_id_ = 1;
    bool stopping_ = false;
    std::thread worker_;
};

std::string default_socket_path();

}  // namespace nexus
