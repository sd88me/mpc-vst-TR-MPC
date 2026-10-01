#!/usr/bin/env bash
# Offline audio test for TR-Kit. Run from a path without spaces:   bash ports/trkit/test/run.sh
set -euo pipefail
P="$(cd "$(dirname "$0")/../.." && pwd)"   # ports/
O=/tmp/trkit_audio; mkdir -p $O
CF="-I$P/trkit/src -I$P/6w6/src/dsp -I$P/6w6/src/vendor/606 -I$P/8w8/src/dsp -I$P/cw78/src/dsp -I$P/9w9/src/dsp -O2 -g -fsanitize=address,undefined"
g++ -std=gnu++14 $CF -c $P/trkit/src/trkit.cpp -o $O/trkit.o
g++ -std=gnu++14 $CF -c $P/6w6/src/dsp/sd606_engine.cpp -o $O/a.o
g++ -std=gnu++14 $CF -c $P/8w8/src/dsp/sc808_engine.cpp -o $O/b.o
g++ -std=gnu++14 $CF -c $P/cw78/src/dsp/cr78_engine.cpp -o $O/c.o
gcc -std=gnu11 $CF -c $P/9w9/src/dsp/er99_engine.c -o $O/d.o
g++ -std=gnu++14 $CF -c $P/trkit/test/audio_test.cpp -o $O/t.o
g++ -fsanitize=address,undefined $O/*.o -lm -o $O/audio_test
TRKIT_DIR="$P/9w9/src" $O/audio_test
