#pragma once

#include "core/device/managers/Manager.hpp"
#include "starlight/wrappers/graphics/StarCommandPool.hpp"

namespace star::core::device::manager
{

struct CommandPoolRequest
{
    uint32_t familyIndex;
    bool setAutoReset;
};

struct CommandPoolRecord
{
    StarCommandPool commandPool;

    bool isReady() const
    {
        return true;
    }

    void cleanupRender(device::StarDevice &device)
    {
        commandPool.cleanupRender(device.getVulkanDevice());
    }
};

class CommandPool : public Manager<CommandPoolRecord, CommandPoolRequest, 32>
{
  public:
    CommandPool() : Manager<CommandPoolRecord, CommandPoolRequest, 32>(common::special_types::CommandPoolTypeName)
    {
    }
    CommandPool(const CommandPool &) = delete;
    CommandPool &operator=(const CommandPool &) = delete;
    CommandPool(CommandPool &&) = default;
    CommandPool &operator=(CommandPool &&) = default;

    virtual ~CommandPool() = default;

  private:
    virtual CommandPoolRecord createRecord(CommandPoolRequest &&request) const;
};
} // namespace star::core::device::manager