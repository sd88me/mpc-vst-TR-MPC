/* l909_levels MODULE_DIR: each voice hit at velocity 127 (default panel) for 1.5 s: its peak out of the engine,
 * before master volume, and with the Lemus engine (ER99_ENGINE_LEMUS=1) each board lane's peak (DC-blocked).
 * Builds both ways; kLemTrim / kLemGain in er99_engine.c come from the two runs (9X9 calibration). */
#include "er99_engine.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
static er99_engine_t E;
int main(int argc, char **argv)
{
    const char *dir = argc > 1 ? argv[1] : NULL;
    for(int t = 0; t < ER99_NUM_TRIGGERS; ++t)
    {
        er99_engine_init(&E, 44100.0f, dir);
#if ER99_ENGINE_LEMUS
        E.latency = 0;
#endif
        float out[128];
        for(int k = 0; k < 50; ++k) er99_engine_render(&E, out, 128);
        er99_engine_trigger(&E, (er99_trigger_t)t, 127);
        double pk = 0, lpk = 0, dcx = 0, dcy = 0;
        for(int k = 0; k < 517; ++k)
        {
            er99_engine_render(&E, out, 128);
            for(int i = 0; i < 128; ++i) if(fabs(out[i]) > pk) pk = fabs(out[i]);
#if ER99_ENGINE_LEMUS
            static const int lane[ER99_NUM_TRIGGERS] = {0, 1, 2, 3, 4, 5, 6, 7, 7, 9, 8};
            for(int i = 0; i < 128; ++i)
            {
                const double x = E.lbuf[lane[t]][i], y = x - dcx + 0.99986 * dcy;
                dcx = x; dcy = y;
                if(fabs(y) > lpk) lpk = fabs(y);
            }
#endif
        }
        printf("%s %.5f %.5f\n", er99_trigger_names[t], pk / E.master.volume, lpk);
        er99_engine_free(&E);
    }
    return 0;
}
