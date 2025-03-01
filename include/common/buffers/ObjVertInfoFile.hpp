// #pragma once

// #include "BufferManagerRequest.hpp"
// #include "BufferMemoryTransferRequest.hpp"
// #include "FileHelpers.hpp"
// #include "VulkanVertex.hpp"
// #include "CastHelpers.hpp"

// #include <boost/atomic/atomic.hpp>
// #include <boost/thread/mutex.hpp>

// #include <tiny_obj_loader.h>

// #include <string>
// #include <vector>

// namespace star {

//     class ObjVertTransfer : public BufferMemoryTransferRequest {
//         public:
//             ObjVertTransfer(const std::string& filePath)
//                 : vertices(vertices) {}

//             BufferCreationArgs getCreateArgs() const override {
//                 return BufferCreationArgs{
//                     sizeof(Vertex),
//                     CastHelpers::size_t_to_unsigned_int(this->vertices.size()),
//                     VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
//                     VMA_MEMORY_USAGE_AUTO,
//                     vk::BufferUsageFlagBits::eVertexBuffer,
//                     vk::SharingMode::eConcurrent
//                 };
//             }

//             void writeData(StarBuffer& buffer) const override;

//         protected:
//             std::vector<Vertex> vertices;
//     };

//     class ObjVertInfo : public BufferManagerRequest {
//         public:
//             ObjVertInfo(const std::vector<Vertex> vertices)
//                 : vertices(vertices) {}

//             std::unique_ptr<BufferMemoryTransferRequest> createTransferRequest() const override;

//         protected:
//             const std::vector<Vertex> vertices;
//     };
// }