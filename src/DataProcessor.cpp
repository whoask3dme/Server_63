#include "DataProcessor.h"
#include "Config.h"
#include <stdexcept>
#include <vector>
#include <cstdint>

Config::data_t DataProcessor::computeAverage(const std::vector<Config::data_t>& vector) {
    if (vector.empty()) {
        return 0;
    }
    
    int64_t sum = 0;
    for (Config::data_t value : vector) {
        sum += value;
    }
    
    return handleOverflow(sum, vector.size());
}

Config::data_t DataProcessor::handleOverflow(int64_t sum, size_t count) {
    int64_t average = sum / static_cast<int64_t>(count);
    
    // Проверка на переполнение для int16_t
    if (average > 32767) {
        return 32767;  // MAX для int16_t
    } else if (average < -32768) {
        return -32768; // MIN для int16_t
    }
    
    return static_cast<Config::data_t>(average);
}

std::vector<Config::data_t> DataProcessor::processVectors(
    const std::vector<std::vector<Config::data_t>>& vectors) {
    
    std::vector<Config::data_t> results;
    results.reserve(vectors.size());
    
    for (const auto& vector : vectors) {
        results.push_back(computeAverage(vector));
    }
    
    return results;
}
