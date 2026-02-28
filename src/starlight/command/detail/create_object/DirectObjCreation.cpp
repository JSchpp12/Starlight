#include "starlight/command/detail/create_object/DirectObjCreation.hpp"

namespace star::command::create_object
{
DirectObjCreation::DirectObjCreation(std::shared_ptr<StarObject> object) : m_object(std::move(object))
{
}
} // namespace star::command::create_object