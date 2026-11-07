#include "device/managers/Buffer.hpp"

#include "core/logging/LoggingFactory.hpp"

namespace star::core::device::manager
{
Buffer::Buffer() : Base(GetBufferHandleTypeName, GetBufferEventTypeName)
{
}

BufferRecord Buffer::createRecord(BufferRequest &&request) const
{
    return BufferRecord{std::move(request.buffer)};
}

BufferRecord *Buffer::get(const Handle &handle)
{
    BufferRecord &record = *Base::get(handle);
    resolve(record);

    return &record;
}

const BufferRecord *Buffer::get(const Handle &handle) const
{
    const BufferRecord &record = *Base::get(handle);
    resolve(record);

    return &record;
}

void Buffer::resolve(const BufferRecord &record) noexcept
{
    auto *pending = std::get_if<Pending>(&record.buffer);
    if (pending == nullptr || !pending->isReady())
    {
        return;
    }

    // get() consumes the future, so the slot switches from Pending to a concrete buffer exactly once.
    auto result = pending->take();
    if (result.has_value())
    {
        record.buffer = std::move(*result);
        return;
    }

    // Fail soft: hand consumers a blank buffer (null Vulkan handle, zero size) instead of leaving readiness false
    // forever. Consumers are responsible for checking the resource they receive.
    core::logging::warning("Buffer manager: transfer producer did not publish a buffer (", result.error(),
                           "). Using a blank buffer; consumers must check the resource.");
    record.buffer = star::StarBuffers::Buffer{};
}
} // namespace star::core::device::manager
