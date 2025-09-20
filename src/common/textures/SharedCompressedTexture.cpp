#include "SharedCompressedTexture.hpp"

#include "FileHelpers.hpp"

ktx_transcode_fmt_e star::SharedCompressedTexture::GetResultTargetCompressedFormat(
    const vk::PhysicalDevice &physicalDevice)
{
    std::vector<ktx_transcode_fmt_e> availableFormats;
    std::vector<std::string> availableFormatNames;

    assert(availableFormatNames.size() == availableFormats.size() && "names and available formats do not match");

    GetSupportedCompressedTextureFormats(physicalDevice, availableFormats, availableFormatNames);

    return SelectTranscodeFormat(availableFormats, availableFormatNames);
}

star::SharedCompressedTexture::SharedCompressedTexture(const std::string &pathToFile)
    : pathToFile(pathToFile), selectedTranscodeTargetFormat(KTX_TTF_RGBA32)
{
}

star::SharedCompressedTexture::SharedCompressedTexture(const std::string &pathToFile, ktx_transcode_fmt_e resultFormat)
    : pathToFile(pathToFile), selectedTranscodeTargetFormat(resultFormat)
{
}

star::SharedCompressedTexture::SharedCompressedTexture(const std::string &pathToFile,
                                                       const vk::PhysicalDevice &physicalDevice)
    : pathToFile(pathToFile)
{
    VerifyFiles(pathToFile);

    this->selectedTranscodeTargetFormat = GetResultTargetCompressedFormat(physicalDevice);
}

star::SharedCompressedTexture::~SharedCompressedTexture()
{
    ktxTexture_Destroy((ktxTexture *)this->resource);
}

void star::SharedCompressedTexture::triggerTranscode(){
    boost::unique_lock<boost::mutex> lock; 
    ktxTexture2 *texture = nullptr;

    giveMeTranscodedImage(lock, texture); 
}

void star::SharedCompressedTexture::giveMeTranscodedImage(boost::unique_lock<boost::mutex> &lock, ktxTexture2 *&texture)
{

    // check for load or transcode
    {
        boost::unique_lock<boost::mutex> internalLock = boost::unique_lock<boost::mutex>();
        ktxTexture2 *internalPtr = nullptr;

        this->giveMeResource(internalLock, internalPtr);

        if (this->resource == nullptr)
        {
            loadKTX();
        }

        if (!this->hasBeenTranscoded)
        {
            transcode();
        }
    }

    this->giveMeResource(lock, texture);
}

void star::SharedCompressedTexture::GetSupportedCompressedTextureFormats(
    const vk::PhysicalDevice &physicalDevice, std::vector<ktx_transcode_fmt_e> &availableFormats,
    std::vector<std::string> &availableFormatNames)
{
    availableFormats.clear();
    availableFormatNames.clear();

    vk::PhysicalDeviceFeatures features = physicalDevice.getFeatures();

    // Block compression
    if (features.textureCompressionBC)
    {
        if (IsFormatSupported(physicalDevice, vk::Format::eBc7SrgbBlock))
        {
            availableFormats.push_back(KTX_TTF_BC7_RGBA);
            availableFormatNames.push_back("BC7");
        }
    }

    // adaptive scalable texture compression
    if (features.textureCompressionASTC_LDR)
    {
        std::cout << "This system supports adaptive scalable texture compression! This has not yet been supported by "
                     "starlight."
                  << std::endl;
    }

    // ericsson texture compression
    if (features.textureCompressionETC2)
    {
        std::cout << "This system supports ericsson texture compression! This has not yet been supported by starlight."
                  << std::endl;
    }

    // always add uncompressed RGBA as target
    availableFormats.push_back(KTX_TTF_RGBA32);
    availableFormatNames.push_back("KTX_TTF_RGBA32");
}

bool star::SharedCompressedTexture::IsFormatSupported(const vk::PhysicalDevice &physicalDevice,
                                                      const vk::Format &format)
{
    vk::FormatProperties properties;
    physicalDevice.getFormatProperties(format, &properties);
    return ((properties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eTransferDst) &&
            (properties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImage));
}

ktx_transcode_fmt_e star::SharedCompressedTexture::SelectTranscodeFormat(
    const std::vector<ktx_transcode_fmt_e> &availableFormats, const std::vector<std::string> &availableFormatNames)
{
    assert(availableFormats.size() > 0 && "System does not support any compression formats");

    ktx_transcode_fmt_e targetFormat;

    assert(availableFormats.size() > 0 && "There are no available target transcode formats for this device");

    // find any available format which is not the raw format
    for (const auto &format : availableFormats)
    {
        if (format != KTX_TTF_RGBA32)
        {
            targetFormat = format;
            break;
        }
    }

    if (!targetFormat)
    {
        targetFormat = KTX_TTF_RGBA32;
    }

    return targetFormat;
}

void star::SharedCompressedTexture::VerifyFiles(const std::string &imagePath)
{
    // check extension on file
    assert(file_helpers::GetFileExtension(imagePath) == ".ktx2");

    // ensure file exists
    assert(file_helpers::FileExists(imagePath));
}

void star::SharedCompressedTexture::loadKTX()
{
    assert(this->resource == nullptr && "KTX file has already been loaded");

    KTX_error_code result =
        ktxTexture2_CreateFromNamedFile(pathToFile.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &this->resource);

    if (result != KTX_SUCCESS)
    {
        throw std::runtime_error("Could not load the requested image file");
    }
}

void star::SharedCompressedTexture::transcode()
{
    assert(this->resource != nullptr && "Invalid k texture. Might have failed transcode.");
    assert(this->selectedTranscodeTargetFormat && "Format has not been selected");

    KTX_error_code result;

    if (ktxTexture2_NeedsTranscoding(this->resource))
    {
        result = ktxTexture2_TranscodeBasis(this->resource, this->selectedTranscodeTargetFormat, 0);
        if (result != KTX_SUCCESS)
        {
            throw std::runtime_error("Could not transcode the requested image file");
        }
    }

    this->hasBeenTranscoded = true;
}
