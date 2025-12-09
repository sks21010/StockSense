#include "StockDataLoader.h"
#include "StockAnalytics.h"
#include <iostream>
#include <iomanip>

int main() {
    StockDataLoader loader;
    StockAnalytics analytics;

    auto data = loader.LoadFromCSV("AAPL.csv");
    std::cout << "Loaded " << data.size() << " rows from CSV.\n";

    if (data.empty()) {
        std::cout << "No data, exiting.\n";
        return 0;
    }

    auto returns = analytics.DailyReturns(data);
    auto vol20   = analytics.RollingVolatility(data, 20);
    auto sma20   = analytics.SimpleMovingAverage(data, 20);

    ReturnStats stats = analytics.ComputeReturnStats(returns);
    double sharpe     = analytics.SharpeRatio(returns, 0.0);  // assume 0 risk-free
    double ytd        = analytics.YearToDatePerformance(data);
    double maxDD      = analytics.MaxDrawdown(data);

    std::vector<double> midBB, upBB, lowBB;
    analytics.BollingerBands(data, 20, midBB, upBB, lowBB, 2.0);

    const auto& last = data.back();
    size_t lastIdx = data.size() - 1;

    std::cout << std::fixed << std::setprecision(4);

    std::cout << "\n--- Analysis Summary ---\n";
    std::cout << "Latest date:                " << last.date << "\n";
    std::cout << "Latest close:               " << last.close << "\n";
    std::cout << "20-day SMA:                 " << sma20[lastIdx] << "\n";
    std::cout << "20-day volatility:          " << vol20[lastIdx] << "\n";

    std::cout << "\nDaily Return Stats:\n";
    std::cout << "  Mean:                      " << stats.mean << "\n";
    std::cout << "  Stddev:                    " << stats.stddev << "\n";
    std::cout << "  Max (best day):            " << stats.max << "\n";
    std::cout << "  Min (worst day):           " << stats.min << "\n";

    std::cout << "\nSharpe Ratio (daily):        " << sharpe << "\n";
    std::cout << "Year-to-date performance:    " << ytd * 100.0 << "%\n";
    std::cout << "Max drawdown:                " << maxDD * 100.0 << "%\n";

    std::cout << "\nBollinger Bands (20d, 2σ) on latest date:\n";
    std::cout << "  Middle (SMA):              " << midBB[lastIdx] << "\n";
    std::cout << "  Upper band:                " << upBB[lastIdx] << "\n";
    std::cout << "  Lower band:                " << lowBB[lastIdx] << "\n";

    // Simple trend label
    if (last.close > sma20[lastIdx]) {
        std::cout << "\nTrend signal:                UP (price above 20d SMA)\n";
    } else {
        std::cout << "\nTrend signal:                DOWN/FLAT (price at or below 20d SMA)\n";
    }

    return 0;
}
