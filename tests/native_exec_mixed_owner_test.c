#include "native_exec_memory.h"
#include "../src/native/paula_memory.h"
#define PT_TEST_NATIVE_CHIP
#define malloc native_allocate
#define free native_release
#include "../src/native/eclock_alarm.h"
#include <proto/dos.h>
#define PT_TEST_MIXED_NATIVE_COST
#define PT_TEST_MIXED_EXEC
#include "mixed_owner_test.c"
#define PT_TEST_ADVANCE_NATIVE
#define main bulk_advance_fixture
#include "voice_advance_test.c"
#undef main
int main(void)
{
    int result;
    puts("NATIVE PHASE: memory start");fflush(stdout);
    native_memory_start();fflush(stdout);
    puts("NATIVE PHASE: bulk equivalence start");fflush(stdout);
    assert(!bulk_advance_fixture());fflush(stdout);
    puts("NATIVE PHASE: mixed running cost start");fflush(stdout);
    result=mixed_owner_fixture();fflush(stdout);
    native_memory_finish();fflush(stdout);
    return result;
}
