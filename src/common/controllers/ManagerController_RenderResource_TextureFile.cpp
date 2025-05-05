#include "ManagerController_RenderResource_TextureFile.hpp"

#include "TransferRequest_TextureFile.hpp"
#include "TransferRequest_CompressedTextureFile.hpp"
#include "FileHelpers.hpp"

#include <boost/filesystem.hpp>

star::ManagerController::RenderResource::TextureFile::TextureFile(const std::string& nFilePath) {
    this->filePath = GetFilePath(nFilePath); 
    this->isCompressedTexture = IsCompressedFileType(this->filePath);
}

std::vector<std::unique_ptr<star::TransferRequest::Memory<star::StarTexture::TextureCreateSettings>>> star::ManagerController::RenderResource::TextureFile::createTransferRequests(const vk::PhysicalDevice& physicalDevice){
    std::vector<std::unique_ptr<star::TransferRequest::Memory<star::StarTexture::TextureCreateSettings>>> result = std::vector<std::unique_ptr<star::TransferRequest::Memory<star::StarTexture::TextureCreateSettings>>>();

    if (this->isCompressedTexture){
        std::vector<ktx_transcode_fmt_e> formats;
        std::vector<std::string> names;
        GetSupportedCompressedTextureFormats(physicalDevice, formats, names); 
        result.emplace_back(std::make_unique<TransferRequest::CompressedTextureFile>(this->filePath, formats, names));
    }else{
        result.emplace_back(std::make_unique<TransferRequest::TextureFile>(this->filePath));
    }
    
    return result; 
}

bool star::ManagerController::RenderResource::TextureFile::IsCompressedFileType(const std::string& filePath){
    return FileHelpers::GetFileExtension(filePath) == ".ktx2";
}

std::string star::ManagerController::RenderResource::TextureFile::GetFilePath(const std::string& filePath){
    if (FileHelpers::FileExists(filePath)){
        return filePath;
    }
    
    const std::string name = FileHelpers::GetFileNameWithoutExtension(filePath);
    
    boost::filesystem::path parentDir = boost::filesystem::path(filePath);
    auto result = FileHelpers::FindFileInDirectoryWithSameNameIgnoreFileType(parentDir.parent_path().string(), name); 

    assert(result.has_value() && "Unable to find matching file!"); 

    return result.value(); 
}

void star::ManagerController::RenderResource::TextureFile::GetSupportedCompressedTextureFormats(const vk::PhysicalDevice& physicalDevice, std::vector<ktx_transcode_fmt_e>& availableFormats, std::vector<std::string>& availableFormatNames){
    availableFormats.clear(); 
    availableFormatNames.clear(); 

    vk::PhysicalDeviceFeatures features = physicalDevice.getFeatures(); 

    //Block compression
    if (features.textureCompressionBC){
        if (IsFormatSupported(physicalDevice, vk::Format::eBc7SrgbBlock)){
            availableFormats.push_back(KTX_TTF_BC7_RGBA); 
            availableFormatNames.push_back("BC7");
        }
    }

    //adaptive scalable texture compression
    if (features.textureCompressionASTC_LDR){
        std::cout << "This system supports adaptive scalable texture compression! This has not yet been supported by starlight." << std::endl;
    } 

    //ericsson texture compression
    if (features.textureCompressionETC2){
        std::cout << "This system supports ericsson texture compression! This has not yet been supported by starlight." << std::endl;
    }

    //always add uncompressed RGBA as target
    availableFormats.push_back(KTX_TTF_RGBA32);
    availableFormatNames.push_back("KTX_TTF_RGBA32");
}

bool star::ManagerController::RenderResource::TextureFile::IsFormatSupported(const vk::PhysicalDevice& physicalDevice, const vk::Format& format){
    vk::FormatProperties properties; 
    physicalDevice.getFormatProperties(format, &properties);
    return ((properties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eTransferDst) && (properties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImage));
}
