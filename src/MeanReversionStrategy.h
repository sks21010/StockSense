#pragma once
#include "AnalysisStrategy.h"
#include "StockAnalytics.h"

// Strategy for mean-reverting stocks (H < 0.45)
class MeanReversionStrategy : public AnalysisStrategy {
private:
    StockAnalytics analytics;
    
public:
    double analyze(const std::vector<StockData>& data) override {
        // For mean-reverting stocks: Look for deviations from average
        auto sma20 = analytics.SimpleMovingAverage(data, 20);
        auto vol20 = analytics.RollingVolatility(data, 20);
        
        double currentPrice = data.back().close;
        double average = sma20.back();
        double volatility = vol20.back();
        
        // Z-score: how many standard deviations away from mean
        // Negative score = oversold (buy signal)
        // Positive score = overbought (sell signal)
        double zScore = (currentPrice - average) / (volatility * average);
        
        // Return negative for buy signal (expecting reversion up)
        // Return positive for sell signal (expecting reversion down)
        return -zScore * 100.0;  // Scale to percentage
    }
    
    std::string getName() const override {
        return "Mean Reversion Strategy";
    }
};
