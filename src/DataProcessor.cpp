#include "DataProcessor.h"
#include <iostream>
#include <limits>

Config::data_t DataProcessor::computeAverage(const std::vector<Config::data_t>& vector) {
    if (vector.empty()) {
        return 0;
    }
    
    int64_t sum = 0;
    for (auto value : vector) {
        sum += value;
    }
    
    return handleOverflow(sum, vector.size());
}

Config::data_t DataProcessor::handleOverflow(int64_t sum, size_t count) {
    int64_t average = sum / static_cast<int64_t>(count);
    
    // Проверка переполнения
    if (average > Config::MAX_VALUE) {
        return Config::MAX_VALUE;
    } else if (average < Config::MIN_VALUE) {
        return Config::MIN_VALUE;
    }
    
    return static_cast<Config::data_t>(average);
}

std::vector<Config::data_t> DataProcessor::processVectors(
    const std::vector<std::vector<Config::data_t>>& vectors) {
    
    std::vector<Config::data_t> results;
    
    for (const auto& vector : vectors) {
        results.push_back(computeAverage(vector));
    }
    
    return results;
}
