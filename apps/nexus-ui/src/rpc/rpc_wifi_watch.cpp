#include "rpc/rpc_client.h"

#include <chrono>

namespace nexus {
using namespace std::chrono_literals;

void RpcClient::run_wifi_watch(uint64_t id) {
    bool error_reported = false;
    while (true) {
        auto context = std::make_shared<grpc::ClientContext>();
        {
            std::lock_guard lock(mutex_);
            if (stopping_ || wifi_watch_cancelled_) return;
            wifi_watch_context_ = context;
        }

        nexus::v1::WifiServiceWatchStatusRequest request;
        auto reader = wifi_stub_->WatchStatus(context.get(), request);
        nexus::v1::WifiServiceWatchStatusResponse response;
        while (reader->Read(&response)) {
            RpcResult update;
            update.id = id;
            update.operation = RpcOperation::WifiWatch;
            update.payload = response;
            {
                std::lock_guard lock(mutex_);
                if (stopping_ || wifi_watch_cancelled_) break;
                completed_.push_back(std::move(update));
            }
            error_reported = false;
        }
        auto status = reader->Finish();

        bool should_stop = false;
        {
            std::lock_guard lock(mutex_);
            if (wifi_watch_context_ == context) wifi_watch_context_.reset();
            should_stop = stopping_ || wifi_watch_cancelled_;
        }
        if (should_stop) return;
        if (status.ok()) {
            status = grpc::Status(grpc::StatusCode::UNAVAILABLE, "Wi-Fi status stream ended");
        }
        if (!error_reported) {
            RpcResult failure;
            failure.id = id;
            failure.operation = RpcOperation::WifiWatch;
            failure.status = status;
            {
                std::lock_guard lock(mutex_);
                if (stopping_ || wifi_watch_cancelled_) return;
                completed_.push_back(std::move(failure));
            }
            error_reported = true;
        }
        std::unique_lock lock(mutex_);
        ready_.wait_for(lock, 1s, [this] { return stopping_ || wifi_watch_cancelled_; });
        if (stopping_ || wifi_watch_cancelled_) return;
    }
}

}  // namespace nexus
