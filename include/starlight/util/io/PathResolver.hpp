#pragma once

#include "starlight/util/io/SearchPolicy.hpp"

#include <expected>
#include <filesystem>
#include <type_traits>
#include <utility>

namespace star::io
{
/// @brief An immutable path resolver that can be handed to a secondary thread.
class PathResolver
{
  public:
    explicit PathResolver(SearchPolicy policy = {}) : m_policy(std::move(policy))
    {
    }

    std::expected<std::filesystem::path, IOError> resolve(const std::filesystem::path &candidate, PathKind kind) const
    {
        return resolvePath(m_policy, candidate, kind);
    }

    const SearchPolicy &policy() const noexcept
    {
        return m_policy;
    }

  private:
    SearchPolicy m_policy;
};

// The resolver travels inside job payloads, whose move path is noexcept.
static_assert(std::is_nothrow_move_constructible_v<PathResolver>,
              "PathResolver must be nothrow move constructible to be used as a job payload");
} // namespace star::io
