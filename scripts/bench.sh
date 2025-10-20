#!/usr/bin/env bash
set -euo pipefail

OUT_DIR="perf"
CSV="$OUT_DIR/baseline.csv"
URL="${1:-http://127.0.0.1:8080}"
# Test sizes: 1KB .. 1MB
SIZES=("1k.bin" "2k.bin" "4k.bin" "8k.bin" "16k.bin" "32k.bin" "64k.bin" "128k.bin" "256k.bin" "512k.bin" "1m.bin")
THREADS=4
CONNS=256
DUR=15s

mkdir -p "$OUT_DIR" public

# Generate payloads if missing (zero-filled)
make_bin_kb() {
  local kb="$1" out="$2"
  [ -f "$out" ] || dd if=/dev/zero of="$out" bs=1024 count="$kb" >/dev/null 2>&1
}
make_bin_kb 1    public/1k.bin
make_bin_kb 2    public/2k.bin
make_bin_kb 4    public/4k.bin
make_bin_kb 8    public/8k.bin
make_bin_kb 16   public/16k.bin
make_bin_kb 32   public/32k.bin
make_bin_kb 64   public/64k.bin
make_bin_kb 128  public/128k.bin
make_bin_kb 256  public/256k.bin
make_bin_kb 512  public/512k.bin
# 1024 KB = 1 MB, save as 1m.bin
make_bin_kb 1024 public/1m.bin

echo "file,size_bytes,threads,conns,duration,rps,latency_avg_ms,latency_p99_ms" > "$CSV"

norm() {
  # Normalize '2.31ms' or '0.42s' to milliseconds (string->number)
  local v="$1"
  if [[ "$v" == *ms ]]; then
    echo "${v%ms}"
  elif [[ "$v" == *s ]]; then
    awk -v s="${v%s}" 'BEGIN{printf "%.3f", s*1000}'
  else
    echo "$v"
  fi
}

for f in "${SIZES[@]}"; do
  echo "Benchmarking $f ..."
  RES=$(wrk -t"$THREADS" -c"$CONNS" -d"$DUR" --latency "$URL/$f" 2>/dev/null || true)

  # Parse RPS/latencies from wrk output
  RPS=$(echo "$RES"  | awk '/Requests\/sec/ {print $2}')
  LAVG=$(echo "$RES" | awk '/^ *Latency/ {print $2}' | head -n1)   # e.g., 2.31ms
  P99=$(echo "$RES"  | awk '/^ *99%/ {print $2}')

  # Guard: if RPS empty, mark as NA so CSV still records a row
  : "${RPS:=NA}"
  : "${LAVG:=NA}"
  : "${P99:=NA}"

  LAVG_MS=$( [[ "$LAVG" == "NA" ]] && echo "NA" || norm "$LAVG" )
  P99_MS=$(  [[ "$P99"  == "NA" ]] && echo "NA" || norm "$P99"  )

  # File size (Linux stat first, fallback to macOS)
  SIZE=$(stat -c%s "public/$f" 2>/dev/null || stat -f%z "public/$f")

  echo "$f,$SIZE,$THREADS,$CONNS,$DUR,$RPS,$LAVG_MS,$P99_MS" >> "$CSV"
done

echo "Wrote $CSV"
