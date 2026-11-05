#pragma once

#include <cstddef>
#include <span>

namespace star
{
/// @brief Owning handle to pixel data allocated by stbi_load. Unique ownership: the data is released with
/// stbi_image_free when the object is destroyed.
struct PngData
{
    PngData(unsigned char *data, std::size_t size) noexcept;
    ~PngData();
    PngData(const PngData &) = delete;
    PngData &operator=(const PngData &) = delete;
    PngData(PngData &&other) noexcept;
    PngData &operator=(PngData &&other) noexcept;

    /// @brief View over the pixel data in bytes, empty when the handle holds nothing.
    std::span<unsigned char> data() const noexcept
    {
        return m_data ? std::span<unsigned char>(m_data, m_size) : std::span<unsigned char>();
    }

    bool valid() const noexcept
    {
        return m_data != nullptr;
    }

  private:
    void release() noexcept;

    unsigned char *m_data = nullptr;
    std::size_t m_size = 0;
};
} // namespace star
