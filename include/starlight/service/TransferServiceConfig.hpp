#pragma once

#include "common/ConfigFile.hpp"
#include "enums/Enums.hpp"

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>

namespace star::service
{
struct TransferServiceConfig
{
    star::TransferQueueCapacity highPrioritySize{star::TransferQueueCapacity::Low};
    star::TransferQueueCapacity standardPrioritySize{star::TransferQueueCapacity::Low};
    size_t standardPriorityWorkerCount{0};

    static TransferServiceConfig fromConfigFile()
    {
        TransferServiceConfig cfg{};

        const auto highStr =
            star::ConfigFile::getString(star::Config_Settings::transfer_high_priority_queue_size, "low");
        const auto standardStr =
            star::ConfigFile::getString(star::Config_Settings::transfer_standard_priority_queue_size, "low");

        cfg.highPrioritySize = parseCapacity(highStr);
        cfg.standardPrioritySize = parseCapacity(standardStr);
        cfg.standardPriorityWorkerCount =
            star::ConfigFile::getUint32(star::Config_Settings::transfer_standard_priority_worker_count, 0);

        return cfg;
    }

    static star::TransferQueueCapacity parseCapacity(std::string_view value)
    {
        std::string lowered{value};
        std::transform(lowered.begin(), lowered.end(), lowered.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

        if (lowered == "medium")
            return star::TransferQueueCapacity::Medium;
        if (lowered == "high")
            return star::TransferQueueCapacity::High;
        if (lowered == "ultra")
            return star::TransferQueueCapacity::Ultra;

        return star::TransferQueueCapacity::Low;
    }

    static constexpr size_t capacityToSize(star::TransferQueueCapacity capacity)
    {
        switch (capacity)
        {
        case star::TransferQueueCapacity::Low:
            return 64;
        case star::TransferQueueCapacity::Medium:
            return 128;
        case star::TransferQueueCapacity::High:
            return 256;
        case star::TransferQueueCapacity::Ultra:
            return 512;
        }
        return 64;
    }
};
} // namespace star::service