#include "StockDataLoader.h"
#include "rapidcsv.h"
#include <curl/curl.h>
#include <sstream>
#include <iostream>
#include <fstream>
#include <ctime>

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    size_t totalSize = size * nmemb;
    output->append((char*)contents, totalSize);
    return totalSize;
}

std::vector<StockData> StockDataLoader::LoadFromCSV(const std::string& filepath) {
    std::vector<StockData> data;
    try {
        rapidcsv::Document doc(filepath, rapidcsv::LabelParams(0, -1));

        // Try standard Yahoo Finance format first
        std::vector<std::string> dates;
        std::vector<double> opens, highs, lows, closes, volumes;
        
        try {
            dates = doc.GetColumn<std::string>("Date");
            opens = doc.GetColumn<double>("Open");
            highs = doc.GetColumn<double>("High");
            lows = doc.GetColumn<double>("Low");
            closes = doc.GetColumn<double>("Close");
            volumes = doc.GetColumn<double>("Volume");
        } catch (...) {
            // If that fails, the file might have different format - try to detect
            std::cerr << "Standard format failed, attempting auto-detection...\n";
            
            // Check if columns exist with different names/order
            std::vector<std::string> colNames = doc.GetColumnNames();
            
            for (const auto& col : colNames) {
                std::cout << "Found column: " << col << "\n";
            }
            
            // Handle formats with underscores (Adj_Close, etc.)
            dates = doc.GetColumn<std::string>("Date");
            opens = doc.GetColumn<double>("Open");
            highs = doc.GetColumn<double>("High");
            lows = doc.GetColumn<double>("Low");
            closes = doc.GetColumn<double>("Close");
            volumes = doc.GetColumn<double>("Volume");
        }

        // Clean date format if it has timezone info (e.g., "2010-01-04 00:00:00+00:00")
        for (size_t i = 0; i < dates.size(); ++i) {
            std::string cleanDate = dates[i];
            
            // Extract just the date part (YYYY-MM-DD) if there's a space
            size_t spacePos = cleanDate.find(' ');
            if (spacePos != std::string::npos) {
                cleanDate = cleanDate.substr(0, spacePos);
            }
            
            data.push_back({ cleanDate, opens[i], highs[i], lows[i], closes[i], volumes[i] });
        }
    } 
    catch (const std::exception& e) {
        std::cerr << "Error loading CSV: " << e.what() << std::endl;
    }
    return data;
}

std::string StockDataLoader::FetchFromURL(const std::string& url) {
    CURL* curl = curl_easy_init();
    std::string readBuffer;

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);  // Follow redirects
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);  // Skip SSL verification
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0");  // disguise web app as browser to prevent yf from blocking requests
        
        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "CURL Error: " << curl_easy_strerror(res) << "\n";
            std::cerr << "URL attempted: " << url << "\n";
        }
        curl_easy_cleanup(curl);
    }
    return readBuffer;
}

std::vector<StockData> StockDataLoader::LoadFromAPI(
    const std::string& ticker, const std::string& startDate, const std::string& endDate) {

    // Use dynamic timestamps for the past year
    std::time_t now = std::time(nullptr);
    std::time_t yearAgo = now - (365 * 24 * 60 * 60);
    
    // Try Yahoo Finance with updated endpoint
    std::string url = "https://query1.finance.yahoo.com/v7/finance/download/" + ticker +
                      "?period1=" + std::to_string(yearAgo) + 
                      "&period2=" + std::to_string(now) +
                      "&interval=1d&events=history&includeAdjustedClose=true";
    
    std::cout << "Fetching from: " << url << "\n";
    std::string csvContent = FetchFromURL(url);
    
    if (csvContent.empty() || csvContent.find("Date") == std::string::npos) {
        std::cerr << "Failed to fetch data from Yahoo Finance.\n";
        std::cerr << "Please download " << ticker << ".csv manually or use local file.\n";
        return {};
    }
    
    return ParseCSV(csvContent);
}

std::vector<StockData> StockDataLoader::ParseCSV(const std::string& csvContent) {
    std::vector<StockData> data;
    std::stringstream ss(csvContent);
    std::string line;
    bool firstLine = true;

    while (std::getline(ss, line)) {
        if (firstLine) { firstLine = false; continue; }  // skip header

        std::stringstream lineStream(line);
        std::string cell;
        std::vector<std::string> tokens;

        while (std::getline(lineStream, cell, ',')) tokens.push_back(cell);
        if (tokens.size() < 6) continue;

        try {
            data.push_back({
                tokens[0],
                std::stod(tokens[1]),
                std::stod(tokens[2]),
                std::stod(tokens[3]),
                std::stod(tokens[4]),
                std::stod(tokens[6])
            });
        } catch (...) {
            continue; // skip bad rows
        }
    }
    return data;
}

std::string StockDataLoader::FindTickerCSV(const std::string& ticker) {
    // Try common patterns for CSV files
    std::vector<std::string> patterns = {
        ticker + ".csv",
        ticker,  // If user provides full filename
        "../" + ticker + ".csv",
        "data/" + ticker + ".csv"
    };
    
    for (const auto& pattern : patterns) {
        std::ifstream test(pattern);
        if (test.good()) {
            test.close();
            return pattern;
        }
    }
    
    return "";  // Not found
}

std::vector<StockData> StockDataLoader::LoadByTicker(const std::string& ticker) {
    std::cout << "Loading data for ticker: " << ticker << "\n";
    
    // First, try to find local CSV file
    std::string csvPath = FindTickerCSV(ticker);
    
    if (!csvPath.empty()) {
        std::cout << "Found local CSV: " << csvPath << "\n";
        return LoadFromCSV(csvPath);
    }
    
    // If not found locally, fetch from API
    std::cout << "CSV not found locally, fetching from Yahoo Finance...\n";
    
    // Get current date and 1 year ago for default range
    auto data = LoadFromAPI(ticker, "2024-01-01", "2025-12-31");
    
    if (!data.empty()) {
        std::cout << "Successfully fetched " << data.size() << " data points from API\n";
    } else {
        std::cout << "Warning: No data retrieved for ticker " << ticker << "\n";
    }
    
    return data;
}
