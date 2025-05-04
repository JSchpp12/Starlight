#pragma once

#include "ManagerController_RenderResource_Texture.hpp"
#include "StarTexture.hpp"

#include <ktx.h>

#include <string>
#include <memory>

namespace star::ManagerController::RenderResource{
    class TextureFile : public star::ManagerController::RenderResource::Texture{
        public:
        TextureFile(const std::string& filePath);

        virtual std::unique_ptr<TransferRequest::Memory<StarTexture::TextureCreateSettings>> createTransferRequest(const vk::PhysicalDevice& physicalDevice) override; 

        protected:
        const bool isCompressedTexture; 
        const std::string filePath; 

        static bool IsCompressedFileType(const std::string& filePath); 

        static void GetSupportedCompressedTextureFormats(const vk::PhysicalDevice& physicalDevice, std::vector<ktx_transcode_fmt_e>& availableFormats, std::vector<std::string>& availableFormatNames);
    
        static bool IsFormatSupported(const vk::PhysicalDevice& physicalDevice, const vk::Format& format);
    };

}