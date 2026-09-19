#include <stddef.h>
#include <stdint.h>
#include "mod_inspect.h"
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    struct pt_mod_info info;
    (void)pt_mod_inspect(data, size, &info);
    return 0;
}
