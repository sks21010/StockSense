#pragma once
#include <string>
#include <vector>
#include "StockData.h"

class StockDataLoader {
public:
    std::vector<StockData> LoadFromCSV(const std::string& filepath);

    std::vector<StockData> LoadFromAPI(
        const std::string& ticker,
        const std::string& startDate,
        const std::string& endDate);

    // Load stock data by ticker symbol - automatically finds or downloads CSV
    std::vector<StockData> LoadByTicker(const std::string& ticker);

private:
    std::vector<StockData> ParseCSV(const std::string& csvContent);
    std::string FetchFromURL(const std::string& url);
    std::string FindTickerCSV(const std::string& ticker);
};
