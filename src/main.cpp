#include "StockDataLoader.h"
#include "StockAnalytics.h"
#include "StrategySelector.h"
#include "TrendingStrategy.h"
#include "MeanReversionStrategy.h"
#include "BuyAndHoldStrategy.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <memory>

int main(int argc, char* argv[]) {
    StockDataLoader loader;
    StockAnalytics analytics;

    // Accept ticker from command line, default to AAPL if not provided
    std::string ticker = "AAPL";
    if (argc > 1) {
        ticker = argv[1];
    }
    
    auto data = loader.LoadByTicker(ticker);
    std::cout << "Loaded " << data.size() << " rows for " << ticker << ".\n";

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

    // Bollinger Band position analysis
    double bandWidth = upBB[lastIdx] - lowBB[lastIdx];
    double distFromUpper = upBB[lastIdx] - last.close;
    double distFromLower = last.close - lowBB[lastIdx];
    double distFromMiddle = last.close - midBB[lastIdx];
    
    std::cout << "\nBollinger Band Position:\n";
    std::cout << "  Distance from middle:      " << distFromMiddle << " ";
    if (distFromMiddle > 0) {
        std::cout << "(above average)\n";
    } else {
        std::cout << "(below average)\n";
    }
    
    // Determine overbought/oversold status
    double upperThreshold = bandWidth * 0.15;  // Within 15% of band
    double lowerThreshold = bandWidth * 0.15;
    
    if (distFromUpper < upperThreshold) {
        std::cout << "  Status:                     OVERBOUGHT\n";
        std::cout << "  Recommendation:             Consider SELLING or taking profits\n";
        std::cout << "                              Price at statistical high, likely pullback\n";
    } else if (distFromLower < lowerThreshold) {
        std::cout << "  Status:                     OVERSOLD\n";
        std::cout << "  Recommendation:             Consider BUYING the dip\n";
        std::cout << "                              Price at statistical low, likely bounce\n";
    } else if (last.close > midBB[lastIdx]) {
        std::cout << "  Status:                     Slightly Overbought (upper half)\n";
        std::cout << "  Recommendation:             HOLD or wait for better entry\n";
    } else if (last.close < midBB[lastIdx]) {
        std::cout << "  Status:                     Slightly Oversold (lower half)\n";
        std::cout << "  Recommendation:             Watch for entry opportunity\n";
    } else {
        std::cout << "  Status:                     NEUTRAL (at fair value)\n";
        std::cout << "  Recommendation:             No clear signal\n";
    }

    // Simple trend label
    if (last.close > sma20[lastIdx]) {
        std::cout << "\nTrend signal:                UP (price above 20d SMA)\n";
    } else {
        std::cout << "\nTrend signal:                DOWN/FLAT (price at or below 20d SMA)\n";
    }

    // Autocorrelation analysis
    auto acf = analytics.AutocorrelationFunction(returns, 20);
    
    std::cout << "\nAutocorrelation Analysis (momentum vs mean reversion):\n";
    std::cout << "  Lag-1 (daily):             " << acf[0] << "\n";
    std::cout << "  Lag-5 (weekly):            " << acf[4] << "\n";
    std::cout << "  Lag-20 (monthly):          " << acf[19] << "\n";
    
    std::cout << "\nInterpretation:\n";
    if (acf[0] > 0.1) {
        std::cout << "  Daily pattern:              Strong momentum (trends continue)\n";
    } else if (acf[0] < -0.1) {
        std::cout << "  Daily pattern:              Strong mean reversion (trends reverse)\n";
    } else {
        std::cout << "  Daily pattern:              Weak/random (no clear pattern)\n";
    }

    if (acf[4] > 0.1) {
        std::cout << "  Weekly pattern:              Strong momentum (trends continue)\n";
    } else if (acf[4] < -0.1) {
        std::cout << "  Weekly pattern:              Strong mean reversion (trends reverse)\n";
    } else {
        std::cout << "  Weekly pattern:              Weak/random (no clear pattern)\n";
    }

    if (acf[19] > 0.1) {
        std::cout << "  Monthly pattern:              Strong momentum (trends continue)\n";
    } else if (acf[19] < -0.1) {
        std::cout << "  Monthly pattern:              Strong mean reversion (trends reverse)\n";
    } else {
        std::cout << "  Monthly pattern:              Weak/random (no clear pattern)\n";
    }

    // Hurst Exponent analysis
    double hurst = analytics.HurstExponent(returns);
    
    std::cout << "\nHurst Exponent Analysis:\n";
    std::cout << "  Hurst Exponent:            " << hurst << "\n";
    
    if (!std::isnan(hurst)) {
        if (hurst > 0.55) {
            std::cout << "  Behavior:                   TRENDING/PERSISTENT\n";
            std::cout << "  Trading Strategy:           MOMENTUM - Buy strength, ride trends\n";
            std::cout << "  Example Action:             If price rises, expect it to keep rising\n";
        } else if (hurst < 0.45) {
            std::cout << "  Behavior:                   MEAN-REVERTING\n";
            std::cout << "  Trading Strategy:           CONTRARIAN - Buy dips, sell rallies\n";
            std::cout << "  Example Action:             If price spikes, expect pullback\n";
        } else {
            std::cout << "  Behavior:                   RANDOM WALK\n";
            std::cout << "  Trading Strategy:           EFFICIENT MARKET - Buy & hold, use fundamentals\n";
            std::cout << "  Example Action:             Technical analysis unreliable, focus on value\n";
        }
    }

    // ============================================================
    // AUTOMATIC STRATEGY SELECTION
    // ============================================================
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "AUTOMATED STRATEGY SELECTION\n";
    std::cout << std::string(60, '=') << "\n";

    // Create candidate strategies
    std::vector<std::unique_ptr<AnalysisStrategy>> strategies;
    strategies.push_back(std::make_unique<TrendingStrategy>());
    strategies.push_back(std::make_unique<MeanReversionStrategy>());
    strategies.push_back(std::make_unique<BuyAndHoldStrategy>());

    // Evaluate all strategies
    StrategySelector selector;
    auto performances = selector.evaluateAllStrategies(strategies, data);

    std::cout << "\nStrategy Backtest Results (Last 100 Days):\n";
    std::cout << std::string(60, '-') << "\n";
    
    for (const auto& perf : performances) {
        std::cout << "\n" << perf.strategyName << ":\n";
        std::cout << "  Total Return:               " << perf.totalReturn * 100.0 << "%\n";
        std::cout << "  Sharpe Ratio:               " << perf.sharpeRatio << "\n";
        std::cout << "  Max Drawdown:               " << perf.maxDrawdown * 100.0 << "%\n";
        std::cout << "  Win Rate:                   " << perf.winRate * 100.0 << "%\n";
        std::cout << "  Overall Score:              " << perf.score << "\n";
    }

    // Select the best strategy
    StrategyPerformance bestPerf;
    AnalysisStrategy* bestStrategy = selector.selectBestStrategy(strategies, data, bestPerf);

    std::cout << "\n" << "\n";
    std::cout << "RECOMMENDED STRATEGY: " << bestPerf.strategyName << "\n";
    std::cout << "\n" << "\n";
    std::cout << "Based on comparative performance, this strategy has shown the best risk-adjusted returns.\n";

    // Get current signal from the best strategy
    double signal = bestStrategy->analyze(data);
    
    std::cout << "\nCurrent Signal Strength:     " << signal << "\n";
    std::cout << "\nAction Recommendation:\n";
    
    if (signal > 10.0) {
        std::cout << "  STRONG BUY\n";
        std::cout << "  Strategy indicates favorable entry conditions.\n";
    } else if (signal > 5.0) {
        std::cout << "  BUY\n";
        std::cout << "  Positive signal, consider entering position.\n";
    } else if (signal > -5.0) {
        std::cout << "  HOLD / NEUTRAL\n";
        std::cout << "  No strong signal, maintain current position.\n";
    } else if (signal > -10.0) {
        std::cout << "  SELL\n";
        std::cout << "  Negative signal, consider reducing exposure.\n";
    } else {
        std::cout << "  STRONG SELL\n";
        std::cout << "  Strategy indicates exit conditions.\n";
    }


    return 0;
}
