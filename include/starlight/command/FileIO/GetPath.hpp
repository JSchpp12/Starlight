#pragma once

#include "starlight/util/io/IOError.hpp"

#include <star_common/IServiceCommandWithReply.hpp>

#include <expected>
#include <filesystem>
#include <string_view>
#include <utility>

namespace star::command::file_io
{
namespace get_path
{
inline constexpr const char *GetGetPathCommandTypeName()
{
    return "gtpth";
}
} // namespace get_path

/// @brief Ask the IOService where a requested file/directory actually lives on disk.
class GetPath : public common::IServiceCommandWithReply<std::expected<std::filesystem::path, star::io::IOError>>
{
  public:
    static inline constexpr std::string_view GetUniqueTypeName()
    {
        return get_path::GetGetPathCommandTypeName();
    }

    GetPath() = default;

    GetPath &setCandidate(std::filesystem::path candidate)
    {
        m_candidate = std::move(candidate);
        return *this;
    }

    /// Required; there is no silent default for the requested kind.
    GetPath &setKind(star::io::PathKind kind)
    {
        m_kind = kind;
        return *this;
    }

    const std::filesystem::path &getCandidate() const noexcept
    {
        return m_candidate;
    }

    star::io::PathKind getKind() const noexcept
    {
        return m_kind;
    }

  private:
    std::filesystem::path m_candidate;
    star::io::PathKind m_kind{};
};
} // namespace star::command::file_io