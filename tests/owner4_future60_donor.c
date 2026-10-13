/* Accepted finite97 helpers supply real sampler/version/cache/core ownership
 * and an independent HOST backend that drops its actual borrowed references.
 * Its dormant main and handcrafted action helpers are never called here. */
#define main pt_private_finite97_dormant_main
#include "paula_segment_repeat_readers_test.c"
#undef main
#include "../src/editor/paula_segment_sequence.h"
struct provider {struct model *actual;unsigned refuse,attempts;};
struct sequence_fixture {
    struct fixture f;int32_t master[3][768];struct provider provider;
    struct pt_paula_segment_sequence_config config;
    struct pt_paula_segment_sequence *producer;void *workspace;size_t workspace_bytes;
    uint32_t revision;
};
static int sequence_clock(void *v,uint64_t *ticks,uint32_t *frequency)
{struct provider *p=v;return clock_read(p->actual,ticks,frequency);}
static int sequence_submit(void *v,const struct pt_readers_event *event,const struct pt_paula_segment_plan *plan)
{struct provider *p=v;++p->attempts;
 /* Clean backpressure owns no supplied address and makes no output effect. */
 if(p->refuse){--p->refuse;return 0;}return submit_plan(p->actual,event,plan);}
static enum pt_readers_reply sequence_command(void *v,uint64_t t,struct pt_readers_command_receipt *r)
{return poll_command(((struct provider *)v)->actual,t,r);}
static enum pt_readers_reply sequence_cancel_command(void *v,uint64_t t,struct pt_readers_command_receipt *r)
{return cancel_command(((struct provider *)v)->actual,t,r);}
static enum pt_readers_reply sequence_reader(void *v,const struct pt_readers_domain *d,struct pt_readers_reader_receipt *r)
{return poll_reader(((struct provider *)v)->actual,d,r);}
static enum pt_readers_reply sequence_cancel_reader(void *v,const struct pt_readers_domain *d,struct pt_readers_reader_receipt *r)
{return cancel_reader(((struct provider *)v)->actual,d,r);}
static void sequence_init(struct sequence_fixture *x,unsigned variant)
{
    struct fixture *f=&x->f;struct pt_event *event;unsigned i,j;
    memset(x,0,sizeof(*x));x->revision=17;f->allocator=(struct pt_allocator){&f->fast,allocate,release};
    pt_document_init(&f->document,&f->allocator);assert(pt_document_new(&f->document,4,SIZE_MAX)==PT_PROJECT_OK);
    f->document.project.speed=1;f->document.project.bpm=125;f->document.project.channels.track[0].pan=0;
    for(j=0;j<3;++j){for(i=0;i<768;++i)x->master[j][i]=(int32_t)(i%32U)-16+(int32_t)j;
        memcpy(f->original[j],x->master[j],sizeof(f->original[j]));
        f->document.project.samples[j].pcm=(struct pt_pcm){x->master[j],768,768,8000,1,8};
        f->document.project.samples[j].volume=64;f->document.project.samples[j].loop=PT_LOOP_FORWARD;
        f->document.project.samples[j].loop_start=4;f->document.project.samples[j].loop_end=12;}
    f->document.project.samples[0].loop_start=0;
    assert(f->document.project.samples[0].pcm.frames==768&&
        f->document.project.samples[0].loop_start==0&&f->document.project.samples[0].loop_end==12&&
        f->document.project.samples[1].loop_start==4&&f->document.project.samples[1].loop_end==12&&
        f->document.project.samples[2].loop_start==4&&f->document.project.samples[2].loop_end==12);
    /* Normal complete64-row project. Genuine silent row0 is pre-roll; no flow,
     * pitch, ranges, voice, plan or private sequence field is edited in tests. */
    event=f->document.project.events;
    event[4]=(struct pt_event){428,0,PT_NOTE_PERIOD,1,9,1,0,0};
    event[8]=(struct pt_event){0,0,PT_NOTE_NONE,2,12,32,0,0};
    event[12]=(struct pt_event){0,0,PT_NOTE_NONE,3,12,16,0,0};
    event[16].kind=PT_NOTE_OFF;event[20].effect=15;event[20].parameter=0;
    if(variant==1){event[12].kind=PT_NOTE_PERIOD;event[12].pitch=428;}
    else if(variant==2){event[16].kind=PT_NOTE_NONE;}
    else if(variant==3)f->document.project.bpm=124;
    else if(variant==4){event[12].instrument=2;event[12].effect=0;event[12].parameter=0;}
    assert(pt_project_validate(&f->document.project,NULL)==PT_PROJECT_OK);
    pt_sampler_init(&f->sampler,&f->allocator,1024*1024);
    assert(pt_pattern_history_init(&f->history,&f->document.project,f->command,4,f->changes,4)==PT_EDIT_OK);
    for(i=0;i<3;++i)assert(pt_sampler_pin(&f->sampler,&f->document.project,i,f->sampler.generation,f->pcm+i,f->pin+i)==PT_EDIT_OK);
    assert(pt_sampler_paula_bind(&f->bridge,&f->sampler,&f->document.project,&f->chip,chip_allocate,chip_release,2304));
    f->config=(struct pt_paula_segment_readers_config){0,3,{{0,f->pin[0]},{1,f->pin[1]},{2,f->pin[2]}},
        {100,7,709379,RATE},13,{960,1920,2880,3840},{3546895,124,65535},1024*1024};
    f->model.now=100;f->model.frequency=709379;f->model.raw=1;f->model.adopt=1;
    for(i=0;i<3;++i)f->model.expected[i]=f->pin[i];x->provider.actual=&f->model;
    f->backend=(struct pt_paula_segment_backend){{&x->provider,sizeof(x->provider),{7,4,1},PT_READERS_VERSION,PT_READERS_REQUIRED,1,
        sequence_clock,NULL,sequence_command,sequence_cancel_command,sequence_reader,sequence_cancel_reader},
        PT_PAULA_SEGMENT_BACKEND_VERSION,PT_PAULA_SEGMENT_BACKEND_REQUIRED,sequence_submit};
    x->config.render=(struct pt_render_options){12000,32,RATE,65536,1,0,0,24,0,0,0,0,0};
    x->config.readers=f->config;x->config.origin=0;x->config.control_budget=2*1024*1024;
    x->workspace_bytes=pt_paula_segment_sequence_control_size();x->workspace=allocate(&f->fast,x->workspace_bytes);assert(x->workspace);
}
static void sequence_open(struct sequence_fixture *x)
{assert(pt_paula_segment_sequence_begin(&x->f.allocator,&x->f.bridge,&x->config,&x->f.backend,x->revision,
 x->workspace,x->workspace_bytes,&x->producer)==PT_PAULA_SEQUENCE_PENDING&&x->producer);}
static struct pt_paula_segment_sequence_status sequence_status(struct sequence_fixture *x)
{struct pt_paula_segment_sequence_status s;enum pt_paula_segment_sequence_result r=pt_paula_segment_sequence_get(x->producer,&s);
 assert(r==s.result);return s;}
static enum pt_paula_segment_sequence_result sequence_until(struct sequence_fixture *x,
    enum pt_paula_segment_sequence_result target)
{unsigned n;enum pt_paula_segment_sequence_result r=PT_PAULA_SEQUENCE_PENDING;
 for(n=0;n<4000;++n){r=pt_paula_segment_sequence_step(x->producer,x->revision,127);
    if(r==target||r==PT_PAULA_SEQUENCE_FAILED||r==PT_PAULA_SEQUENCE_STALE||r==PT_PAULA_SEQUENCE_CAPACITY)return r;
    assert(r==PT_PAULA_SEQUENCE_PENDING||r==PT_PAULA_SEQUENCE_WAIT_ACTIVE||r==PT_PAULA_SEQUENCE_WAIT_PRESSURE);}
 assert(0);return r;}
static void sequence_finish(struct sequence_fixture *x)
{
    assert(pt_paula_segment_sequence_close(&x->producer)&&!x->producer);
    release(&x->f.fast,x->workspace);x->workspace=NULL;assert(!x->f.pool);
    close_fixture(&x->f);
}
#include "../src/editor/paula_segment_future.h"
/* Real provider-owned HOST delivery records, distinct from genuine core state.
 * No private sampler/queue adoption, retirement, history or refcount is edited. */
struct future_resource_slot {uint64_t ticket;const struct pt_paula_future_packet *packet;unsigned ack,cancel;};
struct future_resource {
    struct pt_paula_segment_future *future;
    struct future_resource_slot slot[4];unsigned count,locked,enters,leaves;
    uint64_t now;uint32_t frequency;
    unsigned refuse,arms,rearms,effects,controls,stops,reader_probes,source_calls,source_probes;
    int arm_raw,apply_raw,cancel_raw,ack_raw,leave_raw;
    struct pt_paula_segment_backend backend;
    const struct pt_readers_event *input_event;const struct pt_paula_segment_plan *input_plan;
    const struct pt_readers_domain *input_domain;const struct pt_scheduled_span *input_spans;
    uint64_t ticket_before;unsigned span_count_before,fault_phase,fault_mode,injected,check_aliases,alias_checked;
    unsigned allow_reader,allow_source,cross,mutate,reenter;
    const struct pt_pcm *retained_pcm[3];const uint8_t *retained_cache[3];size_t retained_cache_bytes[3];
    unsigned pcm_phase,pcm_injected;
    const uint8_t *current,*pending,*superseded;
    uint64_t phase;struct pt_paula_segment_plan applied[4];
};
struct future_fixture {struct sequence_fixture sequence;struct future_resource resource;
    struct pt_paula_future_ops ops;void *control;size_t control_bytes;};
static unsigned future_cases;
static void resource_mutate_input(struct future_resource *r,unsigned phase)
{struct pt_readers_event *e;struct pt_readers_domain *d;
 if(r->injected||r->fault_phase!=phase||!r->input_event)return;
 e=(struct pt_readers_event *)(uintptr_t)r->input_event;d=(struct pt_readers_domain *)(uintptr_t)r->input_domain;
 ++r->injected;
 if(r->fault_mode==0)d->count=129;
 else if(r->fault_mode==1)d->spans=NULL;
 else if(r->fault_mode==2)e->scheduled.batch.count=PT_READERS_ACTIONS+1;
 else if(r->fault_mode==3)e->reader[0]=NULL;
 else if(r->fault_mode==4)((struct pt_paula_segment_plan *)(uintptr_t)r->input_plan)->frame++;
 else if(r->fault_mode==5)((struct pt_scheduled_span *)(uintptr_t)r->input_spans)->bytes++;
 else{assert(r->fault_mode==6);e->scheduled.ticket++;}}
union alias_fixed {struct pt_readers_event event;struct pt_paula_segment_plan plan;struct pt_readers_domain domain;struct pt_scheduled_span spans[128];uint64_t alignment;unsigned char bytes[8192];};
static void resource_fixed_refusers(struct future_resource *r,const struct pt_readers_event *e,const struct pt_paula_segment_plan *g)
{union alias_fixed ee,gg,dd,ss;unsigned char before[4][sizeof(union alias_fixed)];unsigned which;unsigned enters=r->enters;
 for(which=0;which<8;++which){const struct pt_readers_event *event=&ee.event;const struct pt_paula_segment_plan *plan=&gg.plan;
    memset(&ee,0,sizeof(ee));memset(&gg,0,sizeof(gg));memset(&dd,0,sizeof(dd));memset(&ss,0,sizeof(ss));
    memcpy(&ee.event,e,sizeof(*e));memcpy(&gg.plan,g,sizeof(*g));memcpy(&dd.domain,e->reader[0],sizeof(dd.domain));
    memcpy(ss.spans,e->reader[0]->spans,e->reader[0]->count*sizeof(*ss.spans));ee.event.reader[0]=&dd.domain;dd.domain.spans=ss.spans;
    if(which==0)plan=(const struct pt_paula_segment_plan *)(const void *)&ee;
    else if(which==1)ee.event.reader[0]=(const struct pt_readers_domain *)(const void *)&ee;
    else if(which==2)ee.event.reader[0]=(const struct pt_readers_domain *)(const void *)&gg;
    else if(which==3)dd.domain.spans=(const struct pt_scheduled_span *)(const void *)&ee;
    else if(which==4)dd.domain.spans=(const struct pt_scheduled_span *)(const void *)&gg;
    else if(which==5)dd.domain.spans=(const struct pt_scheduled_span *)(const void *)&dd;
    else if(which==6)ee.event.reader[0]=(const struct pt_readers_domain *)(const void *)(dd.bytes+1);
    else dd.domain.spans=(const struct pt_scheduled_span *)(const void *)(ss.bytes+1);
    memcpy(before[0],&ee,sizeof(ee));memcpy(before[1],&gg,sizeof(gg));memcpy(before[2],&dd,sizeof(dd));memcpy(before[3],&ss,sizeof(ss));
    assert(r->backend.submit_plan(r->backend.readers.context,event,plan)==0);
    assert(!memcmp(before[0],&ee,sizeof(ee))&&!memcmp(before[1],&gg,sizeof(gg))&&!memcmp(before[2],&dd,sizeof(dd))&&!memcmp(before[3],&ss,sizeof(ss)));
    assert(r->enters==enters&&!r->arms&&!r->effects);}
 ++r->alias_checked;}
static struct future_resource *submission_resource;
static int resource_submit(void *v,const struct pt_readers_event *e,const struct pt_paula_segment_plan *g)
{struct future_resource *r=submission_resource;assert(r&&r->future==v&&e->reader[0]);
 r->input_event=e;r->input_plan=g;r->input_domain=e->reader[0];r->input_spans=e->reader[0]->spans;
 r->span_count_before=e->reader[0]->count;r->ticket_before=e->scheduled.ticket;
 if(r->check_aliases&&!r->alias_checked)resource_fixed_refusers(r,e,g);
 return r->backend.submit_plan(v,e,g);}
/* These original descriptor/cache identities arrive in genuine arm packets.
 * They remain provider-visible Source while the independent reader is live;
 * tests never fabricate owners, pins, core retirement or replacement storage. */
static void resource_record_sources(struct future_resource *r,const struct pt_paula_future_packet *packet)
{unsigned i;for(i=0;i<packet->geometry.source_count;++i){const struct pt_paula_segment_source *s=packet->geometry.source+i;
 assert(s->sample<3&&s->descriptor&&s->cache_data&&s->cache_bytes);
 if(r->retained_pcm[s->sample])assert(r->retained_pcm[s->sample]==s->descriptor&&r->retained_cache[s->sample]==s->cache_data&&r->retained_cache_bytes[s->sample]==s->cache_bytes);
 r->retained_pcm[s->sample]=s->descriptor;r->retained_cache[s->sample]=s->cache_data;r->retained_cache_bytes[s->sample]=s->cache_bytes;}}
static void resource_mutate_pcm(struct future_resource *r,unsigned phase)
{unsigned i;if(r->pcm_injected||r->pcm_phase!=phase)return;
 for(i=0;i<3;++i){struct pt_pcm *pcm=(struct pt_pcm *)(uintptr_t)r->retained_pcm[i];assert(pcm&&pcm->rate==8000);++pcm->rate;}
 ++r->pcm_injected;}
static int resource_enter(void *v,enum pt_paula_future_entry entry,uint64_t *token)
{struct future_resource *r=v;assert(entry==PT_PAULA_FUTURE_TASK||entry==PT_PAULA_FUTURE_DELIVERY);
 assert(!r->locked);r->locked=1;*token=++r->enters;resource_mutate_input(r,1);resource_mutate_pcm(r,1);return 1;}
static int resource_leave(void *v,enum pt_paula_future_entry entry,uint64_t token)
{struct future_resource *r=v;(void)entry;assert(r->locked&&token==r->enters);r->locked=0;++r->leaves;resource_mutate_input(r,4);resource_mutate_pcm(r,3);return r->leave_raw;}
static int resource_clock(void *v,uint64_t *tick,uint32_t *frequency)
{struct future_resource *r=v;assert(r->locked);*tick=r->now;*frequency=r->frequency;resource_mutate_input(r,2);return 1;}
static int resource_arm(void *v,const struct pt_paula_future_packet *p)
{struct future_resource *r=v;assert(r->locked&&r->count<4);++r->arms;
 if(r->refuse){--r->refuse;return 0;}
 r->slot[r->count++]=(struct future_resource_slot){p->ticket,p,0,0};resource_record_sources(r,p);resource_mutate_input(r,3);return r->arm_raw;}
static int resource_rearm(void *v,const struct pt_paula_future_packet *p)
{struct future_resource *r=v;unsigned i;assert(r->locked);++r->rearms;
 for(i=0;i<r->count;++i)if(r->slot[i].ticket==p->ticket){assert(r->slot[i].packet==p&&p->first<p->last);return 1;}
 assert(0);return -1;}
static int resource_apply(void *v,const struct pt_paula_future_packet *p)
{struct future_resource *r=v;unsigned i;assert(r->locked&&r->effects<4);i=r->effects;
 memcpy(r->applied+i,&p->geometry,sizeof(p->geometry));++r->effects;
 if(p->geometry.kind==PT_PAULA_SEGMENT_INITIAL){assert(!r->current);r->current=p->geometry.source[0].cache_data;}
 else if(p->geometry.kind==PT_PAULA_SEGMENT_REPROGRAM){assert(r->current&&!p->geometry.write_initial);
    r->superseded=r->pending;r->pending=p->geometry.source[p->geometry.source_count-1].cache_data;r->phase=p->geometry.phase_q32;++r->controls;}
 else {assert(p->geometry.kind==PT_PAULA_SEGMENT_STOP_REQUIREMENTS);++r->stops;}
 if(r->cross)r->now=p->last;
 if(r->mutate)((struct pt_paula_future_packet *)(uintptr_t)p)->geometry.frame++;
 if(r->reenter)assert(pt_paula_segment_future_deliver(r->future,p->ticket)==PT_PAULA_FUTURE_INVALID);
 return r->apply_raw;}
static struct future_resource_slot *resource_slot(struct future_resource *r,uint64_t t)
{unsigned i;for(i=0;i<r->count;++i)if(r->slot[i].ticket==t)return r->slot+i;assert(0);return NULL;}
static int resource_ack(void *v,uint64_t t)
{struct future_resource *r=v;assert(r->locked);++resource_slot(r,t)->ack;return r->ack_raw;}
static int resource_cancel(void *v,uint64_t t)
{struct future_resource *r=v;assert(r->locked);++resource_slot(r,t)->cancel;return r->cancel_raw;}
static int resource_ticket_quiet(void *v,uint64_t t)
{struct future_resource *r=v;struct future_resource_slot *s=resource_slot(r,t);assert(r->locked);
 return !s->packet&&(s->ack||s->cancel)?1:0;}
static int resource_reader_quiet(void *v)
{struct future_resource *r=v;unsigned i;assert(r->locked);++r->reader_probes;
 for(i=0;i<r->count;++i)assert(!r->slot[i].packet);
 if(!r->allow_reader)return 0;
 r->current=r->pending=r->superseded=NULL;memset(r->applied,0,sizeof(r->applied));resource_mutate_pcm(r,2);return 1;}
static int resource_source_close(void *v)
{struct future_resource *r=v;assert(r->locked&&!r->current&&!r->pending&&!r->superseded);assert(++r->source_calls==1);return 1;}
static int resource_source_quiet(void *v)
{struct future_resource *r=v;assert(r->locked&&r->source_calls==1);++r->source_probes;return r->allow_source?1:0;}
static void future_init(struct future_fixture *x)
{memset(x,0,sizeof(*x));sequence_init(&x->sequence,0);
 x->resource.now=100;x->resource.frequency=709379;x->resource.arm_raw=x->resource.apply_raw=x->resource.cancel_raw=x->resource.ack_raw=x->resource.leave_raw=1;
 x->ops=(struct pt_paula_future_ops){&x->resource,sizeof(x->resource),resource_enter,resource_leave,resource_clock,
    resource_arm,resource_rearm,resource_apply,resource_ack,resource_cancel,resource_ticket_quiet,resource_reader_quiet,resource_source_close,resource_source_quiet};
 x->control_bytes=pt_paula_segment_future_size();x->control=allocate(&x->sequence.f.fast,x->control_bytes);assert(x->control);
 memset(x->control,0,x->control_bytes);x->resource.future=x->control;
 assert((uintptr_t)x->control%pt_paula_segment_future_alignment()==0);
 assert(pt_paula_segment_future_init(x->control,x->control_bytes,&x->ops,&x->sequence.config.readers.grid,
    x->sequence.config.readers.frame,&x->sequence.f.backend));
  memcpy(&x->resource.backend,&x->sequence.f.backend,sizeof(x->resource.backend));submission_resource=&x->resource;
  x->sequence.f.backend.submit_plan=resource_submit;sequence_open(&x->sequence);}
static void actual_resource_absence(struct future_fixture *x,unsigned i)
{assert(i<x->resource.count&&(x->resource.slot[i].ack||x->resource.slot[i].cancel));
 /* Exact original provider record loses its actual pending callback reference.
  * This does not dispose a real core command or reader. */
 x->resource.slot[i].packet=NULL;}
static void source_finish(struct future_fixture *x)
{unsigned probes;struct pt_paula_future_status s;
 assert(pt_paula_segment_future_stop(x->resource.future)==PT_PAULA_FUTURE_PENDING);
 assert(x->resource.source_calls==1&&!pt_paula_segment_future_dispose(x->resource.future));probes=x->resource.source_probes;
 assert(pt_paula_segment_future_stop(x->resource.future)==PT_PAULA_FUTURE_PENDING);
 assert(x->resource.source_calls==1&&x->resource.source_probes==probes+1);
 x->resource.allow_source=1;assert(pt_paula_segment_future_stop(x->resource.future)==PT_PAULA_FUTURE_OK);
 assert(!x->resource.locked&&x->resource.enters==x->resource.leaves);
 assert(pt_paula_segment_future_get(x->resource.future,&s)&&s.source_quiet&&s.detached==s.accepted&&s.reader_retired);
 assert(pt_paula_segment_future_dispose(x->resource.future));
 release(&x->sequence.f.fast,x->control);x->control=NULL;sequence_finish(&x->sequence);}
static void future_publish(struct future_fixture *x,unsigned i)
{assert(sequence_until(&x->sequence,PT_PAULA_SEQUENCE_READY)==PT_PAULA_SEQUENCE_READY);
 assert(pt_paula_segment_sequence_publish_next(x->sequence.producer,x->sequence.revision)==PT_SCHEDULED_OK);
 assert(x->resource.count==i+1&&x->resource.slot[i].packet->geometry.frame==x->sequence.config.readers.frame[i]);}
static void future_fire(struct future_fixture *x,unsigned i)
{const struct pt_paula_future_packet *p=x->resource.slot[i].packet;
 assert(p);x->resource.now=p->first;
 assert(pt_paula_segment_future_deliver(x->resource.future,p->ticket)==PT_PAULA_FUTURE_OK);
 assert(pt_paula_segment_sequence_service_command(x->sequence.producer,i,0)==PT_SCHEDULED_PENDING);
 assert(pinned(&x->sequence.f)==3&&x->sequence.f.chip.bytes==2304);storage(&x->sequence.f);}
static void future_detach(struct future_fixture *x,unsigned i)
{actual_resource_absence(x,i);
 assert(pt_paula_segment_sequence_service_command(x->sequence.producer,i,0)==PT_SCHEDULED_OK);}
static void future_positive(unsigned reader_first,unsigned early,unsigned pressure)
{struct future_fixture x;unsigned i;uint64_t old=0;future_init(&x);
 for(i=0;i<4;++i){struct pt_paula_segment_sequence_status before,after;
    assert(sequence_until(&x.sequence,PT_PAULA_SEQUENCE_READY)==PT_PAULA_SEQUENCE_READY);
    if(pressure==i+1){before=sequence_status(&x.sequence);x.resource.refuse=1;
       assert(pt_paula_segment_sequence_publish_next(x.sequence.producer,x.sequence.revision)==PT_SCHEDULED_PENDING);
       after=sequence_status(&x.sequence);assert(before.boundary==after.boundary&&before.ticket[i]==after.ticket[i]&&
         before.phase==after.phase&&before.live_events==after.live_events&&x.resource.count==i);}
    future_publish(&x,i);
    if(i==0){struct pt_paula_segment_sequence_status s;
       assert(sequence_until(&x.sequence,PT_PAULA_SEQUENCE_WAIT_ACTIVE)==PT_PAULA_SEQUENCE_WAIT_ACTIVE);
       s=sequence_status(&x.sequence);assert(s.live_events==1&&s.boundary==1920&&x.resource.effects==0);}
    if(early&&i==1){const struct pt_paula_future_packet *p=x.resource.slot[i].packet;struct pt_paula_future_packet saved;
       memcpy(&saved,p,sizeof(saved));x.resource.now=p->first-1;
       assert(pt_paula_segment_future_deliver(x.resource.future,p->ticket)==PT_PAULA_FUTURE_EARLY);
       assert(!memcmp(&saved,p,sizeof(saved))&&x.resource.rearms==1&&x.resource.effects==1);}
    if(i){unsigned effects=x.resource.effects;assert(pt_paula_segment_future_deliver(x.resource.future,old)==PT_PAULA_FUTURE_STALE);assert(effects==x.resource.effects);}
    future_fire(&x,i);old=x.resource.slot[i].ticket;
    if(i==0)assert(x.resource.applied[0].initial.frame_offset==256&&x.resource.applied[0].initial.frame_length==512);
    if(i==1||i==2)assert(!x.resource.applied[i].write_initial&&x.resource.applied[i].phase_q32);
    if(i<3)future_detach(&x,i);}
 assert(sequence_until(&x.sequence,PT_PAULA_SEQUENCE_DONE)==PT_PAULA_SEQUENCE_DONE);
 assert(x.resource.effects==4&&x.resource.controls==2&&x.resource.stops==1&&x.resource.current&&x.resource.pending&&x.resource.superseded);
 assert(!pt_paula_segment_sequence_close(&x.sequence.producer)&&pinned(&x.sequence.f)==3);
 assert(pt_paula_segment_sequence_service_reader(x.sequence.producer,0)==PT_SCHEDULED_PENDING);
 assert(pinned(&x.sequence.f)==3);actual_resource_absence(&x,3);x.resource.allow_reader=1;
 if(reader_first){assert(pt_paula_segment_sequence_service_reader(x.sequence.producer,0)==PT_SCHEDULED_OK);assert(pinned(&x.sequence.f)==3);}
 assert(pt_paula_segment_sequence_service_command(x.sequence.producer,3,0)==PT_SCHEDULED_OK);
 if(!reader_first){assert(pinned(&x.sequence.f)==3);assert(pt_paula_segment_sequence_service_reader(x.sequence.producer,0)==PT_SCHEDULED_OK);}
 assert(!pinned(&x.sequence.f)&&!x.resource.current&&!x.resource.pending&&!x.resource.superseded);
 source_finish(&x);++future_cases;}
static void future_cancel_before(void)
{struct future_fixture x;future_init(&x);future_publish(&x,0);
 assert(pt_paula_segment_sequence_cancel(x.sequence.producer)==PT_PAULA_SEQUENCE_FAILED);
 assert(pt_paula_segment_sequence_service_command(x.sequence.producer,0,1)==PT_SCHEDULED_PENDING);
 assert(x.resource.slot[0].cancel==1&&x.resource.effects==0&&pinned(&x.sequence.f)==3);
 assert(pt_paula_segment_future_deliver(x.resource.future,x.resource.slot[0].ticket)==PT_PAULA_FUTURE_STALE);
 actual_resource_absence(&x,0);assert(pt_paula_segment_sequence_service_command(x.sequence.producer,0,0)==PT_SCHEDULED_OK);
 assert(pinned(&x.sequence.f)==3);x.resource.allow_reader=1;
 assert(pt_paula_segment_sequence_service_reader(x.sequence.producer,0)==PT_SCHEDULED_OK);assert(!pinned(&x.sequence.f));
 source_finish(&x);++future_cases;}
/* Callback failures are intentionally classified invalid while exact independent
 * envelopes still permit genuine core disposal. No state/error is cleared. */
static void future_unknown(unsigned mode)
{struct future_fixture x;enum pt_scheduled_result r;struct pt_paula_future_status s;future_init(&x);
 if(mode==0)x.resource.arm_raw=-1;
 assert(sequence_until(&x.sequence,PT_PAULA_SEQUENCE_READY)==PT_PAULA_SEQUENCE_READY);
 r=pt_paula_segment_sequence_publish_next(x.sequence.producer,x.sequence.revision);
 if(mode==0)assert(r==PT_SCHEDULED_BACKEND);else{assert(r==PT_SCHEDULED_OK);
    x.resource.now=x.resource.slot[0].packet->first;
    if(mode==1)x.resource.apply_raw=0;else if(mode==2)x.resource.cross=1;else if(mode==3)x.resource.mutate=1;else if(mode==4)x.resource.reenter=1;
    if(mode>=5){x.resource.cancel_raw=mode==5?0:-1;assert(pt_paula_segment_future_stop(x.resource.future)==PT_PAULA_FUTURE_PENDING);}
    else assert(pt_paula_segment_future_deliver(x.resource.future,x.resource.slot[0].ticket)==PT_PAULA_FUTURE_UNKNOWN);}
 assert(pt_paula_segment_future_get(x.resource.future,&s)&&s.failed&&!s.adopted&&!s.detached&&!s.reader_retired);
 assert(pinned(&x.sequence.f)==3&&!pt_paula_segment_sequence_close(&x.sequence.producer));
 r=pt_paula_segment_sequence_service_command(x.sequence.producer,0,1);assert(r==PT_SCHEDULED_BACKEND);
 assert(x.resource.slot[0].cancel==1&&pinned(&x.sequence.f)==3);
 actual_resource_absence(&x,0);x.resource.allow_reader=1;
 if(mode&1U){r=pt_paula_segment_sequence_service_reader(x.sequence.producer,0);assert(r==PT_SCHEDULED_BACKEND);assert(pinned(&x.sequence.f)==3);}
 r=pt_paula_segment_sequence_service_command(x.sequence.producer,0,0);assert(r==PT_SCHEDULED_BACKEND);
 if(!(mode&1U)){assert(pinned(&x.sequence.f)==3);r=pt_paula_segment_sequence_service_reader(x.sequence.producer,0);assert(r==PT_SCHEDULED_BACKEND);}
 assert(!pinned(&x.sequence.f));source_finish(&x);++future_cases;}
static void future_late(void)
{struct future_fixture x;enum pt_scheduled_result r;struct pt_paula_future_status s;future_init(&x);future_publish(&x,0);
 x.resource.now=x.resource.slot[0].packet->last;
 assert(pt_paula_segment_future_deliver(x.resource.future,x.resource.slot[0].ticket)==PT_PAULA_FUTURE_LATE);
 assert(x.resource.effects==0&&pinned(&x.sequence.f)==3);
 assert(pt_paula_segment_sequence_service_command(x.sequence.producer,0,0)==PT_SCHEDULED_BACKEND);
 actual_resource_absence(&x,0);assert(pt_paula_segment_sequence_service_command(x.sequence.producer,0,0)==PT_SCHEDULED_BACKEND);
 assert(pinned(&x.sequence.f)==3);x.resource.allow_reader=1;
 /* Exact positive retirement consumes real owners; the earlier command fault
  * remains latched and its BACKEND result must not become success. */
 r=pt_paula_segment_sequence_service_reader(x.sequence.producer,0);assert(r==PT_SCHEDULED_BACKEND);
 assert(pt_paula_segment_future_get(x.resource.future,&s)&&s.failed&&!s.adopted&&s.accepted==1&&s.detached==1&&s.reader_retired);
 assert(!x.resource.effects&&x.resource.reader_probes==1&&!x.resource.slot[0].packet&&
    !x.resource.current&&!x.resource.pending&&!x.resource.superseded&&!pinned(&x.sequence.f));
 assert(!x.resource.locked&&x.resource.enters==x.resource.leaves);
 source_finish(&x);++future_cases;}
static void constructor_refusers(void)
{struct future_fixture x;struct pt_paula_segment_backend out,saved;unsigned char *copy;unsigned calls;
 /* Use a genuine initialized provider/context but separate never-owned storage. */
 memset(&x,0,sizeof(x));sequence_init(&x.sequence,0);x.control_bytes=pt_paula_segment_future_size();x.control=allocate(&x.sequence.f.fast,x.control_bytes+16);assert(x.control);
 memset(x.control,0,x.control_bytes+16);x.ops=(struct pt_paula_future_ops){&x.resource,sizeof(x.resource),resource_enter,resource_leave,resource_clock,
    resource_arm,resource_rearm,resource_apply,resource_ack,resource_cancel,resource_ticket_quiet,resource_reader_quiet,resource_source_close,resource_source_quiet};
 memset(&out,0x4a,sizeof(out));memcpy(&saved,&out,sizeof(saved));copy=malloc(x.control_bytes+16);assert(copy);memcpy(copy,x.control,x.control_bytes+16);calls=x.resource.enters;
 assert(!pt_paula_segment_future_init((unsigned char *)x.control+1,x.control_bytes,&x.ops,&x.sequence.config.readers.grid,x.sequence.config.readers.frame,&out));
 assert(!pt_paula_segment_future_init(x.control,x.control_bytes,(const struct pt_paula_future_ops *)((unsigned char *)&x.ops+1),&x.sequence.config.readers.grid,x.sequence.config.readers.frame,&out));
 assert(!pt_paula_segment_future_init(x.control,x.control_bytes,&x.ops,&x.sequence.config.readers.grid,x.sequence.config.readers.frame,x.control));
 assert(!memcmp(&saved,&out,sizeof(out))&&!memcmp(copy,x.control,x.control_bytes+16)&&x.resource.enters==calls);free(copy);
 release(&x.sequence.f.fast,x.control);release(&x.sequence.f.fast,x.sequence.workspace);x.sequence.workspace=NULL;close_fixture(&x.sequence.f);++future_cases;}
/* Original callbacks see only authentic borrowed descriptors passed through the
 * real finite facade. No owner function or genuine core state is exposed. */
static void future_callback_input(unsigned phase,unsigned mode)
{struct future_fixture x;struct pt_paula_future_status s;enum pt_scheduled_result r;
 future_init(&x);x.resource.fault_phase=phase;x.resource.fault_mode=mode;
 assert(sequence_until(&x.sequence,PT_PAULA_SEQUENCE_READY)==PT_PAULA_SEQUENCE_READY);
 assert(pt_paula_segment_sequence_publish_next(x.sequence.producer,x.sequence.revision)==PT_SCHEDULED_BACKEND);
 assert(x.resource.injected==1&&!x.resource.locked&&x.resource.enters==x.resource.leaves);
 assert(x.resource.effects==0&&pinned(&x.sequence.f)==3&&x.sequence.f.chip.bytes==2304);
 assert(pt_paula_segment_future_get(x.resource.future,&s)&&s.failed&&s.accepted==1&&!s.adopted&&!s.detached&&!s.reader_retired);
 /* The outer genuine finite facade repairs only its supplied immutable
  * descriptors. The future model never follows or repairs altered counts. */
 assert(x.resource.input_event->scheduled.batch.count==1&&x.resource.input_event->scheduled.ticket==x.resource.ticket_before);
 assert(x.resource.input_domain->count==x.resource.span_count_before&&x.resource.input_domain->spans==x.resource.input_spans);
 assert(x.resource.input_plan->frame==960&&x.resource.input_event->reader[0]==x.resource.input_domain);
 assert(!pt_paula_segment_sequence_close(&x.sequence.producer));
 assert(pt_paula_segment_future_stop(x.resource.future)==PT_PAULA_FUTURE_PENDING);
 if(x.resource.count){assert(x.resource.count==1&&x.resource.slot[0].cancel==1);actual_resource_absence(&x,0);}
 else assert(x.resource.arms==0);
 x.resource.allow_reader=1;
 if(mode&1U){r=pt_paula_segment_sequence_service_reader(x.sequence.producer,0);assert(r==PT_SCHEDULED_BACKEND);assert(pinned(&x.sequence.f)==3);}
 r=pt_paula_segment_sequence_service_command(x.sequence.producer,0,1);assert(r==PT_SCHEDULED_BACKEND);
 if(!(mode&1U)){assert(pinned(&x.sequence.f)==3);r=pt_paula_segment_sequence_service_reader(x.sequence.producer,0);assert(r==PT_SCHEDULED_BACKEND);}
 assert(!pinned(&x.sequence.f));source_finish(&x);++future_cases;}
static void future_cancel_continuation(unsigned index,unsigned reader_first)
{struct future_fixture x;struct pt_readers_command_receipt receipt;unsigned i;
 future_init(&x);
 for(i=0;i<index;++i){future_publish(&x,i);future_fire(&x,i);future_detach(&x,i);}
 future_publish(&x,index);assert(x.resource.effects==index&&pinned(&x.sequence.f)==3);
 assert(pt_paula_segment_sequence_cancel(x.sequence.producer)==PT_PAULA_SEQUENCE_FAILED);
 assert(pt_paula_segment_sequence_service_command(x.sequence.producer,index,1)==PT_SCHEDULED_PENDING);
 assert(x.resource.backend.readers.poll_command(x.resource.backend.readers.context,x.resource.slot[index].ticket,&receipt)==PT_READERS_OBSERVATION);
 assert(receipt.action[0].command==PT_READERS_CANCELLED_BEFORE&&receipt.action[0].adoption==PT_READERS_ADOPTED);
 assert(!receipt.action[0].observed&&!receipt.action[0].issued);
 assert(x.resource.slot[index].cancel==1&&x.resource.effects==index&&pinned(&x.sequence.f)==3);
 assert(pt_paula_segment_future_deliver(x.resource.future,x.resource.slot[index].ticket)==PT_PAULA_FUTURE_STALE);
 actual_resource_absence(&x,index);x.resource.allow_reader=1;
 if(reader_first){assert(pt_paula_segment_sequence_service_reader(x.sequence.producer,0)==PT_SCHEDULED_OK);assert(pinned(&x.sequence.f)==3);}
 assert(pt_paula_segment_sequence_service_command(x.sequence.producer,index,0)==PT_SCHEDULED_OK);
 if(!reader_first){assert(pinned(&x.sequence.f)==3);assert(pt_paula_segment_sequence_service_reader(x.sequence.producer,0)==PT_SCHEDULED_OK);}
 assert(!pinned(&x.sequence.f)&&x.resource.effects==index);source_finish(&x);++future_cases;}
/* Exactly two process-lifetime holders retain all Source/context/control/master
 * allocations when actual matching outer restoration is unknown. They are
 * conservative HOLD cases, not successful cleanup or restoration acceptance. */
static struct future_fixture *held_restore[2];
static void future_leave_hold(unsigned reader)
{struct future_fixture *x;struct pt_paula_future_status s;unsigned probes;unsigned char *copy;size_t bytes;
 assert(reader<2&&!held_restore[reader]);x=calloc(1,sizeof(*x));assert(x);held_restore[reader]=x;
 future_init(x);future_publish(x,0);future_fire(x,0);
 if(reader){future_detach(x,0);assert(pt_paula_segment_sequence_cancel(x->sequence.producer)==PT_PAULA_SEQUENCE_FAILED);
    x->resource.allow_reader=1;bytes=sizeof(*x->resource.input_domain);copy=malloc(bytes);assert(copy);memcpy(copy,x->resource.input_domain,bytes);}
 else{actual_resource_absence(x,0);bytes=sizeof(*x->resource.input_event);copy=malloc(bytes);assert(copy);memcpy(copy,x->resource.input_event,bytes);}
 x->resource.leave_raw=0;
 if(reader)assert(pt_paula_segment_sequence_service_reader(x->sequence.producer,0)==PT_SCHEDULED_BACKEND);
 else assert(pt_paula_segment_sequence_service_command(x->sequence.producer,0,0)==PT_SCHEDULED_BACKEND);
 assert(!memcmp(copy,reader?(const void *)x->resource.input_domain:(const void *)x->resource.input_event,bytes));free(copy);
 assert(pt_paula_segment_future_get(x->resource.future,&s)&&s.failed&&s.accepted==1&&!s.reader_retired&&!s.source_quiet);
 assert(s.detached==reader&&pinned(&x->sequence.f)==3&&x->sequence.f.chip.bytes==2304);
 probes=x->resource.enters;
 assert(!pt_paula_segment_future_get(x->resource.future,(struct pt_paula_future_status *)(uintptr_t)x->resource.input_domain));
 assert(!pt_paula_segment_future_get(x->resource.future,(struct pt_paula_future_status *)(uintptr_t)x->resource.input_spans));
 assert(!pt_paula_segment_future_dispose(x->resource.future)&&x->resource.enters==probes);
 storage(&x->sequence.f);++future_cases;}
static void future_fixed_aliases(void)
{struct future_fixture x;future_init(&x);x.resource.check_aliases=1;future_publish(&x,0);
 assert(x.resource.alias_checked==1&&x.resource.count==1&&x.resource.effects==0);
 assert(pt_paula_segment_sequence_cancel(x.sequence.producer)==PT_PAULA_SEQUENCE_FAILED);
 assert(pt_paula_segment_sequence_service_command(x.sequence.producer,0,1)==PT_SCHEDULED_PENDING);
 actual_resource_absence(&x,0);assert(pt_paula_segment_sequence_service_command(x.sequence.producer,0,0)==PT_SCHEDULED_OK);
 x.resource.allow_reader=1;assert(pt_paula_segment_sequence_service_reader(x.sequence.producer,0)==PT_SCHEDULED_OK);
 assert(!pinned(&x.sequence.f));source_finish(&x);++future_cases;}
static void future_output_aliases(void)
{struct future_fixture x;struct pt_paula_future_status status;const void *where[4];size_t bytes[4];unsigned i,enters;uint32_t frequency;
 future_init(&x);future_publish(&x,0);where[0]=x.resource.input_domain;bytes[0]=sizeof(*x.resource.input_domain);
 where[1]=x.resource.input_spans;bytes[1]=x.resource.span_count_before*sizeof(*x.resource.input_spans);
 where[2]=x.resource.input_domain->binding.context;bytes[2]=x.resource.input_domain->binding.context_bytes;
 where[3]=x.resource.input_plan;bytes[3]=sizeof(*x.resource.input_plan);enters=x.resource.enters;
 for(i=0;i<4;++i){unsigned char *before=malloc(bytes[i]);assert(before&&bytes[i]>=sizeof(status));memcpy(before,where[i],bytes[i]);
    assert(!pt_paula_segment_future_get(x.resource.future,(struct pt_paula_future_status *)(uintptr_t)where[i]));
    assert(!x.resource.backend.readers.read_clock(x.resource.backend.readers.context,(uint64_t *)(uintptr_t)where[i],&frequency));
    assert(!memcmp(before,where[i],bytes[i]));free(before);}
 {unsigned char *command_before=malloc(bytes[2]),*reader_before=malloc(bytes[1]);
    assert(command_before&&reader_before&&bytes[2]>=sizeof(struct pt_readers_command_receipt)&&bytes[1]>=sizeof(struct pt_readers_reader_receipt));
    memcpy(command_before,where[2],bytes[2]);memcpy(reader_before,where[1],bytes[1]);
    assert(x.resource.backend.readers.poll_command(x.resource.backend.readers.context,x.resource.slot[0].ticket,
       (struct pt_readers_command_receipt *)(uintptr_t)where[2])==PT_READERS_UNCERTAIN);
    assert(x.resource.backend.readers.poll_reader(x.resource.backend.readers.context,x.resource.input_domain,
       (struct pt_readers_reader_receipt *)(uintptr_t)where[1])==PT_READERS_UNCERTAIN);
    assert(!memcmp(command_before,where[2],bytes[2])&&!memcmp(reader_before,where[1],bytes[1]));free(command_before);free(reader_before);}
 assert(x.resource.enters==enters&&x.resource.effects==0&&pinned(&x.sequence.f)==3);
 assert(pt_paula_segment_sequence_cancel(x.sequence.producer)==PT_PAULA_SEQUENCE_FAILED);
 assert(pt_paula_segment_sequence_service_command(x.sequence.producer,0,1)==PT_SCHEDULED_PENDING);
 actual_resource_absence(&x,0);assert(pt_paula_segment_sequence_service_command(x.sequence.producer,0,0)==PT_SCHEDULED_OK);
 x.resource.allow_reader=1;assert(pt_paula_segment_sequence_service_reader(x.sequence.producer,0)==PT_SCHEDULED_OK);
 assert(!pinned(&x.sequence.f));source_finish(&x);++future_cases;}
/* Three additional, globally reachable Source-custody HOLD cases. All four
 * genuine commands detach before provider enter/reader_quiet/final-leave
 * mutates the three original project PCM descriptors. No mutation is undone,
 * no storage is freed, and no later probe is treated as recovery permission. */
static struct future_fixture *held_pcm[3];
static void future_reader_pcm_hold(unsigned phase)
{struct future_fixture *x;struct pt_paula_future_status s;struct pt_paula_segment_sequence_status sequence;
 struct pt_readers_domain domain;struct pt_scheduled_span *table;struct pt_pcm original[3],expected;
 unsigned i,fast_live,enters;uint8_t cache[3][768];size_t table_bytes;
 assert(phase>=1&&phase<=3&&!held_pcm[phase-1]);x=calloc(1,sizeof(*x));assert(x);held_pcm[phase-1]=x;
 future_init(x);
 for(i=0;i<4;++i){future_publish(x,i);future_fire(x,i);future_detach(x,i);}
 assert(sequence_until(&x->sequence,PT_PAULA_SEQUENCE_DONE)==PT_PAULA_SEQUENCE_DONE);
 sequence=sequence_status(&x->sequence);
 assert(sequence.detached_mask==15&&!sequence.reader_retired&&x->resource.effects==4&&x->resource.controls==2&&x->resource.stops==1);
 assert(pt_paula_segment_future_get(x->resource.future,&s)&&s.accepted==4&&s.detached==4&&s.adopted&&!s.failed&&!s.reader_retired);
 assert(pinned(&x->sequence.f)==3&&x->sequence.f.chip.bytes==2304&&x->resource.current&&x->resource.pending&&x->resource.superseded);
 for(i=0;i<3;++i){assert(x->resource.retained_pcm[i]==&x->sequence.f.document.project.samples[i].pcm);
   memcpy(original+i,x->resource.retained_pcm[i],sizeof(original[i]));
   assert(original[i].data==x->sequence.f.pcm[i].data&&original[i].capacity==768&&original[i].frames==768&&original[i].rate==8000);
   assert(x->resource.retained_cache_bytes[i]==sizeof(cache[i]));memcpy(cache[i],x->resource.retained_cache[i],sizeof(cache[i]));}
 memcpy(&domain,x->resource.input_domain,sizeof(domain));table_bytes=domain.count*sizeof(*table);table=malloc(table_bytes);assert(table);
 memcpy(table,domain.spans,table_bytes);fast_live=x->sequence.f.fast.live;x->resource.allow_reader=1;x->resource.pcm_phase=phase;
 assert(pt_paula_segment_sequence_service_reader(x->sequence.producer,0)==PT_SCHEDULED_BACKEND);
 assert(x->resource.pcm_injected==1&&!x->resource.locked&&x->resource.enters==x->resource.leaves);
 assert(x->resource.reader_probes==(phase==1?0U:1U)&&x->resource.effects==4&&!x->resource.source_calls);
 assert(pt_paula_segment_future_get(x->resource.future,&s)&&s.failed&&s.accepted==4&&s.detached==4&&!s.reader_retired&&!s.source_attempted&&!s.source_quiet);
 sequence=sequence_status(&x->sequence);assert(sequence.failed&&sequence.detached_mask==15&&!sequence.reader_retired);
 assert(pinned(&x->sequence.f)==3&&x->sequence.f.chip.bytes==2304&&x->sequence.f.fast.live==fast_live);
 assert(!memcmp(&domain,x->resource.input_domain,sizeof(domain))&&!memcmp(table,domain.spans,table_bytes));free(table);
 for(i=0;i<3;++i){memcpy(&expected,original+i,sizeof(expected));++expected.rate;
   assert(!memcmp(&expected,x->resource.retained_pcm[i],sizeof(expected)));
   assert(!memcmp(original[i].data,x->sequence.master[i],768*sizeof(*original[i].data)));
   assert(!memcmp(cache[i],x->resource.retained_cache[i],sizeof(cache[i])));}
 /* Exact positive outer restoration did occur. Source custody still did not;
  * retain the real producer owner, source/timer context, pins and cache leases. */
 enters=x->resource.enters;assert(!pt_paula_segment_future_dispose(x->resource.future));
 assert(!pt_paula_segment_sequence_close(&x->sequence.producer)&&x->sequence.producer&&x->sequence.workspace&&x->control);
 assert(x->resource.enters==enters&&pinned(&x->sequence.f)==3&&x->sequence.f.chip.bytes==2304&&x->sequence.f.fast.live==fast_live);
 storage(&x->sequence.f);++future_cases;}
/* Genuine sampler edit generations begin at zero; cache/grid/lease identifiers
 * remain independently nonzero. No sampler/history counter is assigned here. */
static void future_initial_generation_zero(void)
{struct future_fixture x;const struct pt_paula_future_packet *p;future_init(&x);
 assert(x.sequence.f.sampler.generation==0);future_publish(&x,0);p=x.resource.slot[0].packet;
 assert(p&&p->geometry.source_count==1&&p->geometry.source[0].generation==0&&
    p->geometry.source[0].sampler_owner==&x.sequence.f.sampler&&
    p->geometry.source[0].project_identity==&x.sequence.f.document.project&&
    p->geometry.source[0].version==x.sequence.f.pin[0]&&
    p->geometry.source[0].cache_owner==&x.sequence.f.bridge&&
    p->geometry.source[0].cache_version&&p->geometry.source[0].lease.serial);
 future_fire(&x,0);future_detach(&x,0);
 assert(pinned(&x.sequence.f)==3&&x.sequence.f.chip.bytes==2304);
 assert(pt_paula_segment_sequence_cancel(x.sequence.producer)==PT_PAULA_SEQUENCE_FAILED);
 x.resource.allow_reader=1;
 assert(pt_paula_segment_sequence_service_reader(x.sequence.producer,0)==PT_SCHEDULED_OK);
 assert(!pinned(&x.sequence.f)&&!x.resource.current&&!x.resource.pending&&!x.resource.superseded);
 source_finish(&x);++future_cases;}
static void future_unarmed_source_finish(struct future_fixture *x)
{struct pt_paula_future_status s;
 assert(!x->resource.count&&!x->resource.arms&&!x->resource.effects);
 assert(pt_paula_segment_future_stop(x->resource.future)==PT_PAULA_FUTURE_PENDING);
 assert(x->resource.source_calls==1&&!pt_paula_segment_future_dispose(x->resource.future));
 x->resource.allow_source=1;assert(pt_paula_segment_future_stop(x->resource.future)==PT_PAULA_FUTURE_OK);
 assert(pt_paula_segment_future_get(x->resource.future,&s)&&s.source_quiet&&!s.accepted&&!s.detached&&!s.adopted&&!s.reader_retired);
 assert(!x->resource.locked&&x->resource.enters==x->resource.leaves&&pt_paula_segment_future_dispose(x->resource.future));
 release(&x->sequence.f.fast,x->control);x->control=NULL;sequence_finish(&x->sequence);}
static void future_genuine_edit_generation_stale(unsigned prepared)
{struct future_fixture x;struct pt_pcm before,output;struct pt_sample_version *token;unsigned arms,enters;
 future_init(&x);assert(prepared<2&&x.sequence.f.sampler.generation==0);
 if(prepared)assert(sequence_until(&x.sequence,PT_PAULA_SEQUENCE_READY)==PT_PAULA_SEQUENCE_READY);
 memcpy(&before,&x.sequence.f.pcm[1],sizeof(before));memcpy(&output,&before,sizeof(output));token=x.sequence.f.pin[1];
 assert(pt_sampler_edit(&x.sequence.f.sampler,&x.sequence.f.document.project,&x.sequence.f.history,1,PT_PCM_REVERSE,0,32,0)==PT_EDIT_OK);
 assert(x.sequence.f.sampler.generation==1&&x.sequence.f.document.project.samples[1].pcm.data!=before.data);
 assert(pt_sampler_pin_current(&x.sequence.f.sampler,&x.sequence.f.document.project,1,0,x.sequence.f.pin[1],&output,&token)==PT_EDIT_CONFLICT);
 assert(!memcmp(&output,&before,sizeof(output))&&token==x.sequence.f.pin[1]);storage(&x.sequence.f);
 arms=x.resource.arms;enters=x.resource.enters;
 if(prepared)assert(pt_paula_segment_sequence_publish_next(x.sequence.producer,x.sequence.revision)==PT_SCHEDULED_STALE);
 else assert(sequence_until(&x.sequence,PT_PAULA_SEQUENCE_READY)==PT_PAULA_SEQUENCE_STALE);
 assert(x.resource.arms==arms&&x.resource.enters==enters&&!x.resource.count&&!x.resource.effects);
 assert(pt_paula_segment_sequence_cancel(x.sequence.producer)==PT_PAULA_SEQUENCE_FAILED);
 future_unarmed_source_finish(&x);++future_cases;}
int pt_owner4_dormant_future60_main(void)
{unsigned i,j;future_initial_generation_zero();future_genuine_edit_generation_stale(0);future_genuine_edit_generation_stale(1);future_positive(0,0,0);future_positive(1,1,0);
 for(i=1;i<=4;++i)future_positive(i&1U,0,i);
 future_cancel_before();for(i=0;i<7;++i)future_unknown(i);future_late();constructor_refusers();
 for(i=1;i<=4;++i)for(j=0;j<7;++j)future_callback_input(i,j);
  for(i=1;i<=3;++i)for(j=0;j<2;++j)future_cancel_continuation(i,j);
  future_fixed_aliases();future_output_aliases();future_leave_hold(0);future_leave_hold(1);
  for(i=1;i<=3;++i)future_reader_pcm_hold(i);
   assert(future_cases==60);
 puts("PAULA GEOMETRY FUTURE HOST FIXTURE PASS:60 cases;genuine measured renderer-fixed4 and finite3 ownership;original960-1920-2880-3840 windows;whole task-delivery exclusion;positive original-window ACTIVE before continuation;EARLY same-window rearm and stale refusal;clean pressure preserves original admission;late-no-effect and unknown-arm-apply-crossed-cancel-mutation-reentry retain actual obligations;independent timer-command-reader-source quiet and both retirement orders;once-only source shutdown;constructor and borrowed fixed/output aliases refused before callbacks;enter-clock-arm-leave original descriptor custody;undelivered CONTROL-STOP cancellation preserves independent adopted reader;quiet anchors commit only after positive outer leave;reader-live three-source PCM custody after all command detachment;55 restored cases and5 deliberate process-lifetime HOLD holders(2 restoration and3 Source PCM custody);initial sampler generation0 accepted and genuine sampler edits refused as stale;HOST_ONLY_NATIVE_OUTPUT_NOT_RUN");return 0;}
