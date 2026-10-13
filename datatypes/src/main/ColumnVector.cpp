#include <any>
#include <arrow/api.h>
#include "ArrowTypes.hpp"


class ColumnVector {
public:
    virtual ArrowTypes getType();
    virtual std::any getValue(int i);
    virtual int size();
};
