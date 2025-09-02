#pragma once


#include <cstdint> 
#include <functional>

namespace star::core::device
{
struct DeviceID
{
    uint8_t id; 

    bool operator==(const DeviceID &other) const{
        return id == other.id; 
    }
};
} // namespace star::core::device


namespace std
{
template <>
struct hash<star::core::device::DeviceID>
{
    std::size_t operator()(const star::core::device::DeviceID &deviceID) const noexcept
    {
        return std::hash<uint8_t>{}(deviceID.id);
    }
};
}
