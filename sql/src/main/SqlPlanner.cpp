#pragma once

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <stdexcept>
#include <regex>
#include <iostream>
#include "spdlog/spdlog.h"
#include <arrow/api.h>
#include "LogicalExpr.hpp"
#include "DataFrame.hpp"
#include "SqlExpr.hpp"

class SqlException : public std::logic_error {
public:
    explicit SqlException(const std::string& message) : std::logic_error(message) {}
};

class SqlPlanner {
public:
    std::shared_ptr<DataFrame> createDataFrame(
        std::shared_ptr<SqlSelect> select,
        const std::map<std::string, std::shared_ptr<DataFrame>>& tables) {

        auto it = tables.find(select->tableName);
        if (it == tables.end()) {
            throw SqlException("No table named '" + select->tableName + "'");
        }
        std::shared_ptr<DataFrame> table = it->second;

        // translate projection sql expressions into logical expressions
        std::vector<std::shared_ptr<LogicalExpr>> projectionExpr;
        projectionExpr.reserve(select->projection.size());
        for (const auto& expr : select->projection) {
            projectionExpr.push_back(createLogicalExpr(expr, table));
        }

        // build a list of columns referenced in the projection
        std::set<std::string> columnNamesInProjection = getReferencedColumns(projectionExpr);

        int aggregateExprCount = std::count_if(
            projectionExpr.begin(), projectionExpr.end(),
            [this](const std::shared_ptr<LogicalExpr>& expr) { return isAggregateExpr(expr); }
        );

        if (aggregateExprCount == 0 && !select->groupBy.empty()) {
            throw SqlException("GROUP BY without aggregate expressions is not supported");
        }

        std::set<std::string> columnNamesInSelection = getColumnsReferencedBySelection(select, table);

        std::shared_ptr<DataFrame> plan = table;

        if (aggregateExprCount == 0) {
            return planNonAggregateQuery(
                select, plan, projectionExpr, columnNamesInSelection, columnNamesInProjection);
        } else {
            std::vector<std::shared_ptr<LogicalExpr>> projection;
            std::vector<std::shared_ptr<AggregateExpr>> aggrExpr;
            int numGroupCols = select->groupBy.size();
            int groupCount = 0;

            for (const auto& expr : projectionExpr) {
                // C++ equivalent of Kotlin's when(expr) for type checking
                if (auto agg = std::dynamic_pointer_cast<AggregateExpr>(expr)) {

                    projection.push_back(std::make_shared<ColumnIndex>(numGroupCols + aggrExpr.size()));
                    aggrExpr.push_back(agg);

                } else if (auto alias = std::dynamic_pointer_cast<Alias>(expr)) {

                    auto innerExpr = alias->expr;
                    if (!std::dynamic_pointer_cast<AggregateExpr>(innerExpr)) {
                        throw SqlException("Alias in aggregate query must wrap an aggregate expression");
                    }
                    projection.push_back(std::make_shared<Alias>(
                        std::make_shared<ColumnIndex>(numGroupCols + aggrExpr.size()), alias->alias));
                    aggrExpr.push_back(std::dynamic_pointer_cast<AggregateExpr>(innerExpr));

                } else {

                    projection.push_back(std::make_shared<ColumnIndex>(groupCount));
                    groupCount += 1;

                }
            }

            plan = planAggregateQuery(projectionExpr, select, columnNamesInSelection, plan, aggrExpr);
            plan = plan->project(projection);

            if (select->having) {
                plan = plan->filter(createLogicalExpr(select->having, plan));
            }
            if (select->limit.has_value()) {
                plan = plan->limit(select->limit.value());
            }
            return plan;
        }
    }

private:
    bool isAggregateExpr(const std::shared_ptr<LogicalExpr>& expr) {
        if (std::dynamic_pointer_cast<AggregateExpr>(expr)) {
            return true;
        }
        if (auto alias = std::dynamic_pointer_cast<Alias>(expr)) {
            return std::dynamic_pointer_cast<AggregateExpr>(alias->expr) != nullptr;
        }
        return false;
    }

    std::shared_ptr<DataFrame> planNonAggregateQuery(
        std::shared_ptr<SqlSelect> select,
        std::shared_ptr<DataFrame> df,
        const std::vector<std::shared_ptr<LogicalExpr>>& projectionExpr,
        const std::set<std::string>& columnNamesInSelection,
        const std::set<std::string>& columnNamesInProjection) {

        std::shared_ptr<DataFrame> plan = df;

        if (!select->selection) {
            plan = plan->project(projectionExpr);
            if (select->limit.has_value()) {
                plan = plan->limit(select->limit.value());
            }
            return plan;
        }

        std::set<std::string> missing;
        std::set_difference(
            columnNamesInSelection.begin(), columnNamesInSelection.end(),
            columnNamesInProjection.begin(), columnNamesInProjection.end(),
            std::inserter(missing, missing.begin())
        );

        if (missing.empty()) {
            plan = plan->project(projectionExpr);
            plan = plan->filter(createLogicalExpr(select->selection, plan));
        } else {
            size_t n = projectionExpr.size();
            std::vector<std::shared_ptr<LogicalExpr>> extendedProj = projectionExpr;
            for (const auto& m : missing) {
                extendedProj.push_back(std::make_shared<Column>(m));
            }

            plan = plan->project(extendedProj);
            plan = plan->filter(createLogicalExpr(select->selection, plan));

            std::vector<std::shared_ptr<LogicalExpr>> finalProj;
            for (size_t i = 0; i < n; ++i) {
                finalProj.push_back(std::make_shared<Column>(plan->schema().fields[i].name));
            }
            plan = plan->project(finalProj);
        }

        if (select->limit.has_value()) {
            plan = plan->limit(select->limit.value());
        }

        return plan;
    }

    std::shared_ptr<DataFrame> planAggregateQuery(
        const std::vector<std::shared_ptr<LogicalExpr>>& projectionExpr,
        std::shared_ptr<SqlSelect> select,
        const std::set<std::string>& columnNamesInSelection,
        std::shared_ptr<DataFrame> df,
        const std::vector<std::shared_ptr<AggregateExpr>>& aggregateExpr) {

        std::shared_ptr<DataFrame> plan = df;

        std::vector<std::shared_ptr<LogicalExpr>> projectionWithoutAggregates;
        for (const auto& expr : projectionExpr) {
            if (!isAggregateExpr(expr)) {
                projectionWithoutAggregates.push_back(expr);
            }
        }

        std::vector<std::shared_ptr<LogicalExpr>> aggAsLogical(aggregateExpr.begin(), aggregateExpr.end());
        std::set<std::string> columnNamesInAggregates = getReferencedColumns(aggAsLogical);

        if (select->selection) {
            std::set<std::string> columnNamesInProjectionWithoutAggregates = getReferencedColumns(projectionWithoutAggregates);

            std::set<std::string> allRequiredColumns = columnNamesInProjectionWithoutAggregates;
            allRequiredColumns.insert(columnNamesInSelection.begin(), columnNamesInSelection.end());
            allRequiredColumns.insert(columnNamesInAggregates.begin(), columnNamesInAggregates.end());

            std::set<std::string> missing;
            std::set_difference(
                allRequiredColumns.begin(), allRequiredColumns.end(),
                columnNamesInProjectionWithoutAggregates.begin(), columnNamesInProjectionWithoutAggregates.end(),
                std::inserter(missing, missing.begin())
            );

            if (missing.empty()) {
                plan = plan->project(projectionWithoutAggregates);
                plan = plan->filter(createLogicalExpr(select->selection, plan));
            } else {
                std::vector<std::shared_ptr<LogicalExpr>> extendedProj = projectionWithoutAggregates;
                for (const auto& m : missing) {
                    extendedProj.push_back(std::make_shared<Column>(m));
                }
                plan = plan->project(extendedProj);
                plan = plan->filter(createLogicalExpr(select->selection, plan));
            }
        }

        std::vector<std::shared_ptr<LogicalExpr>> groupByExpr;
        for (const auto& gb : select->groupBy) {
            groupByExpr.push_back(createLogicalExpr(gb, plan));
        }

        return plan->aggregate(groupByExpr, aggregateExpr);
    }

    std::set<std::string> getColumnsReferencedBySelection(
        std::shared_ptr<SqlSelect> select, std::shared_ptr<DataFrame> table) {

        std::set<std::string> accumulator;
        if (select->selection) {
            auto filterExpr = createLogicalExpr(select->selection, table);
            visit(filterExpr, accumulator);

            std::set<std::string> validColumnNames;
            for (const auto& field : table->schema().fields) {
                validColumnNames.insert(field.name);
            }

            for (auto it = accumulator.begin(); it != accumulator.end(); ) {
                if (validColumnNames.find(*it) == validColumnNames.end()) {
                    it = accumulator.erase(it);
                } else {
                    ++it;
                }
            }
        }
        return accumulator;
    }

    std::set<std::string> getReferencedColumns(const std::vector<std::shared_ptr<LogicalExpr>>& exprs) {
        std::set<std::string> accumulator;
        for (const auto& expr : exprs) {
            visit(expr, accumulator);
        }
        return accumulator;
    }

    void visit(std::shared_ptr<LogicalExpr> expr, std::set<std::string>& accumulator) {
        if (auto col = std::dynamic_pointer_cast<Column>(expr)) {
            accumulator.insert(col->name);
        } else if (auto alias = std::dynamic_pointer_cast<Alias>(expr)) {
            visit(alias->expr, accumulator);
        } else if (auto bin = std::dynamic_pointer_cast<BinaryExpr>(expr)) {
            visit(bin->l, accumulator);
            visit(bin->r, accumulator);
        } else if (auto agg = std::dynamic_pointer_cast<AggregateExpr>(expr)) {
            visit(agg->expr, accumulator);
        }
    }

    std::shared_ptr<LogicalExpr> createLogicalExpr(std::shared_ptr<SqlExpr> expr, std::shared_ptr<DataFrame> input) {
        if (auto id = std::dynamic_pointer_cast<SqlIdentifier>(expr)) {
            return std::make_shared<Column>(id->id);
        }
        else if (auto str = std::dynamic_pointer_cast<SqlString>(expr)) {
            return std::make_shared<LiteralString>(str->value);
        }
        else if (auto lval = std::dynamic_pointer_cast<SqlLong>(expr)) {
            return std::make_shared<LiteralLong>(lval->value);
        }
        else if (auto dval = std::dynamic_pointer_cast<SqlDouble>(expr)) {
            return std::make_shared<LiteralDouble>(dval->value);
        }
        else if (auto bin = std::dynamic_pointer_cast<SqlBinaryExpr>(expr)) {
            auto l = createLogicalExpr(bin->left, input);
            auto r = createLogicalExpr(bin->right, input);

            std::string op = bin->op;
            if (op == "=") return std::make_shared<Eq>(l, r);
            if (op == "!=" || op == "<>") return std::make_shared<Neq>(l, r);
            if (op == ">") return std::make_shared<Gt>(l, r);
            if (op == ">=") return std::make_shared<GtEq>(l, r);
            if (op == "<") return std::make_shared<Lt>(l, r);
            if (op == "<=") return std::make_shared<LtEq>(l, r);
            if (op == "AND") return std::make_shared<And>(l, r);
            if (op == "OR") return std::make_shared<Or>(l, r);

            if (op == "+") return std::make_shared<Add>(l, r);
            if (op == "-") return std::make_shared<Subtract>(l, r);
            if (op == "*") return std::make_shared<Multiply>(l, r);
            if (op == "/") return std::make_shared<Divide>(l, r);
            if (op == "%") return std::make_shared<Modulus>(l, r);

            throw SqlException("Invalid operator " + op);
        }
        else if (auto alias = std::dynamic_pointer_cast<SqlAlias>(expr)) {
            return std::make_shared<Alias>(createLogicalExpr(alias->expr, input), alias->alias->id);
        }
        else if (auto castExpr = std::dynamic_pointer_cast<SqlCast>(expr)) {
            return std::make_shared<CastExpr>(createLogicalExpr(castExpr->expr, input), parseDataType(castExpr->dataType->id));
        }
        else if (auto func = std::dynamic_pointer_cast<SqlFunction>(expr)) {
            std::string idStr = func->id;
            std::transform(idStr.begin(), idStr.end(), idStr.begin(), ::toupper);

            if (idStr == "MIN" || idStr == "MAX" || idStr == "SUM" || idStr == "AVG") {
                if (func->args.empty()) {
                    throw SqlException(idStr + "() requires an argument");
                }
                auto arg = createLogicalExpr(func->args.front(), input);

                if (idStr == "MIN") return std::make_shared<Min>(arg);
                if (idStr == "MAX") return std::make_shared<Max>(arg);
                if (idStr == "SUM") return std::make_shared<Sum>(arg);
                if (idStr == "AVG") return std::make_shared<Avg>(arg);
            }
            else if (idStr == "COUNT") {
                if (func->args.empty()) {
                    throw SqlException("COUNT() requires an argument, use COUNT(*) to count all rows");
                }
                auto arg = func->args.front();
                if (auto ident = std::dynamic_pointer_cast<SqlIdentifier>(arg)) {
                    if (ident->id == "*") {
                        return std::make_shared<Count>(std::make_shared<LiteralLong>(1));
                    }
                }
                return std::make_shared<Count>(createLogicalExpr(arg, input));
            }
            throw SqlException("Invalid aggregate function: " + func->id);
        }

        throw SqlException("Cannot create logical expression from sql expression");
    }

    std::shared_ptr<arrow::DataType> parseDataType(const std::string& id) {
        if (id == "double") {
            return arrow::float64();
        }
        throw SqlException("Invalid data type " + id);
    }
};
