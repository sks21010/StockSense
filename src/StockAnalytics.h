#pragma once
#include <vector>
#include "StockData.h"

// Summary statistics for daily returns
struct ReturnStats {
    double mean;     // average daily return
    double stddev;   // standard deviation of returns
    double min;      // worst daily return
    double max;      // best daily return
};

class StockAnalytics {
public:
    // ---- Existing basic analytics ----
    std::vector<double> SimpleMovingAverage(const std::vector<StockData>& data, int window);
    std::vector<double> DailyReturns(const std::vector<StockData>& data);
    std::vector<double> RollingVolatility(const std::vector<StockData>& data, int window);

    // ---- New extras ----

    // Compute summary stats from a vector of returns (e.g., from DailyReturns)
    ReturnStats ComputeReturnStats(const std::vector<double>& returns);

    // Sharpe ratio: (mean_return - risk_free_rate) / stddev(return)
    // risk_free_rate is per-period (daily if returns are daily). Default 0.
    double SharpeRatio(const std::vector<double>& returns, double riskFreeRate = 0.0);

    // Year-to-date performance (or full period performance if you give all data):
    // (last_close - first_close) / first_close
    double YearToDatePerformance(const std::vector<StockData>& data);

    // Maximum drawdown (worst peak-to-trough drop) over the period.
    // Returned as a negative fraction, e.g. -0.20 for -20%.
    double MaxDrawdown(const std::vector<StockData>& data);

    // Bollinger Bands:
    // middle = SMA(window)
    // upper  = SMA + numStdDev * rolling_std
    // lower  = SMA - numStdDev * rolling_std
    // For indices < window-1, values are set to NAN.
    void BollingerBands(const std::vector<StockData>& data,
                        int window,
                        std::vector<double>& middle,
                        std::vector<double>& upper,
                        std::vector<double>& lower,
                        double numStdDev = 2.0);

    // ---- Autocorrelation ----

    // Compute autocorrelation at a specific lag
    // Returns correlation coefficient between values[t] and values[t-lag]
    // Result ranges from -1 (perfect negative correlation) to +1 (perfect positive correlation)
    // Returns NaN if insufficient data or invalid lag
    double Autocorrelation(const std::vector<double>& values, int lag);

    // Compute autocorrelation function for multiple lags
    // Returns vector of autocorrelation values for lag 1, 2, 3, ..., maxLag
    // Useful for detecting patterns at different time scales (momentum, mean reversion)
    std::vector<double> AutocorrelationFunction(const std::vector<double>& values, int maxLag);

    // ---- Hurst Exponent ----

    // Compute Hurst Exponent using Rescaled Range (R/S) analysis
    // H > 0.5: Trending/persistent behavior (momentum)
    // H = 0.5: Random walk (geometric Brownian motion)
    // H < 0.5: Mean-reverting behavior (anti-persistent)
    // Returns NaN if insufficient data
    double HurstExponent(const std::vector<double>& values);
};
