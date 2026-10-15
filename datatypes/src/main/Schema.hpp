#pragma once

#include <arrow/api.h>
#include <vector>
#include <string>
#include <memory>
#include <stdexcept>
#include <algorithm>

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

        std::transform(fields.begin(), fields.end(), std::back_inserter(arrowFields),
            [](const Field& f) { return f.toArrow(); }
        );

        return arrow::schema(arrowFields);
    }

    Schema project(const std::vector<int>& indices) const {
        std::vector<Field> projected_fields;
        projected_fields.reserve(indices.size());

        std::transform(indices.begin(), indices.end(), std::back_inserter(projected_fields),
            [this](int idx) { return fields[idx]; }
        );

        return Schema(projected_fields);
    }

    Schema select(const std::vector<std::string>& names) const {
        std::vector<Field> selected_fields;
        selected_fields.reserve(names.size());

        for (const auto& name : names) {
            auto it = std::find_if(fields.begin(), fields.end(),
                [&name](const Field& f) { return f.name == name; }
            );

            if (it != fields.end()) {
                long match_count = std::count_if(fields.begin(), fields.end(),
                    [&name](const Field& f) { return f.name == name; }
                );

                if (match_count == 1) {
                    selected_fields.push_back(*it);
                } else {
                    throw std::invalid_argument("Field ambiguous: " + name);
                }
            } else {
                throw std::invalid_argument("Field not found: " + name);
            }
        }

        return Schema(selected_fields);
    }
};

class SchemaConverter {
public:
    static Schema fromArrow(const std::shared_ptr<arrow::Schema>& arrowSchema) {
        std::vector<Field> fields;

        const auto& arrow_fields = arrowSchema->fields();
        fields.reserve(arrow_fields.size());

        std::transform(arrow_fields.begin(), arrow_fields.end(), std::back_inserter(fields),
            [](const std::shared_ptr<arrow::Field>& arrowField) {
                return Field(arrowField->name(), arrowField->type());
            }
        );

        return Schema(fields);
    }
};
