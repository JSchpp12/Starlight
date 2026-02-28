#pragma once

#include <star_common/IServiceCommand.hpp>

namespace star::command
{
namespace save_scene_state
{
inline constexpr std::string_view GetSaveSceneStateCommandTypeName = "star::command::save_scene_state";
}

struct SaveSceneState : public common::IServiceCommand
{
    static constexpr std::string_view GetUniqueTypeName()
    {
        return save_scene_state::GetSaveSceneStateCommandTypeName;
    }

    SaveSceneState() = default;
};
} // namespace star::command