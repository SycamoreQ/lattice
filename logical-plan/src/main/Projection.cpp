#include "LogicalPlan.hpp"
#include "LogicalExpr.hpp"
#include "Schema.hpp"
#include <memory>
#include <string>
#include <vector>
#include <sstream>

/**
 * Logical plan representing a projection (evaluating a list of expressions)
 * against an input
 */
class Projection : public LogicalPlan {
public:
    std::shared_ptr<LogicalPlan> input;
    std::vector<std::shared_ptr<LogicalExpr>> expr;

    // Explicit constructor using std::move for efficiency
    Projection(std::shared_ptr<LogicalPlan> input_plan, std::vector<std::shared_ptr<LogicalExpr>> expressions)
        : input(std::move(input_plan)), expr(std::move(expressions)) {}

    Schema schema() const override {
        std::vector<Field> fields;
        fields.reserve(expr.size());

        for (const auto& e : expr) {
            // toField expects a reference to a LogicalPlan, so we dereference input with *
            fields.push_back(e->toField(*input));
        }

        return Schema(fields);
    }

    std::vector<std::shared_ptr<LogicalPlan>> children() const override {
        // Return a vector containing the single input plan
        return {input};
    }

    std::string toString() const override {
        std::stringstream ss;
        ss << "Projection: ";

        // Loop through the expressions and manually join them with a comma
        for (size_t i = 0; i < expr.size(); ++i) {
            ss << expr[i]->toString();
            if (i < expr.size() - 1) {
                ss << ", ";
            }
        }

        return ss.str();
    }
};
