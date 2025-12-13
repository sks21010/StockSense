import requests
import csv
from datetime import datetime

def fetch_to_csv(ticker):
    API_KEY = "24BH48U7GGP2QN3"  

   
    url = (
        "https://www.alphavantage.co/query"
        f"?function=TIME_SERIES_DAILY&symbol={ticker}&apikey={API_KEY}"
    )

    print(f"Calling API for {ticker}...")
    r = requests.get(url)
    data = r.json()

    
    if "Time Series (Daily)" not in data:
        print("API error:", data)
        return

    ts = data["Time Series (Daily)"]

    
    output_path = f"{ticker}.csv"

    with open(output_path, "w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(["date", "open", "high", "low", "close", "volume"])

        for date in sorted(ts.keys()):
            d = ts[date]
            writer.writerow([
                date,
                d["1. open"],
                d["2. high"],
                d["3. low"],
                d["4. close"],
                d["5. volume"]   
            ])

    print("Saved:", output_path)
    return output_path


if __name__ == "__main__":
    import sys
    if len(sys.argv) != 2:
        print("Usage: python fetch_api.py TICKER")
    else:
        fetch_to_csv(sys.argv[1])
