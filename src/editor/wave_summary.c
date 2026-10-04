#include "wave_summary.h"
#include "../core/pcm_internal.h"
#include <string.h>

#define BUILDING 1
#define COMPLETE 2
#define FAILED 3
static int span(const void *p,size_t n)
{ return !n || (p && n<=UINTPTR_MAX-(uintptr_t)p); }
static int overlap(const void *a,size_t an,const void *b,size_t bn)
{
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    if (!an || !bn) return 0;
    if (!span(a,an) || !span(b,bn)) return 1;
    return x<y+bn && y<x+an;
}
static int multiplied(size_t n,size_t width,size_t *bytes)
{
    if (n>SIZE_MAX/width) return 0;
    *bytes=n*width;
    return 1;
}
/* Metadata only. No project_validate/all-master value scan. */
static int project_shape(const struct pt_project *p)
{
    return p && p->sample_count<=PT_PROJECT_SAMPLES &&
        p->pattern_count<=PT_PROJECT_PATTERNS && p->order_count<=PT_PROJECT_ORDERS &&
        p->channels.count<=PT_CHANNEL_LIMIT && p->extension_count<=4090 &&
        span(p->samples,(size_t)p->sample_count*sizeof(*p->samples)) &&
        span(p->orders,(size_t)p->order_count*sizeof(*p->orders)) &&
        span(p->events,(size_t)p->pattern_count*PT_PROJECT_ROWS*p->channels.count*sizeof(*p->events)) &&
        span(p->extensions,(size_t)p->extension_count*sizeof(*p->extensions));
}
static int source_disjoint(const struct pt_project *p,const void *out,size_t n)
{
    size_t bytes;
    unsigned i;
    if (!span(out,n) || !project_shape(p) || overlap(out,n,p,sizeof(*p)) ||
        overlap(out,n,p->samples,(size_t)p->sample_count*sizeof(*p->samples)) ||
        overlap(out,n,p->orders,(size_t)p->order_count*sizeof(*p->orders)) ||
        overlap(out,n,p->events,(size_t)p->pattern_count*PT_PROJECT_ROWS*p->channels.count*sizeof(*p->events)) ||
        overlap(out,n,p->extensions,(size_t)p->extension_count*sizeof(*p->extensions))) return 0;
    for (i=0;i<p->sample_count;++i) {
        const struct pt_sample *s=p->samples+i;
        if (!multiplied(s->pcm.capacity,sizeof(*s->pcm.data),&bytes) || !span(s->pcm.data,bytes) ||
            overlap(out,n,s->pcm.data,bytes) || s->slice_count>PT_PROJECT_SLICES ||
            !span(s->slices,(size_t)s->slice_count*sizeof(*s->slices)) ||
            overlap(out,n,s->slices,(size_t)s->slice_count*sizeof(*s->slices))) return 0;
    }
    for (i=0;i<p->extension_count;++i) if (!span(p->extensions[i].data,p->extensions[i].length) ||
        overlap(out,n,p->extensions[i].data,p->extensions[i].length)) return 0;
    return 1;
}
static int pcm_same(const struct pt_pcm *a,const struct pt_pcm *b)
{
    return a->data==b->data && a->capacity==b->capacity && a->frames==b->frames &&
        a->rate==b->rate && a->channels==b->channels && a->bits==b->bits;
}
int pt_wave_summary_current(const struct pt_wave_summary *s,const struct pt_project *p,uint64_t generation)
{
    /* Fixed header checks precede any dereference of a formerly borrowed table. */
    if (!s || !p || s->project!=p || s->generation!=generation || s->table!=p->samples ||
        s->sample_count!=p->sample_count || s->slot>=p->sample_count || !p->samples ||
        s->orders!=p->orders || s->order_count!=p->order_count || s->events!=p->events ||
        s->pattern_count!=p->pattern_count || s->tracks!=p->channels.count ||
        s->extensions!=p->extensions || s->extension_count!=p->extension_count) return 0;
    return s->sample==p->samples+s->slot && pcm_same(&s->pcm,&p->samples[s->slot].pcm);
}
static void bin_begin(struct pt_wave_summary_job *j)
{
    uint64_t width=(uint64_t)j->summary.view.end-j->summary.view.start;
    j->frame=j->summary.view.start+(uint32_t)((uint64_t)j->column*width/PT_WAVE_SUMMARY_COLUMNS);
    j->last=j->summary.view.start+(uint32_t)((uint64_t)(j->column+1)*width/PT_WAVE_SUMMARY_COLUMNS);
    if (j->last==j->frame) ++j->last;
    if (j->last>j->summary.view.end) j->last=j->summary.view.end;
    j->have_value=0;
}
enum pt_wave_summary_result pt_wave_summary_begin(struct pt_wave_summary_job *j,
    const struct pt_project *p,unsigned slot,uint64_t generation,const struct pt_sample_range *view,
    struct pt_wave_summary_bin *bins,size_t capacity)
{
    struct pt_wave_summary_job value;
    enum pt_pcm_result r;
    size_t bytes,needed;
    if (!j || !view || !p || slot>=p->sample_count || !p->samples || !project_shape(p))
        return PT_WAVE_SUMMARY_INVALID;
    r=pt_pcm_shape(&p->samples[slot].pcm);
    if (r!=PT_PCM_OK) return r==PT_PCM_CAPACITY ? PT_WAVE_SUMMARY_CAPACITY : PT_WAVE_SUMMARY_INVALID;
    if (view->start>=view->end || view->end>p->samples[slot].pcm.frames) return PT_WAVE_SUMMARY_INVALID;
    needed=(size_t)PT_WAVE_SUMMARY_COLUMNS*p->samples[slot].pcm.channels;
    if (capacity<needed) return PT_WAVE_SUMMARY_CAPACITY;
    if (!bins) return PT_WAVE_SUMMARY_INVALID;
    if (!multiplied(capacity,sizeof(*bins),&bytes) || !source_disjoint(p,j,sizeof(*j)) ||
        !source_disjoint(p,bins,bytes) || !span(view,sizeof(*view)) ||
        overlap(j,sizeof(*j),view,sizeof(*view)) || overlap(j,sizeof(*j),bins,bytes) ||
        overlap(bins,bytes,view,sizeof(*view))) return PT_WAVE_SUMMARY_ALIAS;
    memset(&value,0,sizeof(value));
    value.summary.project=p; value.summary.table=p->samples; value.summary.sample=p->samples+slot;
    value.summary.orders=p->orders; value.summary.events=p->events; value.summary.extensions=p->extensions;
    value.summary.order_count=p->order_count; value.summary.pattern_count=p->pattern_count;
    value.summary.extension_count=p->extension_count; value.summary.tracks=p->channels.count;
    value.summary.pcm=p->samples[slot].pcm; value.summary.bins=bins; value.summary.capacity=capacity;
    value.summary.generation=generation; value.summary.view=*view; value.summary.sample_count=p->sample_count;
    value.summary.slot=(uint16_t)slot; value.summary.columns=PT_WAVE_SUMMARY_COLUMNS;
    value.summary.channels=p->samples[slot].pcm.channels; value.bins=bins; value.state=BUILDING;
    bin_begin(&value); *j=value;
    return PT_WAVE_SUMMARY_OK;
}
static int job_output(const struct pt_wave_summary_job *j,const void *out,size_t n)
{
    size_t bytes;
    return multiplied(j->summary.capacity,sizeof(*j->bins),&bytes) &&
        !overlap(out,n,j,sizeof(*j)) && !overlap(out,n,j->bins,bytes) &&
        source_disjoint(j->summary.project,out,n);
}
enum pt_wave_summary_result pt_wave_summary_step(struct pt_wave_summary_job *j,
    uint64_t generation,unsigned *ready)
{
    unsigned used=0;
    if (!j || !ready || (j->state!=BUILDING && j->state!=COMPLETE)) return PT_WAVE_SUMMARY_INVALID;
    /* Guard fixed controller output before changing even stale state. */
    if (!span(ready,sizeof(*ready)) || overlap(ready,sizeof(*ready),j,sizeof(*j))) return PT_WAVE_SUMMARY_ALIAS;
    if (!pt_wave_summary_current(&j->summary,j->summary.project,generation)) {
        j->state=FAILED; return PT_WAVE_SUMMARY_STALE;
    }
    if (!job_output(j,ready,sizeof(*ready))) return PT_WAVE_SUMMARY_ALIAS;
    while (j->state==BUILDING && used<PT_WAVE_SUMMARY_VALUES_PER_STEP) {
        int32_t v=j->summary.pcm.data[(size_t)j->frame*j->summary.channels+j->channel];
        ++used;
        if (!j->have_value) { j->minimum=v; j->maximum=v; j->have_value=1; }
        else { if (v<j->minimum) j->minimum=v; if (v>j->maximum) j->maximum=v; }
        if (++j->frame==j->last) {
            struct pt_wave_summary_bin *b=j->bins+(size_t)j->column*j->summary.channels+j->channel;
            b->minimum=j->minimum; b->maximum=j->maximum;
            if (++j->channel==j->summary.channels) { j->channel=0; ++j->column; }
            if (j->column==PT_WAVE_SUMMARY_COLUMNS) j->state=COMPLETE;
            else bin_begin(j);
        }
    }
    j->last_values=used; *ready=j->state==COMPLETE;
    return j->state==COMPLETE ? PT_WAVE_SUMMARY_OK : PT_WAVE_SUMMARY_PENDING;
}
enum pt_wave_summary_result pt_wave_summary_take(struct pt_wave_summary_job *j,
    uint64_t generation,struct pt_wave_summary *out)
{
    struct pt_wave_summary value;
    if (!j || !out || j->state!=COMPLETE) return PT_WAVE_SUMMARY_INVALID;
    if (!span(out,sizeof(*out)) || overlap(out,sizeof(*out),j,sizeof(*j))) return PT_WAVE_SUMMARY_ALIAS;
    if (!pt_wave_summary_current(&j->summary,j->summary.project,generation)) {
        j->state=FAILED; return PT_WAVE_SUMMARY_STALE;
    }
    if (!job_output(j,out,sizeof(*out))) return PT_WAVE_SUMMARY_ALIAS;
    value=j->summary; *out=value; memset(j,0,sizeof(*j));
    return PT_WAVE_SUMMARY_OK;
}
void pt_wave_summary_cancel(struct pt_wave_summary_job *j)
{ if (j) memset(j,0,sizeof(*j)); }
