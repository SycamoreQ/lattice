#pragma once

#include <string>
#include <vector>
#include <memory>
#include <stdexcept>
#include "DataSource.hpp"
#include "Schema.hpp"
#include "RecordBatch.hpp"

class InMemoryReaderIterator : public RecordBatchIterator {
private:
    Schema projectedSchema;
    std::vector<RecordBatch> data;
    std::vector<int> projectionIndices;
    bool hasProjection;
    size_t currentIndex = 0;

public:
    InMemoryReaderIterator(Schema schema, std::vector<RecordBatch> batches, std::vector<int> indices)
        : projectedSchema(std::move(schema)),
          data(std::move(batches)),
          projectionIndices(std::move(indices)),
          hasProjection(!projectionIndices.empty()) {}

    bool hasNext() override {
        return currentIndex < data.size();
    }

    RecordBatch next() override {
        if (!hasNext()) {
            throw std::out_of_range("Cannot read past the end of InMemoryReaderIterator");
        }

        const RecordBatch& batch = data[currentIndex++];

        // If no projection is specified, return the entire batch
        if (!hasProjection) {
            return batch;
        }

        // Otherwise, construct a new RecordBatch with only the projected columns
        std::vector<std::shared_ptr<ColumnVector>> projected_fields;
        projected_fields.reserve(projectionIndices.size());

        for (int idx : projectionIndices) {
            projected_fields.push_back(batch.field(idx));
        }

        return RecordBatch(projectedSchema, projected_fields);
    }
};

class InMemoryDataSource : public DataSource {
private:
    Schema dataSchema;
    std::vector<RecordBatch> data;

public:
    InMemoryDataSource(Schema s, std::vector<RecordBatch> d)
        : dataSchema(std::move(s)), data(std::move(d)) {}

    Schema schema() const override {
        return dataSchema;
    }

    std::shared_ptr<RecordBatchIterator> scan(const std::vector<std::string>& projection) override {
        if (projection.empty()) {
            return std::make_shared<InMemoryReaderIterator>(dataSchema, data, std::vector<int>{});
        }

        std::vector<int> projectionIndices;
        projectionIndices.reserve(projection.size());

        for (const auto& name : projection) {
            int idx = -1;
            for (size_t i = 0; i < dataSchema.fields.size(); ++i) {
                if (dataSchema.fields[i].name == name) {
                    idx = static_cast<int>(i);
                    break;
                }
            }

            if (idx == -1) {
                throw std::invalid_argument("Column not found in InMemoryDataSource: " + name);
            }
            projectionIndices.push_back(idx);
        }

        Schema projectedSchema = dataSchema.select(projection);

        return std::make_shared<InMemoryReaderIterator>(
            std::move(projectedSchema),
            data,
            std::move(projectionIndices)
        );
    }
};
