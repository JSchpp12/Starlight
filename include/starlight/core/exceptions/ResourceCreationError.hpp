#pragma once

#include <stdexcept>
#include <string>
#include <utility>

namespace star::core
{
/// @brief Describes why a resource could not be created.
/// @details Published by resource producers (for example the transfer workers) so consumers can fail soft and report
/// the reason instead of receiving a broken promise.
class ResourceCreationError : public std::runtime_error
{
  public:
    explicit ResourceCreationError(std::string message) : std::runtime_error(std::move(message))
    {
    }
};
} // namespace star::core
