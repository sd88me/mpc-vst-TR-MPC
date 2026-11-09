#!/bin/bash
# Install every converted folder on a Live III (aarch64). Usage: tools/deploy_live3.sh [ip]   (restart is separate: ask first)
IP=${1:-192.168.2.217}
SDK=${MPC_SDK:-$HOME/mpc-310-private/mpc-plugin-builder-sdk}
cd "$(dirname "$0")/.." || exit 1
for d in build/conv/*/*/; do python3 "$SDK/tools/mpc_remote.py" "$IP" --yes update "${d%/}" || exit 1; done
