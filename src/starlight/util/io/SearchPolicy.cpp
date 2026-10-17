#include "starlight/util/io/SearchPolicy.hpp"

#include "starlight/common/helpers/FileHelpers.hpp"

#include <optional>
#include <system_error>
#include <utility>

namespace star::io
{

static std::filesystem::file_type statProbe(const std::filesystem::path &probe, std::error_code &ec)
{
    return std::filesystem::status(probe, ec).type();
}

/// Does a stat'ed type satisfy the requested kind?
static bool typeMatchesKind(std::filesystem::file_type type, PathKind kind)
{
    return kind == PathKind::file ? type == std::filesystem::file_type::regular
                                  : type == std::filesystem::file_type::directory;
}

/// Locate the first matching location, recording nothing.
template <class Emit> static std::optional<std::filesystem::path> locateWinner(Emit &emit, PathKind kind)
{
    std::optional<std::filesystem::path> winner;

    emit([&](const std::filesystem::path &probe) {
        std::error_code statusEc;
        const auto type = statProbe(probe, statusEc);
        if (!typeMatchesKind(type, kind))
        {
            // Missing, wrong kind, or a stat error: keep probing. Errors are
            // only worth reporting when the whole resolution fails, which the
            // second pass below handles.
            return true;
        }

        std::error_code canonicalEc;
        auto canonical = std::filesystem::canonical(probe, canonicalEc);
        if (canonicalEc)
        {
            return true;
        }

        winner = std::move(canonical);
        return false; // first hit wins: stop
    });

    return winner;
}

/// Cold path, reached only when locateWinner found nothing
template <class Emit>
static std::expected<std::filesystem::path, IOError> probeForError(Emit &emit, const std::filesystem::path &candidate,
                                                                   PathKind kind)
{
    IOError error;
    error.requested = candidate;
    error.kind = kind;

    std::filesystem::path winner;
    bool found = false;

    emit([&](const std::filesystem::path &probe) {
        // Record the attempt up front so a failed resolution's IOError lists
        // every offered location, in order.
        error.attempted.push_back(probe);

        std::error_code statusEc;
        const auto type = statProbe(probe, statusEc);

        // Checked before the error code so a location that simply does not
        // exist is a silent miss even on implementations that surface
        // not-found through `ec` rather than through the type.
        if (type == std::filesystem::file_type::not_found)
        {
            return true; // keep going
        }

        if (statusEc)
        {
            // A probe whose stat fails with a real filesystem error (e.g.
            // permission) records ec.message() and probing continues.
            if (error.detail.empty())
            {
                error.detail = statusEc.message();
            }
            return true; // keep going
        }

        if (!typeMatchesKind(type, kind))
        {
            return true; // wrong kind: not an error, keep going
        }

        std::error_code canonicalEc;
        const auto canonical = std::filesystem::canonical(probe, canonicalEc);
        if (!canonicalEc)
        {
            winner = canonical;
            found = true;
            return false; // first hit wins: stop
        }
        if (error.detail.empty())
        {
            error.detail = canonicalEc.message();
        }

        return true; // keep going
    });

    if (found)
    {
        // Only reachable if the filesystem changed between passes; report the
        // winner rather than a stale failure.
        return winner;
    }

    return std::unexpected(std::move(error));
}

/// Shared probing semantics for every SearchPolicy.
template <class Emit>
static std::expected<std::filesystem::path, IOError> probeEach(Emit &&emit, const std::filesystem::path &candidate,
                                                               PathKind kind)
{
    if (auto winner = locateWinner(emit, kind))
    {
        return std::move(*winner);
    }

    return probeForError(emit, candidate, kind);
}

std::expected<std::filesystem::path, IOError> MediaSearchPolicy::resolvePath(const std::filesystem::path &candidate,
                                                                             PathKind kind) const
{
    return probeEach(
        [&](auto &&sink) {
            // 1. The candidate as-is. Relative paths resolve against the
            //    process CWD -- identical to today's behavior, so all
            //    existing absolute/valid paths keep working unchanged.
            if (!sink(candidate))
            {
                return;
            }

            if (candidate.is_absolute())
            {
                // An absolute candidate can only refer to itself.
                return;
            }

            // An empty candidate would join to the roots themselves
            // (dir / "" == dir/), which never represents the requested
            // path; only derive root probes from a non-empty candidate.
            if (candidate.empty())
            {
                return;
            }

            // 2. Under the media directory (next to the executable).
            if (!sink(star::file_helpers::GetMediaDirectory() / candidate))
            {
                return;
            }
            // 3. Under the executable directory.
            sink(star::file_helpers::GetExecutableDirectory() / candidate);
        },
        candidate, kind);
}

std::expected<std::filesystem::path, IOError> resolvePath(const SearchPolicy &policy,
                                                          const std::filesystem::path &candidate, PathKind kind)
{
    return std::visit([&](const auto &searchPolicy) { return searchPolicy.resolvePath(candidate, kind); }, policy);
}

std::expected<std::filesystem::path, IOError> resolvePath(const std::filesystem::path &candidate, PathKind kind)
{
    return resolvePath(SearchPolicy{}, candidate, kind);
}

std::expected<std::filesystem::path, IOError> resolvePath(std::span<const std::filesystem::path> probeOrder,
                                                          const std::filesystem::path &candidate, PathKind kind)
{
    return probeEach(
        [&](auto &&sink) {
            for (const auto &probe : probeOrder)
            {
                if (!sink(probe))
                {
                    return;
                }
            }
        },
        candidate, kind);
}
} // namespace star::io
