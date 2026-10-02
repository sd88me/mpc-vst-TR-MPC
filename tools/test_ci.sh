#!/usr/bin/env bash
# Offline host test for one port. From the repo root: bash tools/test_ci.sh 9w9
set -euo pipefail
cd "$(dirname "$0")/.."
MPC_VST="${MPC_VST:-$PWD/../mpc-vst-plugins}"
bash "$MPC_VST/tools/test_port.sh" "ports/$1/vst.json"
