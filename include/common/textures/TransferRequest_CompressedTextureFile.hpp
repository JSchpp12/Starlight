#pragma once

#include "TransferRequest_Memory.hpp"
#include "StarTexture.hpp"

#include <ktx.h>
#include <ktxvulkan.h>

#include <vector>
#include <memory>

namespace star::TransferRequest{
    class CompressedTextureFile : public Memory<star::StarTexture::TextureCreateSettings>{
        public:
        CompressedTextureFile(const std::string& imagePath, 
            std::vector<ktx_transcode_fmt_e>& availableFormats, std::vector<std::string>& availableFormatNames);

        StarTexture::TextureCreateSettings getCreateArgs(const vk::PhysicalDeviceProperties& deviceProperties) const override;
        
        void beforeCreate() override; 

        void writeData(StarBuffer& buffer) const override; 

        void afterCreate() override;

        private:
        const std::string imagePath; 
        std::vector<ktx_transcode_fmt_e> availableFormats = std::vector<ktx_transcode_fmt_e>(); 
        std::vector<std::string> availableFormatNames = std::vector<std::string>();

        size_t sizeOfCompressedTexture = 0; 
        ktxTexture2* kTexture = nullptr; 
        std::unique_ptr<ktx_transcode_fmt_e> selectedTranscodeFormat = std::unique_ptr<ktx_transcode_fmt_e>();

        static void getTextureInfo(const std::string& imagePath, int& width, int& height, int& channels);

        static void verifyFiles(const std::string& imagePath); 

        void setTranscodeTargetFormat(); 

        void loadKTX(); 

        void transcode(); 
    };
}