#pragma once

#include <arrow/api.h>
#include <memory>
#include <any>

class ColumnVector {
public:
    virtual ~ColumnVector() = default;

    virtual std::shared_ptr<arrow::DataType> getType() const = 0;
    virtual std::any getValue(int i) const = 0;
    virtual int size() const = 0;

    // Equivalent to Kotlin's AutoCloseable
    virtual void close() = 0;
};
