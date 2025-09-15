#pragma once

#include "Enums.hpp"

namespace star
{
struct Handle
{
    static Handle getDefault()
    {
        return Handle(Handle_Type::defaultHandle);
    }

    Handle() : Handle(Handle_Type::defaultHandle, 0)
    {
    }
    Handle(Handle_Type type) : Handle(type, 0)
    {
    }
    Handle(Handle_Type type, uint32_t id) : type(type), id(id), global_id(id_counter++)
    {
    }

    Handle(const Handle &) = default;
    Handle &operator=(const Handle &) = default;
    ~Handle() = default;

    bool operator==(const Handle &other) const
    {
        return global_id == other.global_id && id == other.id && type == other.type;
    }

    bool isInitialized() const
    {
        return type != Handle_Type::null;
    }

    uint32_t getID() const
    {
        return id;
    }
    uint32_t getGlobalID() const
    {
        return global_id;
    }
    Handle_Type getType() const
    {
        return type;
    }
    bool isSameElementAs(const Handle &other) const
    {
        return type == other.type && id == other.id;
    }

    void setType(Handle_Type newType)
    {
        type = newType;
    }
    void setID(uint32_t newID)
    {
        id = newID;
    }

  private:
    static uint32_t id_counter;
    Handle_Type type = Handle_Type::null;
    uint32_t id = 0;
    uint32_t global_id = 0;
};

struct HandleHash
{
    size_t operator()(const Handle &handle) const noexcept;
};

inline bool operator<(const Handle &lhs, const Handle &rhs)
{
    return lhs.getGlobalID() < rhs.getGlobalID();
}
} // namespace star
