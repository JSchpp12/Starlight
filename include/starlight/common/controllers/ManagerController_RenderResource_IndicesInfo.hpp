// #pragma once

// #include "ManagerController_RenderResource_Buffer.hpp"
// #include "TransferRequest_Buffer.hpp"

// namespace star::ManagerController::RenderResource
// {
// class IndicesInfo : public Buffer
// {
//   public:
//     IndicesInfo(const std::vector<uint32_t> &indices) : Buffer(), indices(std::make_unique<std::vector<uint32_t>>(indices))
//     {
//     }

//     virtual ~IndicesInfo() = default;

//     std::unique_ptr<TransferRequest::Buffer> createTransferRequest(core::device::StarDevice &device, const uint8_t &numFramesInFlight) override;

//   protected:
//     std::unique_ptr<std::vector<uint32_t>> indices;
// };
// } // namespace star::ManagerController::RenderResource