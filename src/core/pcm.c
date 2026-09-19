#include <limits.h>
#include "pcm.h"

static int32_t maximum(unsigned int bits) { return ((int32_t)1 << (bits - 1)) - 1; }
static int32_t clip(int64_t value, unsigned int bits)
{
    int32_t high = maximum(bits), low = -high - 1;
    return value > high ? high : value < low ? low : (int32_t)value;
}
static enum pt_pcm_result shape(const struct pt_pcm *pcm)
{
    if (!pcm || (pcm->bits != 8 && pcm->bits != 16 && pcm->bits != 24) ||
        (pcm->channels != 1 && pcm->channels != 2) || !pcm->rate || pcm->rate > 192000 ||
        (pcm->frames && !pcm->data)) return PT_PCM_INVALID;
    if (pcm->frames > pcm->capacity / pcm->channels ||
        pcm->frames > SIZE_MAX / sizeof(int32_t) / pcm->channels) return PT_PCM_CAPACITY;
    return PT_PCM_OK;
}
enum pt_pcm_result pt_pcm_validate(const struct pt_pcm *pcm)
{
    size_t i, count;
    int32_t high, low;
    enum pt_pcm_result result = shape(pcm);
    if (result != PT_PCM_OK) return result;
    high = maximum(pcm->bits); low = -high - 1;
    count = (size_t)pcm->frames * pcm->channels;
    for (i = 0; i < count; ++i) if (pcm->data[i] < low || pcm->data[i] > high) return PT_PCM_INVALID;
    return PT_PCM_OK;
}
enum pt_pcm_result pt_pcm_edit(struct pt_pcm *pcm, enum pt_pcm_edit op,
                                uint32_t start, uint32_t end, unsigned int gain)
{
    uint32_t frame, length;
    unsigned int channel;
    int32_t peak = 0;
    int64_t sums[2] = {0, 0};
    enum pt_pcm_result result = pt_pcm_validate(pcm);
    if (result != PT_PCM_OK) return result;
    if (start > end || end > pcm->frames || op < PT_PCM_REVERSE || op > PT_PCM_REMOVE_DC ||
        (op == PT_PCM_GAIN && gain > 8000)) return PT_PCM_INVALID;
    length = end - start;
    if (!length) return PT_PCM_OK;
    if (op == PT_PCM_REVERSE) {
        uint32_t left = start, right = end - 1;
        while (left < right) {
            for (channel = 0; channel < pcm->channels; ++channel) {
                size_t a = (size_t)left * pcm->channels + channel;
                size_t b = (size_t)right * pcm->channels + channel;
                int32_t swap = pcm->data[a]; pcm->data[a] = pcm->data[b]; pcm->data[b] = swap;
            }
            ++left; --right;
        }
        return PT_PCM_OK;
    }
    if (op == PT_PCM_NORMALIZE || op == PT_PCM_REMOVE_DC) {
        for (frame = start; frame < end; ++frame) for (channel = 0; channel < pcm->channels; ++channel) {
            int32_t value = pcm->data[(size_t)frame * pcm->channels + channel];
            int32_t absolute = value < 0 ? -value : value;
            if (absolute > peak) peak = absolute;
            sums[channel] += value;
        }
        if (op == PT_PCM_NORMALIZE && !peak) return PT_PCM_OK;
    }
    for (frame = start; frame < end; ++frame) for (channel = 0; channel < pcm->channels; ++channel) {
        size_t index = (size_t)frame * pcm->channels + channel;
        int64_t value = pcm->data[index];
        if (op == PT_PCM_NORMALIZE) value = value * maximum(pcm->bits) / peak;
        else if (op == PT_PCM_GAIN) value = value * gain / 1000;
        else if (op == PT_PCM_REMOVE_DC) value -= sums[channel] / length;
        else if (length == 1) value = 0;
        else if (op == PT_PCM_FADE_IN) value = value * (frame - start) / (length - 1);
        else value = value * (end - 1 - frame) / (length - 1);
        pcm->data[index] = clip(value, pcm->bits);
    }
    return PT_PCM_OK;
}

static int overlap(const struct pt_pcm *a, const struct pt_pcm *b)
{
    uintptr_t x = (uintptr_t)a->data, y = (uintptr_t)b->data;
    size_t na = (size_t)a->frames * a->channels * sizeof(int32_t);
    size_t nb = (size_t)b->frames * b->channels * sizeof(int32_t);
    if (!na || !nb) return 0;
    if (na > UINTPTR_MAX - x || nb > UINTPTR_MAX - y) return 1;
    return x < y + nb && y < x + na;
}
enum pt_pcm_result pt_pcm_convert(const struct pt_pcm *source, struct pt_pcm *dest)
{
    size_t i, count;
    enum pt_pcm_result result = pt_pcm_validate(source);
    if (result != PT_PCM_OK) return result;
    result = shape(dest);
    if (result != PT_PCM_OK) return result;
    if (dest->frames != source->frames || dest->channels != source->channels || dest->rate != source->rate)
        return PT_PCM_INVALID;
    if (source->data != dest->data && overlap(source, dest)) return PT_PCM_ALIAS;
    count = (size_t)source->frames * source->channels;
    for (i = 0; i < count; ++i) {
        int32_t value = source->data[i];
        if (dest->bits > source->bits) value *= (int32_t)1 << (dest->bits - source->bits);
        else if (dest->bits < source->bits) {
            int32_t divisor = (int32_t)1 << (source->bits - dest->bits);
            value = value < 0 ? -((-value + divisor / 2) / divisor) : (value + divisor / 2) / divisor;
        }
        dest->data[i] = clip(value, dest->bits);
    }
    return PT_PCM_OK;
}
enum pt_pcm_result pt_pcm_resampled_frames(const struct pt_pcm *source, uint32_t rate, uint32_t *out)
{
    uint64_t frames;
    enum pt_pcm_result result = shape(source);
    if (result != PT_PCM_OK) return result;
    if (!out || !rate || rate > 192000) return PT_PCM_INVALID;
    frames = ((uint64_t)source->frames * rate + source->rate - 1) / source->rate;
    if (frames > UINT32_MAX) return PT_PCM_CAPACITY;
    *out = (uint32_t)frames;
    return PT_PCM_OK;
}
enum pt_pcm_result pt_pcm_resample(const struct pt_pcm *source, struct pt_pcm *dest)
{
    uint32_t needed, frame;
    unsigned int channel;
    enum pt_pcm_result result = pt_pcm_validate(source);
    if (result != PT_PCM_OK) return result;
    result = shape(dest);
    if (result != PT_PCM_OK) return result;
    result = pt_pcm_resampled_frames(source, dest->rate, &needed);
    if (result != PT_PCM_OK) return result;
    if (dest->bits != source->bits || dest->channels != source->channels || dest->frames != needed)
        return PT_PCM_INVALID;
    if (overlap(source, dest)) return PT_PCM_ALIAS;
    for (frame = 0; frame < needed; ++frame) {
        uint64_t position = (uint64_t)frame * source->rate;
        uint32_t left = (uint32_t)(position / dest->rate);
        uint32_t fraction = (uint32_t)(position % dest->rate);
        uint32_t right = left + (left + 1 < source->frames);
        for (channel = 0; channel < source->channels; ++channel) {
            int64_t a = source->data[(size_t)left * source->channels + channel];
            int64_t b = source->data[(size_t)right * source->channels + channel];
            dest->data[(size_t)frame * dest->channels + channel] =
                (int32_t)((a * (dest->rate - fraction) + b * fraction) / dest->rate);
        }
    }
    return PT_PCM_OK;
}
