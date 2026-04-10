#pragma once

#include <vulkan/vulkan.hpp>

#include <ktx.h>

namespace star
{
class SharedCompressedTexture
{
  public:
    static ktx_transcode_fmt_e GetResultTargetCompressedFormat(const vk::PhysicalDevice &physicalDevice);

    /// @brief Will create compressed texture with default non-compressed texture format
    /// @param pathToFile 
    SharedCompressedTexture(const std::string &pathToFile); 

    SharedCompressedTexture(const std::string &pathToFile, ktx_transcode_fmt_e resultFormat);

    SharedCompressedTexture(const std::string &pathToFile, const vk::PhysicalDevice &physicalDevice);

    virtual ~SharedCompressedTexture();

    void triggerTranscode(); 

    void giveMeTranscodedImage(ktxTexture2 *&texture);

    uint8_t getHeight() const;
    uint8_t getWidth() const;
    uint8_t getDepth() const;
    uint8_t getNumMipMaps() const;
    std::string getPathToFile()
    {
        return this->pathToFile;
    }

  private:
    const std::string pathToFile;
    bool hasBeenTranscoded = false;
    ktx_transcode_fmt_e selectedTranscodeTargetFormat;
    ktxTexture2 *m_compTexture{nullptr};

    static void GetSupportedCompressedTextureFormats(const vk::PhysicalDevice &physicalDevice,
                                                     std::vector<ktx_transcode_fmt_e> &availableFormats,
                                                     std::vector<std::string> &availableFormatNames);

    static bool IsFormatSupported(const vk::PhysicalDevice &physicalDevice, const vk::Format &format);

    static ktx_transcode_fmt_e SelectTranscodeFormat(const std::vector<ktx_transcode_fmt_e> &availableFormats,
                                                     const std::vector<std::string> &availableFormatNames);

    static bool VerifyFiles(const std::string &imagePath);

    void loadKTX();

    void transcode();
};
} // namespace star