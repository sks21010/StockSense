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

// ------------------- Autocorrelation -------------------
// Autocorrelation: how much today's value is related to/influenced by past values

double StockAnalytics::Autocorrelation(const std::vector<double>& values, int lag) {

    // ensure lag is valid (positive and less than data size)
    if (lag <= 0 || static_cast<size_t>(lag) >= values.size()) {
        return std::numeric_limits<double>::quiet_NaN();
    }

    // Calculate mean (excluding NaN values)
    double sum = 0.0;
    int count = 0;
    for (double v : values) {
        if (!std::isnan(v)) {
            sum += v;
            count++;
        }
    }

    if (count == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }

    double mean = sum / count;

    // Calculate autocovariance and variance
    double autocovariance = 0.0;
    double variance = 0.0;
    int pairs = 0;

    for (size_t i = lag; i < values.size(); ++i) {
        if (std::isnan(values[i]) || std::isnan(values[i - lag])) {
            continue;
        }

        double diff_t = values[i] - mean;
        double diff_lag = values[i - lag] - mean;

        autocovariance += diff_t * diff_lag;
        variance += diff_t * diff_t;
        pairs++;
    }

    if (pairs == 0 || variance == 0.0) {
        return std::numeric_limits<double>::quiet_NaN();
    }

    // Autocorrelation = autocovariance / variance
    // Positive Autocorrelation = today's movement follows yesterday's (momentum)
    // Negative Autocorrelation = today's movement opposed yesterday's (mean reversion)
    // Near Zero Autocorrelation = no pattern (random walk)
    double autocorrelation = autocovariance / variance;
    return autocorrelation;
}

std::vector<double> StockAnalytics::AutocorrelationFunction(const std::vector<double>& values, int maxLag) {
    std::vector<double> acf;
    acf.reserve(maxLag);

    for (int lag = 1; lag <= maxLag; ++lag) {
        acf.push_back(Autocorrelation(values, lag));
    }

    return acf;
}

// ------------------- Hurst Exponent -------------------

double StockAnalytics::HurstExponent(const std::vector<double>& values) {
    // Need sufficient data for meaningful analysis
    if (values.size() < 20) {
        return std::numeric_limits<double>::quiet_NaN();
    }

    // Remove NaN values
    std::vector<double> cleanValues;
    for (double v : values) {
        if (!std::isnan(v)) {
            cleanValues.push_back(v);
        }
    }

    if (cleanValues.size() < 20) {
        return std::numeric_limits<double>::quiet_NaN();
    }

    // Use Rescaled Range (R/S) analysis
    // Test multiple window sizes and compute R/S for each
    std::vector<int> windowSizes;
    std::vector<double> rescaledRanges;

    // Generate window sizes (powers and mid-points)
    int minWindow = 10;
    int maxWindow = cleanValues.size() / 4;
    
    for (int w = minWindow; w <= maxWindow; w = static_cast<int>(w * 1.5)) {
        if (w >= static_cast<int>(cleanValues.size())) break;
        windowSizes.push_back(w);
    }

    if (windowSizes.size() < 3) {
        return std::numeric_limits<double>::quiet_NaN();
    }

    // For each window size, compute average R/S
    for (int window : windowSizes) {
        std::vector<double> rsValues;

        // Slide window through the data
        for (size_t start = 0; start + window <= cleanValues.size(); ++start) {
            // Extract window
            std::vector<double> segment(cleanValues.begin() + start, 
                                       cleanValues.begin() + start + window);

            // Calculate mean
            double mean = 0.0;
            for (double v : segment) mean += v;
            mean /= segment.size();

            // Calculate cumulative deviations from mean
            std::vector<double> cumDev(window);
            cumDev[0] = segment[0] - mean;
            for (int i = 1; i < window; ++i) {
                cumDev[i] = cumDev[i-1] + (segment[i] - mean);
            }

            // Calculate range R
            double maxCumDev = cumDev[0];
            double minCumDev = cumDev[0];
            for (double cd : cumDev) {
                if (cd > maxCumDev) maxCumDev = cd;
                if (cd < minCumDev) minCumDev = cd;
            }
            double range = maxCumDev - minCumDev;

            // Calculate standard deviation S
            double variance = 0.0;
            for (double v : segment) {
                double diff = v - mean;
                variance += diff * diff;
            }
            variance /= segment.size();
            double stddev = std::sqrt(variance);

            // Avoid division by zero
            if (stddev > 1e-10) {
                rsValues.push_back(range / stddev);
            }
        }

        // Average R/S for this window size
        if (!rsValues.empty()) {
            double avgRS = 0.0;
            for (double rs : rsValues) avgRS += rs;
            avgRS /= rsValues.size();
            rescaledRanges.push_back(avgRS);
        }
    }

    if (rescaledRanges.size() < 3) {
        return std::numeric_limits<double>::quiet_NaN();
    }

    // Fit log(R/S) = H * log(n) + constant
    // Hurst exponent H is the slope
    std::vector<double> logN, logRS;
    for (size_t i = 0; i < windowSizes.size() && i < rescaledRanges.size(); ++i) {
        if (rescaledRanges[i] > 0) {
            logN.push_back(std::log(windowSizes[i]));
            logRS.push_back(std::log(rescaledRanges[i]));
        }
    }

    if (logN.size() < 3) {
        return std::numeric_limits<double>::quiet_NaN();
    }

    // Simple linear regression to find slope (Hurst exponent)
    double meanLogN = 0.0, meanLogRS = 0.0;
    for (size_t i = 0; i < logN.size(); ++i) {
        meanLogN += logN[i];
        meanLogRS += logRS[i];
    }
    meanLogN /= logN.size();
    meanLogRS /= logN.size();

    double numerator = 0.0, denominator = 0.0;
    for (size_t i = 0; i < logN.size(); ++i) {
        numerator += (logN[i] - meanLogN) * (logRS[i] - meanLogRS);
        denominator += (logN[i] - meanLogN) * (logN[i] - meanLogN);
    }

    if (denominator < 1e-10) {
        return std::numeric_limits<double>::quiet_NaN();
    }

    double hurst = numerator / denominator;

    // Clamp to reasonable range [0, 1]
    if (hurst < 0.0) hurst = 0.0;
    if (hurst > 1.0) hurst = 1.0;

    return hurst;
}
