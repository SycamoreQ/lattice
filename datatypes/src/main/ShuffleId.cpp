#include "ShuffleId.hpp"
#include <utility>

ShuffleId::ShuffleId(std::string job_uuid, uint32_t stage_id, uint32_t partition_id)
    : jobUuid(std::move(job_uuid)), stageId(stage_id), partitionId(partition_id) {}

bool ShuffleId::operator==(const ShuffleId& other) const {
    return jobUuid == other.jobUuid &&
           stageId == other.stageId &&
           partitionId == other.partitionId;
}
