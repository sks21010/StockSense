#pragma once
#include <vector>
#include <string>
#include "StockData.h"

// Abstract base class (interface) for analysis strategies
class AnalysisStrategy {
public:
    virtual ~AnalysisStrategy() = default;
    
    virtual double analyze(const std::vector<StockData>& data) = 0;
    
    virtual std::string getName() const = 0;
};