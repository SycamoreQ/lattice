#include "LogicalExpr.hpp"
#include "LogicalPlan.hpp"
#include "Schema.hpp"
#include <memory>
#include <utility>
#include <vector>
#include <string>


class DataFrame {
public:
    // Virtual destructor is required for abstract base classes
    virtual ~DataFrame() = default;

    // Apply a projection
    virtual std::shared_ptr<DataFrame> project(std::vector<std::shared_ptr<LogicalExpr>> expr) const = 0;

    // Apply a filter
    virtual std::shared_ptr<DataFrame> filter(std::shared_ptr<LogicalExpr> expr) const = 0;

    // Aggregate
    virtual std::shared_ptr<DataFrame> aggregate(
        std::vector<std::shared_ptr<LogicalExpr>> groupBy,
        std::vector<std::shared_ptr<AggregateExpr>> aggregateExpr) const = 0;

    virtual std::shared_ptr<DataFrame> limit(int n) const = 0;

    virtual std::shared_ptr<DataFrame> join(
        std::shared_ptr<DataFrame> right,
        JoinType joinType,
        std::vector<std::pair<std::string, std::string>> on) const = 0;

    virtual Schema schema() const = 0;

    virtual std::shared_ptr<LogicalPlan> logicalPlan() const = 0;
};


class DataFrameImpl : public DataFrame {
private:
    std::shared_ptr<LogicalPlan> plan;

public:
    explicit DataFrameImpl(std::shared_ptr<LogicalPlan> p) : plan(std::move(p)) {}

    std::shared_ptr<DataFrame> project(std::vector<std::shared_ptr<LogicalExpr>> expr) const override {
        auto new_plan = std::make_shared<Projection>(plan, std::move(expr));
        return std::make_shared<DataFrameImpl>(new_plan);
    }

    std::shared_ptr<DataFrame> filter(std::shared_ptr<LogicalExpr> expr) const override {
        auto new_plan = std::make_shared<Selection>(plan, std::move(expr));
        return std::make_shared<DataFrameImpl>(new_plan);
    }

    std::shared_ptr<DataFrame> aggregate(
        std::vector<std::shared_ptr<LogicalExpr>> groupBy,
        std::vector<std::shared_ptr<AggregateExpr>> aggregateExpr) const override {

        auto new_plan = std::make_shared<Aggregate>(plan, std::move(groupBy), std::move(aggregateExpr));
        return std::make_shared<DataFrameImpl>(new_plan);
    }

    std::shared_ptr<DataFrame> limit(int n) const override {
        auto new_plan = std::make_shared<Limit>(plan, n);
        return std::make_shared<DataFrameImpl>(new_plan);
    }

    std::shared_ptr<DataFrame> join(
        std::shared_ptr<DataFrame> right,
        JoinType joinType,
        std::vector<std::pair<std::string, std::string>> on) const override {

        // Note: right->logicalPlan() extracts the underlying plan from the other DataFrame
        auto new_plan = std::make_shared<Join>(plan, right->logicalPlan(), joinType, std::move(on));
        return std::make_shared<DataFrameImpl>(new_plan);
    }

    Schema schema() const override {
        return plan->schema();
    }

    std::shared_ptr<LogicalPlan> logicalPlan() const override {
        return plan;
    }
};
