// #pragma once

// #include "ManagerController_Controller.hpp"
// #include "TransferRequest_Texture.hpp"

// namespace star::ManagerController::RenderResource
// {
// class Texture : public star::ManagerController::Controller<TransferRequest::Texture>
// {
//   public:
//     Texture() = default;
//     virtual ~Texture() = default;

//     Texture(const uint8_t &frameInFlightIndexToUpdateOn)
//         : star::ManagerController::Controller<TransferRequest::Texture>(frameInFlightIndexToUpdateOn)
//     {
//     }

//     virtual std::unique_ptr<TransferRequest::Texture> createTransferRequest(
//         core::device::StarDevice &device) override = 0;
// };
// } // namespace star::ManagerController::RenderResource