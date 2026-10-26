#include "rpc/rpc_client.h"

namespace nexus {

RpcResult RpcClient::invoke(const Task& task) {
    RpcResult result;
    result.id = task.id;
    result.operation = task.operation;

    switch (task.operation) {
        case RpcOperation::WifiStatus: {
            nexus::v1::WifiServiceGetStatusRequest request;
            nexus::v1::WifiServiceGetStatusResponse response;
            result.status = wifi_stub_->GetStatus(task.context.get(), request, &response);
            result.payload = std::move(response);
            break;
        }
        case RpcOperation::WifiWatch:
            result.status = grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Wi-Fi watch is not a unary operation");
            break;
        case RpcOperation::WifiScan: {
            nexus::v1::ScanRequest request;
            request.set_refresh(task.refresh);
            nexus::v1::ScanResponse response;
            result.status = wifi_stub_->Scan(task.context.get(), request, &response);
            result.payload = std::move(response);
            break;
        }
        case RpcOperation::HardwareStatus: {
            nexus::v1::HardwareServiceGetStatusRequest request;
            nexus::v1::HardwareServiceGetStatusResponse response;
            result.status = hardware_stub_->GetStatus(task.context.get(), request, &response);
            result.payload = std::move(response);
            break;
        }
        case RpcOperation::HardwareWatch:
            result.status = grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "hardware watch is not a unary operation");
            break;
        case RpcOperation::SystemInfo: {
            nexus::v1::GetInfoRequest request;
            nexus::v1::GetInfoResponse response;
            result.status = system_stub_->GetInfo(task.context.get(), request, &response);
            result.payload = std::move(response);
            break;
        }
        case RpcOperation::SetBrightness: {
            nexus::v1::SetBrightnessRequest request;
            request.set_percent(task.percent);
            request.set_confirmed(true);
            nexus::v1::SetBrightnessResponse response;
            result.status = hardware_stub_->SetBrightness(task.context.get(), request, &response);
            result.payload = std::move(response);
            break;
        }
        case RpcOperation::SetVolume: {
            nexus::v1::SetVolumeRequest request;
            request.set_percent(task.percent);
            request.set_confirmed(true);
            nexus::v1::SetVolumeResponse response;
            result.status = hardware_stub_->SetVolume(task.context.get(), request, &response);
            result.payload = std::move(response);
            break;
        }
    }
    return result;
}

}  // namespace nexus
