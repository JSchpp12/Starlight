#pragma once

#include "ManagerController_RenderResource_Texture.hpp"
#include "StarTexture.hpp"

#include <string>
#include <memory>

namespace star::ManagerController::RenderResource{
    class FileTexture : public star::ManagerController::RenderResource::Texture{
        public:
        FileTexture(const std::string& filePath) : filePath(filePath){}; 

        virtual std::unique_ptr<TransferRequest::Memory<StarTexture::TextureCreateSettings>> createTransferRequest() const override; 

        protected:
        const std::string filePath; 
    };
}