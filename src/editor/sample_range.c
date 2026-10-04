#include "sample_range.h"
#include "../core/pcm_internal.h"
#include <string.h>

static int overlap(const void *a,size_t an,const void *b,size_t bn)
{
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    if (!an || !bn) return 0;
    if (!a || !b || an>UINTPTR_MAX-x || bn>UINTPTR_MAX-y) return 1;
    return x<y+bn && y<x+an;
}
static int output_span(const void *p,size_t n)
{
    return !n || (p && n<=UINTPTR_MAX-(uintptr_t)p);
}
static enum pt_sample_range_result meta(const struct pt_sample *s)
{
    enum pt_pcm_result r;
    if (!s) return PT_SAMPLE_RANGE_INVALID;
    r=pt_pcm_shape(&s->pcm);
    if (r!=PT_PCM_OK) return r==PT_PCM_CAPACITY ? PT_SAMPLE_RANGE_CAPACITY : PT_SAMPLE_RANGE_INVALID;
    if (s->loop>PT_LOOP_CROSSFADE || s->slice_count>PT_PROJECT_SLICES ||
        (s->slice_count && !s->slices)) return PT_SAMPLE_RANGE_INVALID;
    if (s->loop==PT_LOOP_NONE) {
        if (s->loop_start || s->loop_end || s->crossfade) return PT_SAMPLE_RANGE_INVALID;
    } else {
        if (s->loop_start>=s->loop_end || s->loop_end>s->pcm.frames) return PT_SAMPLE_RANGE_INVALID;
        if (s->loop==PT_LOOP_CROSSFADE) {
            if (!s->crossfade || s->crossfade>(s->loop_end-s->loop_start)/2) return PT_SAMPLE_RANGE_INVALID;
        } else if (s->crossfade) return PT_SAMPLE_RANGE_INVALID;
    }
    if (s->pcm.capacity>SIZE_MAX/sizeof(int32_t) ||
        !output_span(s->pcm.data,s->pcm.capacity*sizeof(int32_t)) ||
        !output_span(s->slices,(size_t)s->slice_count*sizeof(uint32_t))) return PT_SAMPLE_RANGE_ALIAS;
    return PT_SAMPLE_RANGE_OK;
}
static int source_output(const struct pt_sample *s,const void *out,size_t n)
{
    size_t bytes;
    if (!output_span(out,n) || s->pcm.capacity>SIZE_MAX/sizeof(int32_t)) return 0;
    bytes=s->pcm.capacity*sizeof(int32_t);
    if (!output_span(s->pcm.data,bytes) ||
        !output_span(s->slices,(size_t)s->slice_count*sizeof(uint32_t))) return 0;
    return !overlap(out,n,s,sizeof(*s)) && !overlap(out,n,s->pcm.data,bytes) &&
        !overlap(out,n,s->slices,(size_t)s->slice_count*sizeof(uint32_t));
}
enum pt_sample_range_result pt_sample_range_validate(const struct pt_sample *s,
    const struct pt_sample_range *range)
{
    enum pt_sample_range_result r=meta(s);
    if (r!=PT_SAMPLE_RANGE_OK) return r;
    if (!range || range->start>=range->end || range->end>s->pcm.frames) return PT_SAMPLE_RANGE_INVALID;
    return PT_SAMPLE_RANGE_OK;
}
enum pt_sample_range_result pt_sample_range_loop(const struct pt_sample *s,struct pt_sample_range *out)
{
    struct pt_sample_range value;
    enum pt_sample_range_result r=meta(s);
    if (r!=PT_SAMPLE_RANGE_OK) return r;
    if (!out) return PT_SAMPLE_RANGE_INVALID;
    if (s->loop==PT_LOOP_NONE) return PT_SAMPLE_RANGE_NO_LOOP;
    if (!source_output(s,out,sizeof(*out))) return PT_SAMPLE_RANGE_ALIAS;
    value.start=s->loop_start; value.end=s->loop_end;
    *out=value;
    return PT_SAMPLE_RANGE_OK;
}
enum pt_sample_range_result pt_sample_range_centre(uint32_t frames,
    const struct pt_sample_range *view,uint32_t boundary,struct pt_sample_range *out)
{
    struct pt_sample_range value;
    uint32_t span,start;
    if (!view || !out || view->start>=view->end || view->end>frames || boundary>frames)
        return PT_SAMPLE_RANGE_INVALID;
    if (!output_span(view,sizeof(*view)) || !output_span(out,sizeof(*out)) ||
        (out!=view && overlap(out,sizeof(*out),view,sizeof(*view)))) return PT_SAMPLE_RANGE_ALIAS;
    span=view->end-view->start;
    start=boundary>span/2 ? boundary-span/2 : 0;
    if (start>frames-span) start=frames-span;
    value.start=start; value.end=start+span;
    *out=value;
    return PT_SAMPLE_RANGE_OK;
}
enum pt_sample_range_result pt_sample_range_copy_loop(const struct pt_sample *s,
    const struct pt_sample_range *range,struct pt_sample_loop *out)
{
    struct pt_sample_loop value;
    enum pt_sample_range_result r=pt_sample_range_validate(s,range);
    if (r!=PT_SAMPLE_RANGE_OK) return r;
    if (!out) return PT_SAMPLE_RANGE_INVALID;
    if (!source_output(s,out,sizeof(*out)) || overlap(out,sizeof(*out),range,sizeof(*range)))
        return PT_SAMPLE_RANGE_ALIAS;
    memset(&value,0,sizeof(value));
    if (s->loop!=PT_LOOP_NONE && s->loop_start>=range->start && s->loop_end<=range->end) {
        value.kind=s->loop; value.start=s->loop_start-range->start;
        value.end=s->loop_end-range->start; value.crossfade=s->crossfade;
    }
    *out=value;
    return PT_SAMPLE_RANGE_OK;
}
enum pt_sample_range_result pt_sample_range_copy_markers(const struct pt_sample *s,
    const struct pt_sample_range *range,uint32_t *out,size_t capacity,size_t *count)
{
    size_t i,n=0,j=0;
    enum pt_sample_range_result r=pt_sample_range_validate(s,range);
    if (r!=PT_SAMPLE_RANGE_OK) return r;
    if (!count) return PT_SAMPLE_RANGE_INVALID;
    for (i=0;i<s->slice_count;++i) {
        if (s->slices[i]>=s->pcm.frames || (i && s->slices[i]<=s->slices[i-1])) return PT_SAMPLE_RANGE_INVALID;
        if (s->slices[i]>=range->start && s->slices[i]<range->end) ++n;
    }
    if (n>capacity) return PT_SAMPLE_RANGE_CAPACITY;
    if (n && !out) return PT_SAMPLE_RANGE_INVALID;
    if (!source_output(s,count,sizeof(*count)) || overlap(count,sizeof(*count),range,sizeof(*range)) ||
        (n && (!source_output(s,out,n*sizeof(*out)) || overlap(out,n*sizeof(*out),range,sizeof(*range)) ||
         overlap(count,sizeof(*count),out,n*sizeof(*out))))) return PT_SAMPLE_RANGE_ALIAS;
    for (i=0;i<s->slice_count;++i) if (s->slices[i]>=range->start && s->slices[i]<range->end)
        out[j++]=s->slices[i]-range->start;
    *count=n;
    return PT_SAMPLE_RANGE_OK;
}
