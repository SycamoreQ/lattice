#pragma once

#include <string>
#include <cstdint>

class ShuffleLocation {
public:
    std::string jobUuid;
    uint32_t stageId;
    uint32_t partitionId;
    std::string executionUuid;

    // Constructor declaration
    ShuffleLocation(std::string job_uuid, uint32_t stage_id, uint32_t partition_id, std::string execution_uuid);

    // Overload the == operator
    bool operator==(const ShuffleLocation& other) const;
};
