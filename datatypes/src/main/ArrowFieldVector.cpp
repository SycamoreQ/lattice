#include <arrow/api.h>
#include <memory>
#include <stdexcept>
#include <any>
#include "ArrowTypes.hpp"

class IllegalStateException : public std::logic_error {
public:
    explicit IllegalStateException(const std::string& message)
        : std::logic_error(message) {}
};

class ArrowAllocator {
public:
    static arrow::MemoryPool* getRootPool() {
        return arrow::default_memory_pool();
    }
};

class ColumnVector {
public:
    virtual ~ColumnVector() = default;
    virtual std::shared_ptr<arrow::DataType> getType() const = 0;
    virtual std::any getValue(int i) const = 0;
    virtual int size() const = 0;
};

class FieldVectorFactory {
public:
    static std::unique_ptr<arrow::ArrayBuilder> create(
        const std::shared_ptr<arrow::DataType>& arrowType, int initialCapacity) {

        std::unique_ptr<arrow::ArrayBuilder> builder;
        arrow::Status status = arrow::MakeBuilder(
            ArrowAllocator::getRootPool(), arrowType, &builder);

        if (!status.ok()) {
            throw IllegalStateException("Unsupported type: " + arrowType->ToString());
        }

        if (initialCapacity > 0) {
            // Ignore status for brevity, but best practice is to check it
            (void)builder->Reserve(initialCapacity);
        }

        return builder;
    }
};

class ArrowFieldVector : public ColumnVector {
private:
    std::shared_ptr<arrow::Array> field;

public:
    explicit ArrowFieldVector(std::shared_ptr<arrow::Array> array)
        : field(std::move(array)) {}

    std::shared_ptr<arrow::DataType> getType() const override {
        return field->type();
    }

    int size() const override {
        return field->length();
    }

    std::any getValue(int i) const override {
        if (field->IsNull(i)) {
            return {};
        }

        switch (field->type_id()) {
            case arrow::Type::BOOLEAN:
                return std::static_pointer_cast<arrow::BooleanArray>(field)->Value(i);
            case arrow::Type::INT8:
                return std::static_pointer_cast<arrow::Int8Array>(field)->Value(i);
            case arrow::Type::INT16:
                return std::static_pointer_cast<arrow::Int16Array>(field)->Value(i);
            case arrow::Type::INT32:
                return std::static_pointer_cast<arrow::Int32Array>(field)->Value(i);
            case arrow::Type::INT64:
                return std::static_pointer_cast<arrow::Int64Array>(field)->Value(i);
            case arrow::Type::FLOAT:
                return std::static_pointer_cast<arrow::FloatArray>(field)->Value(i);
            case arrow::Type::DOUBLE:
                return std::static_pointer_cast<arrow::DoubleArray>(field)->Value(i);
            case arrow::Type::STRING:
                return std::static_pointer_cast<arrow::StringArray>(field)->GetString(i);
            case arrow::Type::DATE32:
                return std::static_pointer_cast<arrow::Date32Array>(field)->Value(i);
            default:
                throw IllegalStateException("Unsupported vector type: " + field->type()->ToString());
        }
    }

    void close() {
        field.reset();
    }
};
