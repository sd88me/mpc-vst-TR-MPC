#!/usr/bin/env bash
# Regenerate a port's skin, params.h and plugin-list entry WITHOUT recompiling: the quick path for skin / layout / params edits.
#   bash tools/skin_only.sh trkit        (after python3 tools/gen_trkit_params.py and tools/trmpc_skin.py, as for a full build)
# It is steps 1-2 of mpc-vst-plugins' tools/build_port.sh; the .so files in ports/<kit>/build are left as they are, so it is only
# valid while the C++ is unchanged (a params.json change that adds or reorders parameters needs a full build: the .so embeds them).
# Output: ports/<kit>/build/skin/<vendor> - VST - <name>/ , like a full build.
set -euo pipefail
cd "$(dirname "$0")/.."
MPC_VST="${MPC_VST:-$PWD/../mpc-vst-plugins}"
kit="$1"
[ "$kit" = trkit ] && bash tools/apply_trkit_patches.sh
CFG="$PWD/ports/$kit/vst.json"
eval "$(python3 "$MPC_VST/tools/gen_vst.py" "$CFG" --shell)"
U="$(id -u):$(id -g)"
if [ "$ART" = html ]; then
  docker build -q -t mpc-vst-html-art "$MPC_VST/tools/html_art" >/dev/null
  docker run --rm -u "$U" -e HOME=/tmp -v "$ROOT":/w -v "$MPC_VST":/mv:ro -w /w mpc-vst-html-art python3 /mv/tools/gen_vst.py "$PORT/vst.json"
else
  echo "skin_only.sh: only the html-art ports are supported (this one is '$ART')"; exit 1
fi
echo "skin: $PORT/build/skin  (the .so is untouched)"
