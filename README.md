# StockSense

Installation
Clone the repository:
git clone https://github.com/your-username/StockSense.git
cd StockSense

Install Python dependencies:
pip install -r requirements.txt

Compile the C++ backend:
cd src
g++ -std=c++17 main.cpp StockDataLoader.cpp StockAnalytics.cpp StrategySelector.cpp -o stocks

Verify the compiled executable:
ls -la stocks

Usage
Navigate to the frontend folder:
cd ../frontend

Run the Streamlit app:
streamlit run app.py

Open the app in your browser (usually at http://localhost:8501).
