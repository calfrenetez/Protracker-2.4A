#ifndef PT_MOD_INSPECT_H
#define PT_MOD_INSPECT_H
#include <stddef.h>
#include <stdint.h>

enum pt_mod_status {
    PT_MOD_OK, PT_MOD_SHORT_HEADER, PT_MOD_UNSUPPORTED_FORMAT,
    PT_MOD_BAD_SONG_LENGTH, PT_MOD_PATTERN_LIMIT, PT_MOD_SHORT_PATTERNS,
    PT_MOD_SHORT_SAMPLES, PT_MOD_BAD_INSTRUMENT, PT_MOD_INVALID_ARGUMENT
};
enum pt_mod_warning {
    PT_MOD_WARN_TRAILING = 1, PT_MOD_WARN_VOLUME = 2,
    PT_MOD_WARN_FINETUNE = 4, PT_MOD_WARN_LOOP = 8
};
struct pt_mod_info {
    uint32_t sample_offset, sample_bytes, required_bytes;
    uint16_t patterns;
    uint8_t song_length, extended_pattern_marker;
    unsigned int warnings;
};

/* Allocation-free, non-mutating preflight of 31-sample four-channel MODs.
 * The output is only committed on success. Unknown formats are not guessed.
 * Limits describe the pinned native 2.3F editor, not every MOD dialect. */
enum pt_mod_status pt_mod_inspect(const uint8_t *, size_t, struct pt_mod_info *);
const char *pt_mod_status_name(enum pt_mod_status);
#endif
