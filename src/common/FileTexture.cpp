#include "FileTexture.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

//star::FileTexture::FileTexture(int texWidth, int texHeight)
//	: width(texWidth), height(texHeight), channels(4), onDisk(false) {}
//
//star::FileTexture::FileTexture(const int& texWidth, const int& texHeight, const int& channels, vk::SubresourceLayout texLayout, unsigned char* raw, bool swizzle)
//	: onDisk(false) {
//	this->rawData = std::make_optional<std::vector<std::vector<Color>>>(std::vector<std::vector<Color>>(texHeight, std::vector<Color>(texWidth, Color{})));
//    for (int i = 0; i < texHeight; i++) {
//        unsigned int* row = (unsigned int*)raw;
//
//		for (int j = 0; j < texWidth; j++) {
//            unsigned char *r, *g, *b, *a = nullptr; 
//            if (swizzle) {
//                a = (unsigned char*)row + 3;
//                r = (unsigned char*)row + 2;
//                g = (unsigned char*)row + 1;
//                b = (unsigned char*)row;
//            }
//            else {
//                r = (unsigned char*)row; 
//                g = (unsigned char*)row + 1;
//                b = (unsigned char*)row + 2;
//                a = (unsigned char*)row + 3;
//            }
//
//            {
//                unsigned char fr = r != nullptr ? *r : 0;
//                unsigned char fg = g != nullptr ? *g : 0;
//                unsigned char fb = b != nullptr ? *b : 0;
//                this->rawData.value().at(i).at(j) = Color(fr, fg, fb);
//            }
//
//            row++;
//		}
//        raw += texLayout.rowPitch;
//	}
////}
//star::FileTexture::FileTexture(std::vector<std::vector<Color>> rawData)
//	: rawData(rawData), width(rawData.size()), 
//	height(rawData.front().size()), channels(4) {}

//star::FileTexture::FileTexture(int texWidth, int texHeight, std::vector<std::vector<Color>> rawData)
//	: rawData(rawData), width(texWidth),
//	height(texHeight), channels(4), onDisk(false) {}

star::FileTexture::FileTexture(const std::string& pathToImage)
    : pathToFile(pathToImage), 
    StarTexture(TextureCreateSettings{
        getTextureWidth(pathToImage),
        getTextureHeight(pathToImage),
        4,
        1,
        1,
        vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst,
        vk::Format::eR8G8B8A8Srgb,
        vk::ImageAspectFlagBits::eColor,
        VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO,
        VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
        vk::ImageLayout::eShaderReadOnlyOptimal,
        false, true})
{

}

std::optional<std::unique_ptr<unsigned char>> star::FileTexture::data()
{
    //load from disk

    int l_width, l_height, l_channels = 0; 
    std::unique_ptr<unsigned char> pixelData(stbi_load(pathToFile.c_str(), &l_width, &l_height, &l_channels, STBI_rgb_alpha));

    //apply overriden alpha value if needed
    if (this->overrideAlpha.has_value() && this->getChannels() == 4) {
        for (int i = 0; i < l_height; i++) {
            for (int j = 0; j < l_width; j++) {
                unsigned char* pixelOffset = pixelData.get() + (i + this->getHeight() * j) * this->getChannels();
                pixelOffset[3] = this->overrideAlpha.value().raw_a();
            }
        }
    }

    if (!pixelData) {
        throw std::runtime_error("Unable to load image");
    }

    return std::move(pixelData);
    //}
    //else if (rawData.has_value()) {
    //    int numPix = this->getHeight() * this->getWidth() * this->getChannels();
    //    int pixCounter = 0;

    //    auto data = std::vector<unsigned char>(numPix);
    //    for (int i = 0; i < this->getHeight(); i++) {
    //        for (int j = 0; j < this->getWidth(); j++) {
    //            if (this->getChannels() == 3) {
    //                data.at(pixCounter * this->getChannels() + 0) = rawData.value().at(i).at(j).raw_r();
    //                data.at(pixCounter * this->getChannels() + 1) = rawData.value().at(i).at(j).raw_g();
    //                data.at(pixCounter * this->getChannels() + 2) = rawData.value().at(i).at(j).raw_b();
    //            }
    //            else if (this->getChannels() == 4) {
    //                data.at(getChannels() * pixCounter + 0) = rawData.value().at(i).at(j).raw_r();
    //                data.at(getChannels() * pixCounter + 1) = rawData.value().at(i).at(j).raw_g();
    //                data.at(getChannels() * pixCounter + 2) = rawData.value().at(i).at(j).raw_b();
    //                if (this->overrideAlpha.has_value())
    //                    data.at(getChannels() * pixCounter + 3) = this->overrideAlpha.value().raw_a();
    //                else
    //                    data.at(getChannels() * pixCounter + 3) = rawData.value().at(i).at(j).raw_a();
    //            }
    //            pixCounter++;
    //        }
    //    }

    //    auto prepData = std::unique_ptr<unsigned char>(new unsigned char[data.size()]);
    //    std::copy(data.begin(), data.end(), prepData.get());

    //    return std::move(prepData);

    //}

	//return std::optional<std::unique_ptr<unsigned char>>();
}

void star::FileTexture::saveToDisk(const std::string& path)
{
    auto possibleData = this->data(); 
    if (this->rawData && possibleData.has_value()) {
        std::unique_ptr<unsigned char>& data = possibleData.value(); 

        stbi_write_png(path.c_str(), this->getWidth(), this->getHeight(), this->getChannels(), data.get(), this->getWidth() * this->getChannels());
    }
}

int star::FileTexture::getTextureHeight(const std::string& pathToFile) {
    int width, height, channels = 0; 
    stbi_info(pathToFile.c_str(), &width, &height, &channels);

    return height;
}

int star::FileTexture::getTextureWidth(const std::string& pathToFile) {
    int width, height, channels = 0; 
    stbi_info(pathToFile.c_str(), &width, &height, &channels); 

    return width; 
}

int star::FileTexture::getTextureChannels(const std::string& pathToFile) {
    int width, height, channels = 0; 
    stbi_info(pathToFile.c_str(), &width, &height, &channels);

    return channels; 
}