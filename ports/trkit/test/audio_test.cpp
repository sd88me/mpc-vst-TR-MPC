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
    // pan and sends belong to the slot: hard left silences the right channel; a reverb send adds tail
    auto hit_lr = [&](const char *pan, const char *rev, double *l, double *r, double *tail) {
        e->set_param(in, "s01_src", "21");   // 808 closed hat: dry tail is short, so a reverb send is obvious
        e->set_param(in, "s01_pan", pan);
        e->set_param(in, "s01_rev", rev);
        rms_of(e, in, 1500);
        const uint8_t on[3] = {0x90, 36, 110};
        e->midi(in, on, 3);
        int16_t buf[128 * 2];
        double al = 0, ar = 0;
        for (int b = 0; b < 40; ++b) {
            e->render(in, buf, 128);
            for (int i = 0; i < 128; ++i) { al += (buf[i * 2] / 32768.0) * (buf[i * 2] / 32768.0); ar += (buf[i * 2 + 1] / 32768.0) * (buf[i * 2 + 1] / 32768.0); }
        }
        *l = sqrt(al / 5120); *r = sqrt(ar / 5120);
        *tail = rms_of(e, in, 400);
    };
    double l, r, t0, t1;
    hit_lr("0", "0", &l, &r, &t0);
    const bool pan_ok = l > 1e-3 && r < l * 0.01;
    hit_lr("64", "0", &l, &r, &t0);
    hit_lr("64", "127", &l, &r, &t1);
    const bool rev_ok = t1 > t0 * 2 && fabs(l - r) < l * 0.05;
    printf("%s pan hard left (centre now: L %.5f R %.5f)\n%s reverb send: tail %.6f -> %.6f\n", pan_ok ? "ok  " : "FAIL", l, r, rev_ok ? "ok  " : "FAIL", t0, t1);
    fails += !pan_ok + !rev_ok;
    // a project chunk ("state") must restore every slot's voice and settings into a fresh instance
    {
        const char *set[][2] = {{"s03_src", "30"}, {"s03_pan", "10"}, {"s03_rev", "90"}, {"s03_level", "100"}, {"s03_tune", "20"},
                                {"s16_src", "48"}, {"s16_decay", "33"}, {"fx_rev_decay", "11"}, {"fx_dly_time", "3"},
                                {"fx_master_dist", "4"}, {"fx_volume", "77"}};
        for (auto &kv : set) e->set_param(in, kv[0], kv[1]);
        static char st[8192];
        int n = e->get_param(in, "state", st, sizeof st);
        void *in2 = e->create(dir ? dir : "");
        e->set_param(in2, "state", st);
        int bad = 0;
        for (auto &kv : set) {
            char b[32];
            if (e->get_param(in2, kv[0], b, sizeof b) <= 0 || strcmp(b, kv[1])) { printf("FAIL state: %s = %s, want %s\n", kv[0], b, kv[1]); ++bad; }
        }
        printf("%s state chunk: %d bytes, %d of %d values restored\n", bad ? "FAIL" : "ok  ", n, (int)(sizeof set / sizeof *set) - bad, (int)(sizeof set / sizeof *set));
        fails += bad;
        e->destroy(in2);
    }
    e->destroy(in);
    printf(fails ? "FAILED %d\n" : "PASSED\n", fails);
    return fails != 0;
}
