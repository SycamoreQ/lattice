#include <string>
#include <vector>
#include <memory>
#include <stdexcept>
#include <iostream>
#include <arrow/api.h>
#include <arrow/io/api.h>
#include <parquet/arrow/reader.h>

#include "DataSource.hpp"
#include "Schema.hpp"
#include "RecordBatch.hpp"
#include "ArrowFieldVector.hpp"

class ParquetReaderIterator : public RecordBatchIterator {
private:
    Schema customSchema;
    std::shared_ptr<arrow::RecordBatchReader> batch_reader;

    bool started = false;
    std::shared_ptr<arrow::RecordBatch> current_arrow_batch;

    void fetchNext() {
        // ReadNext automatically reads the next row group and converts it to an Arrow RecordBatch
        arrow::Status status = batch_reader->ReadNext(&current_arrow_batch);
        if (!status.ok()) {
            throw std::runtime_error("Failed to read Parquet batch: " + status.ToString());
        }
    }

public:
    ParquetReaderIterator(Schema schema, std::shared_ptr<arrow::RecordBatchReader> reader)
        : customSchema(std::move(schema)), batch_reader(std::move(reader)) {}

    bool hasNext() override {
        if (!started) {
            started = true;
            fetchNext();
        }
        return current_arrow_batch != nullptr;
    }

    RecordBatch next() override {
        if (!started) {
            hasNext();
        }

        if (current_arrow_batch == nullptr) {
            throw std::out_of_range("Cannot read past the end of ParquetReaderIterator");
        }

        // Convert the Arrow RecordBatch into our custom kquery RecordBatch
        std::vector<std::shared_ptr<ColumnVector>> custom_columns;
        for (int i = 0; i < current_arrow_batch->num_columns(); ++i) {
            custom_columns.push_back(
                std::make_shared<ArrowFieldVector>(current_arrow_batch->column(i))
            );
        }

        RecordBatch out(customSchema, custom_columns);

        // Advance the iterator for the next call
        fetchNext();

        return out;
    }
};

class ParquetDataSource : public DataSource {
private:
    std::string filename;

    // Helper to open the file and create the Parquet reader
    std::unique_ptr<parquet::arrow::FileReader> getReader() const {
        std::shared_ptr<arrow::io::ReadableFile> infile;
        arrow::Status st = arrow::io::ReadableFile::Open(filename, arrow::default_memory_pool()).Value(&infile);

        if (!st.ok()) {
            throw std::runtime_error("Failed to open file: " + filename);
        }

        std::unique_ptr<parquet::arrow::FileReader> reader;
        parquet::arrow::OpenFile(infile, arrow::default_memory_pool(), &reader);
        return reader;
    }

public:
    explicit ParquetDataSource(std::string fn) : filename(std::move(fn)) {}

    Schema schema() const override {
        auto reader = getReader();
        std::shared_ptr<arrow::Schema> arrow_schema;

        arrow::Status st = reader->GetSchema(&arrow_schema);
        if (!st.ok()) {
            throw std::runtime_error("Failed to read Parquet schema: " + st.ToString());
        }

        return SchemaConverter::fromArrow(arrow_schema);
    }

    std::shared_ptr<RecordBatchIterator> scan(const std::vector<std::string>& projection) override {
        auto reader = getReader();
        std::shared_ptr<arrow::Schema> arrow_schema;
        reader->GetSchema(&arrow_schema);

        std::vector<int> column_indices;
        Schema final_custom_schema = SchemaConverter::fromArrow(arrow_schema);

        // Map projected column names to their integer indices in the Parquet file
        if (projection.empty()) {
            // Select all columns
            for (int i = 0; i < arrow_schema->num_fields(); ++i) {
                column_indices.push_back(i);
            }
        } else {
            // Select only projected columns
            for (const auto& name : projection) {
                int idx = arrow_schema->GetFieldIndex(name);
                if (idx == -1) {
                    throw std::invalid_argument("Column not found in Parquet file: " + name);
                }
                column_indices.push_back(idx);
            }
            final_custom_schema = final_custom_schema.select(projection);
        }

        // We want to read all row groups in the file
        std::vector<int> row_group_indices;
        for (int i = 0; i < reader->num_row_groups(); ++i) {
            row_group_indices.push_back(i);
        }

        // Let the C++ Parquet reader handle the complex vectorized decoding!
        std::shared_ptr<arrow::RecordBatchReader> batch_reader;
        arrow::Status st = reader->GetRecordBatchReader(row_group_indices, column_indices, &batch_reader);

        if (!st.ok()) {
            throw std::runtime_error("Failed to create Parquet batch reader: " + st.ToString());
        }

        return std::make_shared<ParquetReaderIterator>(std::move(final_custom_schema), std::move(batch_reader));
    }
};
