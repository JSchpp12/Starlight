#pragma once

#include "starlight/util/io/IOError.hpp"

#include <expected>
#include <filesystem>
#include <span>
#include <variant>

namespace star::io
{
struct MediaSearchPolicy
{
    /// Resolve candidate, requiring the winner to match kind. Never throws.
    /// See the free resolvePath below for the shared probing semantics.
    std::expected<std::filesystem::path, IOError> resolvePath(const std::filesystem::path &candidate,
                                                              PathKind kind) const;
};

using SearchPolicy = std::variant<MediaSearchPolicy /*, FutureApproachB, ... */>;

std::expected<std::filesystem::path, IOError> resolvePath(const SearchPolicy &policy,
                                                          const std::filesystem::path &candidate, PathKind kind);

std::expected<std::filesystem::path, IOError> resolvePath(const std::filesystem::path &candidate, PathKind kind);

/// Probe an explicit order (test seam / advanced use). Same semantics as above.
std::expected<std::filesystem::path, IOError> resolvePath(std::span<const std::filesystem::path> probeOrder,
                                                          const std::filesystem::path &candidate, PathKind kind);
} // namespace star::io
