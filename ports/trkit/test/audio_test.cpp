// Offline audio check for TR-MPC: every flat voice, played from slot 1, must make sound and then decay;
// a retargeted slot must change what the same note plays. Build: see ports/trkit/test/run.sh
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
extern "C" {
#include "engine.h"
}

static double rms_of(const mpc_engine_t *e, void *in, int blocks) {
    int16_t buf[128 * 2];
    double acc = 0; long n = 0;
    for (int b = 0; b < blocks; ++b) {
        memset(buf, 0, sizeof buf);
        e->render(in, buf, 128);
        for (int i = 0; i < 128; ++i) { double s = buf[i * 2] / 32768.0; acc += s * s; ++n; }
    }
    return sqrt(acc / n);
}

int main() {
    const mpc_engine_t *e = mpc_engine();
    const char *dir = getenv("TRKIT_DIR");   // folder holding samples/ (the 909 hats and cymbals)
    void *in = e->create(dir ? dir : "");
    int fails = 0;
    const int kVoices = 49;
    for (int v = 0; v < kVoices; ++v) {
        char val[16];
        snprintf(val, sizeof val, "%d", v);
        e->set_param(in, "s01_src", val);
        rms_of(e, in, 400);                         // flush the previous voice's tail
        const uint8_t on[3] = {0x90, 36, 110};
        e->midi(in, on, 3);
        const double hit = rms_of(e, in, 40);       // ~116 ms right after the hit
        const double tail = rms_of(e, in, 800);     // 2.3 s later window
        const double after = rms_of(e, in, 200);
        const bool ok = hit > 1e-4 && hit < 1.0 && after < hit;
        if (!ok) ++fails;
        printf("%s voice %2d  hit %.5f  window2 %.5f  window3 %.5f\n", ok ? "ok  " : "FAIL", v, hit, tail, after);
    }
    e->destroy(in);
    printf(fails ? "FAILED %d\n" : "PASSED\n", fails);
    return fails != 0;
}
