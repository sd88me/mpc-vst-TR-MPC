/* l808_levels — each lane alone at its default knobs: an unaccented (vel 64) and an accented (127) hit,
 * 2 s, peak and RMS of the engine's output, and its own render time. Build twice (SC808_ENGINE_LEMUS=0/1)
 * to set kLemusTrim. Optional: l808_levels OUTDIR writes each lane's WAV (float 32). */
#include "sc808_engine.h"
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>
static void wav(const char *dir, const char *n, const std::vector<float> &d, int sr)
{
    char p[512]; snprintf(p, sizeof p, "%s/%s.wav", dir, n); FILE *f = fopen(p, "wb"); if(!f) return;
    auto u32 = [&](uint32_t v){ fwrite(&v,4,1,f); }; auto u16 = [&](uint16_t v){ fwrite(&v,2,1,f); };
    uint32_t b = d.size()*4; fwrite("RIFF",1,4,f); u32(36+b); fwrite("WAVEfmt ",1,8,f); u32(16); u16(3); u16(1);
    u32(sr); u32(sr*4); u16(4); u16(32); fwrite("data",1,4,f); u32(b); fwrite(d.data(),4,d.size(),f); fclose(f);
}
int main(int argc, char **argv)
{
    const int sr = 48000, n = 2 * sr;
    for(int v = 0; v < 16; ++v)
        for(int vel : {64, 127}) {
            sc808_engine_t *e = sc808_create(sr);
            std::vector<float> o(n);
            sc808_trigger(e, v, vel);
            auto t0 = std::chrono::steady_clock::now();
            for(int i = 0; i < n; i += 128) sc808_render(e, &o[i], 128);
            double dt = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
            float pk = 0; double ss = 0; int last = 0;
            for(int i = 0; i < n; ++i) { float a = fabsf(o[i]); if(a > pk) pk = a; ss += o[i]*o[i]; }
            for(int i = 0; i < n; ++i) if(fabsf(o[i]) > pk * 0.001f) last = i;
            printf("%-3s vel%3d peak %.4f rms %.4f  -60dB at %5.0f ms  cpu %.1f%%\n", sc808_voice_id(v), vel, pk,
                   sqrt(ss / n), last * 1000.0 / sr, 100 * dt / 2.0);
            if(argc > 1) { char nm[32]; snprintf(nm, sizeof nm, "%s_%d", sc808_voice_id(v), vel); wav(argv[1], nm, o, sr); }
            sc808_destroy(e);
        }
}
