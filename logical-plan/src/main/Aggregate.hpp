#pragma once

#include "LogicalPlan.hpp"
#include "LogicalExpr.hpp"
#include "Schema.hpp"
#include <memory>
#include <vector>
#include <string>
#include <sstream>

class Aggregate : public LogicalPlan {
public:
    std::shared_ptr<LogicalPlan> input;
    std::vector<std::shared_ptr<LogicalExpr>> groupExpr;
    std::vector<std::shared_ptr<AggregateExpr>> aggregateExpr;

    Aggregate(std::shared_ptr<LogicalPlan> input_plan,
              std::vector<std::shared_ptr<LogicalExpr>> group_expr,
              std::vector<std::shared_ptr<AggregateExpr>> agg_expr)
        : input(std::move(input_plan)),
          groupExpr(std::move(group_expr)),
          aggregateExpr(std::move(agg_expr)) {}

    Schema schema() const override {
        std::vector<Field> fields;
        // Pre-allocate memory for efficiency (groupExpr size + aggregateExpr size)
        fields.reserve(groupExpr.size() + aggregateExpr.size());

        for (const auto& expr : groupExpr) {
            fields.push_back(expr->toField(*input));
        }

        for (const auto& expr : aggregateExpr) {
            fields.push_back(expr->toField(*input));
        }

        return Schema(fields);
    }

    std::vector<std::shared_ptr<LogicalPlan>> children() const override {
        return {input};
    }

    std::string toString() const override {
        std::stringstream ss;
        ss << "Aggregate: groupExpr=[";

        for (size_t i = 0; i < groupExpr.size(); ++i) {
            ss << groupExpr[i]->toString();
            if (i < groupExpr.size() - 1) {
                ss << ", ";
            }
        }

        ss << "], aggregateExpr=[";

        for (size_t i = 0; i < aggregateExpr.size(); ++i) {
            ss << aggregateExpr[i]->toString();
            if (i < aggregateExpr.size() - 1) {
                ss << ", ";
            }
        }

        ss << "]";
        return ss.str();
    }
};
