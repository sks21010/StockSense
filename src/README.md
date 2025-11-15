**Stock Data Loader (C++ Module)**

This module handles the data input layer of our group project: reading, cleaning, and preparing stock market data for analysis and visualization.

It supports:

- **CSV Loading:** Loading stock data from CSV files using `rapidcsv`.
- **API Fetching:** Fetching stock data from online APIs (for example, Yahoo Finance) using `libcurl`.
- **Parsing & Formatting:** Basic parsing and formatting into a reusable `StockData` struct.

This is the foundation layer the rest of the project builds on.

**Features**

- **Read candles:** Open, High, Low, Close, Volume, Date for each day.
- **Skip malformed rows:** Rows that fail validation are ignored to keep the dataset clean.
- **Fetch remote CSVs:** Fetch CSV-formatted market data directly from a URL and parse it.
- **Return ready data:** Returns a clean `std::vector<StockData>` ready for downstream modules such as moving averages, volatility calculations, and graphing.

**How It Works**

1. **StockData Struct**

	 A simple POD struct that stores one day of stock data. Example:

	 - `struct StockData { std::string date; double open, high, low, close; uint64_t volume; }`

2. **StockDataLoader Class**

	 Handles all input operations. Example usage:

	 - Load from CSV:
		 - `auto data = loader.LoadFromCSV("AAPL.csv");`

	 - Fetch from online API:
		 - `auto data = loader.LoadFromAPI("AAPL", "2024-01-01", "2025-11-10");`

	 Internally:

	 - Uses `rapidcsv` for fast CSV parsing.
	 - Uses `libcurl` to fetch remote CSV data.
	 - Cleans and validates each row before storing.

**Dependencies**

Install or include:

- **rapidcsv**
	- GitHub: https://github.com/d99kris/rapidcsv
	- Header-only — drop `rapidcsv.h` into your project include path.

- **libcurl**
	- On Linux:

		```bash
		sudo apt install libcurl4-openssl-dev
		```

	- On macOS:

		```bash
		brew install curl
		```

	- On Windows (vcpkg):

		```bash
		vcpkg install curl
		```

**Notes**

- Place `rapidcsv.h` somewhere in your project's include path and ensure your build links with `libcurl`.
- The loader focuses on robustness: malformed rows are skipped and parsing errors are handled internally to avoid crashing upstream modules.

This module is the reliable data input layer other components depend on for analytics and visualization.

