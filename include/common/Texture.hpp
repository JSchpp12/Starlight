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
#include <optional>

namespace star {

    class Texture : public StarTexture {
    public:
        std::string pathToFile = "";
        bool onDisk = false;

        Texture(const vk::Image& image, const vk::ImageLayout& layout, const vk::Format& format); 

        Texture(int texWidth, int texHeight);

		Texture(const int& texWidth, const int& texHeight, StarTexture::TextureCreateSettings& settings)
			: StarTexture(settings), width(texWidth), 
            height(texHeight), channels(4), 
            onDisk(false) {};

        Texture(const int& texWidth, const int& texHeight, const int& channels, TextureCreateSettings& settings)
            : StarTexture(settings), rawData(std::make_optional<std::vector<std::vector<Color>>>(std::vector<std::vector<Color>>(texWidth, std::vector<Color>(texHeight, Color{})))),
            width(texWidth), height(texHeight), channels(channels), onDisk(false) {};

        Texture(const int& texWidth, const int& texHeight, const int& channels, vk::SubresourceLayout texLayout, unsigned char* raw, bool swizzle = false);

        Texture(std::vector<std::vector<Color>> rawData);

        Texture(int texWidth, int texHeight, std::vector<std::vector<Color>> rawData);

        /// <summary>
        /// Initalize texture with provided raw data. For use with programatically generated textures. 
        /// </summary>
        /// <param name="rawTextureData"></param>
        /// <param name="texWidth"></param>
        /// <param name="texHeight"></param>
        /// <param name="texChannels"></param>
        Texture(const std::string& pathToImage);

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