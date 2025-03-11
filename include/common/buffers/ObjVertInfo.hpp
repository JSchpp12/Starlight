#pragma once

#include "ManagerController_RenderResource_Buffer.hpp"
#include "TransferRequest_Memory.hpp"
#include "FileHelpers.hpp"
#include "VulkanVertex.hpp"
#include "CastHelpers.hpp"

#include <boost/atomic/atomic.hpp>
#include <boost/thread/mutex.hpp>
#include <tiny_obj_loader.h>

#include <string>

namespace star{
    namespace TransferRequest{
        class ObjVert : public TransferRequest::Memory<StarBuffer::BufferCreationArgs>{
            public:
            // ObjVertTransfer(const uint32_t& numVerts) : numVerts(numVerts){}
    
            ObjVert(std::vector<Vertex> vertices) 
                : vertices(vertices){}
    
            // ObjVertTransfer(const std::string& filePath, const size_t& shapeIndex, 
            //     const size_t& numVerts, boost::atomic<bool>& readingDoneFlag, 
            //     AABBInterThreadData& aabbData) 
            // : filePath(filePath), shapeIndex(shapeIndex), 
            // numVerts(static_cast<uint32_t>(numVerts)), readingDoneFlag(readingDoneFlag), 
            // aabbData(aabbData) {
            //     assert(!readingDoneFlag.load() && "Reading flag is already set");
            // }
    
            StarBuffer::BufferCreationArgs getCreateArgs() const override{
                return StarBuffer::BufferCreationArgs{
                    sizeof(Vertex), 
                    CastHelpers::size_t_to_unsigned_int(this->vertices.size()),
                    VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
                    VMA_MEMORY_USAGE_AUTO,
                    vk::BufferUsageFlagBits::eVertexBuffer,
                    vk::SharingMode::eConcurrent
                };
            }
    
            void writeData(StarBuffer& buffer) const override; 
            
            protected:
            std::vector<Vertex> vertices;
    
            // boost::atomic<bool>& readingDoneFlag; 
            // const std::string filePath; 
            // const uint32_t shapeIndex;
            // AABBInterThreadData& aabbData;
    
            virtual std::vector<Vertex> getVertices() const; 
        };    
    }
    class ObjVertInfo : public ManagerController::RenderResource::Buffer{
        public:
        // ObjVertInfo(const std::string& filePath, AABBInterThreadData& aabbData) : filePath(filePath), aabbData(aabbData) {
        //     assert(star::FileHelpers::FileExists(filePath) && "Provided file does not exist");
        // }

        ObjVertInfo(const std::vector<Vertex> vertices) : vertices(vertices){}

        // ObjVertInfo(const std::string& filePath, AABBInterThreadData& aabbData, const size_t& shapeIndex, const size_t& numVerts); 

        std::unique_ptr<TransferRequest::Memory<StarBuffer::BufferCreationArgs>> createTransferRequest() const override;

        protected:
        const std::vector<Vertex> vertices;

        // static size_t getNumVerts(const std::string& filePath, const size_t& shapeIndex);
    };
}