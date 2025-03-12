#include "ManagerController_RenderResource_TextureFile.hpp"

#include "TransferRequest_TextureFile.hpp"

std::unique_ptr<star::TransferRequest::Memory<star::StarTexture::TextureCreateSettings>> star::ManagerController::RenderResource::TextureFile::createTransferRequest() const{
    return std::make_unique<TransferRequest::TextureFile>(this->filePath);
}