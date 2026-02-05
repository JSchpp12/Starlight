#pragma once

#include <star_common/IServiceCommandWithReply.hpp>

#include <string_view>

namespace star::headless_render_result_write
{
namespace get_file_name_for_frame
{
inline constexpr const char *GetFileNameForFrameTypeName()
{
    return "hrGetFName";
}
} // namespace get_file_name_for_frame

class GetFileNameForFrame : public common::IServiceCommandWithReply<std::string>
{
  public:
    static inline constexpr std::string_view GetUniqueTypeName()
    {
        return get_file_name_for_frame::GetFileNameForFrameTypeName();
    }

    GetFileNameForFrame() = default;
    GetFileNameForFrame(const GetFileNameForFrame &) = default;
    GetFileNameForFrame &operator=(const GetFileNameForFrame &) = default;
    GetFileNameForFrame(GetFileNameForFrame &&) = default;
    GetFileNameForFrame &operator=(GetFileNameForFrame &&) = default;
    virtual ~GetFileNameForFrame() = default;
};
} // namespace star::headless_render_result_write