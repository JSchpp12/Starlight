#include "ManagerController_RenderResource_FileTexture.hpp"

#include "TransferRequest_FileTexture.hpp"

std::unique_ptr<star::TransferRequest::Memory<star::StarTexture::TextureCreateSettings>> star::ManagerController::RenderResource::FileTexture::createTransferRequest() const{
    return std::make_unique<TransferRequest::FileTexture>(this->filePath);
}