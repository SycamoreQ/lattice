#pragma once

#include <string>
#include <vector>
#include <memory>
#include <sstream>

#include "Schema.hpp"

/**
 * Represents a node in the logical query plan.
 */
class LogicalPlan {
public:
    virtual ~LogicalPlan() = default;

    /**
     * Returns the schema of the data that this plan node will produce.
     */
    virtual Schema schema() const = 0;

    /**
     * Returns the child plan nodes of this node.
     */
    virtual std::vector<std::shared_ptr<LogicalPlan>> children() const = 0;

    /**
     * Returns a string representation of this node (useful for EXPLAIN).
     */
    virtual std::string toString() const = 0;
};

/**
 * Formats a logical plan as a hierarchical tree string.
 */
inline std::string format(const LogicalPlan& plan, int indent = 0) {
    std::stringstream b;
    b << std::string(indent, '\t');
    b << plan.toString() << "\n";

    for (const auto& child : plan.children()) {
        if (child) {
            b << format(*child, indent + 1);
        }
    }

    return b.str();
}
