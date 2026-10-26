#include "LogicalPlan.hpp"
#include "LogicalExpr.hpp"
#include "Schema.hpp"
#include <memory>
#include <string>
#include <vector>

class Selection : public LogicalPlan {
public:
    std::shared_ptr<LogicalPlan> input;
    std::shared_ptr<LogicalExpr> expr;

    Selection(std::shared_ptr<LogicalPlan> input_plan, std::shared_ptr<LogicalExpr> filter_expr)
        : input(std::move(input_plan)), expr(std::move(filter_expr)) {}

    Schema schema() const override {
        return input->schema();
    }

    std::vector<std::shared_ptr<LogicalPlan>> children() const override {
        return {input};
    }

    std::string toString() const override {
        return "Selection: " + expr->toString();
    }
};
