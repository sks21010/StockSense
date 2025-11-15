#include "StockDataLoader.h"
#include <iostream>

int main() {
    StockDataLoader loader;

    // Example 1: Load local CSV
    auto data = loader.LoadFromCSV("AAPL.csv");
    std::cout << "Loaded " << data.size() << " rows from CSV.\n";

    // Example 2: Fetch from API
    auto apiData = loader.LoadFromAPI("AAPL", "2024-01-01", "2025-11-10");
    std::cout << "Fetched " << apiData.size() << " rows from API.\n";

    // Print a sample
    if (!apiData.empty()) {
        const auto& d = apiData.front();
        std::cout << d.date << " | Open: " << d.open << " | Close: " << d.close << "\n";
    }

    return 0;
}
