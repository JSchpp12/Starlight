#include "device/managers/Buffer.hpp"

#include <cassert>
#include <variant>

namespace star::core::device::manager
{
Buffer::Buffer() : Base(GetBufferEventTypeName)
{
}

BufferRecord Buffer::createRecord(BufferRequest &&request) const
{
    auto *buffer = std::get_if<star::StarBuffers::Buffer>(&request.payload);
    assert(buffer != nullptr && "A pending buffer request must be handled by createSlot");
    return BufferRecord{std::move(*buffer)};
}
} // namespace star::core::device::manager
