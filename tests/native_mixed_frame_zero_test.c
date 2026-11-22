#define PT_MIXED_CAUSAL_TEST_MAIN all_mixed_causal_cases
#include "mixed_readers_causal_test.c"

int main(void)
{
    unsigned mode;
    for (mode = 0; mode < 4; ++mode) {
        printf("PT frame-zero case %u entering\n", mode);
        fflush(stdout);
        ct_first_frame(mode);
        printf("PT frame-zero case %u finished\n", mode);
        fflush(stdout);
    }
    assert(ct_cases == 4);
    puts("PT FRAME ZERO PASS: four original boundary cases; SOFTWARE_ONLY");
    return 0;
}
