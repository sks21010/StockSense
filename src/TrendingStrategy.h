#pragma once
#include "AnalysisStrategy.h"
#include "StockAnalytics.h"

// Strategy for trending/momentum stocks (H > 0.5)
class TrendingStrategy : public AnalysisStrategy {
private:
    StockAnalytics analytics;
    
public:
    double analyze(const std::vector<StockData>& data) override {
        // For trending stocks: Use momentum indicators
        auto returns = analytics.DailyReturns(data);
        auto sma50 = analytics.SimpleMovingAverage(data, 50);
        
        // Simple momentum score: current price vs long-term average
        double currentPrice = data.back().close;
        double longTermAvg = sma50.back();
        
        // Positive if above average (buy signal), negative if below
        return (currentPrice - longTermAvg) / longTermAvg * 100.0;
    }
    
    std::string getName() const override {
        return "Momentum/Trending Strategy";
    }
};