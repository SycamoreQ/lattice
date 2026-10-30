#include <cppconn/exception.h>
#include <future>
#include <memory>
#include "spdlog/spdlog.h"
#include "PrattParser.hpp"
#include "Tokens.hpp"
#include <stdexcept>
#include <string>

class IllegalStateException : public std::logic_error {
public:
    explicit IllegalStateException(const std::string& message)
        : std::logic_error(message) {}
};

class SqlParser: public PrattParser{
    public:
        TokenStream tokens;

        int nextPrecedence() override {
            TokenStream token = tokens.peek();

            if (!token){
                return 0;
            }

            int precedence;
            if (std::holds_alternative<Keyword>(token.type)) {
                switch (std::get<Keyword>(token.type)) {
                    case Keyword::AS:
                    case Keyword::ASC:
                    case Keyword::DESC: return 10;
                    case Keyword::OR: return 20;
                    case Keyword::AND: return 30;
                    default: return 0;
                }
            }

            else if (std::holds_alternative<Symbol>(token.type)) {
                switch (std::get<Symbol>(token.type)) {
                    case Symbol::LT:
                    case Symbol::LT_EQ:
                    case Symbol::EQ:
                    case Symbol::BANG_EQ:
                    case Symbol::GT_EQ:
                    case Symbol::GT: return 40;
                    case Symbol::PLUS:
                    case Symbol::SUB: return 50;
                    case Symbol::STAR:
                    case Symbol::SLASH: return 60;
                    case Symbol::LEFT_PAREN: return 70;
                    default: return 0;
                }
            }

            spdlog::info("next precedence {}, returning {}", token, precedence);
            return precedence;
        };


    std::shared_ptr<SqlExpr> parsePrefix() override {
        spdlog::info("parsePrefix() next token {}", tokens.peek().value_or(Token{}).toString());


        std::optional<Token> tokenOpt = tokens.next();
        if (!tokenOpt) {
            return nullptr;
        }

        Token token = *tokenOpt;
        std::shared_ptr<SqlExpr> expr;

        if (std::holds_alternative<Keyword>(token.type)) {
            switch (std::get<Keyword>(token.type)) {
                case Keyword::SELECT:   expr = parseSelect(); break;
                case Keyword::CAST:     expr = parseCast(); break;
                case Keyword::DATE:     expr = parseDate(); break;
                case Keyword::INTERVAL: expr = parseInterval(); break;
                case Keyword::MIN:
                case Keyword::MAX:
                case Keyword::SUM:
                case Keyword::AVG:
                case Keyword::COUNT:
                case Keyword::INT:
                case Keyword::DOUBLE:
                    expr = std::make_shared<SqlIdentifier>(token.text);
                    break;

                default:
                    throw std::runtime_error("Unexpected keyword token: " + token.toString());
            }
        }

        else if (std::holds_alternative<LiteralType>(token.type)) {
            switch (std::get<LiteralType>(token.type)) {
                case LiteralType::IDENTIFIER:
                    expr = std::make_shared<SqlIdentifier>(token.text);
                    break;
                case LiteralType::STRING:
                    expr = std::make_shared<SqlString>(token.text);
                    break;
                case LiteralType::LONG:
                    expr = std::make_shared<SqlLong>(std::stoll(token.text));
                    break;
                case LiteralType::DOUBLE:
                    expr = std::make_shared<SqlDouble>(std::stod(token.text));
                    break;
                default:
                    throw std::runtime_error("Unexpected literal token: " + token.toString());
            }
        }

        else if (std::holds_alternative<Symbol>(token.type)) {
            switch (std::get<Symbol>(token.type)) {
                case Symbol::LEFT_PAREN:
                    expr = parseExpr();

                    if (!expr) {
                        throw std::runtime_error("Expected expression after '('");
                    }

                    if (!tokens.consumeTokenType(Symbol::RIGHT_PAREN)) {
                        throw std::runtime_error("Expected ')' after expression");
                    }
                    break;

                case Symbol::STAR:
                    expr = std::make_shared<SqlIdentifier>("*");
                    break;

                default:
                    throw std::runtime_error("Unexpected symbol token: " + token.toString());
            }
        } else {
            throw std::runtime_error("Unexpected token: " + token.toString());
        }
        if (expr) {
            spdlog::info("parsePrefix() returning {}", expr->toString());
        } else {
            spdlog::info("parsePrefix() returning nullptr");
        }

        return expr;
    }


    std::shared_ptr<SqlExpr> parseInfix(std::shared_ptr<SqlExpr> left, int precedence) override {
        auto tokenOpt = tokens.peek();
        if (!tokenOpt) {
            throw std::runtime_error("Unexpected EOF in parseInfix");
        }
        Token token = *tokenOpt;
        std::shared_ptr<SqlExpr> expr;

        if (std::holds_alternative<Symbol>(token.type)) {
            switch (std::get<Symbol>(token.type)) {
                case Symbol::PLUS:
                case Symbol::SUB:
                case Symbol::STAR:
                case Symbol::SLASH:
                case Symbol::EQ:
                case Symbol::GT:
                case Symbol::LT:
                case Symbol::GT_EQ:
                case Symbol::LT_EQ:
                case Symbol::BANG_EQ:
                case Symbol::LT_GT: {
                    tokens.next(); // consume the token
                    auto right = parse(precedence);
                    if (!right) throw std::runtime_error("Error parsing infix");
                    expr = std::make_shared<SqlBinaryExpr>(left, token.text, right);
                    break;
                }
                case Symbol::LEFT_PAREN: {
                    auto leftId = std::dynamic_pointer_cast<SqlIdentifier>(left);
                    if (leftId) {
                        tokens.next();

                        std::vector<std::shared_ptr<SqlExpr>> args;
                        auto nextToken = tokens.peek();

                        if (nextToken && std::holds_alternative<Symbol>(nextToken->type) &&
                            std::get<Symbol>(nextToken->type) == Symbol::RIGHT_PAREN) {
                        } else {
                            args = parseExprList();
                        }

                        if (!tokens.consumeTokenType(Symbol::RIGHT_PAREN)) {
                            throw std::runtime_error("Expected ')' after function arguments");
                        }
                        expr = std::make_shared<SqlFunction>(leftId->id, args);
                    } else {
                        throw std::runtime_error("Unexpected LPAREN");
                    }
                    break;
                }
                default:
                    throw std::runtime_error("Unexpected infix symbol token");
            }
        }
        else if (std::holds_alternative<Keyword>(token.type)) {
            switch (std::get<Keyword>(token.type)) {
                case Keyword::AS: {
                    tokens.next();
                    expr = std::make_shared<SqlAlias>(left, parseIdentifier());
                    break;
                }
                case Keyword::AND:
                case Keyword::OR: {
                    tokens.next();
                    auto right = parse(precedence);
                    if (!right) throw std::runtime_error("Error parsing infix");
                    expr = std::make_shared<SqlBinaryExpr>(left, token.text, right);
                    break;
                }
                case Keyword::ASC:
                case Keyword::DESC: {
                    tokens.next();
                    expr = std::make_shared<SqlSort>(left, std::get<Keyword>(token.type) == Keyword::ASC);
                    break;
                }
                default:
                    throw std::runtime_error("Unexpected infix keyword token");
            }
        } else {
            throw std::runtime_error("Unexpected infix token");
        }

        if (expr) spdlog::info("parseInfix() returning {}", expr->toString());
        return expr;
    }

private:
    std::vector<std::shared_ptr<SqlSort>> parseOrder() {
        std::vector<std::shared_ptr<SqlSort>> sortList;
        auto expr = parseExpr();

        while (expr) {
            std::shared_ptr<SqlSort> sort;

            if (auto id = std::dynamic_pointer_cast<SqlIdentifier>(expr)) {
                sort = std::make_shared<SqlSort>(id, true);
            }
            else if (auto s = std::dynamic_pointer_cast<SqlSort>(expr)) {
                sort = s;
            }
            else {
                throw std::runtime_error("Unexpected expression after order by.");
            }
            sortList.push_back(sort);

            auto nextToken = tokens.peek();
            if (nextToken && std::holds_alternative<Symbol>(nextToken->type) &&
                std::get<Symbol>(nextToken->type) == Symbol::COMMA) {
                tokens.next();
            } else {
                break;
            }
            expr = parseExpr();
        }
        return sortList;
    }

    std::shared_ptr<SqlCast> parseCast() {
        if (!tokens.consumeTokenType(Symbol::LEFT_PAREN)) {
            throw std::runtime_error("Expected '(' after CAST");
        }

        auto expr = parseExpr();
        if (!expr) throw std::runtime_error("Expected expression in CAST");

        auto aliasExpr = std::dynamic_pointer_cast<SqlAlias>(expr);
        if (!aliasExpr) {
            throw std::runtime_error("Expected 'AS type' in CAST expression");
        }

        if (!tokens.consumeTokenType(Symbol::RIGHT_PAREN)) {
            throw std::runtime_error("Expected ')' after CAST expression");
        }

        return std::make_shared<SqlCast>(aliasExpr->expr, aliasExpr->alias);
    }

    std::shared_ptr<SqlDate> parseDate() {
        auto tokenOpt = tokens.next();
        if (!tokenOpt) throw std::runtime_error("Expected date string after DATE keyword");

        Token token = *tokenOpt;
        if (!std::holds_alternative<LiteralType>(token.type) ||
            std::get<LiteralType>(token.type) != LiteralType::STRING) {
            throw std::runtime_error("Expected date string after DATE keyword");
        }
        return std::make_shared<SqlDate>(token.text);
    }

    std::shared_ptr<SqlInterval> parseInterval() {
        auto tokenOpt = tokens.next();
        if (!tokenOpt) throw std::runtime_error("Expected interval string after INTERVAL keyword");

        Token token = *tokenOpt;
        if (!std::holds_alternative<LiteralType>(token.type) ||
            std::get<LiteralType>(token.type) != LiteralType::STRING) {
            throw std::runtime_error("Expected interval string after INTERVAL keyword");
        }
        return std::make_shared<SqlInterval>(token.text);
    }

    std::shared_ptr<SqlSelect> parseSelect() {
        auto projection = parseExprList();

        if (tokens.consumeKeyword("FROM")) {
            auto tableExpr = parseExpr();
            if (!tableExpr) throw std::runtime_error("Expected table name after FROM");

            auto tableId = std::dynamic_pointer_cast<SqlIdentifier>(tableExpr);
            if (!tableId) {
                throw std::runtime_error("Expected table name after FROM");
            }

            // parse optional WHERE clause
            std::shared_ptr<SqlExpr> filterExpr = nullptr;
            if (tokens.consumeKeyword("WHERE")) {
                filterExpr = parseExpr();
            }

            // parse optional GROUP BY clause
            std::vector<std::shared_ptr<SqlExpr>> groupBy;
            if (tokens.consumeKeywords({"GROUP", "BY"})) {
                groupBy = parseExprList();
            }

            // parse optional HAVING clause
            std::shared_ptr<SqlExpr> havingExpr = nullptr;
            if (tokens.consumeKeyword("HAVING")) {
                havingExpr = parseExpr();
            }

            // parse optional ORDER BY clause
            std::vector<std::shared_ptr<SqlSort>> orderBy;
            if (tokens.consumeKeywords({"ORDER", "BY"})) {
                orderBy = parseOrder();
            }

            // parse optional LIMIT clause
            std::optional<int> limit;
            if (tokens.consumeKeyword("LIMIT")) {
                auto limitExpr = parseExpr();
                if (!limitExpr) throw std::runtime_error("Expected limit value after LIMIT");

                auto limitLong = std::dynamic_pointer_cast<SqlLong>(limitExpr);
                if (limitLong) {
                    limit = static_cast<int>(limitLong->value);
                } else {
                    throw std::runtime_error("LIMIT must be a numeric value");
                }
            }

            return std::make_shared<SqlSelect>(
                projection, filterExpr, groupBy, orderBy, havingExpr, limit, tableId->id);
        } else {
            throw std::runtime_error("Expected FROM keyword");
        }
    }

    std::vector<std::shared_ptr<SqlExpr>> parseExprList() {
        spdlog::info("parseExprList()");
        std::vector<std::shared_ptr<SqlExpr>> list;
        auto expr = parseExpr();

        while (expr) {
            list.push_back(expr);
            auto nextToken = tokens.peek();
            if (nextToken && std::holds_alternative<Symbol>(nextToken->type) &&
                std::get<Symbol>(nextToken->type) == Symbol::COMMA) {
                tokens.next();
            } else {
                break;
            }
            expr = parseExpr();
        }
        return list;
    }

    std::shared_ptr<SqlExpr> parseExpr() {
        return parse(0);
    }

    std::shared_ptr<SqlIdentifier> parseIdentifier() {
        auto expr = parseExpr();
        if (!expr) throw std::runtime_error("Expected identifier, found EOF");

        auto idExpr = std::dynamic_pointer_cast<SqlIdentifier>(expr);
        if (idExpr) {
            return idExpr;
        } else {
            throw std::runtime_error("Expected identifier");
        }
    }
}
