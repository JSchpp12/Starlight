#include "TransferRequest_CompressedTextureFile.hpp"

#include "FileHelpers.hpp"

#include <assert.h>

star::TransferRequest::CompressedTextureFile::CompressedTextureFile(const std::string& imagePath, std::vector<ktx_transcode_fmt_e>& availableFormats, std::vector<std::string>& availableFormatNames) 
: imagePath(imagePath), availableFormats(availableFormats), availableFormatNames(availableFormatNames)
{
    assert(this->availableFormatNames.size() == this->availableFormats.size() && "names and available formats do not match");

    verifyFiles(this->imagePath);
}

star::StarTexture::TextureCreateSettings star::TransferRequest::CompressedTextureFile::getCreateArgs(const vk::PhysicalDeviceProperties& deviceProperties) const{
    StarTexture::TextureCreateSettings createArgs;

    createArgs.width = this->kTexture->baseWidth;
    createArgs.height = this->kTexture->baseHeight;
    createArgs.depth = this->kTexture->baseDepth; 
    createArgs.overrideImageMemorySize = this->kTexture->dataSize;
    
    return createArgs;
}

void star::TransferRequest::CompressedTextureFile::beforeCreate() {
    //transcode operation
    loadKTX(); 

    setTranscodeTargetFormat();

    transcode();
}

void star::TransferRequest::CompressedTextureFile::writeData(StarBuffer& buffer) const{
    assert(this->kTexture != nullptr && "Invalid k texture. Might have failed transcode.");


}

void star::TransferRequest::CompressedTextureFile::afterCreate(){
    if (this->kTexture != nullptr){
        ktxTexture_Destroy((ktxTexture*)this->kTexture); 
        this->kTexture = nullptr;
    }
}

void star::TransferRequest::CompressedTextureFile::verifyFiles(const std::string& imagePath){
    //check extension on file
    assert(FileHelpers::GetFileExtension(imagePath) == ".ktx2");

    //ensure file exists
    assert(FileHelpers::FileExists(imagePath));
}

void star::TransferRequest::CompressedTextureFile::setTranscodeTargetFormat(){
    assert(this->availableFormats.size() > 0 && "System does not support any compression formats"); 
    
    ktx_transcode_fmt_e targetFormat; 

    assert(availableFormats.size() > 0 && "There are no available target transcode formats for this device"); 

    //find any available format which is not the raw format
    for (const auto& format : this->availableFormats){
        if (format != KTX_TTF_RGBA32){
            targetFormat = format; 
            break;
        }
    }

    if (!targetFormat){
        targetFormat = KTX_TTF_RGBA32; 
    }

    this->selectedTranscodeFormat = std::make_unique<ktx_transcode_fmt_e>(targetFormat);
}


void star::TransferRequest::CompressedTextureFile::loadKTX(){
    KTX_error_code result = ktxTexture_CreateFromNamedFile(this->imagePath.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, reinterpret_cast<ktxTexture **>(&this->kTexture)); 
    
    if (result != KTX_SUCCESS){
        throw std::runtime_error("Could not load the requested image file"); 
    }
}

void star::TransferRequest::CompressedTextureFile::transcode(){
    assert(this->kTexture != nullptr && "Invalid ktx texture");
    assert(this->selectedTranscodeFormat != nullptr && "Format has not been selected"); 

    KTX_error_code result; 

    if (ktxTexture2_NeedsTranscoding(this->kTexture)){
        result = ktxTexture2_TranscodeBasis(this->kTexture, *this->selectedTranscodeFormat, 0); 
        if (result != KTX_SUCCESS){
            throw std::runtime_error("Could not transcode the requested image file"); 
        }
    }
}