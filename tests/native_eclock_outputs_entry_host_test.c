/* Execute the exact native entry body against the existing fake-Exec API. */
#define main eclock_output_helpers_main
#include "native_eclock_output_alias_test.c"
#undef main
#define PT_ECLOCK_OUTPUTS_HOLD() assert(0 && "native fixture unexpectedly retained owner")
#define main native_eclock_outputs_main
#include "native_exec_eclock_outputs_test.c"
#undef main
int main(void)
{
    memset(&request_storage,0,sizeof(request_storage));memset(&port_storage,0,sizeof(port_storage));
    memset(&device_storage,0,sizeof(device_storage));device_storage.value.dd_Library.lib_Version=36;
    assert(native_eclock_outputs_main()==0);
    assert(reads==25 && !ports && !requests && !opens);return 0;
}
