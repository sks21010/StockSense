#include <iostream>
#include <vector>
#include <cmath>
#include "StockData.h"
#include "StockAnalytics.h"

bool approxEqual(double a, double b, double eps = 1e-6) {
    return std::fabs(a - b) < eps;
}

int main() {
    // 5 days of fake closing prices: 100, 110, 120, 130, 140
    std::vector<StockData> data = {
        {"2025-01-01", 0,0,0,100,0},
        {"2025-01-02", 0,0,0,110,0},
        {"2025-01-03", 0,0,0,120,0},
        {"2025-01-04", 0,0,0,130,0},
        {"2025-01-05", 0,0,0,140,0}
    };

    StockAnalytics analytics;

    // ---- Test 1: Simple Moving Average (window = 3) ----
    auto sma3 = analytics.SimpleMovingAverage(data, 3);

    // Expected SMA for closing prices:
    // index 0: NAN
    // index 1: NAN
    // index 2: (100 + 110 + 120) / 3 = 110
    // index 3: (110 + 120 + 130) / 3 = 120
    // index 4: (120 + 130 + 140) / 3 = 130

    bool sma_ok =
        std::isnan(sma3[0]) &&
        std::isnan(sma3[1]) &&
        approxEqual(sma3[2], 110.0) &&
        approxEqual(sma3[3], 120.0) &&
        approxEqual(sma3[4], 130.0);

    std::cout << "SMA test: " << (sma_ok ? "PASS" : "FAIL") << "\n";

    // ---- Test 2: Daily Returns ----
    auto rets = analytics.DailyReturns(data);

    // Expected returns ( (P_t - P_{t-1}) / P_{t-1} )
    // index 0: NAN
    // index 1: (110 - 100) / 100 = 0.10
    // index 2: (120 - 110) / 110 ≈ 0.090909...
    // index 3: (130 - 120) / 120 = 0.083333...
    // index 4: (140 - 130) / 130 ≈ 0.076923...

    bool ret_ok =
        std::isnan(rets[0]) &&
        approxEqual(rets[1], 0.10) &&
        approxEqual(rets[2], 1.0/11.0) &&      // ~0.090909
        approxEqual(rets[3], 1.0/12.0) &&      // ~0.083333
        approxEqual(rets[4], 1.0/13.0);        // ~0.076923

    std::cout << "Returns test: " << (ret_ok ? "PASS" : "FAIL") << "\n";

    // ---- Test 3: Rolling Volatility (window = 3) ----
    // With monotonically increasing prices, volatility shouldn't be crazy.
    auto vol3 = analytics.RollingVolatility(data, 3);

    std::cout << "Rolling vol (3-day) values:\n";
    for (size_t i = 0; i < vol3.size(); ++i) {
        std::cout << "  day " << i << ": " << vol3[i] << "\n";
    }

    return 0;
}
