#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "channels.h"
#include "pcm.h"
#include "wav.h"

static uint32_t seed = 0x24a;
static uint32_t next(void) { seed ^= seed << 13; seed ^= seed >> 17; seed ^= seed << 5; return seed; }
int main(void)
{
    struct pt_channels channels, before;
    int32_t samples[32], decoded[32];
    struct pt_pcm pcm = {samples, 32, 16, 48000, 2, 24};
    uint8_t wav[256];
    size_t written;
    unsigned int i, iteration, accepted_wav = 0;
    pt_channels_init(&channels);
    assert(pt_channels_resize(&channels, 16) == PT_CHANNEL_OK);
    for (i = 0; i < 32; ++i) samples[i] = (int32_t)(next() % 16777216) - 8388608;
    assert(pt_wav_encode(&pcm, wav, sizeof(wav), &written) == PT_WAV_OK);
    for (iteration = 0; iteration < 100000; ++iteration) {
        size_t length = iteration % 3 ? written : next() % (written + 1);
        uint8_t *data = malloc(length ? length : 1);
        struct pt_wav_info info;
        unsigned int edits = next() % 6, start = next() % 17, end = next() % 17;
        enum pt_channel_result result;
        int8_t map[16];
        assert(data);
        memcpy(data, wav, length);
        for (i = 0; i < edits && length; ++i) data[next() % length] = (uint8_t)next();
        if (pt_wav_inspect(data, length, &info) == PT_WAV_OK && info.frames <= 32U / info.channels) {
            struct pt_pcm output = {decoded, 32, info.frames, info.rate, info.channels, info.bits};
            assert(pt_wav_decode(data, length, &output) == PT_WAV_OK);
            assert(pt_pcm_validate(&output) == PT_PCM_OK);
            ++accepted_wav;
        }
        free(data);
        before = channels;
        result = pt_channels_route(&channels, next() % 18, (enum pt_route)(next() % 9));
        if (result != PT_CHANNEL_OK) assert(!memcmp(&before, &channels, sizeof(before)));
        assert(pt_channels_validate(&channels) == PT_CHANNEL_OK);
        assert(pt_channels_paula_map(&channels, NULL, map) == PT_CHANNEL_OK);
        if (start > end) { unsigned int swap = start; start = end; end = swap; }
        assert(pt_pcm_edit(&pcm, (enum pt_pcm_edit)(next() % 6), start, end, next() % 8001) == PT_PCM_OK);
        assert(pt_pcm_validate(&pcm) == PT_PCM_OK);
    }
    printf("CORE MUTATIONS PASS seed=0x24a iterations=100000 decoded_wav=%u route_and_pcm_sequences=100000\n", accepted_wav);
    return 0;
}
