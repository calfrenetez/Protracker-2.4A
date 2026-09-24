#include "native_exec_memory.h"
#include "../src/native/amigus_reservation.h"
#define malloc native_allocate
#define free native_release
#define PT_RESERVED_SESSION_NATIVE
#include "amigus_reserved_session_test.c"
int main(void)
{
    struct pt_native_amigus_library library={0};
    struct pt_amigus_reservation_api api=pt_native_amigus_reservation_api(&library);
    int result;
    assert(api.context==&library && api.open && !library.base);
    native_memory_start();result=reserved_session_fixture_main();native_memory_finish();return result;
}
