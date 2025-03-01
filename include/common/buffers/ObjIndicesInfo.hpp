#pragma once

#include "BufferManagerRequest.hpp"
#include "BufferMemoryTransferRequest.hpp"
#include "CastHelpers.hpp"

#include <vector>

namespace star{
    class ObjIndicesTransfer : public BufferMemoryTransferRequest {
        public:
        ObjIndicesTransfer(const std::vector<uint32_t>& indices) : indices(indices){}

        void writeData(StarBuffer& buffer) const override;


		BufferCreationArgs getCreateArgs() const override{
			return BufferCreationArgs{
				sizeof(uint32_t),
				CastHelpers::size_t_to_unsigned_int(this->indices.size()),
                VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
				VMA_MEMORY_USAGE_AUTO, 
				vk::BufferUsageFlagBits::eIndexBuffer, 
				vk::SharingMode::eConcurrent
			};
		}

        protected:
        const std::vector<uint32_t> indices;
    };

    class ObjIndicesInfo : public BufferManagerRequest{
        public:
        ObjIndicesInfo(const std::vector<uint32_t>& indices) : indices(indices){};

        std::unique_ptr<BufferMemoryTransferRequest> createTransferRequest() const override;

        protected:
        const std::vector<uint32_t> indices;
    };
}