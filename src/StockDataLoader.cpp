#include "StockDataLoader.h"
#include "rapidcsv.h"
#include <curl/curl.h>
#include <sstream>
#include <iostream>

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    size_t totalSize = size * nmemb;
    output->append((char*)contents, totalSize);
    return totalSize;
}

std::vector<StockData> StockDataLoader::LoadFromCSV(const std::string& filepath) {
    std::vector<StockData> data;
    try {
        rapidcsv::Document doc(filepath, rapidcsv::LabelParams(0, -1));

        auto dates = doc.GetColumn<std::string>("Date");
        auto opens = doc.GetColumn<double>("Open");
        auto highs = doc.GetColumn<double>("High");
        auto lows = doc.GetColumn<double>("Low");
        auto closes = doc.GetColumn<double>("Close");
        auto volumes = doc.GetColumn<double>("Volume");

        for (size_t i = 0; i < dates.size(); ++i) {
            data.push_back({ dates[i], opens[i], highs[i], lows[i], closes[i], volumes[i] });
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
        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK)
            std::cerr << "CURL Error: " << curl_easy_strerror(res) << "\n";
        curl_easy_cleanup(curl);
    }
    return readBuffer;
}

std::vector<StockData> StockDataLoader::LoadFromAPI(
    const std::string& ticker, const std::string& startDate, const std::string& endDate) {

    // Can convert startDate/endDate to UNIX timestamps if you want more accuracy
    std::string url = "https://query1.finance.yahoo.com/v7/finance/download/" + ticker +
                      "?period1=1704067200&period2=1735689600&interval=1d&events=history";
    std::string csvContent = FetchFromURL(url);
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
