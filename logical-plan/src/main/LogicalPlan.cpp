#include "datatypes/src/main/Schema.cpp"
#include <memory>
#include <vector>
#include <string>
#include <sstream>


class LogicalPlan {
    public:

        virtual ~LogicalPlan() = default;

        /**
         * Returns the schema of the data that will be produced by this logical plan.
         */
        virtual Schema schema();

        /**
         * Returns the children (inputs) of this logical plan. This method is used to
         * enable use of the visitor pattern to walk a query tree.
         */
        virtual std::vector<std::shared_ptr<LogicalPlan>> children();

        virtual std::string pretty() const {
            return format(*this);
        }
};


std::string format(const LogicalPlan& plan, int indent = 0) {
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
