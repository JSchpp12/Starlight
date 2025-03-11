#pragma once

#include "TransferRequest_Memory.hpp"
#include "StarTexture.hpp"

#include <string>

namespace star::TransferRequest{
    class FileTexture : public Memory<star::StarTexture::TextureCreateSettings>{
        public:
        FileTexture(const std::string& imagePath);

        star::StarTexture::TextureCreateSettings getCreateArgs(const vk::PhysicalDeviceProperties& deviceProperties) const override;

        void writeData(star::StarBuffer& stagingBuffer) const override;

        protected:
        virtual float selectAnisotropyLevel(const vk::PhysicalDeviceProperties& deviceProperties) const;
        virtual vk::Filter selectTextureFiltering(const vk::PhysicalDeviceProperties& deviceProperties) const;
        
        private:
        const std::string imagePath; 

        static void getTextureInfo(const std::string& imagePath, int& width, int& height, int& channels);
    };
}