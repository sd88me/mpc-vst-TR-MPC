#!/usr/bin/env bash
# Regenerate src/vendor/lemus/generated/tr808/netlists.h from the vendored netlists (Lemus's tools/cmake/netlists.cmake).
set -euo pipefail
D="$(cd "$(dirname "$0")/.." && pwd)/src/vendor/lemus"
N=$(ls "$D/tr808/core/netlists" | sed 's/\.cir$//' | paste -sd,)
cmake -DDIR="$D/tr808/core/netlists" -DOUT="$D/generated/tr808/netlists.h" -DNAMES="$N" -P "$D/tools/netlists.cmake"
