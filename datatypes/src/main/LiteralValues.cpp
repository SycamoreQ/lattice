#include <arrow/api.h>
#include <memory>
#include <any>
#include <stdexcept>
#include <string>
#include "ColumnVector.cpp"

class LiteralValueVector : public ColumnVector {
private:
    std::shared_ptr<arrow::DataType> arrowType;
    std::any value;
    int vector_size;

public:
    LiteralValueVector(std::shared_ptr<arrow::DataType> type, std::any val, int size)
        : arrowType(std::move(type)), value(std::move(val)), vector_size(size) {}

    std::shared_ptr<arrow::DataType> getType() const override {
        return arrowType;
    }

    std::any getValue(int i) const override {
        if (i < 0 || i >= vector_size) {
            throw std::out_of_range("Index out of bounds: " + std::to_string(i));
        }
        return value;
    }

    int size() const override {
        return vector_size;
    }

    void close() {
        // No resources to release for literal values,
        // std::any cleans up its own memory automatically.
    }
};
