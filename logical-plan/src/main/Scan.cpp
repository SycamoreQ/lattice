#include "DataSource.hpp"
#include "LogicalPlan.hpp"
#include "Schema.hpp"
#include <memory>
#include <vector>
#include <string>
#include <sstream>

class Scan : public LogicalPlan {
public:
    std::string path;
    std::shared_ptr<DataSource> dataSource;
    std::vector<std::string> projection;

    Scan(std::string p, std::shared_ptr<DataSource> ds, std::vector<std::string> proj)
        : path(std::move(p)),
          dataSource(std::move(ds)),
          projection(std::move(proj)),
          m_schema(deriveSchema()) {}

    Schema schema() const override {
        return m_schema;
    }

    std::vector<std::shared_ptr<LogicalPlan>> children() const override {
        return {};
    }

    std::string toString() const override {
        if (projection.empty()) {
            return "Scan: " + path + "; projection=None";
        } else {
            std::stringstream ss;
            ss << "Scan: " << path << "; projection=[";

            for (size_t i = 0; i < projection.size(); ++i) {
                ss << projection[i];
                if (i < projection.size() - 1) {
                    ss << ", ";
                }
            }
            ss << "]";

            return ss.str();
        }
    }

private:
    Schema m_schema;

    Schema deriveSchema() const {
        Schema s = dataSource->schema();

        if (projection.empty()) {
            return s;
        } else {
            return s.select(projection);
        }
    }
};
