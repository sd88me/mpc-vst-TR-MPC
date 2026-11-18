/* er99_pot_defaults: every pot's default position (0..127) as the engine seeds it, as JSON {key: pot}. 9X9's gen_module.py
 * writes them into its module.mpc.json ("default"), so the panel can mark where each control leaves the 909 as it is.
 *   cc -Isrc -Isrc/dsp -Isrc/host src/tools/er99_pot_defaults.c src/dsp/er99_engine.c -lm && ./a.out > ../9x9/pot_defaults.json */
#include "er99_engine.h"
#include <stdio.h>
static er99_engine_t E;
int main(void)
{
    er99_engine_init(&E, 44100.0f, NULL);
    printf("{");
    for(int i = 0; i < ER99_POT_COUNT; ++i)
        printf("%s\"%s\": %d", i ? ", " : "", g_er99_pots[i].key, (int)E.pots[i]);
    printf("}\n");
    er99_engine_free(&E);
    return 0;
}
