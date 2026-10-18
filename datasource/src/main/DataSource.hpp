#pragma once

#include <vector>
#include <string>
#include <memory>

#include "Schema.hpp"
#include "RecordBatch.hpp"

class RecordBatchIterator {
public:
    virtual ~RecordBatchIterator() = default;

    // Returns true if there is another batch available
    virtual bool hasNext() = 0;

    // Returns the next batch
    virtual RecordBatch next() = 0;
};

class DataSource {
public:
    // A virtual destructor is mandatory for C++ interfaces
    virtual ~DataSource() = default;

    /** Return the schema for the underlying data source */
    virtual Schema schema() const = 0;

    virtual std::shared_ptr<RecordBatchIterator> scan(const std::vector<std::string>& projection) = 0;
};
