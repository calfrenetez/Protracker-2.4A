#include <string.h>
#include "wav.h"

static uint32_t le16(const uint8_t *p) { return p[0] | ((uint32_t)p[1] << 8); }
static uint32_t le32(const uint8_t *p) { return le16(p) | (le16(p + 2) << 16); }
static void put16(uint8_t *p, uint32_t value) { p[0] = (uint8_t)value; p[1] = (uint8_t)(value >> 8); }
static void put32(uint8_t *p, uint32_t value) { put16(p, value); put16(p + 2, value >> 16); }
static int overlaps(const void *a, size_t na, const void *b, size_t nb)
{
    uintptr_t x = (uintptr_t)a, y = (uintptr_t)b;
    if (!na || !nb) return 0;
    if (na > UINTPTR_MAX - x || nb > UINTPTR_MAX - y) return 1;
    return x < y + nb && y < x + na;
}
enum pt_wav_result pt_wav_inspect_reader(pt_wav_read read,void *context,size_t length,struct pt_wav_info *out)
{
    struct pt_wav_info info = {0};uint8_t data[16];
    size_t end, pos = 12;
    unsigned int chunks = 0, fmt = 0, audio = 0;
    uint32_t align = 0;
    if (!read || !out) return PT_WAV_INVALID;
    if (length < 12) return PT_WAV_TRUNCATED;
    if (read(context,0,data,12)!=1)return PT_WAV_TRUNCATED;
    if (memcmp(data, "RIFF", 4) || memcmp(data + 8, "WAVE", 4)) return PT_WAV_UNSUPPORTED;
    if (le32(data + 4) < 4) return PT_WAV_INVALID;
    if (le32(data + 4) > length - 8) return PT_WAV_TRUNCATED;
    end = (size_t)le32(data + 4) + 8;
    while (pos < end) {
        uint32_t size;
        size_t body;
        if (++chunks > 4096) return PT_WAV_INVALID;
        if (end - pos < 8) return PT_WAV_TRUNCATED;
        if(read(context,pos,data,8)!=1)return PT_WAV_TRUNCATED;
        size = le32(data + 4); body = pos + 8;
        if (size > end - body) return PT_WAV_TRUNCATED;
        if (!memcmp(data, "fmt ", 4)) {
            uint32_t channels, bits;
            if (fmt++) return PT_WAV_INVALID;
            if (size < 16) return PT_WAV_TRUNCATED;
            if(read(context,body,data,16)!=1)return PT_WAV_TRUNCATED;
            if (le16(data) != 1) return PT_WAV_UNSUPPORTED;
            channels = le16(data + 2); bits = le16(data + 14);
            if ((channels != 1 && channels != 2) || (bits != 8 && bits != 16 && bits != 24))
                return PT_WAV_UNSUPPORTED;
            info.channels = (uint8_t)channels; info.bits = (uint8_t)bits;
            info.rate = le32(data + 4);
            if (!info.rate || info.rate > 192000) return PT_WAV_UNSUPPORTED;
            align = channels * (bits / 8);
            if (le16(data + 12) != align || le32(data + 8) != info.rate * align)
                return PT_WAV_INVALID;
        } else if (!memcmp(data, "data", 4)) {
            if (audio++) return PT_WAV_INVALID;
            if (body > UINT32_MAX) return PT_WAV_CAPACITY;
            info.data_offset = (uint32_t)body; info.data_bytes = size;
        }
        pos = body + size;
        if (size & 1) {
            if (pos == end) return PT_WAV_TRUNCATED;
            ++pos;
        }
    }
    if (!fmt || !audio || info.data_bytes % align) return PT_WAV_INVALID;
    info.frames = info.data_bytes / align;
    *out = info;
    return PT_WAV_OK;
}
struct memory_reader {const uint8_t *data;};
static int memory_read(void *context,size_t offset,uint8_t *out,size_t n)
{struct memory_reader *r=context;memcpy(out,r->data+offset,n);return 1;}
enum pt_wav_result pt_wav_inspect(const uint8_t *data,size_t length,struct pt_wav_info *out)
{
    struct memory_reader r={data};if(!data)return PT_WAV_INVALID;
    return pt_wav_inspect_reader(memory_read,&r,length,out);
}
enum pt_wav_result pt_wav_decode(const uint8_t *data, size_t length, struct pt_pcm *dest)
{
    struct pt_wav_info info;
    enum pt_wav_result result = pt_wav_inspect(data, length, &info);
    size_t count, i;
    unsigned int width;
    const uint8_t *p;
    if (result != PT_WAV_OK) return result;
    if (!dest || dest->channels != info.channels || dest->bits != info.bits ||
        dest->frames != info.frames || dest->rate != info.rate || (info.frames && !dest->data))
        return PT_WAV_INVALID;
    if (info.frames > dest->capacity / info.channels || info.frames > SIZE_MAX / sizeof(int32_t) / info.channels)
        return PT_WAV_CAPACITY;
    count = (size_t)info.frames * info.channels;
    if (overlaps(data, length, dest->data, count * sizeof(int32_t))) return PT_WAV_ALIAS;
    p = data + info.data_offset; width = info.bits / 8;
    for (i = 0; i < count; ++i, p += width) {
        uint32_t value = p[0];
        if (width >= 2) value |= (uint32_t)p[1] << 8;
        if (width == 3) value |= (uint32_t)p[2] << 16;
        if (width == 1) dest->data[i] = (int32_t)value - 128;
        else dest->data[i] = (int32_t)value - ((value & (1UL << (info.bits - 1))) ? (1L << info.bits) : 0);
    }
    return PT_WAV_OK;
}
enum pt_wav_result pt_wav_size(const struct pt_pcm *pcm, size_t *out)
{
    uint64_t bytes;
    if (!out || pt_pcm_validate(pcm) != PT_PCM_OK) return PT_WAV_INVALID;
    bytes = (uint64_t)pcm->frames * pcm->channels * (pcm->bits / 8);
    bytes += 44 + (bytes & 1);
    if (bytes > UINT32_MAX || bytes > SIZE_MAX) return PT_WAV_CAPACITY;
    *out = (size_t)bytes;
    return PT_WAV_OK;
}
enum pt_wav_result pt_wav_encode(const struct pt_pcm *pcm, uint8_t *out, size_t capacity, size_t *written)
{
    size_t size, i, count;
    unsigned int width;
    uint8_t *p;
    enum pt_wav_result result = pt_wav_size(pcm, &size);
    if (result != PT_WAV_OK) return result;
    if (!out || !written) return PT_WAV_INVALID;
    if (capacity < size) return PT_WAV_CAPACITY;
    count = (size_t)pcm->frames * pcm->channels;
    if (overlaps(out, size, pcm->data, count * sizeof(int32_t))) return PT_WAV_ALIAS;
    width = pcm->bits / 8;
    memcpy(out, "RIFF", 4); put32(out + 4, (uint32_t)size - 8);
    memcpy(out + 8, "WAVEfmt ", 8); put32(out + 16, 16); put16(out + 20, 1);
    put16(out + 22, pcm->channels); put32(out + 24, pcm->rate);
    put32(out + 28, pcm->rate * pcm->channels * width);
    put16(out + 32, pcm->channels * width); put16(out + 34, pcm->bits);
    memcpy(out + 36, "data", 4); put32(out + 40, (uint32_t)(count * width));
    p = out + 44;
    for (i = 0; i < count; ++i, p += width) {
        uint32_t value = width == 1 ? (uint32_t)(pcm->data[i] + 128) : (uint32_t)pcm->data[i];
        p[0] = (uint8_t)value;
        if (width >= 2) p[1] = (uint8_t)(value >> 8);
        if (width == 3) p[2] = (uint8_t)(value >> 16);
    }
    if ((count * width) & 1) out[size - 1] = 0;
    *written = size;
    return PT_WAV_OK;
}
