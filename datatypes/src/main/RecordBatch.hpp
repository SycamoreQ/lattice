#pragma once

#include <memory>
#include <any>
#include <stdexcept>
#include <string>
#include <vector>
#include <sstream>

#include "Schema.hpp"
#include "ColumnVector.hpp"

class RecordBatch {
private:
    Schema schema;
    std::vector<std::shared_ptr<ColumnVector>> fields;

public:
    RecordBatch(Schema s, std::vector<std::shared_ptr<ColumnVector>> f)
        : schema(std::move(s)), fields(std::move(f)) {}

    int rowCount() const {
        if (fields.empty()) return 0;
        return fields.front()->size();
    }

    int columnCount() const {
        return fields.size();
    }

    std::shared_ptr<ColumnVector> field(int i) const {
        return fields[i];
    }

    std::string toCSV() const {
        std::stringstream b;
        int colCount = schema.fields.size();
        int rows = rowCount();

        for (int rowIndex = 0; rowIndex < rows; ++rowIndex) {
            for (int colIndex = 0; colIndex < colCount; ++colIndex) {
                if (colIndex > 0) {
                    b << ",";
                }

                auto v = fields[colIndex];
                std::any value = v->getValue(rowIndex);

                if (!value.has_value()) {
                    b << "null";
                }
                else if (value.type() == typeid(std::string)) {
                    b << std::any_cast<std::string>(value);
                }
                else if (value.type() == typeid(int32_t)) {
                    b << std::any_cast<int32_t>(value);
                }
                else if (value.type() == typeid(int64_t)) {
                    b << std::any_cast<int64_t>(value);
                }
                else if (value.type() == typeid(double)) {
                    b << std::any_cast<double>(value);
                }
                else if (value.type() == typeid(bool)) {
                    b << (std::any_cast<bool>(value) ? "true" : "false");
                }
                else {
                    b << "[Unsupported Type]";
                }
            }
            b << "\n";
        }
        return b.str();
    }

    std::string toString() const {
        return toCSV();
    }

    void close() {
        for (auto& f : fields) {
            if (f) {
                f->close();
            }
        }
    }
};
