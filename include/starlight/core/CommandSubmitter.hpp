#pragma once

#include "starlight/core/CommandBus.hpp"
#include "starlight/core/Exceptions.hpp"

#include <star_common/IServiceCommand.hpp>

namespace star::core
{
class CommandSubmitter
{
  public:
    using SubmitToDeviceContextFunction = std::function<void(star::common::IServiceCommand &)>;

    CommandSubmitter() = default;
    CommandSubmitter(SubmitToDeviceContextFunction submitFun, star::core::CommandBus &cmdBus)
        : m_cachedType(0), m_submitFun(submitFun), m_isTypeCached(false), m_cmdBus(&cmdBus)
    {
    }

    template <typename T> CommandSubmitter &set(T &command)
    {
        std::string_view name = command.GetUniqueTypeName();
        cacheType(name);

        m_command = &command;
        return *this;
    }

    CommandSubmitter &update(star::common::IServiceCommand &command)
    {
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
    CommandSubmitter &setType(std::string_view name)
    {
        cacheType(name);

        return *this;
    }

    /// @brief Manually define a unique name for the type of command to submit.
    /// @param name The unique name to be used in lookups for this type of command
    /// @return
    CommandSubmitter &setType(uint16_t type)
    {
        cacheType(type);

        return *this;
    }

    void submit()
    {
        assert(m_command != nullptr);
        assert(m_submitFun);
        assert(m_cmdBus != nullptr);

        if (!m_cachedType)
        {
            STAR_THROW("No type information provided");
        }

        m_command->setType(m_cachedType);
        m_submitFun(*m_command);

        m_command = nullptr;
    }

    bool isInitialized() const
    {
        return m_submitFun != nullptr && m_cmdBus != nullptr;
    }

  private:
    uint16_t m_cachedType;
    SubmitToDeviceContextFunction m_submitFun = nullptr;
    bool m_isTypeCached = false;
    star::core::CommandBus *m_cmdBus = nullptr;
    star::common::IServiceCommand *m_command = nullptr;

    void cacheType(std::string_view name)
    {
        cacheType(m_cmdBus->getRegistry().registerType(name));
    }

    void cacheType(uint16_t type)
    {
        m_cachedType = std::move(type);
        m_isTypeCached = true;
    }
};
} // namespace star::core