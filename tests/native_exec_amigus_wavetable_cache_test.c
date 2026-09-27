#include "native_exec_memory.h"
#include "../src/native/amigus_reservation.h"
#include <amigus/amigus.h>
#define malloc native_allocate
#define free native_release
#define PT_WAVETABLE_NATIVE
#include "amigus_wavetable_cache_test.c"
int main(void)
{
    struct pt_native_amigus_library library={0};
    struct pt_amigus_reservation_api api=pt_native_amigus_reservation_api(&library);
    struct AmiGUS descriptor={0};int result;
    assert(api.context==&library && api.open && !library.base);
    /* Production descriptor inspection only; no library call or card pointer. */
    descriptor.agus_TypeId=AmiGUS_mini;descriptor.agus_PcmBase=&descriptor;
    assert(api.supported(api.context,&descriptor,PT_AMIGUS_PCM));
    assert(!api.supported(api.context,&descriptor,PT_AMIGUS_WAVETABLE));
    descriptor.agus_PcmBase=0;descriptor.agus_WavetableBase=&descriptor;
    assert(!api.supported(api.context,&descriptor,PT_AMIGUS_PCM));
    assert(api.supported(api.context,&descriptor,PT_AMIGUS_WAVETABLE));
    assert(!api.supported(api.context,&descriptor,3));
    descriptor.agus_TypeId=0;assert(!api.supported(api.context,&descriptor,PT_AMIGUS_WAVETABLE));
    native_memory_start();result=wavetable_fixture_main();native_memory_finish();return result;
}
