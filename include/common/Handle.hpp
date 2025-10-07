#pragma once

#include "Enums.hpp"

namespace star
{
struct Handle
{
    bool operator==(const Handle &other) const
    {
        return id == other.id && type == other.type;
    }

    bool operator!() const noexcept{
        return isInitialized(); 
    }

    bool isInitialized() const noexcept
    {
        return type != Handle_Type::none;
    }

    uint32_t getID() const
    {
        return id;
    }
    Handle_Type getType() const
    {
        return type;
    }
    bool isSameElementAs(const Handle &other) const
    {
        return type == other.type && id == other.id;
    }

    Handle_Type type = Handle_Type::none;
    uint32_t id = 0;
};

struct HandleHash
{
    size_t operator()(const Handle &handle) const noexcept;
};

inline bool operator<(const Handle &lhs, const Handle &rhs)
{
    return lhs.getID() < rhs.getID();
}
} // namespace star
