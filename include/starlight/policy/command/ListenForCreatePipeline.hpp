#pragma once

#include "starlight/command/pipeline/CreatePipeline.hpp"
#include "starlight/policy/command/ListenFor.hpp"

#include <concepts>

namespace star::policy::command
{
template <typename T>
concept ValidCreatePipelineHandler = requires(T obj) {
    { &T::onCreatePipeline } -> std::same_as<void (T::*)(star::command::pipeline::CreatePipeline &)>;
};

template <typename T>
    requires ValidCreatePipelineHandler<T>
using ListenForCreatePipeline =
    ListenFor<T, star::command::pipeline::CreatePipeline, star::command::pipeline::create_pipeline::GetUniqueTypeName,
              &T::onCreatePipeline>;
} // namespace star::policy::command
