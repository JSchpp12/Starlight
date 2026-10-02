#pragma once

#include <ktx.h>
#include <vulkan/vulkan.hpp>


namespace star
{
class SharedCompressedTexture
{
  public:
    class Builder
    {
      public:
        Builder() = default;
        Builder &setPath(std::string path);
        Builder &setAttemptGPUCompressionScheme(vk::PhysicalDevice physicalDevice);
        Builder &setNoAttemptGPUCompression();
        SharedCompressedTexture build();

      private:
        std::string m_path;
        std::optional<bool> m_shouldAttemptGPUCompression;
        vk::PhysicalDevice m_physicalDevice{VK_NULL_HANDLE};
    };
    SharedCompressedTexture(const SharedCompressedTexture &) = delete;
    SharedCompressedTexture &operator=(const SharedCompressedTexture &) = delete;
    SharedCompressedTexture(SharedCompressedTexture &&) = default;
    SharedCompressedTexture &operator=(SharedCompressedTexture &&) = default;
    ~SharedCompressedTexture();

    void triggerTranscode();

    void giveMeTranscodedImage(ktxTexture2 *&texture);

    std::string getPathToFile() const
    {
        return this->m_pathToFile;
    }

  private:
    std::string m_pathToFile;
    ktx_transcode_fmt_e selectedTranscodeTargetFormat;
    ktxTexture2 *m_compTexture{nullptr};
    bool hasBeenTranscoded = false;

    /// @brief Will create compressed texture with default non-compressed texture format
    /// @param pathToFile
    SharedCompressedTexture(std::string pathToFile);

    SharedCompressedTexture(std::string pathToFile, ktx_transcode_fmt_e resultFormat);

    SharedCompressedTexture(std::string pathToFile, const vk::PhysicalDevice &physicalDevice);

    void loadKTX();

    void transcode();
};
} // namespace star