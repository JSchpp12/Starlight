#pragma once

#include "starlight/core/CommandBus.hpp"

#include <star_common/IServiceCommand.hpp>

namespace star::core
{
class CommandSubmitter
{
  public:
    using SubmitToDeviceContextFunction = std::function<void(star::common::IServiceCommand &)>; 

    CommandSubmitter() = default;
    CommandSubmitter(SubmitToDeviceContextFunction submitFun, star::core::CommandBus &cmdBus)
        : m_submitFun(submitFun), m_cmdBus(&cmdBus)
    {
    }
    template <typename T> CommandSubmitter &set(T &command)
    {
        m_name = command.GetUniqueTypeName();
        m_command = &command;
        return *this;
    }
    CommandSubmitter &setCommand(star::common::IServiceCommand &command)
    {
        m_command = &command;
        return *this;
    }
    /// @brief Manually define a unique name for the type of command to submit.
    /// @param name The unique name to be used in lookups for this type of command
    /// @return
    CommandSubmitter &setName(std::string_view name)
    {
        m_name = name;
        return *this;
    }

    void submit()
    {
        assert(m_command != nullptr);
        assert(m_submitFun);
        assert(m_cmdBus != nullptr);

        m_command->setType(m_cmdBus->getRegistry().getTypeGuaranteedExist(m_name));
        m_submitFun(*m_command);
    }

    bool isInitialized() const
    {
        return m_submitFun != nullptr && m_cmdBus != nullptr;
    }

  private:
    SubmitToDeviceContextFunction m_submitFun = nullptr;
    star::core::CommandBus *m_cmdBus = nullptr;

    std::string_view m_name;
    star::common::IServiceCommand *m_command = nullptr;
};
} // namespace star::core