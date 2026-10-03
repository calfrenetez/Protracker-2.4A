#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "pcm.h"
#include "wav.h"

static void wav_output_alias_cases(void)
{
    static const uint8_t wire8[] = {0, 127, 128, 255};
    static const uint8_t wire16[] = {0, 128, 255, 255, 0, 0, 255, 127};
    static const uint8_t wire24[] = {1, 0, 128, 255, 255, 255, 0, 0, 0, 254, 255, 127};
    struct source { struct pt_pcm pcm; int32_t data[32]; } v, old;
    union bytes { size_t align; uint8_t data[256];
        struct { uint8_t prefix[128]; size_t written; } scalar; } out, saved;
    uint8_t expected[64];
    void *alias[6];
    size_t n, w, i, body;
    unsigned bits, channels, align, byte_rate;
    for (bits = 8; bits <= 24; bits += 8) for (channels = 1; channels <= 2; ++channels) {
        const uint8_t *wire = bits == 8 ? wire8 : bits == 16 ? wire16 : wire24;
        memset(&v, 0, sizeof(v));
        v.pcm = (struct pt_pcm){v.data, 32, 4 / channels, 48000, (uint8_t)channels, (uint8_t)bits};
        v.data[0] = bits == 24 ? -8388607 : -((int32_t)1 << (bits - 1));
        v.data[1] = -1; v.data[2] = 0;
        v.data[3] = bits == 24 ? 8388606 : ((int32_t)1 << (bits - 1)) - 1;
        for (i = 4; i < 32; ++i) v.data[i] = INT32_MAX; /* Padding is not active PCM. */
        old = v;
        body = 4 * (bits / 8); align = channels * (bits / 8); byte_rate = 48000 * align;
        memset(expected, 0, sizeof(expected));
        memcpy(expected, "RIFF", 4); expected[4] = (uint8_t)(36 + body);
        memcpy(expected + 8, "WAVEfmt ", 8); expected[16] = 16; expected[20] = 1;
        expected[22] = (uint8_t)channels; expected[24] = 0x80; expected[25] = 0xbb;
        expected[28] = (uint8_t)byte_rate; expected[29] = (uint8_t)(byte_rate >> 8);
        expected[30] = (uint8_t)(byte_rate >> 16);
        expected[32] = (uint8_t)align; expected[34] = (uint8_t)bits;
        memcpy(expected + 36, "data", 4); expected[40] = (uint8_t)body;
        memcpy(expected + 44, wire, body);
        n = 123; assert(pt_wav_size(&v.pcm, &n) == PT_WAV_OK && n == 44 + body);
        memset(&out, 0x55, sizeof(out));
        assert(pt_wav_encode(&v.pcm, out.data, sizeof(out.data), &w) == PT_WAV_OK && w == n);
        assert(!memcmp(out.data, expected, n) && !memcmp(&v, &old, sizeof(v)));
        for (i = n; i < sizeof(out.data); ++i) assert(out.data[i] == 0x55);
        memset(&out, 0x55, sizeof(out)); saved = out;
        alias[0] = &v.pcm; alias[1] = (uint8_t *)&v.pcm + sizeof(v.pcm) - 1;
        alias[2] = v.data; alias[3] = v.data + 3;
        alias[4] = v.data + 4; alias[5] = (uint8_t *)v.data + sizeof(v.data) - 1;
        for (i = 0; i < 6; ++i) {
            assert(pt_wav_size(&v.pcm, (size_t *)alias[i]) == PT_WAV_ALIAS);
            assert(!memcmp(&v, &old, sizeof(v)) && !memcmp(&out, &saved, sizeof(out)));
            assert(pt_wav_encode(&v.pcm, out.data, sizeof(out.data), (size_t *)alias[i]) == PT_WAV_ALIAS);
            assert(!memcmp(&v, &old, sizeof(v)) && !memcmp(&out, &saved, sizeof(out)));
            w = 123; assert(pt_wav_encode(&v.pcm, alias[i], sizeof(out.data), &w) == PT_WAV_ALIAS && w == 123);
            assert(!memcmp(&v, &old, sizeof(v)) && !memcmp(&out, &saved, sizeof(out)));
        }
        assert(pt_wav_size(&v.pcm, (size_t *)(UINTPTR_MAX - 1)) == PT_WAV_ALIAS);
        w = 123;
        assert(pt_wav_encode(&v.pcm, (uint8_t *)(UINTPTR_MAX - 1), sizeof(out.data), &w) == PT_WAV_ALIAS && w == 123);
        assert(pt_wav_encode(&v.pcm, out.data, sizeof(out.data), (size_t *)(UINTPTR_MAX - 1)) == PT_WAV_ALIAS);
        assert(pt_wav_encode(&v.pcm, out.data, sizeof(out.data), &out.align) == PT_WAV_ALIAS);
        assert(pt_wav_encode(&v.pcm, out.data, sizeof(out.data), (size_t *)(out.data + n - 1)) == PT_WAV_ALIAS);
        assert(!memcmp(&v, &old, sizeof(v)) && !memcmp(&out, &saved, sizeof(out)));
        /* Only actual encoded bytes overlap: disjoint written in spare output capacity is legal. */
        assert(pt_wav_encode(&v.pcm, out.data, sizeof(out.data), &out.scalar.written) == PT_WAV_OK);
        assert(!memcmp(out.data, expected, n) && out.scalar.written == n && !memcmp(&v, &old, sizeof(v)));
        memset(&out, 0x55, sizeof(out)); saved = out; w = 123;
        assert(pt_wav_encode(&v.pcm, (uint8_t *)v.data, n - 1, (size_t *)v.data) == PT_WAV_CAPACITY);
        assert(pt_wav_encode(&v.pcm, out.data, n - 1, &w) == PT_WAV_CAPACITY && w == 123);
        assert(pt_wav_encode(&v.pcm, NULL, sizeof(out.data), &w) == PT_WAV_INVALID && w == 123);
        assert(pt_wav_encode(&v.pcm, out.data, sizeof(out.data), NULL) == PT_WAV_INVALID);
        assert(pt_wav_size(&v.pcm, NULL) == PT_WAV_INVALID);
        assert(!memcmp(&v, &old, sizeof(v)) && !memcmp(&out, &saved, sizeof(out)));
        /* Odd mono payloads retain the exact RIFF pad byte and data length. */
        v.pcm.channels = 1; v.pcm.frames = 3; old = v; body = 3 * (bits / 8);
        assert(pt_wav_size(&v.pcm, &n) == PT_WAV_OK && n == 44 + body + (body & 1));
        assert(pt_wav_encode(&v.pcm, out.data, sizeof(out.data), &w) == PT_WAV_OK && w == n);
        assert(out.data[4] == n - 8 && out.data[40] == body && !memcmp(out.data + 44, wire, body));
        if (body & 1) assert(out.data[n - 1] == 0);
        assert(out.data[n] == 0x55 && !memcmp(&v, &old, sizeof(v)));
    }
    /* Invalid descriptors and active samples retain existing precedence. */
    memset(&out, 0x55, sizeof(out)); saved = out; n = w = 123;
    v.pcm.rate = 0; old = v;
    assert(pt_wav_size(&v.pcm, &n) == PT_WAV_INVALID && n == 123);
    assert(pt_wav_encode(&v.pcm, out.data, sizeof(out.data), (size_t *)v.data) == PT_WAV_INVALID);
    assert(!memcmp(&v, &old, sizeof(v)) && !memcmp(&out, &saved, sizeof(out)));
    v.pcm.rate = 48000; v.pcm.bits = 12; old = v;
    assert(pt_wav_size(&v.pcm, &n) == PT_WAV_INVALID && n == 123);
    assert(pt_wav_encode(&v.pcm, out.data, sizeof(out.data), &w) == PT_WAV_INVALID && w == 123);
    assert(!memcmp(&v, &old, sizeof(v)) && !memcmp(&out, &saved, sizeof(out)));
    v.pcm.bits = 24; v.data[0] = 8388608; old = v;
    assert(pt_wav_size(&v.pcm, &n) == PT_WAV_INVALID && n == 123);
    assert(pt_wav_encode(&v.pcm, out.data, sizeof(out.data), (size_t *)v.data) == PT_WAV_INVALID);
    assert(!memcmp(&v, &old, sizeof(v)) && !memcmp(&out, &saved, sizeof(out)));
    v.data[0] = -8388607; v.pcm.capacity = 2; old = v;
    assert(pt_wav_size(&v.pcm, &n) == PT_WAV_INVALID && n == 123);
    assert(pt_wav_encode(&v.pcm, out.data, sizeof(out.data), &w) == PT_WAV_INVALID && w == 123);
    assert(!memcmp(&v, &old, sizeof(v)) && !memcmp(&out, &saved, sizeof(out)));
    v.pcm.capacity = SIZE_MAX / sizeof(int32_t) + 1; old = v;
    assert(pt_wav_size(&v.pcm, &n) == PT_WAV_ALIAS && n == 123);
    assert(pt_wav_encode(&v.pcm, out.data, sizeof(out.data), &w) == PT_WAV_ALIAS && w == 123);
    assert(!memcmp(&v, &old, sizeof(v)) && !memcmp(&out, &saved, sizeof(out)));
    v.pcm.frames = 0; v.pcm.capacity = 1; v.pcm.data = (int32_t *)(UINTPTR_MAX - 1); old = v;
    assert(pt_wav_size(&v.pcm, &n) == PT_WAV_ALIAS && n == 123);
    assert(pt_wav_encode(&v.pcm, out.data, sizeof(out.data), &w) == PT_WAV_ALIAS && w == 123);
    assert(!memcmp(&v, &old, sizeof(v)) && !memcmp(&out, &saved, sizeof(out)));
    v.pcm.data = NULL; old = v;
    assert(pt_wav_size(&v.pcm, &n) == PT_WAV_ALIAS && n == 123 && !memcmp(&v, &old, sizeof(v)));
    v.pcm.capacity = 0; old = v;
    assert(pt_wav_size(&v.pcm, &n) == PT_WAV_OK && n == 44);
    assert(pt_wav_encode(&v.pcm, NULL, 44, &w) == PT_WAV_INVALID && w == 123);
    assert(pt_wav_encode(&v.pcm, out.data, 43, &w) == PT_WAV_CAPACITY && w == 123);
    assert(!memcmp(&out, &saved, sizeof(out)));
    assert(pt_wav_encode(&v.pcm, out.data, sizeof(out.data), &w) == PT_WAV_OK && w == 44);
    assert(out.data[4] == 36 && out.data[40] == 0 && out.data[44] == 0x55 && !memcmp(&v, &old, sizeof(v)));
}

static void resampled_frame_output_cases(void)
{
    struct source {struct pt_pcm pcm;int32_t data[16];uint32_t adjacent;} v, old;
    uint32_t out, *aliases[6];unsigned bits, channels, i;
    for(bits=8;bits<=24;bits+=8)for(channels=1;channels<=2;++channels) {
        memset(&v,0,sizeof(v));
        v.pcm=(struct pt_pcm){v.data,16,4,48000,(uint8_t)channels,(uint8_t)bits};
        for(i=0;i<16;++i)v.data[i]=i<4*channels?1:INT32_MAX;
        if(bits==24)v.data[0]=0x123457; /* Preserve the low eight source bits. */
        v.adjacent=123;memcpy(&old,&v,sizeof(v));out=123;
        assert(pt_pcm_resampled_frames(&v.pcm,96000,&out)==PT_PCM_OK && out==8);
        assert(pt_pcm_resampled_frames(&v.pcm,44100,&out)==PT_PCM_OK && out==4);
        assert(pt_pcm_resampled_frames(&v.pcm,12000,&out)==PT_PCM_OK && out==1);
        assert(!memcmp(&v,&old,sizeof(v)));
        aliases[0]=(uint32_t *)v.data;aliases[1]=(uint32_t *)(v.data+4*channels-1);
        aliases[2]=(uint32_t *)(v.data+4*channels);aliases[3]=(uint32_t *)(v.data+15);
        aliases[4]=&v.pcm.frames;aliases[5]=&v.pcm.rate;
        for(i=0;i<6;++i) {
            assert(pt_pcm_resampled_frames(&v.pcm,96000,aliases[i])==PT_PCM_ALIAS);
            assert(!memcmp(&v,&old,sizeof(v)));
        }
        /* An actual scalar immediately outside declared master storage is legal. */
        assert(pt_pcm_resampled_frames(&v.pcm,96000,&v.adjacent)==PT_PCM_OK && v.adjacent==8);
        assert(!memcmp(&v,&old,offsetof(struct source,adjacent)));v.adjacent=123;
        v.pcm.frames=0;memcpy(&old,&v,sizeof(v));out=123;
        assert(pt_pcm_resampled_frames(&v.pcm,96000,&out)==PT_PCM_OK && out==0);
        assert(pt_pcm_resampled_frames(&v.pcm,96000,(uint32_t *)(v.data+15))==PT_PCM_ALIAS);
        assert(!memcmp(&v,&old,sizeof(v)));
        /* Count remains metadata-only, even with values invalid at declared precision. */
        v.pcm.frames=1;v.data[0]=INT32_MAX;memcpy(&old,&v,sizeof(v));
        assert(pt_pcm_resampled_frames(&v.pcm,48000,&out)==PT_PCM_OK && out==1);
        assert(!memcmp(&v,&old,sizeof(v)));
    }
    out=123;v.pcm=(struct pt_pcm){v.data,16,4,48000,1,16};memcpy(&old,&v,sizeof(v));
    assert(pt_pcm_resampled_frames(&v.pcm,0,(uint32_t *)v.data)==PT_PCM_INVALID);
    assert(pt_pcm_resampled_frames(&v.pcm,192001,(uint32_t *)v.data)==PT_PCM_INVALID);
    assert(pt_pcm_resampled_frames(&v.pcm,48000,NULL)==PT_PCM_INVALID);
    assert(pt_pcm_resampled_frames(NULL,48000,&out)==PT_PCM_INVALID && out==123);
    assert(!memcmp(&v,&old,sizeof(v)));
    v.pcm.rate=0;memcpy(&old,&v,sizeof(v));
    assert(pt_pcm_resampled_frames(&v.pcm,48000,(uint32_t *)v.data)==PT_PCM_INVALID);
    assert(!memcmp(&v,&old,sizeof(v)));
    v.pcm.rate=48000;v.pcm.capacity=1;memcpy(&old,&v,sizeof(v));
    assert(pt_pcm_resampled_frames(&v.pcm,48000,(uint32_t *)v.data)==PT_PCM_CAPACITY);
    assert(pt_pcm_resampled_frames(&v.pcm,0,NULL)==PT_PCM_CAPACITY);
    assert(!memcmp(&v,&old,sizeof(v)));
    v.pcm=(struct pt_pcm){v.data,UINT32_MAX,UINT32_MAX,1,1,16};memcpy(&old,&v,sizeof(v));
    assert(pt_pcm_resampled_frames(&v.pcm,192000,(uint32_t *)v.data)==PT_PCM_CAPACITY);
    assert(!memcmp(&v,&old,sizeof(v)));
    v.pcm=(struct pt_pcm){v.data,16,0,48000,1,16};memcpy(&old,&v,sizeof(v));
    assert(pt_pcm_resampled_frames(&v.pcm,48000,(uint32_t *)(UINTPTR_MAX-(sizeof(uint32_t)-1)))==PT_PCM_ALIAS);
    assert(!memcmp(&v,&old,sizeof(v)));
    v.pcm.capacity=SIZE_MAX/sizeof(*v.data)+1;memcpy(&old,&v,sizeof(v));
    assert(pt_pcm_resampled_frames(&v.pcm,48000,&out)==PT_PCM_ALIAS && out==123);
    assert(pt_pcm_resampled_frames(&v.pcm,0,NULL)==PT_PCM_INVALID);
    assert(!memcmp(&v,&old,sizeof(v)));
    v.pcm.capacity=1;v.pcm.data=(int32_t *)(UINTPTR_MAX-(sizeof(int32_t)-1));memcpy(&old,&v,sizeof(v));
    assert(pt_pcm_resampled_frames(&v.pcm,48000,&out)==PT_PCM_ALIAS && out==123);
    assert(!memcmp(&v,&old,sizeof(v)));
    v.pcm.data=NULL;memcpy(&old,&v,sizeof(v));
    assert(pt_pcm_resampled_frames(&v.pcm,48000,&out)==PT_PCM_ALIAS && out==123);
    assert(!memcmp(&v,&old,sizeof(v)));
    v.pcm.capacity=0;memcpy(&old,&v,sizeof(v));
    assert(pt_pcm_resampled_frames(&v.pcm,48000,&out)==PT_PCM_OK && out==0);
    assert(!memcmp(&v,&old,sizeof(v)));
    puts("PCM FRAME OUTPUT PASS: metadata-only 8/16/24 mono/stereo counts, descriptor and full capacity protection, empty/NULL/precedence and fail-closed spans");
}

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
    resampled_frame_output_cases();
    wav_output_alias_cases();
    puts("PCM/WAV PASS: 8/16/24-bit precision, stereo edits, conversion, resampling, bounds and round trips");
    return 0;
}
