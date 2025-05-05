#pragma once

#include "ManagerController_RenderResource_Texture.hpp"
#include "TransferRequest_Texture.hpp"
#include "StarTexture.hpp"

#include <ktx.h>

#include <string>
#include <memory>

namespace star::ManagerController::RenderResource{
    class TextureFile : public star::ManagerController::RenderResource::Texture{
        public:
        TextureFile(const std::string& nFilePath);

        virtual std::unique_ptr<TransferRequest::Texture> createTransferRequest(const vk::PhysicalDevice& physicalDevice) override; 

        protected:
        bool isCompressedTexture; 
        std::string filePath; 

        static std::string GetFilePath(const std::string& filePath); 

        static bool IsCompressedFileType(const std::string& filePath); 
    };

}