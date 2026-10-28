#pragma once

#include <vector>

#include <vulkan/vulkan.hpp>

namespace star
{
class StarPipelineConstants
{
  public:
    template <typename T> StarPipelineConstants &addConstant(int key, const T &value)
    {
        static_assert(std::is_trivially_copyable<T>::value, "Pipeline constant must be trivially copyable.");
        if constexpr (std::is_same_v<T, bool>)
        {
            vk::Bool32 v = static_cast<bool>(value) ? VK_TRUE : VK_FALSE;
            appendConstant(key, &v, sizeof(v));
        }
        else
        {
            appendConstant(key, &value, sizeof(T));
        }

        return *this;
    }

    bool isEmpty() const
    {
        return m_data.empty();
    }

    vk::SpecializationInfo getSpecializationInfo() const;

  private:
    std::vector<uint8_t> m_data;
    std::vector<vk::SpecializationMapEntry> m_specMapEntries;

    void appendConstant(int key, const void *data, size_t size);

    bool isKeyAlreadyRegistered(int key) const;
};

} // namespace star