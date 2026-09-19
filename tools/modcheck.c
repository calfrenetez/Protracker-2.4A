/* Read-only preflight. Does not load a song into, or modify, the tracker. */
#include <stdio.h>
#include <stdlib.h>
#include "mod_inspect.h"

int main(int argc, char **argv)
{
    FILE *file;
    long size;
    uint8_t *data;
    size_t got;
    int close_result;
    struct pt_mod_info info;
    enum pt_mod_status result;
    if (argc != 2) { fputs("usage: PTModCheck FILE\n", stderr); return 20; }
    file = fopen(argv[1], "rb");
    if (!file) { fputs("ERROR open\n", stderr); return 20; }
    if (fseek(file, 0, SEEK_END) || (size = ftell(file)) < 0 ||
        size > 16L * 1024 * 1024 || fseek(file, 0, SEEK_SET)) {
        fclose(file); fputs("ERROR size-or-seek (limit 16 MiB)\n", stderr); return 20;
    }
    data = malloc(size ? (size_t)size : 1);
    if (!data) { fclose(file); fputs("ERROR allocation\n", stderr); return 20; }
    got = fread(data, 1, (size_t)size, file);
    close_result = fclose(file);
    if (got != (size_t)size || close_result) {
        free(data); fputs("ERROR read-or-close\n", stderr); return 20;
    }
    result = pt_mod_inspect(data, got, &info);
    free(data);
    printf("MODCHECK schema=1 result=%s", pt_mod_status_name(result));
    if (result == PT_MOD_OK)
        printf(" orders=%u patterns=%u sample_offset=%lu sample_bytes=%lu "
               "required_bytes=%lu warnings=0x%x", (unsigned)info.song_length,
               (unsigned)info.patterns, (unsigned long)info.sample_offset,
               (unsigned long)info.sample_bytes, (unsigned long)info.required_bytes,
               info.warnings);
    puts("");
    return result == PT_MOD_OK ? (info.warnings ? 5 : 0) : 20;
}
