#pragma once

#include "LogicalPlan.hpp"
#include "LogicalExpr.hpp"
#include "Schema.hpp"
#include <memory>
#include <string>
#include <vector>

/** Logical plan representing a selection (a.k.a. filter) against an input */
class Selection : public LogicalPlan {
public:
    std::shared_ptr<LogicalPlan> input;
    std::shared_ptr<LogicalExpr> expr;

    // Explicit constructor using std::move for efficiency
    Selection(std::shared_ptr<LogicalPlan> input_plan, std::shared_ptr<LogicalExpr> filter_expr)
        : input(std::move(input_plan)), expr(std::move(filter_expr)) {}

    Schema schema() const override {
        // selection does not change the schema of the input
        // Use -> because input is a shared_ptr
        return input->schema();
    }

    std::vector<std::shared_ptr<LogicalPlan>> children() const override {
        // Return a vector containing the single input plan
        return {input};
    }

    std::string toString() const override {
        // Call toString() on the expression pointer
        return "Selection: " + expr->toString();
    }
};
