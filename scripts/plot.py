#!/usr/bin/env python3
import sys
import pandas as pd
import matplotlib.pyplot as plt

csv_path = sys.argv[1] if len(sys.argv) > 1 else "perf/baseline.csv"

df = pd.read_csv(csv_path)

# Convert columns
for col in ["size_bytes", "rps", "latency_avg_ms"]:
    df[col] = pd.to_numeric(df[col], errors="coerce")

df = df.dropna(subset=["size_bytes"]).copy()
df["size_kb"] = df["size_bytes"] / 1024.0

# avg vs size
avg_df = df.dropna(subset=["latency_avg_ms"])
plt.figure()
plt.plot(avg_df["size_kb"], avg_df["latency_avg_ms"], marker="o")
plt.xscale("log", base=2)
plt.xlabel("File size (KB)")
plt.ylabel("avg latency (ms)")
plt.title("avg Latency vs File Size")
plt.grid(True)
plt.tight_layout()
plt.savefig("avg_vs_size.png", dpi=160)

# RPS vs size
rps_df = df.dropna(subset=["rps"])
plt.figure()
plt.plot(rps_df["size_kb"], rps_df["rps"], marker="o")
plt.xscale("log", base=2)
plt.xlabel("File size (KB)")
plt.ylabel("Requests/sec")
plt.title("Throughput vs File Size")
plt.grid(True)
plt.tight_layout()
plt.savefig("rps_vs_size.png", dpi=160)

print("Wrote: avg_vs_size.png, rps_vs_size.png")