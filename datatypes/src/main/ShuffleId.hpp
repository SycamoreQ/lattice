#pragma once

#include <string>
#include <cstdint>

class ShuffleId {
public:
    std::string jobUuid;
    uint32_t stageId;
    uint32_t partitionId;

    // Constructor declaration
    ShuffleId(std::string job_uuid, uint32_t stage_id, uint32_t partition_id);

    // Overload the == operator to mimic Kotlin's data class equality check
    bool operator==(const ShuffleId& other) const;
};
