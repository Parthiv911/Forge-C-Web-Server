#!/usr/bin/env python3
import sys
import pandas as pd
import matplotlib.pyplot as plt

csv_path = sys.argv[1] if len(sys.argv) > 1 else "perf/baseline.csv"

df = pd.read_csv(csv_path)

numeric_cols = [
    "size_bytes",
    "threads",
    "conns",
    "total_requests",
    "rps",
    "latency_avg_ms",
    "latency_stdev_ms",
    "latency_max_ms",
    "latency_p50_ms",
    "latency_p75_ms",
    "latency_p90_ms",
    "latency_p99_ms",
    "latency_implied_ms",
]

for col in numeric_cols:
    if col in df.columns:
        df[col] = pd.to_numeric(df[col], errors="coerce")

df = df.dropna(subset=["conns"]).copy()
df = df.sort_values("conns")

# Latency vs concurrency
lat_df = df.dropna(subset=[
    "latency_avg_ms",
    "latency_p50_ms",
    "latency_p90_ms",
    "latency_p99_ms",
])

plt.figure()
plt.plot(lat_df["conns"], lat_df["latency_avg_ms"], marker="o", label="avg")
plt.plot(lat_df["conns"], lat_df["latency_p50_ms"], marker="o", label="p50")
plt.plot(lat_df["conns"], lat_df["latency_p90_ms"], marker="o", label="p90")
plt.plot(lat_df["conns"], lat_df["latency_p99_ms"], marker="o", label="p99")
plt.xscale("log", base=2)
plt.xlabel("Client concurrency")
plt.ylabel("Latency (ms)")
plt.title("Latency vs Client Concurrency")
plt.grid(True, which="both")
plt.legend()
plt.tight_layout()
plt.savefig("latency_vs_concurrency.png", dpi=160)

# Throughput vs concurrency
rps_df = df.dropna(subset=["rps"])

plt.figure()
plt.plot(rps_df["conns"], rps_df["rps"], marker="o")
plt.xscale("log", base=2)
plt.xlabel("Client concurrency")
plt.ylabel("Requests/sec")
plt.title("Throughput vs Client Concurrency")
plt.grid(True, which="both")
plt.tight_layout()
plt.savefig("rps_vs_concurrency.png", dpi=160)

# Combined: throughput and p99 latency on same x-axis
combo_df = df.dropna(subset=["rps", "latency_p99_ms"])

fig, ax1 = plt.subplots()

ax1.plot(combo_df["conns"], combo_df["rps"], marker="o", label="rps")
ax1.set_xscale("log", base=2)
ax1.set_xlabel("Client concurrency")
ax1.set_ylabel("Requests/sec")
ax1.grid(True, which="both")

ax2 = ax1.twinx()
ax2.plot(combo_df["conns"], combo_df["latency_p99_ms"], marker="o", linestyle="--", label="p99 latency")
ax2.set_ylabel("p99 latency (ms)")

plt.title("Throughput and p99 Latency vs Client Concurrency")
fig.tight_layout()
plt.savefig("rps_p99_vs_concurrency.png", dpi=160)

print("Wrote: latency_vs_concurrency.png, rps_vs_concurrency.png, rps_p99_vs_concurrency.png")
