#pragma once

#include <star_common/FrameTracker.hpp>
#include <star_common/IEvent.hpp>

#include <string_view>

namespace star::event
{
namespace prep_for_next_frame
{
inline constexpr const char *GetUniqueTypeName()
{
    return "eFrPrep";
}

} // namespace prep_for_next_frame

class PrepForNextFrame : public common::IEvent
{
  public:
    static constexpr std::string_view GetUniqueTypeName()
    {
        return prep_for_next_frame::GetUniqueTypeName();
    }

    explicit PrepForNextFrame(common::FrameTracker &frameTracker);
    virtual ~PrepForNextFrame() = default;

    common::FrameTracker *getFrameTracker() const
    {
        return m_frameTracker;
    }

  private:
    mutable common::FrameTracker *m_frameTracker = nullptr;
};
} // namespace star::event