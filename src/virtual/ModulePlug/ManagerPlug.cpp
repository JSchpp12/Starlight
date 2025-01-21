#include "ManagerPlug.hpp"

star::Handle& star::ManagerPlug::getHandle() const
{
	if (!this->handle)
		throw std::exception("Handle requested but not yet available");

	return *this->handle;
}

void star::ManagerPlug::setHandle(const Handle& resource)
{
	if (this->handle)
		throw std::exception("Handle registration called but a handle has already been registered");

	this->handle = std::make_unique<Handle>(resource);
}
