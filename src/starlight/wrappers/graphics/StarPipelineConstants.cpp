#include "starlight/wrappers/graphics/StarPipelineConstants.hpp"

#include <star_common/helper/CastHelpers.hpp>

#include "starlight/core/Exceptions.hpp"

namespace star
{

vk::SpecializationInfo StarPipelineConstants::getSpecializationInfo() const
{
    vk::SpecializationInfo specInfo;
    specInfo.mapEntryCount = static_cast<uint32_t>(m_specMapEntries.size());
    specInfo.pMapEntries = m_specMapEntries.data();
    specInfo.dataSize = m_data.size();
    specInfo.pData = m_data.data();
    return specInfo;
}

void StarPipelineConstants::appendConstant(int key, const void *data, size_t size)
{
    assert(!isKeyAlreadyRegistered(key) && "Duplicate key registration is not allowed");

    size_t offset = m_data.size();
    m_data.resize(offset + size);
    std::memcpy(m_data.data() + offset, data, size);

    vk::SpecializationMapEntry entry;
    entry.constantID = key;
    if (!star::common::casts::SafeCast(offset, entry.offset))
    {
        // Handle the error appropriately, e.g., throw an exception or assert
        STAR_THROW("Failed to properly cast offset when parsing vulkan specialization map entry");
    }

    entry.size = size;
    m_specMapEntries.push_back(entry);
}

bool StarPipelineConstants::isKeyAlreadyRegistered(int key) const
{
    for (const auto &entry : m_specMapEntries)
    {
        if (entry.constantID == key)
        {
            return true;
        }
    }
    return false;
}
} // namespace star