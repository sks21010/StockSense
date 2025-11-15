#pragma once
#include <string>
#include <vector>
#include "StockData.h"

class StockDataLoader {
public:
    // Load data from a local CSV file
    std::vector<StockData> LoadFromCSV(const std::string& filepath);

    // Fetch data from Yahoo Finance API
    std::vector<StockData> LoadFromAPI(const std::string& ticker, 
                                       const std::string& startDate, 
                                       const std::string& endDate);

private:
    std::vector<StockData> ParseCSV(const std::string& csvContent);
    std::string FetchFromURL(const std::string& url);
};
