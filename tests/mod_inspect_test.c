#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "mod_inspect.h"

static void header(uint8_t *data, size_t size, int patterns)
{
    memset(data, 0, size);
    data[950] = 1;
    data[952] = (uint8_t)(patterns - 1);
    memcpy(data + 1080, patterns > 64 ? "M!K!" : "M.K.", 4);
}
static void reject(const uint8_t *data, size_t size, enum pt_mod_status expected)
{
    struct pt_mod_info info, before;
    memset(&info, 0xa5, sizeof(info)); memcpy(&before, &info, sizeof(info));
    assert(pt_mod_inspect(data, size, &info) == expected);
    assert(!memcmp(&info, &before, sizeof(info)));
}
int main(void)
{
    const size_t capacity = 1084 + 100 * 1024 + 31 * 65535 * 2;
    uint8_t *data = malloc(capacity + 1);
    struct pt_mod_info info;
    size_t n;
    assert(data);
    header(data, capacity + 1, 1);
    for (n = 0; n < 1084; ++n) reject(data, n, PT_MOD_SHORT_HEADER);
    for (n = 1084; n < 2108; ++n) reject(data, n, PT_MOD_SHORT_PATTERNS);
    assert(pt_mod_inspect(data, 2108, &info) == PT_MOD_OK && !info.warnings);
    data[950] = 0; reject(data, 2108, PT_MOD_BAD_SONG_LENGTH);
    data[950] = 129; reject(data, 2108, PT_MOD_BAD_SONG_LENGTH);
    data[950] = 1; data[1080] = 'F'; reject(data, 2108, PT_MOD_UNSUPPORTED_FORMAT);
    header(data, capacity + 1, 100);
    assert(pt_mod_inspect(data, 103484, &info) == PT_MOD_OK && info.patterns == 100);
    data[1079] = 100; reject(data, capacity, PT_MOD_PATTERN_LIMIT);
    data[1079] = 255; reject(data, capacity, PT_MOD_PATTERN_LIMIT);
    header(data, capacity + 1, 65);
    memcpy(data + 1080, "M.K.", 4); reject(data, capacity, PT_MOD_PATTERN_LIMIT);
    header(data, capacity + 1, 1);
    data[1079] = 1;
    assert(pt_mod_inspect(data, 3132, &info) == PT_MOD_OK && info.sample_offset == 3132);
    header(data, capacity + 1, 1);
    data[42] = 0; data[43] = 32;
    for (n = 2108; n < 2172; ++n) reject(data, n, PT_MOD_SHORT_SAMPLES);
    assert(pt_mod_inspect(data, 2172, &info) == PT_MOD_OK && info.sample_bytes == 64);
    data[1084] = 0x20; reject(data, 2172, PT_MOD_BAD_INSTRUMENT);
    data[1084] = 0x10; data[1086] = 0xf0;
    assert(pt_mod_inspect(data, 2172, &info) == PT_MOD_OK);
    data[44] = 0x80; data[45] = 65; data[46] = 0xff; data[47] = 0xff; data[49] = 2;
    assert(pt_mod_inspect(data, 2173, &info) == PT_MOD_OK && info.warnings == 15);
    header(data, capacity + 1, 100);
    for (n = 0; n < 31; ++n) { data[42 + n * 30] = 255; data[43 + n * 30] = 255; }
    assert(pt_mod_inspect(data, capacity, &info) == PT_MOD_OK);
    assert(info.required_bytes == capacity && info.sample_bytes == 4063170);
    reject(NULL, 0, PT_MOD_INVALID_ARGUMENT);
    assert(pt_mod_inspect(data, capacity, NULL) == PT_MOD_INVALID_ARGUMENT);
    free(data);
    puts("mod inspection: truncation, bounds, limits and non-mutation passed");
    return 0;
}
