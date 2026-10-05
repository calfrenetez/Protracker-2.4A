/* Optional public-registration shared diagnostic with real Exec RAM adapters.
 * SOURCE_ONLY_NOT_RUN: this is a distinct target from the ordinary-malloc build.
 * TypeOfMem/zero-budget assertions become placement evidence only after exact
 * runtime qualification. Synthetic receipts remain software ownership only.
 * No DMA/audio/device/card/IRQ operation or stack/launch clearance is implied.
 */
#include "native_exec_memory.h"
#include "../src/native/paula_memory.h"
#define PT_COMPACT_NO_MAIN
#include "readers_backend_registration_test.c"

static size_t registration_native_chip_bytes;
static unsigned registration_native_chip_calls;
static void *registration_native_allocate(void *context,size_t bytes)
{(void)context;return native_allocate(bytes);}
static void registration_native_release(void *context,void *p)
{(void)context;native_release(p);}
static void *registration_native_chip_allocate(void *context,size_t bytes)
{
    void *p=pt_paula_chip_allocate(context,bytes);
    if(p){
        assert((TypeOfMem(p)&(MEMF_FAST|MEMF_CHIP))==MEMF_CHIP);
        ++registration_native_chip_calls;registration_native_chip_bytes+=bytes;
    }
    return p;
}
static void registration_native_chip_release(void *context,void *p,size_t bytes)
{
    assert(p&&bytes<=registration_native_chip_bytes);
    assert((TypeOfMem(p)&(MEMF_FAST|MEMF_CHIP))==MEMF_CHIP);
    registration_native_chip_bytes-=bytes;pt_paula_chip_release(context,p,bytes);
}
int main(void)
{
    const struct pt_compact_song_memory memory={{NULL,registration_native_allocate,registration_native_release},
        NULL,registration_native_chip_allocate,registration_native_chip_release};
    int result;
    native_memory_start();result=pt_readers_backend_registration_test(&memory);
    assert(registration_native_chip_calls&&!registration_native_chip_bytes);
    printf("EXEC READERS REGISTRATION CHIP PASS: %u Chip-only allocations, zero owned bytes; no DMA\n",registration_native_chip_calls);
    native_memory_finish();return result;
}
