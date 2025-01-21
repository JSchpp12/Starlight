#pragma once 

#include "Color.hpp"

#include "StarTexture.hpp"

#include <vulkan/vulkan.hpp>
#include <memory> 
#include <vector> 
#include <string> 
#include <stdexcept>
#include <optional>
#include <iostream>

namespace star {

    class FileTexture : public StarTexture {
    public:
        std::string pathToFile = "";
        bool onDisk = false;

        FileTexture(const vk::Image& image, const vk::ImageLayout& layout, const vk::Format& format); 

        FileTexture(int texWidth, int texHeight);

		FileTexture(const int& texWidth, const int& texHeight, StarTexture::TextureCreateSettings& settings)
			: StarTexture(settings), width(texWidth), 
            height(texHeight), channels(4), 
            onDisk(false) {};

        FileTexture(const int& texWidth, const int& texHeight, const int& channels, TextureCreateSettings& settings)
            : StarTexture(settings), rawData(std::make_optional<std::vector<std::vector<Color>>>(std::vector<std::vector<Color>>(texWidth, std::vector<Color>(texHeight, Color{})))),
            width(texWidth), height(texHeight), channels(channels), onDisk(false) {};

        FileTexture(const int& texWidth, const int& texHeight, const int& channels, vk::SubresourceLayout texLayout, unsigned char* raw, bool swizzle = false);

        FileTexture(std::vector<std::vector<Color>> rawData);

        FileTexture(int texWidth, int texHeight, std::vector<std::vector<Color>> rawData);

        /// <summary>
        /// Initalize texture with provided raw data. For use with programatically generated textures. 
        /// </summary>
        /// <param name="rawTextureData"></param>
        /// <param name="texWidth"></param>
        /// <param name="texHeight"></param>
        /// <param name="texChannels"></param>
        FileTexture(const std::string& pathToImage);

        int getHeight() override {return this->height; }
        int getWidth() override { return this->width; }
        int getChannels() override { return 4; }
        int getDepth() override { return 1; }

        void loadFromDisk();


        virtual std::optional<std::unique_ptr<unsigned char>> data() override;

        /// <summary>
        /// Get a pointer to the raw storage data of an in-memory texture
        /// </summary>
        /// <returns></returns>
        std::vector<std::vector<Color>>* getRawData() {
            if (!rawData.has_value())
                return nullptr;
            return &rawData.value();
        }

        void setAlpha(Color setAlpha) {
            this->overrideAlpha = setAlpha; 
        }

        void saveToDisk(const std::string& path);

    protected:
        std::optional<Color> overrideAlpha; 
        std::optional<std::vector<std::vector<Color>>> rawData;
        int width = 0, height = 0, channels = 0;
    };
}