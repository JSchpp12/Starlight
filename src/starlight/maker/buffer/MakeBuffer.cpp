#include "starlight/maker/buffer/MakeBuffer.hpp"

#include "starlight/factory/BufferFactory.hpp"

#include <cassert>
#include <new>

namespace star::maker::buffer
{
static std::unexpected<BufferMakeError> Err(BufferMakeError::Cause cause, std::string detail, size_t built = 0)
{
    return std::unexpected(BufferMakeError{cause, std::move(detail), built});
}

MakeBuffer::MakeBuffer(core::device::StarDevice &device) : m_device(device)
{
}

MakeBuffer &MakeBuffer::setCreateInfo(VmaAllocationCreateInfo nAllocInfo, vk::BufferCreateInfo nCreateInfo,
                                      std::string nAllocationName)
{
    m_createNewAllocationInfo = std::make_optional<StarBuffers::Buffer::Creators>(
        StarBuffers::Buffer::Creators(std::move(nCreateInfo), std::move(nAllocInfo), std::move(nAllocationName)));
    return *this;
}

MakeBuffer &MakeBuffer::setInstanceCount(uint32_t nInstanceCount)
{
    m_instanceCount = nInstanceCount;
    return *this;
}

MakeBuffer &MakeBuffer::setInstanceSize(vk::DeviceSize nInstanceSize)
{
    m_instanceSize = nInstanceSize;
    return *this;
}

MakeBuffer &MakeBuffer::setMinOffsetAlignment(vk::DeviceSize nMinOffsetAlignment)
{
    m_minOffsetAlignment = nMinOffsetAlignment;
    return *this;
}

std::expected<size_t, BufferMakeError> MakeBuffer::make(StarBuffers::Buffer *dst, size_t num) noexcept
{
    if (!m_createNewAllocationInfo.has_value())
    {
        return Err(BufferMakeError::Cause::invalid_config, "Buffer create info must be set before building a buffer");
    }

    if (m_instanceCount == 0)
    {
        return Err(BufferMakeError::Cause::invalid_config, "Instance count must be set before building a buffer");
    }

    if (m_instanceSize == 0)
    {
        return Err(BufferMakeError::Cause::invalid_config, "Instance size must be set before building a buffer");
    }

    size_t created = 0;
    try
    {
        for (; created < num; ++created)
        {
            new (dst + created) StarBuffers::Buffer(createBuffer());
        }
    }
    catch (const vk::OutOfHostMemoryError &e)
    {
        return Err(BufferMakeError::Cause::out_of_memory, e.what(), created);
    }
    catch (const vk::OutOfDeviceMemoryError &e)
    {
        return Err(BufferMakeError::Cause::out_of_memory, e.what(), created);
    }
    catch (const vk::OutOfPoolMemoryError &e)
    {
        return Err(BufferMakeError::Cause::out_of_memory, e.what(), created);
    }
    catch (const vk::MemoryMapFailedError &e)
    {
        return Err(BufferMakeError::Cause::out_of_memory, e.what(), created);
    }
    catch (const vk::DeviceLostError &e)
    {
        return Err(BufferMakeError::Cause::device_lost, e.what(), created);
    }
    catch (const std::bad_alloc &)
    {
        return Err(BufferMakeError::Cause::out_of_memory, "Failed to allocate memory for buffer", created);
    }
    catch (const std::exception &e)
    {
        return Err(BufferMakeError::Cause::invalid_config, e.what(), created);
    }
    catch (...)
    {
        return Err(BufferMakeError::Cause::invalid_config, "Unknown error while creating buffer", created);
    }

    return created;
}

StarBuffers::Buffer MakeBuffer::createBuffer()
{
    assert(m_createNewAllocationInfo.has_value());
    const StarBuffers::Buffer::Creators &creators = m_createNewAllocationInfo.value();

    return StarBuffers::BufferFactory::CreateBuffer(m_device, creators, m_instanceCount, m_instanceSize,
                                                    m_minOffsetAlignment);
}
} // namespace star::maker::buffer
