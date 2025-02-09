#pragma once

#include "BufferModifier.hpp"
#include "Vertex.hpp"
#include "Vertex.hpp"

#include <memory>
#include <vector>

// namespace star{
//     class MeshVertexInfo : public BufferModifier {
//         public:
//             MeshVertexInfo(std::unique_ptr<std::vector<Vertex>> vertices)
//                 : vertices(vertices), BufferModifier(
//                     VmaAllocationCreateFlags::VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
//                     VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO,
//                     vk::BufferUsageFlagBits::eVertexBuffer,
//                     sizeof(Vertex),
//                     vertices->size(),
//                     vk::SharingMode::eConcurrent,
//                     0
//                 ){}

//         protected:
    
//         virtual void writeBufferData(StarBuffer& buffer); 

//         private:
//         std::unique_ptr<std::vector<Vertex>> vertices;
//     }
// }