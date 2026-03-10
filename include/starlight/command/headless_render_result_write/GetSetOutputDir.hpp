#pragma once

#include <star_common/IServiceCommandWithReply.hpp>

#include <starlight/core/Exceptions.hpp>

#include <cassert>
#include <filesystem>
#include <optional>
#include <string_view>

namespace star::headless_render_result_write
{
namespace get_set_output_dir
{
inline constexpr const char *GetUniqueTypeName()
{
    return "hrSOut";
};
} // namespace get_set_output_dir

class GetSetOutputDir : public common::IServiceCommandWithReply<std::filesystem::path>
{
  public:
    class Builder
    {
      public:
        Builder() = default;
        Builder &setGet()
        {
            m_isGetter = true;
            return *this;
        }
        Builder &setSetDir(std::filesystem::path outputDir)
        {
            m_isGetter = false;
            m_outputDirPath = std::move(outputDir);
            return *this;
        }
        GetSetOutputDir build() const
        {
            if (m_isGetter)
            {
                return GetSetOutputDir();
            }

            return GetSetOutputDir(std::move(m_outputDirPath));
        }

      private:
        std::filesystem::path m_outputDirPath;
        bool m_isGetter = false;
    };

    static inline constexpr std::string_view GetUniqueTypeName()
    {
        return get_set_output_dir::GetUniqueTypeName();
    }
    const bool isGetter() const noexcept
    {
        return !m_setterProvidedOutputDir.has_value();
    }
    const bool isSetter() const noexcept
    {
        return m_setterProvidedOutputDir.has_value();
    }
    const std::filesystem::path *getSetterProvidedOutputDir() const noexcept
    {
        assert(m_setterProvidedOutputDir.has_value() && "Get output");

        return &m_setterProvidedOutputDir.value();
    }

  private:
    std::optional<std::filesystem::path> m_setterProvidedOutputDir;

    GetSetOutputDir() = default;
    explicit GetSetOutputDir(std::filesystem::path outputDir)
        : m_setterProvidedOutputDir(std::make_optional(std::move(outputDir)))
    {
    }
};
} // namespace star::headless_render_result_write