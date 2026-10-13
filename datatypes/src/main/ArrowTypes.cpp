#include <arrow/api.h>

namespace ArrowTypes {
    inline const std::shared_ptr<arrow::DataType> BooleanType = arrow::boolean();
    inline const std::shared_ptr<arrow::DataType> Int8Type = arrow::int8();
    inline const std::shared_ptr<arrow::DataType> Int16Type = arrow::int16();
    inline const std::shared_ptr<arrow::DataType> Int32Type = arrow::int32();
    inline const std::shared_ptr<arrow::DataType> Int64Type = arrow::int64();
    inline const std::shared_ptr<arrow::DataType> UInt8Type = arrow::uint8();
    inline const std::shared_ptr<arrow::DataType> UInt16Type = arrow::uint16();
    inline const std::shared_ptr<arrow::DataType> UInt32Type = arrow::uint32();
    inline const std::shared_ptr<arrow::DataType> UInt64Type = arrow::uint64();
    inline const std::shared_ptr<arrow::DataType> FloatType = arrow::float32();
    inline const std::shared_ptr<arrow::DataType> DoubleType = arrow::float64();
    inline const std::shared_ptr<arrow::DataType> StringType = arrow::utf8();
    inline const std::shared_ptr<arrow::DataType> BinaryType = arrow::binary();
    inline const std::shared_ptr<arrow::DataType> DateDayType = arrow::date32();
    inline const std::shared_ptr<arrow::DataType> IntervalDayTimeType = arrow::day_time_interval();
}
