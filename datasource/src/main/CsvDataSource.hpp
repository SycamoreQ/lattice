#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <stdexcept>
#include <iostream>
#include <fstream>

#include "DataSource.hpp"
#include "Schema.hpp"
#include "ArrowFieldVector.hpp"
#include "ArrowTypes.hpp"
#include "csv.hpp"

class CsvReaderIterator : public RecordBatchIterator {
private:
    Schema schema;
    std::unique_ptr<csv::CSVReader> parser;
    int batchSize;

    bool started = false;
    std::optional<RecordBatch> next_batch;

    std::optional<RecordBatch> nextBatch() {
        std::vector<csv::CSVRow> rows;
        rows.reserve(batchSize);

        csv::CSVRow row;
        // Read up to batchSize rows
        while (rows.size() < batchSize && parser->read_row(row)) {
            rows.push_back(row);
        }

        if (rows.empty()) {
            return std::nullopt; // No more data
        }

        return createBatch(rows);
    }

    RecordBatch createBatch(const std::vector<csv::CSVRow>& rows) {
        std::vector<std::unique_ptr<arrow::ArrayBuilder>> builders;

        // 1. Create Arrow builders for each field
        for (const auto& field : schema.fields) {
            builders.push_back(FieldVectorFactory::create(field.dataType, rows.size()));
        }

        // 2. Parse strings and append to Arrow builders
        for (const auto& row : rows) {
            for (size_t i = 0; i < schema.fields.size(); ++i) {
                const auto& field = schema.fields[i];
                std::string valueStr = row[field.name].get<>();

                // Trim string
                valueStr.erase(0, valueStr.find_first_not_of(" \t\r\n"));
                valueStr.erase(valueStr.find_last_not_of(" \t\r\n") + 1);

                bool is_null = valueStr.empty();
                auto type_id = field.dataType->id();
                auto* builder = builders[i].get();

                if (is_null) {
                    (void)builder->AppendNull();
                    continue;
                }

                // Downcast builder and append parsed value
                try {
                    switch (type_id) {
                        case arrow::Type::BOOLEAN:
                            (void)static_cast<arrow::BooleanBuilder*>(builder)->Append(valueStr == "true" || valueStr == "1");
                            break;
                        case arrow::Type::INT8:
                            (void)static_cast<arrow::Int8Builder*>(builder)->Append(static_cast<int8_t>(std::stoi(valueStr)));
                            break;
                        case arrow::Type::INT16:
                            (void)static_cast<arrow::Int16Builder*>(builder)->Append(static_cast<int16_t>(std::stoi(valueStr)));
                            break;
                        case arrow::Type::INT32:
                            (void)static_cast<arrow::Int32Builder*>(builder)->Append(std::stoi(valueStr));
                            break;
                        case arrow::Type::INT64:
                            (void)static_cast<arrow::Int64Builder*>(builder)->Append(std::stoll(valueStr));
                            break;
                        case arrow::Type::FLOAT:
                            (void)static_cast<arrow::FloatBuilder*>(builder)->Append(std::stof(valueStr));
                            break;
                        case arrow::Type::DOUBLE:
                            (void)static_cast<arrow::DoubleBuilder*>(builder)->Append(std::stod(valueStr));
                            break;
                        case arrow::Type::STRING:
                            (void)static_cast<arrow::StringBuilder*>(builder)->Append(valueStr);
                            break;
                        default:
                            throw IllegalStateException("Unsupported CSV read type: " + field.dataType->ToString());
                    }
                } catch (const std::exception& e) {
                    // Fallback to null if parsing fails
                    (void)builder->AppendNull();
                }
            }
        }

        // 3. Finish builders to create Arrow Arrays
        std::vector<std::shared_ptr<ColumnVector>> arrow_vectors;
        for (auto& builder : builders) {
            std::shared_ptr<arrow::Array> array;
            (void)builder->Finish(&array);
            arrow_vectors.push_back(std::make_shared<ArrowFieldVector>(array));
        }

        return RecordBatch(schema, arrow_vectors);
    }

public:
    CsvReaderIterator(Schema s, std::unique_ptr<csv::CSVReader> p, int batch)
        : schema(std::move(s)), parser(std::move(p)), batchSize(batch) {}

    bool hasNext() override {
        if (!started) {
            started = true;
            next_batch = nextBatch();
        }
        return next_batch.has_value();
    }

    RecordBatch next() override {
        if (!started) {
            hasNext();
        }

        if (!next_batch.has_value()) {
            throw std::out_of_range("Cannot read past the end of CsvReaderIterator");
        }

        RecordBatch out = std::move(next_batch.value());
        next_batch = nextBatch();
        return out;
    }
};

class CsvDataSource : public DataSource {
private:
    std::string filename;
    std::optional<Schema> provided_schema;
    bool hasHeaders;
    int batchSize;

    Schema finalSchema;

    csv::CSVFormat defaultSettings() const {
        csv::CSVFormat format;
        format.delimiter({',', ';', '\t', '|'});
        return format;
    }

    Schema inferSchema() const {
        std::ifstream file(filename);
        if (!file.good()) {
            throw std::runtime_error("File not found: " + filename);
        }

        csv::CSVFormat format = defaultSettings();
        format.header_row(hasHeaders ? 0 : -1);

        csv::CSVReader parser(filename, format);
        std::vector<std::string> headers = parser.get_col_names();

        std::vector<Field> fields;
        for (size_t i = 0; i < headers.size(); ++i) {
            std::string colName = hasHeaders ? headers[i] : "field_" + std::to_string(i + 1);
            fields.emplace_back(colName, ArrowTypes::StringType);
        }

        return Schema(fields);
    }

public:
    CsvDataSource(std::string fn, std::optional<Schema> s, bool headers, int batch)
        : filename(std::move(fn)), provided_schema(std::move(s)), hasHeaders(headers), batchSize(batch),
          finalSchema(provided_schema.has_value() ? provided_schema.value() : inferSchema()) {}

    Schema schema() const override {
        return finalSchema;
    }

    std::shared_ptr<RecordBatchIterator> scan(const std::vector<std::string>& projection) override {
        std::ifstream file(filename);
        if (!file.good()) {
            throw std::runtime_error("File not found: " + filename);
        }

        Schema readSchema = finalSchema;
        if (!projection.empty()) {
            readSchema = finalSchema.select(projection);
        }

        csv::CSVFormat format = defaultSettings();
        format.header_row(hasHeaders ? 0 : -1);

        if (!hasHeaders) {
            std::vector<std::string> col_names;
            for (const auto& field : finalSchema.fields) {
                col_names.push_back(field.
