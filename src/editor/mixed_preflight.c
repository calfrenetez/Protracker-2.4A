#include "mixed_preflight.h"
#include <limits.h>
#include <string.h>
struct workspace {struct pt_render_plan all,paula,amigus;unsigned index[2][PT_RENDER_ACTIONS];};
struct pt_mixed_preflight {
    struct workspace batch;struct pt_allocator allocator;
    const struct pt_project *project;struct pt_render_options options;
    struct pt_paula_render_caps caps;struct pt_playback_format format;
    struct pt_render_sequence *sequence;struct pt_render_interval span;
    struct pt_mixed_report report;uint16_t held[2];uint32_t remaining;
    unsigned pc,ac,phase;
};
static void init(struct pt_mixed_report *r)
{unsigned i;memset(r,0,sizeof(*r));r->result=PT_MIXED_INVALID;r->action=r->channel=UINT_MAX;r->paula=PT_PAULA_INVALID;r->amigus=PT_WAVETABLE_INVALID;for(i=0;i<PT_CHANNEL_LIMIT;++i)r->map[i]=-1;}
static void report(const struct pt_mixed_report *r,struct pt_mixed_report *out)
{*out=*r;if(r->result!=PT_MIXED_OK)memset(out->samples,0,sizeof(out->samples));}
void pt_mixed_preflight_close(struct pt_mixed_preflight **work)
{
    struct pt_mixed_preflight *w;struct pt_allocator a;
    if(!work || !*work)return;
    w=*work;a=w->allocator;pt_render_sequence_close(w->sequence);
    a.release(a.context,w);*work=NULL;
}
static enum pt_mixed_result batch(const struct pt_project *p,unsigned rate,struct workspace *w,
    const struct pt_paula_render_caps *caps,const struct pt_playback_format *f,unsigned pc,unsigned ac,
    uint16_t held[2],struct pt_mixed_report *r)
{
    struct pt_paula_preflight_report pr;struct pt_wavetable_preflight_report ar;
    uint16_t next[2]={held[0],held[1]};unsigned i,k,pi=UINT_MAX,ai=UINT_MAX;
    w->paula.count=w->amigus.count=0;
    for(i=0;i<w->all.count;++i) {
        const struct pt_render_action *a=&w->all.action[i];unsigned route;
        if(a->channel>=p->channels.count){r->action=i;r->channel=a->channel;return PT_MIXED_ROUTE;}
        route=p->channels.track[a->channel].route;
        if(route!=PT_PAULA && route!=PT_AMIGUS){r->action=i;r->channel=a->channel;r->kind=a->kind;return PT_MIXED_ROUTE;}
        k=route==PT_PAULA?0:1;
        if(k){w->index[k][w->amigus.count]=i;w->amigus.action[w->amigus.count++]=*a;}
        else {w->index[k][w->paula.count]=i;w->paula.action[w->paula.count++]=*a;}
    }
    r->paula=pt_paula_check_plan(p,rate,r->map,&w->paula,caps,pc,&next[0],&pr);
    r->amigus=pt_wavetable_check_plan(p,rate,&w->amigus,f,ac,&next[1],&ar);
    if(pr.action<w->paula.count)pi=w->index[0][pr.action];
    if(ar.action<w->amigus.count)ai=w->index[1][ar.action];
    if(r->paula!=PT_PAULA_COMPATIBLE || r->amigus!=PT_WAVETABLE_COMPATIBLE) {
        unsigned backend=r->paula!=PT_PAULA_COMPATIBLE && (r->amigus==PT_WAVETABLE_COMPATIBLE || pi<=ai)?0:1;
        r->action=backend?ai:pi;
        if(r->action<w->all.count){r->channel=w->all.action[r->action].channel;r->kind=w->all.action[r->action].kind;}
        return backend?PT_MIXED_AMIGUS:PT_MIXED_PAULA;
    }
    for(i=0;i<PT_PROJECT_SAMPLES;++i){r->samples[0][i]|=pr.samples[i];r->samples[1][i]|=ar.samples[i];}
    held[0]=next[0];held[1]=next[1];return PT_MIXED_OK;
}
enum pt_mixed_result pt_mixed_preflight_begin(const struct pt_project *p,const struct pt_render_options *o,
    const int8_t *previous,const struct pt_paula_render_caps *caps,const struct pt_playback_format *f,
    unsigned pc,unsigned ac,const struct pt_allocator *a,struct pt_mixed_report *out,struct pt_mixed_preflight **work)
{
    struct pt_mixed_report r;struct pt_mixed_preflight *w=NULL;unsigned i;
    init(&r);if(!out)return PT_MIXED_INVALID;
    if(!work || *work || !p || !o || !a || !a->allocate || !a->release || pc>1 || ac>1 || (o->rate!=44100 && o->rate!=48000) ||
       !pt_paula_render_caps_valid(caps) || !f || (f->bits!=8 && f->bits!=16) ||
       f->channel || f->word_pad || f->little_endian>1 || pt_project_validate(p,NULL)!=PT_PROJECT_OK ||
       pt_channels_paula_map(&p->channels,previous,r.map)!=PT_CHANNEL_OK)goto done;
    if(o->row_range){r.result=PT_MIXED_RANGE;goto done;}
    for(i=0;i<p->channels.count;++i)if((o->tracks&(1U<<i)) &&
       p->channels.track[i].route!=PT_PAULA && p->channels.track[i].route!=PT_AMIGUS){r.result=PT_MIXED_ROUTE;r.channel=i;goto done;}
    w=a->allocate(a->context,sizeof(*w));if(!w){r.result=PT_MIXED_MEMORY;goto done;}
    memset(w,0,sizeof(*w));w->allocator=*a;
    r.render_result=pt_render_sequence_begin(p,o,a,&w->sequence);
    if(r.render_result!=PT_RENDER_OK) {
        r.result=r.render_result==PT_RENDER_MEMORY?PT_MIXED_MEMORY:PT_MIXED_RENDER;
        pt_mixed_preflight_close(&w);goto done;
    }
    w->project=p;w->options=*o;w->caps=*caps;w->format=*f;w->pc=pc;w->ac=ac;
    r.result=PT_MIXED_PENDING;w->report=r;*work=w;
done:
    report(&r,out);return r.result;
}
enum pt_mixed_result pt_mixed_preflight_step(struct pt_mixed_preflight *w,struct pt_mixed_report *out)
{
    struct pt_mixed_report *r;unsigned ready;
    if(!w || !out)return PT_MIXED_INVALID;
    r=&w->report;if(r->result!=PT_MIXED_PENDING)goto done;
    switch(w->phase) {
    case 0:
        r->render_result=pt_render_sequence_prepare(w->sequence,256,&ready);
        if(r->render_result==PT_RENDER_OK && ready)w->phase=1;
        break;
    case 1:
        r->render_result=pt_render_sequence_next(w->sequence,&w->span);
        if(r->render_result!=PT_RENDER_OK)break;
        ++r->intervals;w->remaining=w->span.frames;w->phase=w->remaining?2:3;
        break;
    case 2: {
        uint32_t n=w->remaining>256?256:w->remaining;
        r->render_result=pt_render_sequence_consume(w->sequence,n);
        if(r->render_result!=PT_RENDER_OK)break;
        w->remaining-=n;r->frames+=n;if(!w->remaining)w->phase=3;
        break;
    }
    case 3:
        r->render_result=pt_render_sequence_complete(w->sequence,&w->batch.all);
        if(r->render_result!=PT_RENDER_OK)break;
        r->result=batch(w->project,w->options.rate,&w->batch,&w->caps,&w->format,w->pc,w->ac,w->held,r);
        if(r->result==PT_MIXED_OK && !w->span.end){r->result=PT_MIXED_PENDING;w->phase=1;}
        break;
    }
    if(r->render_result!=PT_RENDER_OK)r->result=r->render_result==PT_RENDER_MEMORY?PT_MIXED_MEMORY:PT_MIXED_RENDER;
done:
    report(r,out);return r->result;
}
int pt_mixed_preflight_take(struct pt_mixed_preflight *w,struct pt_render_sequence **out)
{
    if(!w || !out || w->report.result!=PT_MIXED_OK || !w->sequence)return 0;
    w->report.render_result=pt_render_sequence_rewind(w->sequence);
    if(w->report.render_result!=PT_RENDER_OK){w->report.result=PT_MIXED_RENDER;return 0;}
    *out=w->sequence;w->sequence=NULL;return 1;
}
enum pt_mixed_result pt_mixed_preflight(const struct pt_project *p,const struct pt_render_options *o,
    const int8_t *previous,const struct pt_paula_render_caps *caps,const struct pt_playback_format *f,
    unsigned pc,unsigned ac,const struct pt_allocator *a,struct pt_mixed_report *out,struct pt_render_sequence **take)
{
    struct pt_mixed_preflight *w=NULL;
    enum pt_mixed_result result=pt_mixed_preflight_begin(p,o,previous,caps,f,pc,ac,a,out,&w);
    while(result==PT_MIXED_PENDING)result=pt_mixed_preflight_step(w,out);
    if(result==PT_MIXED_OK && take && !pt_mixed_preflight_take(w,take))result=pt_mixed_preflight_step(w,out);
    pt_mixed_preflight_close(&w);return result;
}
