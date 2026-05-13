#!/usr/bin/env bash
set -euo pipefail

OUT_DIR="perf"
CSV="$OUT_DIR/baseline.csv"
URL="${1:-http://127.0.0.1:8080}"

SIZES=("1k.bin" "2k.bin" "4k.bin" "8k.bin" "16k.bin" "32k.bin" "64k.bin" "128k.bin" "256k.bin" "512k.bin" "1m.bin")
THREADS="${THREADS:-1}"
CONNS="${CONNS:-1}"
DUR="${DUR:-15s}"

mkdir -p "$OUT_DIR" public

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
make_bin_kb 1024 public/1m.bin

echo "file,size_bytes,threads,conns,duration,total_requests,rps,latency_avg_ms,latency_stdev_ms,latency_max_ms,latency_p50_ms,latency_p75_ms,latency_p90_ms,latency_p99_ms,latency_implied_ms" > "$CSV"

norm() {
  local v="$1"
  if [[ "$v" == *us ]]; then
    awk -v u="${v%us}" 'BEGIN{printf "%.6f", u/1000}'
  elif [[ "$v" == *ms ]]; then
    echo "${v%ms}"
  elif [[ "$v" == *s ]]; then
    awk -v s="${v%s}" 'BEGIN{printf "%.3f", s*1000}'
  else
    echo "$v"
  fi
}

for f in "${SIZES[@]}"; do
  echo "Benchmarking $f ..."

  RES=$(wrk -t"$THREADS" -c"$CONNS" -d"$DUR" --latency "$URL/$f")

  echo "===== RAW WRK OUTPUT for $f ====="
  echo "$RES"
  echo "================================="

  LAT_LINE=$(echo "$RES" | awk '/^[[:space:]]*Latency[[:space:]]/ {print; exit}')

  LAVG=$(echo "$LAT_LINE" | awk '{print $2}')
  LSTDEV=$(echo "$LAT_LINE" | awk '{print $3}')
  LMAX=$(echo "$LAT_LINE" | awk '{print $4}')

  P50=$(echo "$RES" | awk '/^[[:space:]]*50%/ {print $2; exit}')
  P75=$(echo "$RES" | awk '/^[[:space:]]*75%/ {print $2; exit}')
  P90=$(echo "$RES" | awk '/^[[:space:]]*90%/ {print $2; exit}')
  P99=$(echo "$RES" | awk '/^[[:space:]]*99%/ {print $2; exit}')

  REQS=$(echo "$RES" | awk '/requests in/ {print $1; exit}')
  RPS=$(echo "$RES" | awk '/Requests\/sec:/ {print $2; exit}')

  : "${REQS:=NA}"
  : "${RPS:=NA}"
  : "${LAVG:=NA}"
  : "${LSTDEV:=NA}"
  : "${LMAX:=NA}"
  : "${P50:=NA}"
  : "${P75:=NA}"
  : "${P90:=NA}"
  : "${P99:=NA}"

  LAVG_MS=$( [[ "$LAVG" == "NA" ]] && echo "NA" || norm "$LAVG" )
  LSTDEV_MS=$( [[ "$LSTDEV" == "NA" ]] && echo "NA" || norm "$LSTDEV" )
  LMAX_MS=$( [[ "$LMAX" == "NA" ]] && echo "NA" || norm "$LMAX" )
  P50_MS=$( [[ "$P50" == "NA" ]] && echo "NA" || norm "$P50" )
  P75_MS=$( [[ "$P75" == "NA" ]] && echo "NA" || norm "$P75" )
  P90_MS=$( [[ "$P90" == "NA" ]] && echo "NA" || norm "$P90" )
  P99_MS=$( [[ "$P99" == "NA" ]] && echo "NA" || norm "$P99" )

  IMPLIED_MS=$(awk -v r="$RPS" 'BEGIN{if (r+0 > 0) printf "%.6f", 1000/r; else print "NA"}')

  SIZE=$(stat -c%s "public/$f" 2>/dev/null || stat -f%z "public/$f")

  echo "$f,$SIZE,$THREADS,$CONNS,$DUR,$REQS,$RPS,$LAVG_MS,$LSTDEV_MS,$LMAX_MS,$P50_MS,$P75_MS,$P90_MS,$P99_MS,$IMPLIED_MS" >> "$CSV"
done

echo "Wrote $CSV"
