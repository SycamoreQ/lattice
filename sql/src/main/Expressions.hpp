#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <sstream>

class SqlExpr {
public:
    virtual ~SqlExpr() = default;

    virtual std::string toString() const = 0;
};

class SqlIdentifier : public SqlExpr {
public:
    std::string id;

    explicit SqlIdentifier(std::string i) : id(std::move(i)) {}

    std::string toString() const override {
        return id;
    }
};

class SqlBinaryExpr : public SqlExpr {
public:
    std::shared_ptr<SqlExpr> l;
    std::string op;
    std::shared_ptr<SqlExpr> r;

    SqlBinaryExpr(std::shared_ptr<SqlExpr> left, std::string operatorStr, std::shared_ptr<SqlExpr> right)
        : l(std::move(left)), op(std::move(operatorStr)), r(std::move(right)) {}

    std::string toString() const override {
        return l->toString() + " " + op + " " + r->toString();
    }
};

class SqlString : public SqlExpr {
public:
    std::string value;

    explicit SqlString(std::string v) : value(std::move(v)) {}

    std::string toString() const override {
        return "'" + value + "'";
    }
};

class SqlLong : public SqlExpr {
public:
    int64_t value; // Use int64_t for Kotlin's 64-bit Long

    explicit SqlLong(int64_t v) : value(v) {}

    std::string toString() const override {
        return std::to_string(value);
    }
};

class SqlDouble : public SqlExpr {
public:
    double value;

    explicit SqlDouble(double v) : value(v) {}

    std::string toString() const override {
        return std::to_string(value);
    }
};

class SqlDate : public SqlExpr {
public:
    std::string value;

    explicit SqlDate(std::string v) : value(std::move(v)) {}

    std::string toString() const override {
        return "DATE '" + value + "'";
    }
};

class SqlInterval : public SqlExpr {
public:
    std::string value;

    explicit SqlInterval(std::string v) : value(std::move(v)) {}

    std::string toString() const override {
        return "INTERVAL '" + value + "'";
    }
};

class SqlFunction : public SqlExpr {
public:
    std::string id;
    std::vector<std::shared_ptr<SqlExpr>> args;

    SqlFunction(std::string i, std::vector<std::shared_ptr<SqlExpr>> a)
        : id(std::move(i)), args(std::move(a)) {}

    std::string toString() const override {
        return id;
    }
};

class SqlAlias : public SqlExpr {
public:
    std::shared_ptr<SqlExpr> expr;
    std::shared_ptr<SqlIdentifier> alias;

    SqlAlias(std::shared_ptr<SqlExpr> e, std::shared_ptr<SqlIdentifier> a)
        : expr(std::move(e)), alias(std::move(a)) {}

    std::string toString() const override {
        return expr->toString() + " AS " + alias->toString();
    }
};

class SqlCast : public SqlExpr {
public:
    std::shared_ptr<SqlExpr> expr;
    std::shared_ptr<SqlIdentifier> dataType;

    SqlCast(std::shared_ptr<SqlExpr> e, std::shared_ptr<SqlIdentifier> dt)
        : expr(std::move(e)), dataType(std::move(dt)) {}

    std::string toString() const override {
        return "CAST(" + expr->toString() + " AS " + dataType->toString() + ")";
    }
};

class SqlSort : public SqlExpr {
public:
    std::shared_ptr<SqlExpr> expr;
    bool asc;

    SqlSort(std::shared_ptr<SqlExpr> e, bool a) : expr(std::move(e)), asc(a) {}

    std::string toString() const override {
        return expr->toString() + (asc ? " ASC" : " DESC");
    }
};

class SqlRelation : public SqlExpr {
public:
    virtual ~SqlRelation() = default;
};

class SqlSelect : public SqlRelation {
public:
    std::vector<std::shared_ptr<SqlExpr>> projection;
    std::shared_ptr<SqlExpr> selection;             // nullable
    std::vector<std::shared_ptr<SqlExpr>> groupBy;
    std::vector<std::shared_ptr<SqlExpr>> orderBy;  // Kotlin used List<SqlExpr> here
    std::shared_ptr<SqlExpr> having;                // nullable
    std::optional<int> limit;                       // nullable
    std::string tableName;

    SqlSelect(
        std::vector<std::shared_ptr<SqlExpr>> proj,
        std::shared_ptr<SqlExpr> sel,
        std::vector<std::shared_ptr<SqlExpr>> group,
        std::vector<std::shared_ptr<SqlExpr>> order,
        std::shared_ptr<SqlExpr> hav,
        std::optional<int> lim,
        std::string table
    ) : projection(std::move(proj)),
        selection(std::move(sel)),
        groupBy(std::move(group)),
        orderBy(std::move(order)),
        having(std::move(hav)),
        limit(lim),
        tableName(std::move(table)) {}

    std::string toString() const override {
        std::stringstream ss;
        ss << "SELECT ... FROM " << tableName;
        if (selection) ss << " WHERE " << selection->toString();
        if (limit.has_value()) ss << " LIMIT " << limit.value();
        return ss.str();
    }
};
