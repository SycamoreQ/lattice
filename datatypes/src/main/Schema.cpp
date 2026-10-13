#include <arrow/api.h>
#include <vector>
#include <string>
#include <memory>
#include <stdexcept>

class Field {
public:
    std::string name;
    std::shared_ptr<arrow::DataType> dataType;

    Field(std::string n, std::shared_ptr<arrow::DataType> type)
        : name(std::move(n)), dataType(std::move(type)) {}

    std::shared_ptr<arrow::Field> toArrow() const {
        return arrow::field(name, dataType, true);
    }
};

class Schema {
public:
    std::vector<Field> fields;

    explicit Schema(std::vector<Field> f) : fields(std::move(f)) {}

    std::shared_ptr<arrow::Schema> toArrow() const {
        std::vector<std::shared_ptr<arrow::Field>> arrowFields;
        arrowFields.reserve(fields.size());

        for (const auto& f : fields) {
            arrowFields.push_back(f.toArrow());
        }

        return arrow::schema(arrowFields);
    }

    Schema project(const std::vector<int>& indices) const {
        std::vector<Field> projected_fields;
        projected_fields.reserve(indices.size());

        for (int idx : indices) {
            projected_fields.push_back(fields[idx]);
        }

        return Schema(projected_fields);
    }

    Schema select(const std::vector<std::string>& names) const {
        std::vector<Field> selected_fields;

        for (const auto& name : names) {
            std::vector<Field> matches;
            for (const auto& f : fields) {
                if (f.name == name) {
                    matches.push_back(f);
                }
            }

            if (matches.size() == 1) {
                selected_fields.push_back(matches[0]);
            } else {
                throw std::invalid_argument("Field not found or ambiguous: " + name);
            }
        }

        return Schema(selected_fields);
    }
};

class SchemaConverter {
public:
    static Schema fromArrow(const std::shared_ptr<arrow::Schema>& arrowSchema) {
        std::vector<Field> fields;

        // arrowSchema->fields() returns std::vector<std::shared_ptr<arrow::Field>>
        for (const auto& arrowField : arrowSchema->fields()) {
            fields.emplace_back(arrowField->name(), arrowField->type());
        }

        return Schema(fields);
    }
};
