#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
WRK_DIR="$ROOT/tools/wrk"

sudo apt update
sudo apt install -y build-essential libssl-dev git

cd "$WRK_DIR"
make clean || true
make

echo "Built patched wrk at: $WRK_DIR/wrk"