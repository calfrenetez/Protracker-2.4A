/* Deterministic mutation runner when libFuzzer is not installed.
 * Uses exact-sized allocations so ASan detects reads past the supplied length. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mod_inspect.h"

static uint32_t state = 0x24;
static uint32_t random_word(void)
{
    state ^= state << 13; state ^= state >> 17; state ^= state << 5;
    return state;
}
int main(void)
{
    unsigned int iteration, counts[9] = {0};
    uint8_t *seed = calloc(1, 103484 + 64);
    if (!seed) return 1;
    seed[950] = 1; seed[43] = 32;
    for (iteration = 0; iteration < 200000; ++iteration) {
        uint32_t full, size, edits, j;
        uint8_t *data;
        struct pt_mod_info info;
        enum pt_mod_status result;
        int extended = iteration % 257 == 0;
        seed[952] = extended ? 99 : 0;
        memcpy(seed + 1080, extended ? "M!K!" : "M.K.", 4);
        full = extended ? 103484 + 64 : 2172;
        size = iteration % 3 ? full : random_word() % (full + 1);
        data = malloc(size ? size : 1);
        if (!data) { free(seed); return 1; }
        memcpy(data, seed, size);
        edits = random_word() % 9;
        for (j = 0; j < edits && size; ++j)
            data[random_word() % size] = (uint8_t)random_word();
        result = pt_mod_inspect(data, size, &info);
        if ((unsigned)result >= 9) abort();
        ++counts[result];
        free(data);
    }
    free(seed);
    printf("MODFUZZ seed=0x24 iterations=200000\n");
    for (iteration = 0; iteration < 9; ++iteration)
        printf("%s=%u\n", pt_mod_status_name((enum pt_mod_status)iteration), counts[iteration]);
    return 0;
}
