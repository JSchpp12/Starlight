#pragma once

#include <star_common/Handle.hpp>

namespace star
{
/// @brief One committed material swap.
struct MaterialSwapCommitInfo
{
    Handle material;
    Handle newResource;
};
} // namespace star
