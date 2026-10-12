/* Accepted finite97 helpers supply real sampler/version/cache/core ownership
 * and an independent HOST backend that drops its actual borrowed references.
 * Its dormant main and handcrafted action helpers are never called here. */
#define main pt_private_finite97_dormant_main
#include "paula_segment_repeat_readers_test.c"
#undef main
#include "../src/editor/paula_segment_sequence.h"
struct provider {struct model *actual;unsigned refuse,attempts;
    struct pt_paula_segment_sequence *producer;unsigned reenter_submit,nested;};
struct sequence_fixture {
    struct fixture f;int32_t master[3][768];struct provider provider;
    struct pt_paula_segment_sequence_config config;
    struct pt_paula_segment_sequence *producer;void *workspace;size_t workspace_bytes;
    uint32_t revision;
};
static unsigned sequence_cases;
static int sequence_clock(void *v,uint64_t *ticks,uint32_t *frequency)
{struct provider *p=v;return clock_read(p->actual,ticks,frequency);}
static int sequence_submit(void *v,const struct pt_readers_event *event,const struct pt_paula_segment_plan *plan)
{struct provider *p=v;int raw;++p->attempts;
 /* Clean backpressure owns no supplied address and makes no output effect. */
 if(p->refuse){--p->refuse;return 0;}raw=submit_plan(p->actual,event,plan);
 /* Reenter only after the real backend stores its borrowed references. */
 if(p->reenter_submit){struct pt_paula_segment_sequence_status untouched;
    p->reenter_submit=0;assert(p->producer);
    p->nested=(unsigned)pt_paula_segment_sequence_get(p->producer,&untouched);}
 return raw;}
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
static void sequence_activate(struct sequence_fixture *x,unsigned i,int detach_now)
{
    struct pt_paula_segment_sequence_status before=sequence_status(x);enum pt_scheduled_result r;
    assert(before.phase==PT_PAULA_SEQUENCE_PUBLISH&&before.boundary==x->config.readers.frame[i]&&before.live_events==i);
    assert(pt_paula_segment_sequence_publish_next(x->producer,x->revision)==PT_SCHEDULED_OK);
    assert(x->f.model.count==i+1&&x->f.model.command[i].plan.frame==x->config.readers.frame[i]);
    fire(&x->f,i);if(detach_now)detach(&x->f,i);
    r=pt_paula_segment_sequence_service_command(x->producer,i,0);
    assert(r==(detach_now?PT_SCHEDULED_OK:PT_SCHEDULED_PENDING));
    assert(pinned(&x->f)==3&&x->f.chip.bytes==2304);storage(&x->f);
}
static void sequence_terminal(struct sequence_fixture *x,unsigned reader_first)
{
    struct pt_paula_segment_sequence_status s;
    assert(sequence_until(x,PT_PAULA_SEQUENCE_DONE)==PT_PAULA_SEQUENCE_DONE);
    assert(!pt_paula_segment_sequence_close(&x->producer)&&x->producer&&pinned(&x->f)==3);
    if(reader_first){genuine_quiet(&x->f);assert(pt_paula_segment_sequence_service_reader(x->producer,0)==PT_SCHEDULED_OK);
        assert(pinned(&x->f)==3);}
    detach(&x->f,3);assert(pt_paula_segment_sequence_service_command(x->producer,3,0)==PT_SCHEDULED_OK);
    if(!reader_first){assert(pinned(&x->f)==3);genuine_quiet(&x->f);
        assert(pt_paula_segment_sequence_service_reader(x->producer,0)==PT_SCHEDULED_OK);}
    s=sequence_status(x);assert(s.audit_events==4&&s.live_events==4&&s.published_mask==15&&s.detached_mask==15&&s.reader_retired);
    assert(!pinned(&x->f)&&x->f.model.effects==4&&x->f.model.controls==2&&x->f.model.stops==1);
    assert(s.peak_control_bytes<=x->config.control_budget&&s.boundary==4800);
    sequence_finish(x);
}
static void sequence_positive(unsigned reader_first)
{
    struct sequence_fixture x;unsigned i;sequence_init(&x,0);sequence_open(&x);
    for(i=0;i<4;++i){assert(sequence_until(&x,PT_PAULA_SEQUENCE_READY)==PT_PAULA_SEQUENCE_READY);
        sequence_activate(&x,i,i<3);
        if(i==0)assert(x.f.model.command[0].plan.initial.frame_offset==256&&x.f.model.command[0].plan.initial.frame_length==512);
        if(i==1||i==2)assert(!x.f.model.command[i].plan.write_initial&&x.f.model.command[i].plan.phase_q32);}
    sequence_terminal(&x,reader_first);++sequence_cases;
}
static void sequence_pressure(unsigned which)
{
    struct sequence_fixture x;unsigned i;sequence_init(&x,0);sequence_open(&x);
    for(i=0;i<4;++i){struct pt_paula_segment_sequence_status a,b;
        assert(sequence_until(&x,PT_PAULA_SEQUENCE_READY)==PT_PAULA_SEQUENCE_READY);
        if(i==which){a=sequence_status(&x);x.provider.refuse=1;
            assert(pt_paula_segment_sequence_publish_next(x.producer,x.revision)==PT_SCHEDULED_PENDING);
            b=sequence_status(&x);assert(a.boundary==b.boundary&&a.frame==b.frame&&a.ticket[i]==b.ticket[i]&&
                a.phase==b.phase&&a.live_events==b.live_events&&x.f.model.count==i&&pinned(&x.f)==3);
            assert(pt_paula_segment_sequence_step(x.producer,x.revision,256)==PT_PAULA_SEQUENCE_READY);
            b=sequence_status(&x);assert(b.ticket[i]==a.ticket[i]&&b.boundary==a.boundary);}
        sequence_activate(&x,i,i<3);}
    assert(x.provider.attempts==5);sequence_terminal(&x,which&1U);++sequence_cases;
}
static void sequence_wait_active(void)
{
    struct sequence_fixture x;unsigned i;sequence_init(&x,0);sequence_open(&x);
    assert(sequence_until(&x,PT_PAULA_SEQUENCE_READY)==PT_PAULA_SEQUENCE_READY);
    assert(pt_paula_segment_sequence_publish_next(x.producer,x.revision)==PT_SCHEDULED_OK);
    fire(&x.f,0);
    assert(sequence_until(&x,PT_PAULA_SEQUENCE_WAIT_ACTIVE)==PT_PAULA_SEQUENCE_WAIT_ACTIVE);
    assert(sequence_status(&x).boundary==1920&&x.f.model.count==1&&pinned(&x.f)==3);
    assert(pt_paula_segment_sequence_service_command(x.producer,0,0)==PT_SCHEDULED_PENDING);
    detach(&x.f,0);assert(pt_paula_segment_sequence_service_command(x.producer,0,0)==PT_SCHEDULED_OK);
    for(i=1;i<4;++i){assert(sequence_until(&x,PT_PAULA_SEQUENCE_READY)==PT_PAULA_SEQUENCE_READY);sequence_activate(&x,i,i<3);}
    sequence_terminal(&x,0);++sequence_cases;
}
static void sequence_cancel_preparation(enum pt_paula_segment_sequence_phase target)
{
    struct sequence_fixture x;unsigned n;sequence_init(&x,0);sequence_open(&x);
    for(n=0;n<4000&&sequence_status(&x).phase!=target;++n){
        enum pt_paula_segment_sequence_result r=pt_paula_segment_sequence_step(x.producer,x.revision,127);
        assert(r==PT_PAULA_SEQUENCE_PENDING||r==PT_PAULA_SEQUENCE_READY);}
    assert(n<4000&&x.f.model.count==0);
    assert(pt_paula_segment_sequence_cancel(x.producer)==PT_PAULA_SEQUENCE_FAILED);
    assert(!x.f.model.count&&!x.f.model.effects);sequence_finish(&x);++sequence_cases;
}
static void sequence_cancel_live(void)
{
    struct sequence_fixture x;sequence_init(&x,0);sequence_open(&x);
    assert(sequence_until(&x,PT_PAULA_SEQUENCE_READY)==PT_PAULA_SEQUENCE_READY);sequence_activate(&x,0,0);
    assert(pt_paula_segment_sequence_cancel(x.producer)==PT_PAULA_SEQUENCE_FAILED);
    assert(!pt_paula_segment_sequence_close(&x.producer)&&pinned(&x.f)==3&&!x.f.model.stops);
    genuine_quiet(&x.f);assert(pt_paula_segment_sequence_service_reader(x.producer,1)==PT_SCHEDULED_OK&&pinned(&x.f)==3);
    detach(&x.f,0);assert(pt_paula_segment_sequence_service_command(x.producer,0,1)==PT_SCHEDULED_OK);
    assert(!pinned(&x.f)&&x.f.model.count==1&&!x.f.model.stops);sequence_finish(&x);++sequence_cases;
}
static void sequence_shape_refusal(unsigned variant)
{
    struct sequence_fixture x;sequence_init(&x,variant);sequence_open(&x);
    assert(sequence_until(&x,PT_PAULA_SEQUENCE_READY)==PT_PAULA_SEQUENCE_FAILED);
    assert(!x.f.model.count&&!x.f.model.effects&&!pinned(&x.f));
    assert(pt_paula_segment_sequence_cancel(x.producer)==PT_PAULA_SEQUENCE_FAILED);sequence_finish(&x);++sequence_cases;
}
static void sequence_input_refusal(unsigned variant)
{
    struct sequence_fixture x;unsigned calls;unsigned char *before;
    sequence_init(&x,0);before=malloc(x.workspace_bytes);assert(before);memset(x.workspace,0x73,x.workspace_bytes);
    memcpy(before,x.workspace,x.workspace_bytes);calls=x.f.fast.calls;
    if(!variant)++x.config.readers.frame[1];else if(variant==1)x.config.origin=UINT64_MAX;else x.config.render.tracks=3;
    assert(pt_paula_segment_sequence_begin(&x.f.allocator,&x.f.bridge,&x.config,&x.f.backend,x.revision,
        x.workspace,x.workspace_bytes,&x.producer)==PT_PAULA_SEQUENCE_INVALID);
    assert(!x.producer&&calls==x.f.fast.calls&&!memcmp(before,x.workspace,x.workspace_bytes)&&!x.f.model.count);free(before);
    release(&x.f.fast,x.workspace);close_fixture(&x.f);++sequence_cases;
}
static void sequence_stale(unsigned live)
{
    struct sequence_fixture x;sequence_init(&x,0);sequence_open(&x);
    if(live){assert(sequence_until(&x,PT_PAULA_SEQUENCE_READY)==PT_PAULA_SEQUENCE_READY);sequence_activate(&x,0,1);}
    assert(pt_paula_segment_sequence_step(x.producer,x.revision+1U,256)==PT_PAULA_SEQUENCE_STALE);
    assert(pt_paula_segment_sequence_cancel(x.producer)==PT_PAULA_SEQUENCE_FAILED);
    if(live){assert(!pt_paula_segment_sequence_close(&x.producer)&&pinned(&x.f)==3);
        genuine_quiet(&x.f);assert(pt_paula_segment_sequence_service_reader(x.producer,1)==PT_SCHEDULED_OK);}
    sequence_finish(&x);++sequence_cases;
}
static void sequence_work_refusal(void)
{
    struct sequence_fixture x;struct pt_paula_segment_sequence_status a,b;sequence_init(&x,0);sequence_open(&x);
    assert(pt_paula_segment_sequence_get(x.producer,&a)==PT_PAULA_SEQUENCE_PENDING);
    assert(pt_paula_segment_sequence_step(x.producer,x.revision,0)==PT_PAULA_SEQUENCE_INVALID);
    assert(pt_paula_segment_sequence_step(x.producer,x.revision,257)==PT_PAULA_SEQUENCE_INVALID);
    assert(pt_paula_segment_sequence_get(x.producer,&b)==PT_PAULA_SEQUENCE_PENDING);
    assert(!memcmp(&a,&b,sizeof(a))&&!x.f.model.count);
    pt_paula_segment_sequence_cancel(x.producer);sequence_finish(&x);++sequence_cases;
}
static void sequence_late(void)
{
    struct sequence_fixture x;struct pt_elapsed_clock clock;uint64_t deadline;sequence_init(&x,0);sequence_open(&x);
    assert(sequence_until(&x,PT_PAULA_SEQUENCE_READY)==PT_PAULA_SEQUENCE_READY);
    assert(pt_elapsed_clock_init(&clock,x.config.readers.grid.frequency,RATE,x.config.readers.grid.epoch,0)==PT_ELAPSED_OK);
    assert(pt_elapsed_clock_deadline(&clock,960,&deadline)==PT_ELAPSED_OK);x.f.model.now=deadline;
    assert(pt_paula_segment_sequence_publish_next(x.producer,x.revision)==PT_SCHEDULED_LATE);
    {struct pt_paula_segment_sequence_status s=sequence_status(&x);
        assert(s.failed&&s.phase==PT_PAULA_SEQUENCE_ERROR&&s.result==PT_PAULA_SEQUENCE_FAILED&&
            s.scheduled_result==PT_SCHEDULED_LATE&&!s.published_mask&&!s.live_events&&s.boundary==960&&s.ticket[0]);}
    assert(!x.provider.attempts&&!x.f.model.count&&!x.f.model.effects&&pinned(&x.f)==3);
    assert(pt_paula_segment_sequence_cancel(x.producer)==PT_PAULA_SEQUENCE_FAILED&&!pinned(&x.f));
    sequence_finish(&x);++sequence_cases;
}
/* V2 additions use genuine cache acquisitions and actual typed argument
 * objects. No version/cache entry or core ownership field is fabricated. */
struct allocation_probe {
    struct memory *memory;void *alias;struct pt_paula_segment_sequence **owner;
    unsigned attempts,alias_returns,releases,reenter_release,nested;
};
static void *probe_allocate(void *v,size_t n)
{struct allocation_probe *p=v;++p->attempts;
 if(p->alias){++p->alias_returns;return p->alias;}return allocate(p->memory,n);}
static void probe_release(void *v,void *address)
{struct allocation_probe *p=v;++p->releases;
 if(p->reenter_release){p->reenter_release=0;assert(p->owner&&*p->owner);
    p->nested=(unsigned)pt_paula_segment_sequence_close(p->owner);}
 /* A borrowed-alias refusal must never reach this release callback. */
 assert(address!=p->alias);release(p->memory,address);}
static void sequence_cache_alias(unsigned output_alias)
{
    struct sequence_fixture x;struct fixture *f=&x.f;struct pt_cache_lease lease;
    const uint8_t *cache=NULL;size_t bytes=0;int32_t *original;unsigned i,j,calls;
    unsigned char *saved;struct pt_paula_segment_sequence *owner=NULL;void *workspace;
    struct pt_paula_segment_sequence **slot;size_t capacity;
    memset(&x,0,sizeof(x));x.revision=17;f->allocator=(struct pt_allocator){&f->fast,allocate,release};
    pt_document_init(&f->document,&f->allocator);assert(pt_document_new(&f->document,4,SIZE_MAX)==PT_PROJECT_OK);
    f->document.project.speed=1;f->document.project.bpm=125;f->document.project.channels.track[0].pan=0;
    original=allocate(&f->fast,131070U*sizeof(*original));assert(original);
    memset(original,0,131070U*sizeof(*original));
    for(j=0;j<3;++j){memset(f->original[j],0,sizeof(f->original[j]));
        f->document.project.samples[j].pcm=(struct pt_pcm){j?f->original[j]:original,j?32U:131070U,j?32U:131070U,8000,1,8};}
    assert(pt_project_validate(&f->document.project,NULL)==PT_PROJECT_OK);
    pt_sampler_init(&f->sampler,&f->allocator,2*1024*1024);
    assert(pt_pattern_history_init(&f->history,&f->document.project,f->command,4,f->changes,4)==PT_EDIT_OK);
    for(i=0;i<3;++i)assert(pt_sampler_pin(&f->sampler,&f->document.project,i,f->sampler.generation,f->pcm+i,f->pin+i)==PT_EDIT_OK);
    assert(pt_sampler_paula_bind(&f->bridge,&f->sampler,&f->document.project,&f->chip,chip_allocate,chip_release,256*1024));
    assert(pt_sampler_paula_acquire(&f->bridge,0,0,0,&lease)==PT_CACHE_LOAD);
    assert(pt_sampler_paula_location(&f->bridge,0,lease,&cache,&bytes)&&bytes==131070&&cache);
    x.provider.actual=&f->model;f->model.now=100;f->model.frequency=709379;f->model.raw=1;f->model.adopt=1;
    for(i=0;i<3;++i)f->model.expected[i]=f->pin[i];
    f->backend=(struct pt_paula_segment_backend){{&x.provider,sizeof(x.provider),{7,4,1},PT_READERS_VERSION,PT_READERS_REQUIRED,1,
        sequence_clock,NULL,sequence_command,sequence_cancel_command,sequence_reader,sequence_cancel_reader},
        PT_PAULA_SEGMENT_BACKEND_VERSION,PT_PAULA_SEGMENT_BACKEND_REQUIRED,sequence_submit};
    x.config.render=(struct pt_render_options){12000,32,RATE,65536,1,0,0,24,0,0,0,0,0};
    x.config.readers=(struct pt_paula_segment_readers_config){0,3,{{0,f->pin[0]},{1,f->pin[1]},{2,f->pin[2]}},
        {100,7,709379,RATE},13,{960,1920,2880,3840},{3546895,124,65535},1024*1024};
    x.config.control_budget=2*1024*1024;capacity=pt_paula_segment_sequence_control_size();
    assert(capacity<=bytes&&!((uintptr_t)cache%pt_paula_segment_sequence_control_alignment()));
    saved=malloc(bytes);assert(saved);memcpy(saved,cache,bytes);
    workspace=output_alias?allocate(&f->fast,capacity):(void *)cache;assert(workspace);
    slot=output_alias?(struct pt_paula_segment_sequence **)(void *)cache:&owner;calls=f->fast.calls;
    assert(pt_paula_segment_sequence_begin(&f->allocator,&f->bridge,&x.config,&f->backend,x.revision,
        workspace,capacity,slot)==PT_PAULA_SEQUENCE_INVALID);
    assert(calls==f->fast.calls&&!owner&&!memcmp(saved,cache,bytes)&&pinned(f)==1&&!f->model.count);
    assert(pt_sampler_paula_location(&f->bridge,0,lease,&cache,&bytes)&&bytes==131070);
    assert(pt_sampler_paula_unpin(&f->bridge,lease)&&!pinned(f));free(saved);
    if(output_alias)release(&f->fast,workspace);
    /* Genuine sampler promotion copied this caller-owned source; the current
     * document/cache now name the independently owned version allocation. */
    assert(f->document.project.samples[0].pcm.data==f->pcm[0].data&&f->pcm[0].data!=original);
    release(&f->fast,original);close_fixture(f);++sequence_cases;
}
static void sequence_constructor_allocation_alias(unsigned which)
{
    struct sequence_fixture x;struct allocation_probe probe;struct pt_allocator *a;
    struct pt_sampler_paula *bridge;struct pt_paula_segment_sequence_config *config;
    struct pt_paula_segment_backend *backend;struct pt_paula_segment_sequence **out;
    void *box[5];unsigned char *saved[5];size_t bytes[5],minimum;unsigned i;
    sequence_init(&x,0);memset(&probe,0,sizeof(probe));probe.memory=&x.f.fast;
    minimum=pt_render_sequence_setup_control_size();
    bytes[0]=sizeof(*a);bytes[1]=sizeof(*bridge);bytes[2]=sizeof(*config);bytes[3]=sizeof(*backend);bytes[4]=sizeof(*out);
    for(i=0;i<5;++i){if(bytes[i]<minimum)bytes[i]=minimum;
        box[i]=allocate(&x.f.fast,bytes[i]);assert(box[i]);memset(box[i],0,bytes[i]);}
    a=box[0];bridge=box[1];config=box[2];backend=box[3];out=box[4];
    *a=(struct pt_allocator){&probe,probe_allocate,probe_release};*config=x.config;*backend=x.f.backend;*out=NULL;
    /* This bridge is genuinely bound at its final aligned address, not copied
     * from an owner. It acquires no cache until later genuine preparation. */
    assert(pt_sampler_paula_bind(bridge,&x.f.sampler,&x.f.document.project,&x.f.chip,chip_allocate,chip_release,2304));
    for(i=0;i<5;++i){saved[i]=malloc(bytes[i]);assert(saved[i]);memcpy(saved[i],box[i],bytes[i]);}
    probe.alias=box[which];
    assert(pt_paula_segment_sequence_begin(a,bridge,config,backend,x.revision,x.workspace,x.workspace_bytes,out)==PT_PAULA_SEQUENCE_FAILED);
    assert(probe.attempts==1&&probe.alias_returns==1&&!probe.releases&&!*out&&!x.f.model.count);
    for(i=0;i<5;++i)assert(!memcmp(saved[i],box[i],bytes[i]));
    assert(pt_sampler_paula_close(bridge));
    for(i=0;i<5;++i){free(saved[i]);release(&x.f.fast,box[i]);}
    release(&x.f.fast,x.workspace);close_fixture(&x.f);++sequence_cases;
}
static void sequence_release_reentry(void)
{
    struct sequence_fixture x;struct allocation_probe probe;struct pt_allocator allocator;
    struct pt_paula_segment_sequence *original;struct pt_paula_segment_sequence_status before,after;
    sequence_init(&x,0);memset(&probe,0,sizeof(probe));probe.memory=&x.f.fast;probe.owner=&x.producer;
    allocator=(struct pt_allocator){&probe,probe_allocate,probe_release};
    assert(pt_paula_segment_sequence_begin(&allocator,&x.f.bridge,&x.config,&x.f.backend,x.revision,
        x.workspace,x.workspace_bytes,&x.producer)==PT_PAULA_SEQUENCE_PENDING&&x.producer);
    before=sequence_status(&x);original=x.producer;probe.reenter_release=1;
    assert(!pt_paula_segment_sequence_close(&x.producer)&&x.producer==original&&!probe.nested&&probe.releases==1);
    after=sequence_status(&x);assert(after.failed&&after.result==PT_PAULA_SEQUENCE_FAILED&&after.phase==PT_PAULA_SEQUENCE_ERROR&&after.callback_faults==before.callback_faults+1U&&
        after.control_bytes==x.workspace_bytes&&before.control_bytes>after.control_bytes&&!x.f.model.count);
    /* Actual startup allocation was consumed once. A second explicit close has
     * no release callback to repeat and clears only the retained outer control. */
    assert(pt_paula_segment_sequence_close(&x.producer)&&!x.producer&&probe.releases==1);
    release(&x.f.fast,x.workspace);close_fixture(&x.f);++sequence_cases;
}
/* V5 refusals exercise genuine elapsed-clock and backend ownership contracts. */
static void sequence_bad_clock(void)
{
    struct sequence_fixture x;struct pt_paula_segment_sequence_status s;
    sequence_init(&x,0);sequence_open(&x);
    assert(sequence_until(&x,PT_PAULA_SEQUENCE_READY)==PT_PAULA_SEQUENCE_READY);
    x.f.model.frequency=x.config.readers.grid.frequency+1U;
    assert(pt_paula_segment_sequence_publish_next(x.producer,x.revision)==PT_SCHEDULED_CLOCK);
    s=sequence_status(&x);assert(s.failed&&s.phase==PT_PAULA_SEQUENCE_ERROR&&s.result==PT_PAULA_SEQUENCE_FAILED&&
        s.scheduled_result==PT_SCHEDULED_CLOCK&&!s.published_mask&&!s.live_events&&s.boundary==960&&s.ticket[0]);
    assert(!x.provider.attempts&&!x.f.model.count&&!x.f.model.effects&&!x.f.model.reader&&pinned(&x.f)==3);
    assert(pt_paula_segment_sequence_cancel(x.producer)==PT_PAULA_SEQUENCE_FAILED&&!pinned(&x.f));
    sequence_finish(&x);++sequence_cases;
}
static void sequence_publish_reentry(unsigned reader_first)
{
    struct sequence_fixture x;struct pt_paula_segment_sequence_status s;
    sequence_init(&x,0);sequence_open(&x);
    assert(sequence_until(&x,PT_PAULA_SEQUENCE_READY)==PT_PAULA_SEQUENCE_READY);
    x.provider.producer=x.producer;x.provider.reenter_submit=1;
    assert(pt_paula_segment_sequence_publish_next(x.producer,x.revision)==PT_SCHEDULED_BACKEND);
    s=sequence_status(&x);assert(s.failed&&s.phase==PT_PAULA_SEQUENCE_ERROR&&s.result==PT_PAULA_SEQUENCE_FAILED&&
        s.scheduled_result==PT_SCHEDULED_BACKEND&&s.published_mask==1&&!s.live_events&&s.callback_faults==1);
    assert(x.provider.attempts==1&&x.provider.nested==PT_PAULA_SEQUENCE_INVALID&&x.f.model.count==1&&
        !x.f.model.effects&&!x.f.model.controls&&!x.f.model.stops&&x.f.model.reader&&
        x.f.model.command[0].borrowed&&x.f.model.command[0].plan_borrowed&&pinned(&x.f)==3);
    assert(!pt_paula_segment_sequence_close(&x.producer)&&x.producer&&pinned(&x.f)==3);
    assert(pt_paula_segment_sequence_cancel(x.producer)==PT_PAULA_SEQUENCE_FAILED);
    assert(!pt_paula_segment_sequence_close(&x.producer)&&x.f.model.command[0].borrowed&&x.f.model.reader&&pinned(&x.f)==3);
    /* No fire/effect/adoption occurred. The independent backend supplies its
     * actual cancelled-before command receipt, then drops each borrowed domain
     * only through the genuine command and reader callbacks in both orders. */
    x.f.model.command[0].receipt.action[0].command=PT_READERS_CANCELLED_BEFORE;
    x.f.model.command[0].receipt.action[0].reader=PT_READERS_NONE;
    if(reader_first){genuine_quiet(&x.f);
        assert(pt_paula_segment_sequence_service_reader(x.producer,1)==PT_SCHEDULED_OK&&
            !x.f.model.reader&&x.f.model.command[0].borrowed&&pinned(&x.f)==3);
        assert(!pt_paula_segment_sequence_close(&x.producer)&&x.producer);}
    /* Preserve the never-issued command snapshot: independent reader quiet
     * does not turn its NONE+UNADOPTED receipt into a RETIRED command receipt.
     * The genuine command callback alone drops its borrowed references. */
    assert(x.f.model.command[0].receipt.action[0].command==PT_READERS_CANCELLED_BEFORE&&
        x.f.model.command[0].receipt.action[0].reader==PT_READERS_NONE&&
        x.f.model.command[0].receipt.action[0].adoption==PT_READERS_UNADOPTED&&
        !x.f.model.command[0].receipt.action[0].observed&&!x.f.model.command[0].receipt.action[0].issued);
    x.f.model.command[0].reply=PT_READERS_COMMAND_DETACHED;
    assert(pt_paula_segment_sequence_service_command(x.producer,0,1)==PT_SCHEDULED_OK&&
        !x.f.model.command[0].borrowed&&!x.f.model.command[0].plan_borrowed&&
        x.f.model.command[0].receipt.action[0].reader==PT_READERS_NONE&&
        x.f.model.command[0].receipt.action[0].adoption==PT_READERS_UNADOPTED);
    if(!reader_first){assert(x.f.model.reader&&pinned(&x.f)==3&&!pt_paula_segment_sequence_close(&x.producer));
        genuine_quiet(&x.f);assert(pt_paula_segment_sequence_service_reader(x.producer,1)==PT_SCHEDULED_OK);}
    s=sequence_status(&x);assert(s.failed&&s.cancelled&&s.detached_mask==1&&s.reader_retired&&
        !pinned(&x.f)&&!x.f.model.reader&&!x.f.model.effects&&!x.f.model.stops);
    storage(&x.f);sequence_finish(&x);++sequence_cases;
}
int pt_packet32_sequence35_dormant_main(void)
{
    unsigned i;sequence_positive(0);sequence_positive(1);
    for(i=0;i<4;++i)sequence_pressure(i);sequence_wait_active();
    sequence_cancel_preparation(PT_PAULA_SEQUENCE_SETUP);sequence_cancel_preparation(PT_PAULA_SEQUENCE_MEASURE);
    sequence_cancel_preparation(PT_PAULA_SEQUENCE_AUDIT);sequence_cancel_preparation(PT_PAULA_SEQUENCE_POOL_PREPARE);
    sequence_cancel_preparation(PT_PAULA_SEQUENCE_PUBLISH);sequence_cancel_live();
    for(i=1;i<=4;++i)sequence_shape_refusal(i);for(i=0;i<3;++i)sequence_input_refusal(i);
    sequence_stale(0);sequence_stale(1);sequence_work_refusal();sequence_late();assert(sequence_cases==24);
    sequence_cache_alias(0);sequence_cache_alias(1);
    for(i=0;i<5;++i)sequence_constructor_allocation_alias(i);sequence_release_reentry();assert(sequence_cases==32);
    sequence_bad_clock();sequence_publish_reentry(0);sequence_publish_reentry(1);assert(sequence_cases==35);
    puts("PAULA SEGMENT SEQUENCE HOST FIXTURE PASS:35 cases;genuine measured sequence and same-owner rewind;silent normal-row preroll;actual960-1920-2880-3840 plans;original3 current masters and selective Chip leases;positive original reader key;explicit pressure preserves original plan-ticket-deadline;independent command-reader retirement both orders;incremental cancellation-stale-shape-work-late refusers;genuine held-cache workspace-output refusers;five borrowed-argument allocation aliases refused before ownership;close release reentry retains owner without child resurrection;exact LATE-CLOCK refusals and submit reentry retained until independent proofs both orders;legacy producer unchanged;HOST_ONLY_NATIVE_OUTPUT_NOT_RUN");return 0;
}
