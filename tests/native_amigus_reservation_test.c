/* Link the production library adapter but exercise only the fake-library owner.
 * Taking its callback table does not open amigus.library or reserve a card. */
#include "../src/native/amigus_reservation.h"
#define main reservation_fixture_main
#include "amigus_reservation_test.c"
#undef main
int main(void)
{
    struct pt_native_amigus_library library = {0};
    struct pt_amigus_reservation_api api = pt_native_amigus_reservation_api(&library);
    assert(api.context == &library && api.open && api.close && api.reserve && api.release);
    assert(!library.base);
    return reservation_fixture_main();
}
