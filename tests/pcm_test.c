#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "pcm.h"
#include "wav.h"

int main(void)
{
    int32_t data[16] = {1, 101, 2, 102, 3, 103, 4, 104}, dest[32] = {0}, copy[16];
    struct pt_pcm pcm = {data, 16, 4, 48000, 2, 24}, output = {dest, 32, 4, 48000, 2, 24};
    uint8_t wav[256], malformed[256];
    size_t written, i;
    uint32_t frames;
    struct pt_wav_info info;
    assert(pt_pcm_edit(&pcm, PT_PCM_REVERSE, 0, 4, 0) == PT_PCM_OK);
    assert(data[0] == 4 && data[1] == 104 && data[6] == 1 && data[7] == 101);
    memcpy(copy, data, sizeof(data));
    assert(pt_pcm_edit(&pcm, PT_PCM_GAIN, 0, 5, 1000) == PT_PCM_INVALID);
    assert(!memcmp(copy, data, sizeof(data)));
    assert(pt_pcm_edit(&pcm, PT_PCM_GAIN, 0, 4, 1000) == PT_PCM_OK);
    assert(!memcmp(copy, data, sizeof(data))); /* Retain 24-bit low bits. */
    pcm.channels = 1; pcm.frames = 4;
    data[0] = -8388608; data[1] = 8388607; data[2] = 128; data[3] = -128;
    output.channels = 1; output.bits = 16;
    assert(pt_pcm_convert(&pcm, &output) == PT_PCM_OK);
    assert(dest[0] == -32768 && dest[1] == 32767 && dest[2] == 1 && dest[3] == -1);
    data[0] = 0; data[1] = 256; data[2] = 512; data[3] = 768;
    output.bits = 24; output.rate = 96000; output.frames = 8;
    assert(pt_pcm_resampled_frames(&pcm, 96000, &frames) == PT_PCM_OK && frames == 8);
    assert(pt_pcm_resample(&pcm, &output) == PT_PCM_OK);
    assert(dest[0] == 0 && dest[1] == 128 && dest[2] == 256 && dest[6] == 768 && dest[7] == 768);
    output.data = data; output.capacity = 16;
    assert(pt_pcm_resample(&pcm, &output) == PT_PCM_ALIAS);
    output.data = dest; output.capacity = 32;
    data[0] = 100; data[1] = 100; data[2] = 100; data[3] = 100;
    assert(pt_pcm_edit(&pcm, PT_PCM_FADE_IN, 0, 4, 0) == PT_PCM_OK);
    assert(data[0] == 0 && data[1] == 33 && data[2] == 66 && data[3] == 100);
    assert(pt_pcm_edit(&pcm, PT_PCM_FADE_OUT, 3, 4, 0) == PT_PCM_OK && data[3] == 0);
    data[0] = -2; data[1] = 0; data[2] = 2; data[3] = 4;
    assert(pt_pcm_edit(&pcm, PT_PCM_REMOVE_DC, 0, 4, 0) == PT_PCM_OK);
    assert(data[0] == -3 && data[1] == -1 && data[2] == 1 && data[3] == 3);
    pcm.bits = 8;
    assert(pt_pcm_edit(&pcm, PT_PCM_NORMALIZE, 0, 4, 0) == PT_PCM_OK);
    assert(data[0] == -127 && data[3] == 127);
    assert(pt_pcm_edit(&pcm, PT_PCM_GAIN, 0, 4, 8000) == PT_PCM_OK);
    assert(data[0] == -128 && data[3] == 127);
    memset(data, 0, sizeof(data));
    assert(pt_pcm_edit(&pcm, PT_PCM_NORMALIZE, 0, 4, 0) == PT_PCM_OK && data[0] == 0);
    for (pcm.bits = 8; pcm.bits <= 24; pcm.bits += 8) {
        int32_t maximum = ((int32_t)1 << (pcm.bits - 1)) - 1;
        pcm.frames = 3; pcm.channels = 1;
        data[0] = -maximum - 1; data[1] = 1; data[2] = maximum;
        output = pcm; output.data = dest; output.capacity = 32;
        assert(pt_wav_encode(&pcm, wav, sizeof(wav), &written) == PT_WAV_OK);
        assert(pt_wav_inspect(wav, written, &info) == PT_WAV_OK);
        assert(info.bits == pcm.bits && info.frames == 3 && info.rate == 48000);
        assert(pt_wav_decode(wav, written, &output) == PT_WAV_OK);
        assert(!memcmp(data, dest, 3 * sizeof(int32_t)));
        for (i = 0; i < written; ++i) assert(pt_wav_inspect(wav, i, &info) != PT_WAV_OK);
    }
    pcm.bits = 24; pcm.frames = 4; pcm.channels = 2;
    data[0] = 0x123456; data[1] = -0x123457; data[2] = 1; data[3] = -1;
    data[4] = 8388607; data[5] = -8388608; data[6] = 127; data[7] = -128;
    output = pcm; output.data = dest; output.capacity = 32;
    assert(pt_wav_encode(&pcm, wav, sizeof(wav), &written) == PT_WAV_OK);
    assert(pt_wav_decode(wav, written, &output) == PT_WAV_OK && !memcmp(data, dest, 32));
    memcpy(malformed, wav, written); malformed[40] = 255;
    assert(pt_wav_inspect(malformed, written, &info) == PT_WAV_TRUNCATED);
    memcpy(malformed, wav, written); malformed[32] = 1;
    assert(pt_wav_inspect(malformed, written, &info) == PT_WAV_INVALID);
    memcpy(malformed, wav, written); malformed[20] = 3;
    assert(pt_wav_inspect(malformed, written, &info) == PT_WAV_UNSUPPORTED);
    memset(malformed, 0xa5, sizeof(malformed));
    assert(pt_wav_encode(&pcm, malformed, written - 1, &i) == PT_WAV_CAPACITY && malformed[0] == 0xa5);
    data[7] = 8388608;
    memcpy(copy, data, sizeof(data));
    assert(pt_pcm_edit(&pcm, PT_PCM_REVERSE, 0, 4, 0) == PT_PCM_INVALID);
    assert(!memcmp(copy, data, sizeof(data)));
    puts("PCM/WAV PASS: 8/16/24-bit precision, stereo edits, conversion, resampling, bounds and round trips");
    return 0;
}
