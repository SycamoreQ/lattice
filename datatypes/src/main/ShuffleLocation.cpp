#include "ShuffleLocation.hpp"
#include <utility>

ShuffleLocation::ShuffleLocation(std::string job_uuid, uint32_t stage_id, uint32_t partition_id, std::string execution_uuid)
    : jobUuid(std::move(job_uuid)),
      stageId(stage_id),
      partitionId(partition_id),
      executionUuid(std::move(execution_uuid)) {}

bool ShuffleLocation::operator==(const ShuffleLocation& other) const {
    return jobUuid == other.jobUuid &&
           stageId == other.stageId &&
           partitionId == other.partitionId &&
           executionUuid == other.executionUuid;
}
