#pragma once
#include "AnalysisStrategy.h"
#include "StockAnalytics.h"

// Buy and Hold strategy - passive baseline for comparison
class BuyAndHoldStrategy : public AnalysisStrategy {
private:
    StockAnalytics analytics;
    
public:
    double analyze(const std::vector<StockData>& data) override {
        // Buy and hold doesn't actively trade based on signals, just holds the position regardless of market conditions
        // Return a constant positive signal indicating "stay invested"
        
        if (data.empty()) {
            return 0.0;
        }
        
        // Small positive signal means "hold your position"
        // Not too high (not a strong buy), not negative (never sell)
        return 5.0;
    }
    
    std::string getName() const override {
        return "Buy & Hold Strategy";
    }
};
