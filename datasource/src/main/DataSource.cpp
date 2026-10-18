class RecordBatchIterator {
public:
    virtual ~RecordBatchIterator() = default;

    // Returns true if there is another batch available
    virtual bool hasNext() = 0;

    // Returns the next batch
    virtual RecordBatch next() = 0;
};

class DataSource {
public:
    virtual ~DataSource() = default;
    virtual Schema schema() const = 0;

    // Now returns a lazy iterator instead of a giant vector
    virtual std::shared_ptr<RecordBatchIterator> scan(const std::vector<std::string>& projection) = 0;
};
