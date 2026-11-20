#include "internals/OneTimeCommandBufferSynchronizer.hpp"

#include "core/device/managers/Semaphore.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <unordered_map>

namespace
{
using Synchronizer = star::OneTimeCommandBufferSynchronizer;
using SemaphoreRecord = star::core::device::manager::SemaphoreRecord;

/// @brief Minimal manager stand-in: returns a record per Handle id without touching Vulkan.
class FakeSemaphore : public star::core::device::manager::Semaphore
{
  public:
    SemaphoreRecord *get(const star::Handle &handle) override
    {
        return &records[handle.getID()];
    }

    std::unordered_map<uint32_t, SemaphoreRecord> records;
};

star::Handle Record(uint32_t id)
{
    return star::Handle{.type = 1, .id = id};
}

star::StarCommandBuffer::Data ManualData()
{
    return star::StarCommandBuffer::Data{.semaphore = vk::Semaphore{},
                                         .waitPoints = vk::PipelineStageFlagBits::eFragmentShader,
                                         .signaledValue = std::nullopt};
}
} // namespace

TEST(OneTimeCommandBufferSynchronizer, DuplicateRegistrationMergesStages)
{
    FakeSemaphore semaphores;
    semaphores.records[1].timelineValue = 3;
    Synchronizer sync;

    sync.registerWait(Record(1), vk::PipelineStageFlagBits::eVertexShader);
    sync.registerWait(Record(1), vk::PipelineStageFlagBits::eFragmentShader);
    sync.prepRender(semaphores);

    const auto waits = sync.drain();
    ASSERT_EQ(waits.size(), 1u);
    EXPECT_EQ(static_cast<VkPipelineStageFlags>(waits[0].waitPoints),
              static_cast<VkPipelineStageFlags>(vk::PipelineStageFlagBits::eVertexShader |
                                                vk::PipelineStageFlagBits::eFragmentShader));
    ASSERT_TRUE(waits[0].signaledValue.has_value());
    EXPECT_EQ(*waits[0].signaledValue, 3u);
}

TEST(OneTimeCommandBufferSynchronizer, DistinctRecordsProduceDistinctWaits)
{
    FakeSemaphore semaphores;
    semaphores.records[1].timelineValue = 5;
    semaphores.records[2].timelineValue = 9;
    Synchronizer sync;

    sync.registerWait(Record(1), vk::PipelineStageFlagBits::eVertexShader);
    sync.registerWait(Record(2), vk::PipelineStageFlagBits::eFragmentShader);
    sync.prepRender(semaphores);

    const auto waits = sync.drain();
    ASSERT_EQ(waits.size(), 2u);
    ASSERT_TRUE(waits[0].signaledValue.has_value());
    ASSERT_TRUE(waits[1].signaledValue.has_value());
    EXPECT_EQ(*waits[0].signaledValue, 5u);
    EXPECT_EQ(*waits[1].signaledValue, 9u);
}

TEST(OneTimeCommandBufferSynchronizer, BinaryRecordHasNoSignaledValue)
{
    FakeSemaphore semaphores;
    Synchronizer sync;

    sync.registerWait(Record(1), vk::PipelineStageFlagBits::eFragmentShader);
    sync.prepRender(semaphores);

    const auto waits = sync.drain();
    ASSERT_EQ(waits.size(), 1u);
    EXPECT_FALSE(waits[0].signaledValue.has_value());
}

TEST(OneTimeCommandBufferSynchronizer, ManualDataIsAppendedAfterResolvedWaits)
{
    FakeSemaphore semaphores;
    semaphores.records[1].timelineValue = 4;
    Synchronizer sync;

    sync.registerWait(Record(1), vk::PipelineStageFlagBits::eVertexShader);
    sync.prepRender(semaphores);
    sync.manual_add_data(ManualData());

    const auto waits = sync.drain();
    ASSERT_EQ(waits.size(), 2u);
    EXPECT_EQ(static_cast<VkPipelineStageFlags>(waits[1].waitPoints),
              static_cast<VkPipelineStageFlags>(vk::PipelineStageFlagBits::eFragmentShader));
}

TEST(OneTimeCommandBufferSynchronizer, DrainIsOneShot)
{
    FakeSemaphore semaphores;
    semaphores.records[1].timelineValue = 2;
    Synchronizer sync;

    sync.registerWait(Record(1), vk::PipelineStageFlagBits::eVertexShader);
    sync.prepRender(semaphores);
    (void)sync.drain();

    sync.prepRender(semaphores);
    EXPECT_TRUE(sync.drain().empty());
}
