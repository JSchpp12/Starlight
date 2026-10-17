#pragma once

#include "starlight/util/io/IOError.hpp"

#include <star_common/IServiceCommandWithReply.hpp>

#include <cassert>
#include <expected>
#include <filesystem>
#include <optional>
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
    class Builder
    {
      public:
        Builder() = default;

        Builder &setCandidate(std::filesystem::path candidate)
        {
            m_candidate = std::move(candidate);
            return *this;
        }

        /// Required; there is no silent default for the requested kind.
        Builder &setKind(star::io::PathKind kind)
        {
            m_kind = kind;
            return *this;
        }

        GetPath build() const
        {
            assert(!m_candidate.empty() && "GetPath requires a non-empty candidate path");
            assert(m_kind.has_value() && "GetPath requires a kind (file or directory)");
            return GetPath(m_candidate, *m_kind);
        }

      private:
        std::filesystem::path m_candidate;
        std::optional<star::io::PathKind> m_kind;
    };

    static inline constexpr std::string_view GetUniqueTypeName()
    {
        return get_path::GetGetPathCommandTypeName();
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

    GetPath() = default;
    GetPath(std::filesystem::path candidate, star::io::PathKind kind) : m_candidate(std::move(candidate)), m_kind(kind)
    {
    }
};
} // namespace star::command::file_io
