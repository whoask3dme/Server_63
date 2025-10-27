#ifndef DATA_PROCESSOR_H
#define DATA_PROCESSOR_H

#include "Config.h"
#include <vector>

class DataProcessor {
private:
    Config::data_t computeAverage(const std::vector<Config::data_t>& vector);
    Config::data_t handleOverflow(int64_t sum, size_t count);

public:
    DataProcessor() = default;
    std::vector<Config::data_t> processVectors(
        const std::vector<std::vector<Config::data_t>>& vectors);
};

#endif
