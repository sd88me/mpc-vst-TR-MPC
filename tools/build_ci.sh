#!/usr/bin/env bash
# Build one port with mpc-vst-plugins' build_port.sh (CI and local). From the repo root: bash tools/build_ci.sh 9w9
# MPC_VST = a mpc-vst-plugins checkout (default: sibling). Output: ports/<kit>/build/.
set -euo pipefail
cd "$(dirname "$0")/.."
MPC_VST="${MPC_VST:-$PWD/../mpc-vst-plugins}"
bash "$MPC_VST/tools/build_port.sh" "ports/$1/vst.json"
