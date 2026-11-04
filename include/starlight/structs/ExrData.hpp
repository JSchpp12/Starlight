#pragma once

#include <cstddef>
#include <span>

namespace star
{
/// @brief Owning handle to pixel data allocated by tinyexr's LoadEXR.
class ExrData
{
  public:
    /// @param data Pixel data allocated by LoadEXR; must be released with free().
    /// @param size Number of floats in the buffer.
    ExrData(float *data, std::size_t size) noexcept;
    ~ExrData();
    ExrData(const ExrData &) = delete;
    ExrData &operator=(const ExrData &) = delete;
    ExrData(ExrData &&other) noexcept;
    ExrData &operator=(ExrData &&other) noexcept;

    /// @brief View over the pixel data, in floats, empty when the handle holds nothing.
    std::span<float> data() const noexcept
    {
        return m_data ? std::span<float>(m_data, m_size) : std::span<float>();
    }

    bool valid() const noexcept
    {
        return m_data != nullptr;
    }

  private:
    void release() noexcept;

    float *m_data = nullptr;
    std::size_t m_size = 0;
};
} // namespace star
