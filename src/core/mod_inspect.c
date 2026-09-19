#include "mod_inspect.h"

static uint32_t be16(const uint8_t *p)
{ return ((uint32_t)p[0] << 8) | p[1]; }

enum pt_mod_status pt_mod_inspect(const uint8_t *data, size_t length,
                                struct pt_mod_info *out)
{
    struct pt_mod_info info = {0};
    uint32_t i, maximum = 0;
    if (!data || !out) return PT_MOD_INVALID_ARGUMENT;
    if (length < 1084) return PT_MOD_SHORT_HEADER;
    if (data[1080] != 'M' || data[1082] != 'K' ||
        !((data[1081] == '.' && data[1083] == '.') ||
          (data[1081] == '!' && data[1083] == '!')))
        return PT_MOD_UNSUPPORTED_FORMAT;
    info.extended_pattern_marker = data[1081] == '!';
    info.song_length = data[950];
    if (!info.song_length || info.song_length > 128) return PT_MOD_BAD_SONG_LENGTH;
    /* Native 2.3F reads all 128 orders, including currently unused positions.
     * Do not derive the sample offset from active positions alone. */
    for (i = 0; i < 128; ++i) if (data[952 + i] > maximum) maximum = data[952 + i];
    info.patterns = (uint16_t)(maximum + 1);
    if (info.patterns > (info.extended_pattern_marker ? 100 : 64))
        return PT_MOD_PATTERN_LIMIT;
    info.sample_offset = 1084 + (uint32_t)info.patterns * 1024;
    if (length < info.sample_offset) return PT_MOD_SHORT_PATTERNS;
    for (i = 0; i < 31; ++i) {
        const uint8_t *sample = data + 20 + i * 30;
        uint32_t words = be16(sample + 22), loop_start = be16(sample + 26);
        uint32_t loop_words = be16(sample + 28);
        info.sample_bytes += words * 2;
        if (sample[25] > 64) info.warnings |= PT_MOD_WARN_VOLUME;
        if (sample[24] > 15) info.warnings |= PT_MOD_WARN_FINETUNE;
        if (loop_words > 1 && (loop_start > words || loop_words > words - loop_start))
            info.warnings |= PT_MOD_WARN_LOOP;
    }
    /* Fixed maxima: 100 patterns + 31 * 65535 sample words fit uint32_t. */
    info.required_bytes = info.sample_offset + info.sample_bytes;
    if (length < info.required_bytes) return PT_MOD_SHORT_SAMPLES;
    if (length > info.required_bytes) info.warnings |= PT_MOD_WARN_TRAILING;
    for (i = 1084; i < info.sample_offset; i += 4) {
        unsigned int instrument = (data[i] & 0xf0) | (data[i + 2] >> 4);
        if (instrument > 31) return PT_MOD_BAD_INSTRUMENT;
    }
    *out = info;
    return PT_MOD_OK;
}

const char *pt_mod_status_name(enum pt_mod_status status)
{
    switch (status) {
    case PT_MOD_OK: return "ok";
    case PT_MOD_SHORT_HEADER: return "short-header";
    case PT_MOD_UNSUPPORTED_FORMAT: return "unsupported-format";
    case PT_MOD_BAD_SONG_LENGTH: return "bad-song-length";
    case PT_MOD_PATTERN_LIMIT: return "pattern-limit";
    case PT_MOD_SHORT_PATTERNS: return "short-patterns";
    case PT_MOD_SHORT_SAMPLES: return "short-samples";
    case PT_MOD_BAD_INSTRUMENT: return "bad-instrument";
    default: return "invalid-argument";
    }
}
