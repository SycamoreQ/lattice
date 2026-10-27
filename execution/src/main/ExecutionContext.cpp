#include <string>
#include <map>
#include <memory>
#include <vector>
#include "DataFrame.hpp"
#include "LogicalPlan.hpp"
#include "DataSource.hpp"
#include "CsvDataSource.hpp"
#include "RecordBatch.hpp"

class ExecutionContext {
public:
    std::map<std::string, std::string> settings;
    int batchSize;

    explicit ExecutionContext(std::map<std::string, std::string> config)
        : settings(std::move(config)) {

        auto it = settings.find("kquery.csv.batchSize");
        if (it != settings.end()) {
            batchSize = std::stoi(it->second);
        } else {
            batchSize = 1024;
        }
    }

    std::shared_ptr<DataFrame> sql(const std::string& sqlQuery) {
        auto tokens = SqlTokenizer(sqlQuery).tokenize();

        auto ast = std::dynamic_pointer_cast<SqlSelect>(SqlParser(tokens).parse());

        auto df = SqlPlanner().createDataFrame(ast, tables);

        return std::make_shared<DataFrameImpl>(df->logicalPlan());
    }

    std::shared_ptr<DataFrame> csv(const std::string& filename) {
        auto dataSource = std::make_shared<CsvDataSource>(filename, nullptr, true, batchSize);
        auto scan = std::make_shared<Scan>(filename, dataSource, std::vector<std::string>{});

        return std::make_shared<DataFrameImpl>(scan);
    }

    void registerTable(const std::string& tablename, std::shared_ptr<DataFrame> df) {
        tables[tablename] = std::move(df);
    }

    void registerDataSource(const std::string& tablename, std::shared_ptr<DataSource> datasource) {
        auto scan = std::make_shared<Scan>(tablename, std::move(datasource), std::vector<std::string>{});
        registerTable(tablename, std::make_shared<DataFrameImpl>(scan));
    }

    void registerCsv(const std::string& tablename, const std::string& filename) {
        registerTable(tablename, csv(filename));
    }

    std::shared_ptr<RecordBatchIterator> execute(std::shared_ptr<DataFrame> df) {
        return execute(df->logicalPlan());
    }

    std::shared_ptr<RecordBatchIterator> execute(std::shared_ptr<LogicalPlan> plan) {
        auto optimizedPlan = Optimizer().optimize(plan);
        auto physicalPlan = QueryPlanner().createPhysicalPlan(optimizedPlan);
        return physicalPlan->execute();
    }

private:
    /** Tables registered with this context */
    std::map<std::string, std::shared_ptr<DataFrame>> tables;
};
