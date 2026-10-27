#pragma once

#include "LogicalPlan.hpp"
#include "Schema.hpp"
#include <memory>
#include <vector>
#include <string>
#include <utility>
#include <unordered_set>
#include <sstream>

enum class JoinType {
    Inner,
    Left,
    Right
};

class Join : public LogicalPlan {
public:
    std::shared_ptr<LogicalPlan> left;
    std::shared_ptr<LogicalPlan> right;
    JoinType joinType;
    std::vector<std::pair<std::string, std::string>> on;

    Join(std::shared_ptr<LogicalPlan> l,
         std::shared_ptr<LogicalPlan> r,
         JoinType type,
         std::vector<std::pair<std::string, std::string>> join_on)
        : left(std::move(l)), right(std::move(r)), joinType(type), on(std::move(join_on)) {}

    Schema schema() const override {
        std::unordered_set<std::string> duplicateKeys;
        for (const auto& pair : on) {
            if (pair.first == pair.second) {
                duplicateKeys.insert(pair.first);
            }
        }

        std::vector<Field> fields;
        const auto& leftFields = left->schema().fields;
        const auto& rightFields = right->schema().fields;

        // 2. Handle the different join types
        if (joinType == JoinType::Inner || joinType == JoinType::Left) {

            // Add all left fields
            fields.insert(fields.end(), leftFields.begin(), leftFields.end());

            // Add right fields that are NOT in the duplicateKeys set
            for (const auto& f : rightFields) {
                if (duplicateKeys.find(f.name) == duplicateKeys.end()) {
                    fields.push_back(f);
                }
            }

        } else if (joinType == JoinType::Right) {

            // Add left fields that are NOT in the duplicateKeys set
            for (const auto& f : leftFields) {
                if (duplicateKeys.find(f.name) == duplicateKeys.end()) {
                    fields.push_back(f);
                }
            }

            // Add all right fields
            fields.insert(fields.end(), rightFields.begin(), rightFields.end());
        }

        return Schema(fields);
    }

    std::vector<std::shared_ptr<LogicalPlan>> children() const override {
        return {left, right};
    }

    std::string toString() const override {
        std::string typeStr;
        switch (joinType) {
            case JoinType::Inner: typeStr = "Inner"; break;
            case JoinType::Left:  typeStr = "Left"; break;
            case JoinType::Right: typeStr = "Right"; break;
        }

        std::stringstream ss;
        ss << "Join: type=" << typeStr << ", on=[";

        for (size_t i = 0; i < on.size(); ++i) {
            ss << "(" << on[i].first << ", " << on[i].second << ")";
            if (i < on.size() - 1) {
                ss << ", ";
            }
        }
        ss << "]";

        return ss.str();
    }
};
