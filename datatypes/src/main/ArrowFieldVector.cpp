#include <arrow/memory_pool.h>
#include <arrow/buffer.h>
#include "ArrowTypes.hpp"
#include <arrow/api.h>
#include "Schema.hpp"
#include <vector>
#include <memory>

class ArrowAllocator {
public:
    arrow::MemoryPool rootBuffer = arrow::default_memory_pool();
};

class IllegalStateException: public std::logic_error{
    public:
        explicit IllegalStateException(const std::string& message)
            : std::logic_error(message) {}

};

class FieldVectorFactory {
public:
    std::vector<Field> create(const std::shared_ptr<arrow::DataType>& arrowType, int initialCapacity){
        std::vector<arrow::Array> fieldVector;

        if (arrowType -> id() == arrow::Type::BOOLEAN) {
            arrow::BooleanBuilder bit_vector_builder(rootBuilder);
        }

        if (arrowType -> id() == arrow::Type::INT8){
            arrow::Int8Builder tiny_int_vector_builder(rootBuilder);
        }

        if (arrowType -> id() == arrow::Type::INT16){
            arrow::Int16Builder small_int_vector_builder(rootBuilder);
        }

        if (arrowType -> id == arrow::Type::INT32){
            arrow::Int32Builder int_vector_builder(rootBuilder);
        }

        if (arrowType -> id == arrow::Type::INT64){
            arrow::Int64Builder big_int_vector_builder(rootBuilder);
        }

        if (arrowType -> id == arrow::Type::FLOAT){
            arrow::Float4Builder float_vector_builder(rootBuilder);
        }

        if (arrowType -> id == arrow::Type::DOUBLE){
            arrow::Float8Builder double_vector_builder(rootBuilder);
        }

        if (arrowType -> id == arrow::Type::STRING){
            arrow::StringBuilder string_vector_builder(rootBuilder);
        }

        if (arrowType -> id == arrow::Type::DATE32){
            arrow::Date32Builder date32_vector_builder(rootBuilder);
        }

        if (arrowType -> id == arrow::Type::INTERVAL_DAY_TIME){
            arrow::MonthDayNanoIntervalBuilder interval_nano_vector_builder(rootBuilder);
        }

        else {
            throw IllegalStateException("Type does not exist: Cannot build memory pool");
        }
    }
}
