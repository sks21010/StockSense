#include "StockAnalytics.h"
#include <cmath>
#include <limits>
#include <algorithm>

// ------------------- Basic analytics -------------------

std::vector<double> StockAnalytics::SimpleMovingAverage(const std::vector<StockData>& data,
                                                        int window) {
    std::vector<double> sma;
    sma.reserve(data.size());

    double sum = 0.0;

    for (int i = 0; i < static_cast<int>(data.size()); ++i) {
        sum += data[i].close;

        if (i >= window) {
            sum -= data[i - window].close;
        }

        if (i >= window - 1) {
            sma.push_back(sum / window);
        } else {
            sma.push_back(std::numeric_limits<double>::quiet_NaN());
        }
    }
    return sma;
}

std::vector<double> StockAnalytics::DailyReturns(const std::vector<StockData>& data) {
    std::vector<double> ret;
    ret.reserve(data.size());

    if (data.empty()) {
        return ret;
    }

    // First day has no prior day; mark as NaN
    ret.push_back(std::numeric_limits<double>::quiet_NaN());

    for (size_t i = 1; i < data.size(); ++i) {
        if (data[i - 1].close == 0.0) {
            ret.push_back(std::numeric_limits<double>::quiet_NaN());
            continue;
        }

        double r = (data[i].close - data[i - 1].close) / data[i - 1].close;
        ret.push_back(r);
    }
    return ret;
}

std::vector<double> StockAnalytics::RollingVolatility(const std::vector<StockData>& data,
                                                      int window) {
    auto ret = DailyReturns(data);
    std::vector<double> vol;
    vol.reserve(ret.size());

    for (int i = 0; i < static_cast<int>(ret.size()); ++i) {
        if (i < window) {
            vol.push_back(std::numeric_limits<double>::quiet_NaN());
            continue;
        }

        double mean = 0.0;
        int count = 0;
        for (int j = i - window + 1; j <= i; ++j) {
            if (std::isnan(ret[j])) continue;
            mean += ret[j];
            ++count;
        }
        if (count == 0) {
            vol.push_back(std::numeric_limits<double>::quiet_NaN());
            continue;
        }
        mean /= count;

        double variance = 0.0;
        int count2 = 0;
        for (int j = i - window + 1; j <= i; ++j) {
            if (std::isnan(ret[j])) continue;
            double diff = ret[j] - mean;
            variance += diff * diff;
            ++count2;
        }
        if (count2 <= 1) {
            vol.push_back(std::numeric_limits<double>::quiet_NaN());
        } else {
            variance /= (count2 - 1);  // sample variance
            vol.push_back(std::sqrt(variance));
        }
    }
    return vol;
}

// ------------------- New extras -------------------

ReturnStats StockAnalytics::ComputeReturnStats(const std::vector<double>& returns) {
    ReturnStats stats;
    stats.mean = 0.0;
    stats.stddev = 0.0;
    stats.min = std::numeric_limits<double>::infinity();
    stats.max = -std::numeric_limits<double>::infinity();

    double sum = 0.0;
    double sumSq = 0.0;
    int count = 0;

    for (double r : returns) {
        if (std::isnan(r)) continue;
        sum += r;
        sumSq += r * r;
        stats.min = std::min(stats.min, r);
        stats.max = std::max(stats.max, r);
        ++count;
    }

    if (count == 0) {
        stats.mean = stats.stddev = stats.min = stats.max =
            std::numeric_limits<double>::quiet_NaN();
        return stats;
    }

    stats.mean = sum / count;

    double variance = (sumSq / count) - (stats.mean * stats.mean);
    if (variance < 0.0) variance = 0.0; // guard against tiny negatives
    stats.stddev = std::sqrt(variance);

    return stats;
}

double StockAnalytics::SharpeRatio(const std::vector<double>& returns, double riskFreeRate) {
    // riskFreeRate is per period (e.g., daily), same units as returns
    // Using ComputeReturnStats for mean & stddev
    ReturnStats stats = ComputeReturnStats(returns);

    if (std::isnan(stats.mean) || std::isnan(stats.stddev) || stats.stddev == 0.0) {
        return std::numeric_limits<double>::quiet_NaN();
    }

    double excessMean = stats.mean - riskFreeRate;
    return excessMean / stats.stddev;
}

double StockAnalytics::YearToDatePerformance(const std::vector<StockData>& data) {
    if (data.size() < 2) {
        return std::numeric_limits<double>::quiet_NaN();
    }

    double firstClose = data.front().close;
    double lastClose  = data.back().close;

    if (firstClose == 0.0) {
        return std::numeric_limits<double>::quiet_NaN();
    }

    return (lastClose - firstClose) / firstClose;
}

double StockAnalytics::MaxDrawdown(const std::vector<StockData>& data) {
    if (data.empty()) {
        return std::numeric_limits<double>::quiet_NaN();
    }

    double maxPeak = data.front().close;
    double maxDrawdown = 0.0; // we’ll keep this as a negative number

    for (size_t i = 1; i < data.size(); ++i) {
        double price = data[i].close;
        if (price > maxPeak) {
            maxPeak = price;
        }

        if (maxPeak > 0.0) {
            double drawdown = (price - maxPeak) / maxPeak; // will be <= 0
            if (drawdown < maxDrawdown) {
                maxDrawdown = drawdown;
            }
        }
    }

    return maxDrawdown; // e.g. -0.25 means -25% from peak
}

void StockAnalytics::BollingerBands(const std::vector<StockData>& data,
                                    int window,
                                    std::vector<double>& middle,
                                    std::vector<double>& upper,
                                    std::vector<double>& lower,
                                    double numStdDev) {
    const size_t n = data.size();
    middle.assign(n, std::numeric_limits<double>::quiet_NaN());
    upper.assign(n,  std::numeric_limits<double>::quiet_NaN());
    lower.assign(n,  std::numeric_limits<double>::quiet_NaN());

    if (n == 0 || window <= 0 || static_cast<size_t>(window) > n) {
        return;
    }

    // Rolling mean and std on closing prices
    double sum = 0.0;
    double sumSq = 0.0;

    for (int i = 0; i < static_cast<int>(n); ++i) {
        double price = data[i].close;
        sum += price;
        sumSq += price * price;

        if (i >= window) {
            double oldPrice = data[i - window].close;
            sum   -= oldPrice;
            sumSq -= oldPrice * oldPrice;
        }

        if (i >= window - 1) {
            int count = window;
            double mean = sum / count;
            double variance = (sumSq / count) - (mean * mean);
            if (variance < 0.0) variance = 0.0;
            double stddev = std::sqrt(variance);

            middle[i] = mean;
            upper[i]  = mean + numStdDev * stddev;
            lower[i]  = mean - numStdDev * stddev;
        }
    }
}
