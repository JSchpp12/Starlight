#pragma once

#include <star_common/Handle.hpp>

namespace star::service::command_order
{
struct EdgeDescription
{
    Handle producer; 
    Handle consumer; 
};
} // namespace star::service::command_order