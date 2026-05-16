#!/usr/bin/env python3
import sys
import pandas as pd
import matplotlib.pyplot as plt

csv_path = sys.argv[1] if len(sys.argv) > 1 else "perf/baseline.csv"

df = pd.read_csv(csv_path)

# Convert columns
numeric_cols = [
    "size_bytes",
    "rps",
    "latency_avg_ms",
    "latency_p90_ms",
    "latency_p99_ms",
    "latency_implied_ms",
]

for col in numeric_cols:
    if col in df.columns:
        df[col] = pd.to_numeric(df[col], errors="coerce")

df = df.dropna(subset=["size_bytes"]).copy()
df["size_kb"] = df["size_bytes"] / 1024.0

# avg / p90 / p99 latency vs size
lat_df = df.dropna(subset=["latency_avg_ms", "latency_p90_ms", "latency_p99_ms"])

plt.figure()
plt.plot(lat_df["size_kb"], lat_df["latency_avg_ms"], marker="o", label="avg")
plt.plot(lat_df["size_kb"], lat_df["latency_p90_ms"], marker="o", label="p90")
plt.plot(lat_df["size_kb"], lat_df["latency_p99_ms"], marker="o", label="p99")
#plt.xscale("log", base=2)
plt.xlabel("File size (KB)")
plt.ylabel("Latency (ms)")
plt.title("Latency vs File Size")
plt.grid(True)
plt.legend()
plt.tight_layout()
plt.savefig("latency_vs_size.png", dpi=160)


# RPS vs size
rps_df = df.dropna(subset=["rps"])

plt.figure()
plt.plot(rps_df["size_kb"], rps_df["rps"], marker="o")
#plt.xscale("log", base=2)
plt.xlabel("File size (KB)")
plt.ylabel("Requests/sec")
plt.title("Throughput vs File Size")
plt.grid(True)
plt.tight_layout()
plt.savefig("rps_vs_size.png", dpi=160)

print("Wrote: latency_vs_size.png, p99_vs_size.png, rps_vs_size.png")