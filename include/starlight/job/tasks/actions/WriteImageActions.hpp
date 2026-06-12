#pragma once

#include "StarBuffers/Buffer.hpp"

#include <string>
#include <string_view>
#include <vulkan/vulkan.hpp>

namespace star::job::tasks::actions
{

inline static constexpr std::string_view WriteImageActionsTypeName = "star::job::tasks::actions";

void ValidateExtension(const std::string &path, const std::string &expectedExt);

} // namespace star::job::tasks::actions