#include <string>
#include <vector>
#include <memory>
#include <stdexcept>
#include <sstream>
#include <algorithm>

#include "ArrowTypes.hpp"
#include "Schema.hpp"
#include "LogicalExpr.hpp"
#include "LogicalPlan.hpp"


class Column : public LogicalExpr {
public:
    std::string name;

    explicit Column(std::string n) : name(std::move(n)) {}

    Field toField(const LogicalPlan& input) const override {
        const auto& fields = input.schema().fields;
        auto it = std::find_if(fields.begin(), fields.end(),
            [this](const Field& f) { return f.name == this->name; }
        );

        if (it != fields.end()) {
            return *it;
        }

        std::stringstream err_msg;
        err_msg << "No column named '" << name << "' in [";
        for (size_t i = 0; i < fields.size(); ++i) {
            err_msg << fields[i].name;
            if (i < fields.size() - 1) err_msg << ", ";
        }
        err_msg << "]";

        throw std::invalid_argument(err_msg.str());
    }

    std::string toString() const override {
        return "#" + name;
    }
};

class ColumnIndex : public LogicalExpr {
public:
    int i;

    explicit ColumnIndex(int index) : i(index) {}

    Field toField(const LogicalPlan& input) const override {
        return input.schema().fields[i];
    }

    std::string toString() const override {
        return "#" + std::to_string(i);
    }
};

class LiteralString : public LogicalExpr {
public:
    std::string str;
    explicit LiteralString(std::string val) : str(std::move(val)) {}

    Field toField(const LogicalPlan& input) const override {
        return Field(str, ArrowTypes::StringType);
    }
    std::string toString() const override {
        return "'" + str + "'";
    }
};

class LiteralLong : public LogicalExpr {
public:
    int64_t n;
    explicit LiteralLong(int64_t val) : n(val) {}

    Field toField(const LogicalPlan& input) const override {
        return Field(std::to_string(n), ArrowTypes::Int64Type);
    }
    std::string toString() const override {
        return std::to_string(n);
    }
};

class LiteralFloat : public LogicalExpr {
public:
    float n;
    explicit LiteralFloat(float val) : n(val) {}

    Field toField(const LogicalPlan& input) const override {
        return Field(std::to_string(n), ArrowTypes::FloatType);
    }
    std::string toString() const override {
        return std::to_string(n);
    }
};

class LiteralDouble : public LogicalExpr {
public:
    double n;
    explicit LiteralDouble(double val) : n(val) {}

    Field toField(const LogicalPlan& input) const override {
        return Field(std::to_string(n), ArrowTypes::DoubleType);
    }
    std::string toString() const override {
        return std::to_string(n);
    }
};

class UnaryExpr : public LogicalExpr {
public:
    std::string name;
    std::string op;
    std::shared_ptr<LogicalExpr> expr;

    UnaryExpr(std::string n, std::string o, std::shared_ptr<LogicalExpr> e)
        : name(std::move(n)), op(std::move(o)), expr(std::move(e)) {}

    std::string toString() const override {
        return op + " " + expr->toString();
    }
};

class BinaryExpr : public LogicalExpr {
public:
    std::string name;
    std::string op;
    std::shared_ptr<LogicalExpr> l;
    std::shared_ptr<LogicalExpr> r;

    BinaryExpr(std::string n, std::string o, std::shared_ptr<LogicalExpr> left, std::shared_ptr<LogicalExpr> right)
        : name(std::move(n)), op(std::move(o)), l(std::move(left)), r(std::move(right)) {}

    std::string toString() const override {
        return l->toString() + " " + op + " " + r->toString();
    }
};

class BooleanBinaryExpr : public BinaryExpr {
public:
    BooleanBinaryExpr(std::string n, std::string o, std::shared_ptr<LogicalExpr> left, std::shared_ptr<LogicalExpr> right)
        : BinaryExpr(std::move(n), std::move(o), std::move(left), std::move(right)) {}

    Field toField(const LogicalPlan& input) const override {
        return Field(name, ArrowTypes::BooleanType);
    }
};

class MathExpr : public BinaryExpr {
public:
    MathExpr(std::string n, std::string o, std::shared_ptr<LogicalExpr> left, std::shared_ptr<LogicalExpr> right)
        : BinaryExpr(std::move(n), std::move(o), std::move(left), std::move(right)) {}

    Field toField(const LogicalPlan& input) const override {
        return Field(name, l->toField(input).dataType);
    }
};

class Not : public UnaryExpr {
public:
    explicit Not(std::shared_ptr<LogicalExpr> expr) : UnaryExpr("not", "NOT", std::move(expr)) {}
    Field toField(const LogicalPlan& input) const override { return Field(name, ArrowTypes::BooleanType); }
};

class Eq : public BooleanBinaryExpr { public: Eq(std::shared_ptr<LogicalExpr> l, std::shared_ptr<LogicalExpr> r) : BooleanBinaryExpr("eq", "=", std::move(l), std::move(r)) {} };
class Neq : public BooleanBinaryExpr { public: Neq(std::shared_ptr<LogicalExpr> l, std::shared_ptr<LogicalExpr> r) : BooleanBinaryExpr("neq", "!=", std::move(l), std::move(r)) {} };
class Gt : public BooleanBinaryExpr { public: Gt(std::shared_ptr<LogicalExpr> l, std::shared_ptr<LogicalExpr> r) : BooleanBinaryExpr("gt", ">", std::move(l), std::move(r)) {} };
class GtEq : public BooleanBinaryExpr { public: GtEq(std::shared_ptr<LogicalExpr> l, std::shared_ptr<LogicalExpr> r) : BooleanBinaryExpr("gteq", ">=", std::move(l), std::move(r)) {} };
class Lt : public BooleanBinaryExpr { public: Lt(std::shared_ptr<LogicalExpr> l, std::shared_ptr<LogicalExpr> r) : BooleanBinaryExpr("lt", "<", std::move(l), std::move(r)) {} };
class LtEq : public BooleanBinaryExpr { public: LtEq(std::shared_ptr<LogicalExpr> l, std::shared_ptr<LogicalExpr> r) : BooleanBinaryExpr("lteq", "<=", std::move(l), std::move(r)) {} };
class And : public BooleanBinaryExpr { public: And(std::shared_ptr<LogicalExpr> l, std::shared_ptr<LogicalExpr> r) : BooleanBinaryExpr("and", "AND", std::move(l), std::move(r)) {} };
class Or : public BooleanBinaryExpr { public: Or(std::shared_ptr<LogicalExpr> l, std::shared_ptr<LogicalExpr> r) : BooleanBinaryExpr("or", "OR", std::move(l), std::move(r)) {} };

class Add : public MathExpr { public: Add(std::shared_ptr<LogicalExpr> l, std::shared_ptr<LogicalExpr> r) : MathExpr("add", "+", std::move(l), std::move(r)) {} };
class Subtract : public MathExpr { public: Subtract(std::shared_ptr<LogicalExpr> l, std::shared_ptr<LogicalExpr> r) : MathExpr("subtract", "-", std::move(l), std::move(r)) {} };
class Multiply : public MathExpr { public: Multiply(std::shared_ptr<LogicalExpr> l, std::shared_ptr<LogicalExpr> r) : MathExpr("mult", "*", std::move(l), std::move(r)) {} };
class Divide : public MathExpr { public: Divide(std::shared_ptr<LogicalExpr> l, std::shared_ptr<LogicalExpr> r) : MathExpr("div", "/", std::move(l), std::move(r)) {} };
class Modulus : public MathExpr { public: Modulus(std::shared_ptr<LogicalExpr> l, std::shared_ptr<LogicalExpr> r) : MathExpr("mod", "%", std::move(l), std::move(r)) {} };

class CastExpr : public LogicalExpr {
public:
    std::shared_ptr<LogicalExpr> expr;
    std::shared_ptr<arrow::DataType> dataType;

    CastExpr(std::shared_ptr<LogicalExpr> e, std::shared_ptr<arrow::DataType> dt)
        : expr(std::move(e)), dataType(std::move(dt)) {}

    Field toField(const LogicalPlan& input) const override {
        return Field(expr->toField(input).name, dataType);
    }
    std::string toString() const override {
        return "CAST(" + expr->toString() + " AS " + dataType->ToString() + ")";
    }
};

class Alias : public LogicalExpr {
public:
    std::shared_ptr<LogicalExpr> expr;
    std::string alias;

    Alias(std::shared_ptr<LogicalExpr> e, std::string a) : expr(std::move(e)), alias(std::move(a)) {}

    Field toField(const LogicalPlan& input) const override {
        return Field(alias, expr->toField(input).dataType);
    }
    std::string toString() const override {
        return expr->toString() + " as " + alias;
    }
};


class AggregateExpr : public LogicalExpr {
public:
    std::string name;
    std::shared_ptr<LogicalExpr> expr;

    AggregateExpr(std::string n, std::shared_ptr<LogicalExpr> e)
        : name(std::move(n)), expr(std::move(e)) {}

    Field toField(const LogicalPlan& input) const override {
        return Field(name, expr->toField(input).dataType);
    }
    std::string toString() const override {
        return name + "(" + expr->toString() + ")";
    }
};

class Sum : public AggregateExpr { public: explicit Sum(std::shared_ptr<LogicalExpr> input) : AggregateExpr("SUM", std::move(input)) {} };
class Min : public AggregateExpr { public: explicit Min(std::shared_ptr<LogicalExpr> input) : AggregateExpr("MIN", std::move(input)) {} };
class Max : public AggregateExpr { public: explicit Max(std::shared_ptr<LogicalExpr> input) : AggregateExpr("MAX", std::move(input)) {} };
class Avg : public AggregateExpr { public: explicit Avg(std::shared_ptr<LogicalExpr> input) : AggregateExpr("AVG", std::move(input)) {} };

class Count : public AggregateExpr {
public:
    explicit Count(std::shared_ptr<LogicalExpr> input) : AggregateExpr("COUNT", std::move(input)) {}
    Field toField(const LogicalPlan& input) const override { return Field("COUNT", ArrowTypes::Int32Type); }
};


inline std::shared_ptr<LogicalExpr> col(const std::string& name) { return std::make_shared<Column>(name); }
inline std::shared_ptr<LogicalExpr> lit(const std::string& val) { return std::make_shared<LiteralString>(val); }
inline std::shared_ptr<LogicalExpr> lit(int64_t val) { return std::make_shared<LiteralLong>(val); }
inline std::shared_ptr<LogicalExpr> lit(double val) { return std::make_shared<LiteralDouble>(val); }

inline std::shared_ptr<LogicalExpr> cast(std::shared_ptr<LogicalExpr> expr, std::shared_ptr<arrow::DataType> dataType) {
    return std::make_shared<CastExpr>(std::move(expr), std::move(dataType));
}

inline std::shared_ptr<LogicalExpr> alias(std::shared_ptr<LogicalExpr> expr, const std::string& alias_name) {
    return std::make_shared<Alias>(std::move(expr), alias_name);
}

inline std::shared_ptr<LogicalExpr> eq(std::shared_ptr<LogicalExpr> l, std::shared_ptr<LogicalExpr> r) { return std::make_shared<Eq>(std::move(l), std::move(r)); }
inline std::shared_ptr<LogicalExpr> neq(std::shared_ptr<LogicalExpr> l, std::shared_ptr<LogicalExpr> r) { return std::make_shared<Neq>(std::move(l), std::move(r)); }
inline std::shared_ptr<LogicalExpr> gt(std::shared_ptr<LogicalExpr> l, std::shared_ptr<LogicalExpr> r) { return std::make_shared<Gt>(std::move(l), std::move(r)); }
inline std::shared_ptr<LogicalExpr> gteq(std::shared_ptr<LogicalExpr> l, std::shared_ptr<LogicalExpr> r) { return std::make_shared<GtEq>(std::move(l), std::move(r)); }
inline std::shared_ptr<LogicalExpr> lt(std::shared_ptr<LogicalExpr> l, std::shared_ptr<LogicalExpr> r) { return std::make_shared<Lt>(std::move(l), std::move(r)); }
inline std::shared_ptr<LogicalExpr> lteq(std::shared_ptr<LogicalExpr> l, std::shared_ptr<LogicalExpr> r) { return std::make_shared<LtEq>(std::move(l), std::move(r)); }

inline std::shared_ptr<LogicalExpr> add(std::shared_ptr<LogicalExpr> l, std::shared_ptr<LogicalExpr> r) { return std::make_shared<Add>(std::move(l), std::move(r)); }
inline std::shared_ptr<LogicalExpr> sub(std::shared_ptr<LogicalExpr> l, std::shared_ptr<LogicalExpr> r) { return std::make_shared<Subtract>(std::move(l), std::move(r)); }
inline std::shared_ptr<LogicalExpr> mult(std::shared_ptr<LogicalExpr> l, std::shared_ptr<LogicalExpr> r) { return std::make_shared<Multiply>(std::move(l), std::move(r)); }
inline std::shared_ptr<LogicalExpr> div(std::shared_ptr<LogicalExpr> l, std::shared_ptr<LogicalExpr> r) { return std::make_shared<Divide>(std::move(l), std::move(r)); }

inline std::shared_ptr<LogicalExpr> max(std::shared_ptr<LogicalExpr> expr) { return std::make_shared<Max>(std::move(expr)); }
inline std::shared_ptr<LogicalExpr> min(std::shared_ptr<LogicalExpr> expr) { return std::make_shared<Min>(std::move(expr)); }
inline std::shared_ptr<LogicalExpr> sum(std::shared_ptr<LogicalExpr> expr) { return std::make_shared<Sum>(std::move(expr)); }
inline std::shared_ptr<LogicalExpr> count(std::shared_ptr<LogicalExpr> expr) { return std::make_shared<Count>(std::move(expr)); }
