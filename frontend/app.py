import streamlit as st
import subprocess
import re
import os
from pathlib import Path

st.set_page_config(page_title="StockSense", layout="wide")
st.title("📊 StockSense - Advanced Stock Analytics")

# Get project paths - use absolute path since we're in WSL
# __file__ = .../frontend/app.py
# parent = .../frontend
# parent.parent = .../StockSense
current_file = Path(__file__).resolve()
frontend_dir = current_file.parent
project_root = frontend_dir.parent
stocks_exe = project_root / "src" / "stocks"


# Sidebar
ticker = st.sidebar.selectbox("Stock", ["AAPL", "MSFT", "GOOGL", "TSLA", "AMZN", "NVDA", "META"])

if st.sidebar.button("Analyze", type="primary"):
    # Verify executable exists before running
    if not stocks_exe.exists():
        st.error(f"Executable not found at: {stocks_exe}")
        st.info("Please compile the C++ program first:\n```bash\ncd src\ng++ -std=c++17 main.cpp StockDataLoader.cpp StockAnalytics.cpp -o stocks\n```")
    else:
        with st.spinner(f"Analyzing {ticker}..."):
            # Call C++ backend - cwd should be project_root/src (sibling of frontend)
            src_dir = project_root / "src"
            result = subprocess.run(
                [str(stocks_exe), ticker], 
                capture_output=True, 
                text=True,
                cwd=str(src_dir)
            )
        
        if result.returncode != 0:
            st.error("Error running analysis")
            st.code(result.stderr)
        else:
            output = result.stdout
            st.success(f"Analysis complete for {ticker}!")
            
            # Parse the output
            def extract_value(pattern, text, group=1):
                match = re.search(pattern, text)
                return match.group(group) if match else "N/A"
            
            # Extract metrics
            latest_date = extract_value(r"Latest date:\s+(.+)", output)
            latest_close = extract_value(r"Latest close:\s+([\d.]+)", output)
            sma_20 = extract_value(r"20-day SMA:\s+([\d.]+)", output)
            volatility = extract_value(r"20-day volatility:\s+([\d.]+)", output)
            mean_return = extract_value(r"Mean:\s+([\d.-]+)", output)
            sharpe = extract_value(r"Sharpe Ratio \(daily\):\s+([\d.-]+)", output)
            ytd = extract_value(r"Year-to-date performance:\s+([\d.-]+)%", output)
            max_drawdown = extract_value(r"Max drawdown:\s+([\d.-]+)%", output)
            
            # Bollinger Bands
            bb_middle = extract_value(r"Middle \(SMA\):\s+([\d.]+)", output)
            bb_upper = extract_value(r"Upper band:\s+([\d.]+)", output)
            bb_lower = extract_value(r"Lower band:\s+([\d.]+)", output)
            bb_status = extract_value(r"Status:\s+(.+)", output)
            
            # Hurst
            hurst = extract_value(r"Hurst Exponent:\s+([\d.]+)", output)
            hurst_behavior = extract_value(r"Behavior:\s+(.+)", output)
            
            # ACF
            acf_1 = extract_value(r"Lag-1 \(daily\):\s+([\d.-]+)", output)
            acf_5 = extract_value(r"Lag-5 \(weekly\):\s+([\d.-]+)", output)
            acf_20 = extract_value(r"Lag-20 \(monthly\):\s+([\d.-]+)", output)
            
            # Strategy results - match the actual C++ format
            strategies = {}
            # The C++ outputs: "Mean Reversion Strategy:\n  Total Return:..."
            strategy_blocks = re.findall(
                r"^(.+?Strategy):\s*\n\s+Total Return:\s+([\d.-]+)%\s*\n\s+Sharpe Ratio:\s+([\d.-]+)\s*\n\s+Max Drawdown:\s+([\d.-]+)%\s*\n\s+Win Rate:\s+([\d.]+)%\s*\n\s+Overall Score:\s+([\d.-]+)",
                output,
                re.MULTILINE
            )
            
            
            for block in strategy_blocks:
                name, ret, sharpe_s, dd, wr, score = block
                # Remove the word "Strategy" from the name for display
                clean_name = name.replace(" Strategy", "").strip()
                strategies[clean_name] = {
                    'return': ret,
                    'sharpe': sharpe_s,
                    'drawdown': dd,
                    'win_rate': wr,
                    'score': score
                }
            
            recommended_strategy = extract_value(r"RECOMMENDED STRATEGY:\s+(.+)", output)
            signal_strength = extract_value(r"Current Signal Strength:\s+([-\d.]+)", output)
            action = extract_value(r"Action Recommendation:\s+(.+)", output)
            
            # ===== OVERVIEW SECTION =====
            st.header("📈 Market Overview")
            
            col1, col2, col3, col4, col5 = st.columns(5)
            col1.metric("Latest Close", f"${latest_close}", latest_date)
            col2.metric("Volatility", f"{float(volatility)*100:.2f}%" if volatility != "N/A" else "N/A")
            col3.metric("20d SMA", f"${sma_20}")
            col4.metric("Sharpe Ratio", sharpe)
            col5.metric("YTD", f"{ytd}%")
            
            # ===== STRATEGY RECOMMENDATIONS =====
            st.header("🎯 Strategy Recommendation")
            
            # Best strategy highlight
            st.success(f"### Recommended: {recommended_strategy}")
            
            col1, col2 = st.columns([2, 1])
            
            with col1:
                # Strategy comparison table
                if strategies:
                    import pandas as pd
                    strategy_data = {
                        'Strategy': list(strategies.keys()),
                        'Total Return': [f"{v['return']}%" for v in strategies.values()],
                        'Sharpe': [v['sharpe'] for v in strategies.values()],
                        'Drawdown': [f"{v['drawdown']}%" for v in strategies.values()],
                        'Win Rate': [f"{v['win_rate']}%" for v in strategies.values()],
                        'Score': [v['score'] for v in strategies.values()]
                    }
                    df = pd.DataFrame(strategy_data)
                    st.dataframe(df, width="stretch", hide_index=True)
            
            with col2:
                st.metric("Signal Strength", signal_strength)
                
                # Color code the action
                if "BUY" in action.upper():
                    st.success(f"**Action: {action}**")
                elif "SELL" in action.upper():
                    st.error(f"**Action: {action}**")
                else:
                    st.info(f"**Action: {action}**")
                
                # Handle both positive (buy) and negative (sell) signals
                sig_val = float(signal_strength) if signal_strength != "N/A" else 0.0
                
                # Normalize signal to 0-1 range for progress bar
                # Signals range roughly from -10 (strong sell) to +10 (strong buy)
                # Map to 0 (strong sell) to 1 (strong buy), with 0.5 as neutral
                normalized = (sig_val + 10) / 20  # Maps -10 to 0, 0 to 0.5, +10 to 1
                normalized = max(0, min(1, normalized))  # Clamp to [0, 1]
                
                # Color the progress bar based on signal direction
                if sig_val > 5:
                    bar_text = f"🟢 Strong Buy: {sig_val:.1f}"
                elif sig_val > 0:
                    bar_text = f"🟢 Buy: {sig_val:.1f}"
                elif sig_val > -5:
                    bar_text = f"🟡 Neutral: {sig_val:.1f}"
                elif sig_val > -10:
                    bar_text = f"🔴 Sell: {sig_val:.1f}"
                else:
                    bar_text = f"🔴 Strong Sell: {sig_val:.1f}"
                
                st.progress(normalized, text=bar_text)
            
            # ===== TECHNICAL INDICATORS =====
            st.header("📊 Technical Indicators")
            
            tab1, tab2, tab3 = st.tabs(["Bollinger Bands (20-day)", "Hurst Exponent", "Autocorrelation"])
            
            with tab1:
                col1, col2, col3 = st.columns(3)
                col1.metric("Upper Band", f"${bb_upper}")
                col2.metric("Middle (SMA)", f"${bb_middle}")
                col3.metric("Lower Band", f"${bb_lower}")
                
                if "Overbought" in bb_status:
                    st.warning(f"**Status: {bb_status}**")
                elif "Oversold" in bb_status:
                    st.info(f"**Status: {bb_status}**")
                else:
                    st.success(f"**Status: {bb_status}**")
            
            with tab2:
                st.metric("Hurst Exponent", hurst)
                
                interpretation = ""
                if hurst != "N/A":
                    h_val = float(hurst)
                    if h_val > 0.55:
                        interpretation = """
                        **Trending/Persistent Behavior**
                        - H > 0.55 indicates momentum
                        - Trends tend to continue
                        - Use momentum-based strategies
                        """
                    elif h_val < 0.45:
                        interpretation = """
                        **Mean-Reverting Behavior**
                        - H < 0.45 indicates reversals
                        - Prices bounce back to average
                        - Use contrarian strategies
                        """
                    else:
                        interpretation = """
                        **Random Walk Behavior**
                        - H ≈ 0.5 indicates randomness
                        - Past doesn't predict future
                        - Technical analysis less effective
                        """
                
                st.info(interpretation or hurst_behavior)
            
            with tab3:
                import pandas as pd
                acf_data = pd.DataFrame({
                    'Lag': ['Lag-1 (daily)', 'Lag-5 (weekly)', 'Lag-20 (monthly)'],
                    'Value': [acf_1, acf_5, acf_20],
                    'Interpretation': [
                        'Momentum' if float(acf_1) > 0.1 else 'Mean Reversion' if float(acf_1) < -0.1 else 'Random',
                        'Momentum' if float(acf_5) > 0.1 else 'Mean Reversion' if float(acf_5) < -0.1 else 'Random',
                        'Momentum' if float(acf_20) > 0.1 else 'Mean Reversion' if float(acf_20) < -0.1 else 'Random'
                    ]
                })
                st.dataframe(acf_data, hide_index=True, width="stretch")
            
            # ===== RISK METRICS =====
            st.header("⚠️ Risk Analysis")
            
            col1, col2, col3 = st.columns(3)
            col1.metric("Sharpe Ratio", sharpe, "Risk-adjusted return")
            col2.metric("Max Drawdown", f"{max_drawdown}%", "Worst drop")
            col3.metric("Daily Volatility", f"{float(volatility)*100:.2f}%" if volatility != "N/A" else "N/A", "Price swings")
            
            # ===== RAW OUTPUT (OPTIONAL) =====
            with st.expander("🔍 Detailed C++ Output"):
                st.code(output, language='text')
