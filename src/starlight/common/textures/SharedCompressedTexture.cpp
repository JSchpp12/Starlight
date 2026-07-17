#include "SharedCompressedTexture.hpp"

#include "FileHelpers.hpp"
#include "starlight/core/Exceptions.hpp"

ktx_transcode_fmt_e star::SharedCompressedTexture::GetResultTargetCompressedFormat(
    const vk::PhysicalDevice &physicalDevice)
{
    std::vector<ktx_transcode_fmt_e> availableFormats;
    GetSupportedCompressedTextureFormats(physicalDevice, availableFormats);

    return SelectTranscodeFormat(availableFormats);
}

star::SharedCompressedTexture::SharedCompressedTexture(std::string pathToFile)
    : m_pathToFile(std::move(pathToFile)), selectedTranscodeTargetFormat(KTX_TTF_RGBA32)
{
}

star::SharedCompressedTexture::SharedCompressedTexture(std::string pathToFile, ktx_transcode_fmt_e resultFormat)
    : m_pathToFile(std::move(pathToFile)), selectedTranscodeTargetFormat(resultFormat)
{
}

star::SharedCompressedTexture::SharedCompressedTexture(std::string pathToFile, const vk::PhysicalDevice &physicalDevice)
    : m_pathToFile(std::move(pathToFile)),
      selectedTranscodeTargetFormat(GetResultTargetCompressedFormat(physicalDevice))
{
    if (!VerifyFiles(m_pathToFile))
    {
        std::ostringstream oss;
        oss << "File verification failed. Either file is not a ktx2 file type or does not exist: " << m_pathToFile;
        STAR_THROW(oss.str());
    }

    this->selectedTranscodeTargetFormat = GetResultTargetCompressedFormat(physicalDevice);
}

star::SharedCompressedTexture::~SharedCompressedTexture()
{
    if (m_compTexture != nullptr)
    {
        ktxTexture2_Destroy(m_compTexture);
        m_compTexture = nullptr;
    }
}

void star::SharedCompressedTexture::triggerTranscode()
{
    ktxTexture2 *texture = nullptr;

    giveMeTranscodedImage(texture);
}

void star::SharedCompressedTexture::giveMeTranscodedImage(ktxTexture2 *&texture)
{
    if (m_compTexture == nullptr)
        loadKTX();

    if (!hasBeenTranscoded)
        transcode();

    texture = m_compTexture;
}

void star::SharedCompressedTexture::GetSupportedCompressedTextureFormats(
    const vk::PhysicalDevice &physicalDevice, std::vector<ktx_transcode_fmt_e> &availableFormats)
{
    availableFormats.clear();

    vk::PhysicalDeviceFeatures features = physicalDevice.getFeatures();

    // Block compression
    if (features.textureCompressionBC)
    {
        if (IsFormatSupported(physicalDevice, vk::Format::eBc7SrgbBlock))
            availableFormats.push_back(KTX_TTF_BC7_RGBA);
        if (IsFormatSupported(physicalDevice, vk::Format::eBc1RgbaSrgbBlock))
            availableFormats.push_back(KTX_TTF_BC1_RGB);
    }

    // adaptive scalable texture compression
    if (features.textureCompressionASTC_LDR)
    {
        if (IsFormatSupported(physicalDevice, vk::Format::eAstc4x4SrgbBlock))
            availableFormats.push_back(KTX_TTF_ASTC_4x4_RGBA);
    }

    // ericsson texture compression
    if (features.textureCompressionETC2)
    {
        star::core::logging::info(
            "This system supports ericsson texture compression! This has not yet been supported by starlight.");
    }

    // always add uncompressed RGBA as target
    availableFormats.push_back(KTX_TTF_RGBA32);
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
    const std::vector<ktx_transcode_fmt_e> &availableFormats)
{
    assert(availableFormats.size() > 0 && "System does not support any compression formats");

    ktx_transcode_fmt_e targetFormat{};

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

bool star::SharedCompressedTexture::VerifyFiles(const std::string &imagePath)
{
    return file_helpers::GetFileExtension(imagePath) == ".ktx2" && file_helpers::FileExists(imagePath);
}

void star::SharedCompressedTexture::loadKTX()
{
    assert(m_compTexture == nullptr && "KTX file has already been loaded");

    KTX_error_code result =
        ktxTexture2_CreateFromNamedFile(m_pathToFile.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &m_compTexture);

    if (result != KTX_SUCCESS)
    {
        const std::string msg = "KTX Reported an error with the requested ktx file: " + m_pathToFile;
        STAR_THROW(msg);
    }
}

void star::SharedCompressedTexture::transcode()
{
    assert(m_compTexture != nullptr && "Invalid k texture. Might have failed transcode.");
    assert(this->selectedTranscodeTargetFormat && "Format has not been selected");

    KTX_error_code result;

    if (ktxTexture2_NeedsTranscoding(m_compTexture))
    {
        result = ktxTexture2_TranscodeBasis(m_compTexture, this->selectedTranscodeTargetFormat, 0);
        if (result != KTX_SUCCESS)
        {
            const std::string msg = "KTX could not transcode the requested image file: " + m_pathToFile;
            STAR_THROW(msg);
        }
    }

    this->hasBeenTranscoded = true;
}

star::SharedCompressedTexture::Builder &star::SharedCompressedTexture::Builder::setPath(std::string path)
{
    m_path = std::move(path);
    return *this;
}

star::SharedCompressedTexture::Builder &star::SharedCompressedTexture::Builder::setAttemptGPUCompressionScheme(
    vk::PhysicalDevice physicalDevice)
{
    m_shouldAttemptGPUCompression = true;
    m_physicalDevice = std::move(physicalDevice);
    return *this;
}

star::SharedCompressedTexture::Builder &star::SharedCompressedTexture::Builder::setNoAttemptGPUCompression()
{
    m_shouldAttemptGPUCompression = false;
    return *this;
}

star::SharedCompressedTexture star::SharedCompressedTexture::Builder::build()
{
    assert(m_shouldAttemptGPUCompression.has_value() &&
           "Memory storage approach for GPU needs to be provided through either setAttemptGPUCompressionScheme or "
           "setNoAttemptGPUCompression");
    assert(!m_path.empty() && "Path must be provided");

    if (m_shouldAttemptGPUCompression.value())
        return SharedCompressedTexture(std::move(m_path), m_physicalDevice);
    return SharedCompressedTexture(std::move(m_path));
}