#pragma once

#include <star_common/IServiceCommand.hpp>

namespace star::core
{
template <typename TDeviceContext> class CommandSubmitter
{
  public:
    explicit CommandSubmitter(TDeviceContext &me) : m_me(me)
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

        m_command->setType(m_me.m_commandBus.getRegistry().getTypeGuaranteedExist(m_name));
        m_me.submit(*m_command);
    }

  private:
    TDeviceContext &m_me;

    std::string_view m_name;
    star::common::IServiceCommand *m_command = nullptr;
};
} // namespace star::core