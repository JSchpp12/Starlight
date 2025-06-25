#pragma once

#include "Enums.hpp"

namespace star
{
struct Handle
{
    static Handle getDefault()
    {
        Handle newHandle;
        newHandle.type = Handle_Type::defaultHandle;
        return newHandle;
    }
    Handle() : type(Handle_Type::defaultHandle), id(0), global_id(id_counter++) {};
    Handle(const Handle &other) = default;
    Handle(const Handle_Type &type) : isInit(true), type(type){}; 
    Handle(const Handle_Type &type, const uint32_t &id) : isInit(true), type(type), id(id), global_id(id_counter++) {};
    ~Handle() = default;

    bool operator==(const Handle &other) const
    {
        return global_id == other.global_id && id == other.id && type == other.type;
    }

    Handle &operator=(const Handle &other)
    {
        if (this != &other)
        {
            global_id = other.global_id;
            id = other.id;
            type = other.type;
            isInit = other.isInit;
        }
        return *this;
    }

    const bool &isInitialized()
    {
        return isInit;
    }

    const uint32_t &getID() const
    {
        return id;
    }
    const uint32_t &getGlobalID() const
    {
        return global_id;
    }
    const Handle_Type &getType() const
    {
        return type;
    }
    void setType(const Handle_Type &type)
    {
        this->isInit = true;
        this->type = type;
    }
    void setID(const size_t &id)
    {
        this->isInit = true;
        this->id = id;
    }

  private:
    static uint32_t id_counter;
    bool isInit = false;
    Handle_Type type = Handle_Type::null;
    uint32_t id = 0, global_id = 0;
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