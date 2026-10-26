#include "Schema.hpp"

class LogicalPlan;

/**
 * Logical Expression for use in logical query plans. The logical expression
 * provides information needed during the planning phase such as the name and
 * data type of the expression.
 */
class LogicalExpr {
public:
    virtual ~LogicalExpr() = default;

    /**
     * Return meta-data about the value that will be produced by this expression
     * when evaluated against a particular input.
     *
     * Marked 'const' because extracting metadata should not modify the expression.
     */
    virtual Field toField(const LogicalPlan& input) const = 0;
};
