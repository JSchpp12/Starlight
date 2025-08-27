#pragma once


#include <memory> 

namespace star::job::tasks{
using DestroyPayloadFunction = void (*)(void *);
using MovePayloadFunction = void (*)(void *, void *);

template<size_t StorageBytes, size_t StorageAlign>
struct Payload{
    DestroyPayloadFunction destroyPayloadFunction = nullptr;
    MovePayloadFunction movePayloadFunction = nullptr; 
    
    alignas(StorageAlign) std::byte m_data[StorageBytes]; 
};
}