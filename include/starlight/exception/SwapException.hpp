#pragma once

#include <cstdint>
#include <string>

namespace star::exception
{
/// @brief Why a swap request could not be recorded.
struct SwapException
{
    enum class Reason : std::uint8_t
    {
        UnknownMaterial,
        IncompatibleResource,
        PreparationFailed,
        UnknownCommandBufferPass
    };

    Reason reason{Reason::UnknownMaterial};
    std::string detail;

    [[nodiscard]] const char *what() const noexcept
    {
        switch (reason)
        {
        case Reason::UnknownMaterial:
            return "unknown material";
        case Reason::IncompatibleResource:
            return "incompatible incoming resource";
        case Reason::PreparationFailed:
            return "resource preparation failed";
        case Reason::UnknownCommandBufferPass:
            return "the target command buffer for ownership is unknown";
        }

        return "swap rejected";
    }
};
} // namespace star::exception
