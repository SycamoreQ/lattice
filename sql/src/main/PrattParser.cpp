#pragma once

#include <memory>

class PrattParser {
public:
    virtual ~PrattParser() = default;
    std::shared_ptr<SqlExpr> parse(int precedence = 0) {
        std::shared_ptr<SqlExpr> expr = parsePrefix();

        if (!expr) {
            return nullptr;
        }

        while (precedence < nextPrecedence()) {
            expr = parseInfix(expr, nextPrecedence());
        }

        return expr;
    }

    virtual int nextPrecedence() = 0;

    virtual std::shared_ptr<SqlExpr> parsePrefix() = 0;

    virtual std::shared_ptr<SqlExpr> parseInfix(std::shared_ptr<SqlExpr> left, int precedence) = 0;
};
