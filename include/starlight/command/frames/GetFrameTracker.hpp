#pragma once

#include <star_common/FrameTracker.hpp>
#include <star_common/IServiceCommandWithReply.hpp>

namespace star::frames
{
namespace get_frame_tracker
{
inline constexpr const char *GetUniqueTypeName()
{
    return "sfrGetFT";
}
} // namespace get_frame_tracker

struct GetFrameTracker : public star::common::IServiceCommandWithReply<const common::FrameTracker *>
{
    static std::string_view GetUniqueTypeName()
    {
        return get_frame_tracker::GetUniqueTypeName();
    }
};
} // namespace star::frames