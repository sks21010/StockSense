#pragma once
#include "AnalysisStrategy.h"
#include "StockAnalytics.h"
#include <memory>
#include <vector>
#include <algorithm>
#include <cmath>

// Result of strategy evaluation
struct StrategyPerformance {
    std::string strategyName;
    double totalReturn;      // Total return over backtest period
    double sharpeRatio;      // Risk-adjusted return
    double maxDrawdown;      // Worst drawdown (negative value)
    double winRate;          // Percentage of profitable signals
    double score;            // Combined score for ranking
};

class StrategySelector {
private:
    StockAnalytics analytics;
    
    // Backtest a strategy on historical data
    StrategyPerformance backtestStrategy(
        AnalysisStrategy* strategy,
        const std::vector<StockData>& data,
        int lookbackWindow = 100  // How much history to evaluate
    ) {
        StrategyPerformance perf;
        perf.strategyName = strategy->getName();
        
        if (data.size() < lookbackWindow + 20) {
            // Not enough data for meaningful backtest
            perf.totalReturn = 0.0;
            perf.sharpeRatio = 0.0;
            perf.maxDrawdown = 0.0;
            perf.winRate = 0.0;
            perf.score = 0.0;
            return perf;
        }
        
        // Use recent history for backtesting
        size_t startIdx = data.size() - lookbackWindow;
        std::vector<StockData> backtestData(data.begin() + startIdx, data.end());
        
        // Simulate trading based on strategy signals
        std::vector<double> portfolioReturns;
        int wins = 0;
        int totalTrades = 0;
        
        for (size_t i = 20; i < backtestData.size() - 1; ++i) {
            // Get signal from strategy using data up to point i
            std::vector<StockData> historicalData(backtestData.begin(), backtestData.begin() + i + 1);
            double signal = strategy->analyze(historicalData);
            
            // Calculate next-day return
            double nextReturn = (backtestData[i+1].close - backtestData[i].close) / backtestData[i].close;
            
            // If signal is positive (buy signal), take the position
            // If signal is negative (sell signal), inverse the return
            double positionReturn = 0.0;
            if (std::abs(signal) > 5.0) {  // Only trade on strong signals
                totalTrades++;
                positionReturn = (signal > 0) ? nextReturn : -nextReturn;
                portfolioReturns.push_back(positionReturn);
                
                if (positionReturn > 0) wins++;
            }
        }
        
        // Calculate performance metrics
        if (portfolioReturns.empty()) {
            perf.totalReturn = 0.0;
            perf.sharpeRatio = 0.0;
            perf.maxDrawdown = 0.0;
            perf.winRate = 0.0;
        } else {
            // Total return (compounded)
            perf.totalReturn = 0.0;
            for (double r : portfolioReturns) {
                perf.totalReturn += r;
            }
            
            // Sharpe ratio
            perf.sharpeRatio = analytics.SharpeRatio(portfolioReturns, 0.0);
            
            // Max drawdown from equity curve
            std::vector<double> equityCurve;
            double cumReturn = 1.0;
            for (double r : portfolioReturns) {
                cumReturn *= (1.0 + r);
                equityCurve.push_back(cumReturn);
            }
            
            double peak = equityCurve[0];
            double maxDD = 0.0;
            for (double equity : equityCurve) {
                if (equity > peak) peak = equity;
                double drawdown = (equity - peak) / peak;
                if (drawdown < maxDD) maxDD = drawdown;
            }
            perf.maxDrawdown = maxDD;
            
            // Win rate
            perf.winRate = (totalTrades > 0) ? (static_cast<double>(wins) / totalTrades) : 0.0;
        }
        
        // Calculate composite score (higher is better)
        // Weight: 40% total return, 30% sharpe, 20% win rate, 10% drawdown
        perf.score = (perf.totalReturn * 0.4) + 
                     (perf.sharpeRatio * 0.3) + 
                     (perf.winRate * 0.2) - 
                     (perf.maxDrawdown * 0.1);
        
        return perf;
    }
    
public:
    // Select the best strategy from a list of candidates
    AnalysisStrategy* selectBestStrategy(
        std::vector<std::unique_ptr<AnalysisStrategy>>& strategies,
        const std::vector<StockData>& data,
        StrategyPerformance& bestPerformance
    ) {
        if (strategies.empty()) return nullptr;
        
        AnalysisStrategy* bestStrategy = nullptr;
        double bestScore = -1e9;
        
        for (auto& strategy : strategies) {
            StrategyPerformance perf = backtestStrategy(strategy.get(), data);
            
            if (perf.score > bestScore) {
                bestScore = perf.score;
                bestStrategy = strategy.get();
                bestPerformance = perf;
            }
        }
        
        return bestStrategy;
    }
    
    // Evaluate all strategies and return performance metrics
    std::vector<StrategyPerformance> evaluateAllStrategies(
        std::vector<std::unique_ptr<AnalysisStrategy>>& strategies,
        const std::vector<StockData>& data
    ) {
        std::vector<StrategyPerformance> performances;
        
        for (auto& strategy : strategies) {
            performances.push_back(backtestStrategy(strategy.get(), data));
        }
        
        // Sort by score (best first)
        std::sort(performances.begin(), performances.end(),
                  [](const StrategyPerformance& a, const StrategyPerformance& b) {
                      return a.score > b.score;
                  });
        
        return performances;
    }
};
