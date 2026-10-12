/* Immutable exact donor source is compiled only as dormant helper definitions;
 * its original97 main is never invoked by this new focused qualifier. */
#define main pt_paula97_dormant_fixture_main
#include "paula_segment_repeat_readers_test.c"
#undef main
#include "../src/core/paula_segment_boundary.h"
static unsigned boundary_cases;
static void boundary_init(struct fixture *f)
{
    unsigned i,j;memset(f,0,sizeof(*f));f->allocator=(struct pt_allocator){&f->fast,allocate,release};
    pt_document_init(&f->document,&f->allocator);assert(pt_document_new(&f->document,4,SIZE_MAX)==PT_PROJECT_OK);
    for(j=0;j<3;++j){for(i=0;i<32;++i)f->original[j][i]=(int32_t)i-16+(int32_t)j;
        f->document.project.samples[j].pcm=(struct pt_pcm){f->original[j],32,32,8000,1,8};}
    /* Complete immutable sample/event configuration precedes sampler pins and
     * bridge binding. Later calls only advance the renderer's private state. */
    for(i=0;i<3;++i){f->document.project.samples[i].volume=64;
        f->document.project.samples[i].loop=PT_LOOP_FORWARD;
        f->document.project.samples[i].loop_start=4;f->document.project.samples[i].loop_end=12;}
    f->document.project.events[0]=(struct pt_event){0};
    f->document.project.events[0].kind=PT_NOTE_PERIOD;f->document.project.events[0].pitch=428;f->document.project.events[0].instrument=1;
    f->document.project.events[4].instrument=2;f->document.project.events[4].effect=12;f->document.project.events[4].parameter=32;
    f->document.project.events[8].instrument=3;f->document.project.events[8].effect=12;f->document.project.events[8].parameter=16;
    f->document.project.events[12].kind=PT_NOTE_OFF;
    assert(pt_project_validate(&f->document.project,NULL)==PT_PROJECT_OK);
    pt_sampler_init(&f->sampler,&f->allocator,1024*1024);
    assert(pt_pattern_history_init(&f->history,&f->document.project,f->command,4,f->changes,4)==PT_EDIT_OK);
    for(i=0;i<3;++i)assert(pt_sampler_pin(&f->sampler,&f->document.project,i,f->sampler.generation,f->pcm+i,f->pin+i)==PT_EDIT_OK);
    assert(pt_sampler_paula_bind(&f->bridge,&f->sampler,&f->document.project,&f->chip,chip_allocate,chip_release,96));
    f->config=(struct pt_paula_segment_readers_config){0,3,{{0,f->pin[0]},{1,f->pin[1]},{2,f->pin[2]}},
        {100,7,709379,RATE},13,{960,1920,2880,3840},{3546895,124,65535},256*1024};
    f->model.now=100;f->model.frequency=709379;f->model.raw=1;f->model.adopt=1;
    for(i=0;i<3;++i)f->model.expected[i]=f->pin[i];
    f->backend=(struct pt_paula_segment_backend){{&f->model,sizeof(f->model),{7,4,1},PT_READERS_VERSION,PT_READERS_REQUIRED,1,
        clock_read,NULL,poll_command,cancel_command,poll_reader,cancel_reader},PT_PAULA_SEGMENT_BACKEND_VERSION,PT_PAULA_SEGMENT_BACKEND_REQUIRED,submit_plan};
}
struct boundary_renderer {
    struct pt_render_options options;struct pt_flow flow;struct pt_pitch pitch;
    struct pt_render_range ranges[16];struct pt_render_command_state state;
};
static void renderer_init(struct boundary_renderer *r,struct fixture *f)
{memset(r,0,sizeof(*r));r->options.rate=RATE;r->options.bits=24;r->options.tracks=1;r->options.gain_q16=65536;
 r->flow.project=&f->document.project;r->flow.fresh=1;pt_render_commands_init(&r->state);}
static struct pt_paula_segment_boundary_scope boundary_scope(struct fixture *f)
{struct pt_paula_segment_boundary_scope s;unsigned i;memset(&s,0,sizeof(s));s.track=0;s.dependency_count=3;
 for(i=0;i<3;++i)s.dependency[i]=&f->document.project.samples[i].pcm;
 memcpy(s.frame,f->config.frame,sizeof(s.frame));return s;}
static void renderer_plan(struct boundary_renderer *r,unsigned index,struct pt_render_plan *p)
{
    if(index)assert(pt_voice_advance(r->state.voice,16,3)==PT_PCM_OK);
    r->flow.played_row=(uint16_t)index;r->pitch.channel[0].output=(uint16_t)(index==0?856:index==1?428:214);
    r->ranges[0].trigger_start=2;r->ranges[0].trigger_length=16;
    assert(pt_render_commands_plan(r->flow.project,&r->options,&r->flow,&r->pitch,r->ranges,index?0:1,&r->state,p)==PT_RENDER_OK);
    assert(p->count==(index==3?1U:2U));assert(p->action[0].kind==(index==0?PT_RENDER_SEGMENT:index==3?PT_RENDER_STOP:PT_RENDER_REPEAT));
    assert(!p->action[0].gain[0]&&!p->action[0].gain[1]);
    if(index!=3){assert(p->action[1].kind==PT_RENDER_CONTROL&&p->action[1].gain[0]==(index==0?65536U:index==1?32768U:16384U));
        assert(p->action[1].voice.step!=p->action[0].voice.step);}
}
static void normalized(const struct pt_paula_segment_boundary_scope *s,unsigned i,const struct pt_render_plan *p,
    const struct pt_readers_key *key,const struct pt_readers_key *actual,struct pt_render_action *a)
{
    struct pt_render_action expected;struct pt_render_plan before;memcpy(&expected,p->action,sizeof(expected));memcpy(&before,p,sizeof(before));
    if(i!=3){expected.voice.step=p->action[1].voice.step;expected.gain[0]=p->action[1].gain[0];expected.gain[1]=p->action[1].gain[1];}
    assert(pt_paula_segment_boundary_admit(s,i,s->frame[i],p,key,actual,a)==PT_PAULA_BOUNDARY_OK);
    assert(!memcmp(a,&expected,sizeof(expected))&&!memcmp(p,&before,sizeof(before)));
}
static void boundary_positive(unsigned retirement_order)
{
    struct fixture f;struct boundary_renderer r;struct pt_paula_segment_boundary_scope s;struct pt_render_plan p;
    struct pt_render_action a;struct pt_readers_key key,actual;struct pt_readers_command_receipt cr;struct pt_readers_reader_receipt rr;unsigned i;
    boundary_init(&f);s=boundary_scope(&f);renderer_init(&r,&f);open_fixture(&f);prepare(&f);
    for(i=0;i<4;++i){renderer_plan(&r,i,&p);if(i){assert(pt_paula_segment_readers_key(f.pool,&actual)==PT_SCHEDULED_OK&&key_equal(&key,&actual));}
        normalized(&s,i,&p,i?&key:NULL,i?&actual:NULL,&a);
        if(!i)trigger(&f,&a,&key,1);
        else{enqueue(&f,i,&a,&key);assert(pt_paula_segment_readers_publish(f.pool,i)==PT_SCHEDULED_OK);fire(&f,i);
            if(i!=3){detach(&f,i);assert(pt_paula_segment_readers_poll_command(f.pool,i,0,&cr)==PT_SCHEDULED_OK);}}
        assert(f.model.command[i].plan.frame==s.frame[i]&&pinned(&f)==3);storage(&f);
        if(i!=3){assert(f.model.command[i].plan.period&&f.model.command[i].plan.volume==(i==0?64:i==1?32:16));
            assert(f.model.command[i].plan.phase_q32==p.action[0].voice.phase);}
    }
    assert(f.model.effects==4&&f.model.controls==2&&f.model.stops==1&&!pt_paula_segment_readers_close(f.pool));
    assert(f.model.initial&&f.model.pending&&f.model.superseded&&pinned(&f)==3);
    if(!retirement_order){genuine_quiet(&f);assert(pt_paula_segment_readers_poll_reader(f.pool,0,&rr)==PT_SCHEDULED_OK&&pinned(&f)==3);}
    detach(&f,3);assert(pt_paula_segment_readers_poll_command(f.pool,3,0,&cr)==PT_SCHEDULED_OK);
    if(retirement_order){assert(pinned(&f)==3);genuine_quiet(&f);assert(pt_paula_segment_readers_poll_reader(f.pool,0,&rr)==PT_SCHEDULED_OK);}
    assert(!pinned(&f)&&pt_paula_segment_readers_stop(f.pool)==PT_SCHEDULED_OK&&pt_paula_segment_readers_close(f.pool));f.pool=NULL;
    close_fixture(&f);++boundary_cases;
}
static void refused(struct fixture *f,const struct pt_paula_segment_boundary_scope *s,unsigned i,uint64_t frame,
    const struct pt_render_plan *p,const struct pt_readers_key *o,const struct pt_readers_key *k,enum pt_paula_segment_boundary_result result)
{
    struct pt_render_action a,before;struct pt_render_plan saved;unsigned calls=f->fast.calls,count=f->model.count,effects=f->model.effects;
    memcpy(&saved,p,sizeof(saved));memset(&a,0x5a,sizeof(a));memcpy(&before,&a,sizeof(before));
    assert(pt_paula_segment_boundary_admit(s,i,frame,p,o,k,&a)==result);
    assert(!memcmp(&a,&before,sizeof(a))&&!memcmp(p,&saved,sizeof(saved))&&calls==f->fast.calls&&count==f->model.count&&effects==f->model.effects&&pinned(f)==3);
    storage(f);++boundary_cases;
}
static void boundary_refusers(void)
{
    struct fixture f;struct boundary_renderer r;struct pt_paula_segment_boundary_scope scope,s;struct pt_render_plan segment_plan,repeat_plan,p;
    struct pt_render_action a;struct pt_readers_key original,actual,k;unsigned mode,index;uint64_t frame;
    boundary_init(&f);scope=boundary_scope(&f);renderer_init(&r,&f);open_fixture(&f);prepare(&f);
    renderer_plan(&r,0,&segment_plan);normalized(&scope,0,&segment_plan,NULL,NULL,&a);trigger(&f,&a,&original,1);
    assert(pt_paula_segment_readers_key(f.pool,&actual)==PT_SCHEDULED_OK&&key_equal(&original,&actual));renderer_plan(&r,1,&repeat_plan);
    for(mode=0;mode<12;++mode){enum pt_paula_segment_boundary_result expected=PT_PAULA_BOUNDARY_INVALID;
        const struct pt_readers_key *o=NULL,*q=NULL;s=scope;p=segment_plan;index=0;frame=s.frame[0];
        if(mode==0)s.dependency_count=0;else if(mode==1)s.dependency_count=4;else if(mode==2)s.track=16;
        else if(mode==3)s.dependency[1]=s.dependency[0];else if(mode==4)s.dependency[1]=NULL;
        else if(mode==5)s.dependency[1]=(const struct pt_pcm *)((const unsigned char *)s.dependency[1]+1);
        else if(mode==6)s.frame[1]=s.frame[0];else if(mode==7)s.frame[2]=UINT64_MAX;
        else if(mode==8){++frame;expected=PT_PAULA_BOUNDARY_STALE;}
        else if(mode==9)index=4;else if(mode==10){o=&original;q=&actual;}
        else{index=1;frame=s.frame[1];p=repeat_plan;}
        refused(&f,&s,index,frame,&p,o,q,expected);
    }
    for(mode=0;mode<12;++mode){p=segment_plan;
        if(mode==0)p.count=0;else if(mode==1){p.action[2]=p.action[1];p.count=3;}
        else if(mode==2)p.count=PT_RENDER_ACTIONS+1;else if(mode==3){struct pt_render_action swap=p.action[0];p.action[0]=p.action[1];p.action[1]=swap;}
        else if(mode==4)p.action[0].kind=PT_RENDER_TRIGGER;else if(mode==5)p.action[0].kind=PT_RENDER_STOP;
        else if(mode==6)p.action[1].kind=PT_RENDER_REPEAT;else if(mode==7)p.action[0].channel=1;
        else if(mode==8)p.action[1].channel=1;else if(mode==9)p.action[0].gain[0]=1024;
        else if(mode==10)p.action[1].voice.step=0;else p.action[1].gain[0]=65537;
        refused(&f,&scope,0,scope.frame[0],&p,NULL,NULL,PT_PAULA_BOUNDARY_UNSUPPORTED);
    }
    for(mode=0;mode<13;++mode){struct pt_voice *v;p=segment_plan;v=&p.action[1].voice;
        if(mode==0)v->pcm=scope.dependency[1];else if(mode==1)v->repeat_pcm=scope.dependency[1];
        else if(mode==2)++v->phase;else if(mode==3)++v->cycle;else if(mode==4)++v->start;
        else if(mode==5)++v->end;else if(mode==6)++v->loop_start;else if(mode==7)++v->loop_end;
        else if(mode==8)v->loop=PT_VOICE_ONCE;else if(mode==9)v->looped=1;else if(mode==10)v->linear=1;
        else if(mode==11)v->active=0;else v->segment=0;
        refused(&f,&scope,0,scope.frame[0],&p,NULL,NULL,PT_PAULA_BOUNDARY_UNSUPPORTED);
    }
    for(mode=0;mode<8;++mode){k=actual;
        if(mode==0)k.queue=NULL;else if(mode==1)++k.session;else if(mode==2)++k.generation;else if(mode==3)++k.trigger;
        else if(mode==4)++k.owner;else if(mode==5)++k.serial;else if(mode==6)++k.action;else ++k.slot;
        refused(&f,&scope,1,scope.frame[1],&repeat_plan,&original,&k,PT_PAULA_BOUNDARY_STALE);
    }
    {
        struct pt_readers_reader_receipt rr,before;
        const struct pt_readers_domain *domain=f.model.reader;
        const void *initial=f.model.initial,*pending=f.model.pending,*context;
        size_t context_bytes;
        /* trigger(...,1) already obtained genuine exact command detachment.
         * Closing admission cannot retire its still-active independent reader. */
        assert(domain&&initial&&pending&&!f.model.superseded);
        assert(f.model.count==1&&f.model.effects==1&&!f.model.controls&&!f.model.stops);
        assert(!f.model.command[0].borrowed&&!f.model.command[0].plan_borrowed);
        context=domain->binding.context;context_bytes=domain->binding.context_bytes;
        assert(pinned(&f)==3);
        assert(pt_paula_segment_readers_stop(f.pool)==PT_SCHEDULED_PENDING);
        assert(pinned(&f)==3&&!pt_paula_segment_readers_close(f.pool));
        assert(f.model.reader==domain&&f.model.initial==initial&&f.model.pending==pending&&!f.model.superseded);
        assert(f.model.count==1&&f.model.effects==1&&!f.model.controls&&!f.model.stops);storage(&f);
        memset(&rr,0x61,sizeof(rr));memcpy(&before,&rr,sizeof(before));
        assert(pt_paula_segment_readers_poll_reader(f.pool,0,&rr)==PT_SCHEDULED_PENDING);
        assert(!memcmp(&rr,&before,sizeof(rr))&&pinned(&f)==3);
        assert(f.model.reader==domain&&f.model.initial==initial&&f.model.pending==pending&&!f.model.superseded);storage(&f);
        genuine_quiet(&f);
        assert(!f.model.initial&&!f.model.pending&&!f.model.superseded);
        assert(f.model.reader==domain&&pinned(&f)==3);
        assert(pt_paula_segment_readers_poll_reader(f.pool,0,&rr)==PT_SCHEDULED_OK);
        assert(rr.state==PT_READERS_RETIRED&&rr.adoption==PT_READERS_ADOPTED&&key_equal(&rr.key,&original));
        assert(rr.reference==domain&&rr.context==context&&rr.context_bytes==context_bytes);
        assert(!f.model.reader&&!pinned(&f));storage(&f);
        assert(pt_paula_segment_readers_stop(f.pool)==PT_SCHEDULED_OK);
        assert(pt_paula_segment_readers_close(f.pool));f.pool=NULL;close_fixture(&f);
    }
}
static void boundary_aliases(void)
{
    struct fixture f;struct boundary_renderer r;struct pt_paula_segment_boundary_scope s;struct pt_render_plan p;
    union {long double alignment;unsigned char bytes[sizeof(struct pt_render_plan)+sizeof(struct pt_render_action)+32];} box;
    unsigned char before[sizeof(box)];struct pt_render_action out;struct pt_readers_key key;unsigned mode;
    boundary_init(&f);s=boundary_scope(&f);renderer_init(&r,&f);renderer_plan(&r,0,&p);memset(&key,0,sizeof(key));
    for(mode=0;mode<6;++mode){const struct pt_paula_segment_boundary_scope *scope=&s;const struct pt_render_plan *plan=&p;
        const struct pt_readers_key *o=NULL,*actual=NULL;struct pt_render_action *a=&out;unsigned calls=f.fast.calls;
        memset(&box,0x39,sizeof(box));memset(&out,0x27,sizeof(out));
        if(mode==0){memcpy(box.bytes,&s,sizeof(s));scope=(const struct pt_paula_segment_boundary_scope *)box.bytes;a=(struct pt_render_action *)box.bytes;}
        else if(mode==1){memcpy(box.bytes,&p,sizeof(p));plan=(const struct pt_render_plan *)box.bytes;a=(struct pt_render_action *)box.bytes;}
        else if(mode==2){memcpy(box.bytes,&key,sizeof(key));o=&key;actual=(const struct pt_readers_key *)box.bytes;a=(struct pt_render_action *)box.bytes;}
        else if(mode==3)a=(struct pt_render_action *)(box.bytes+1);
        else if(mode==4){memcpy(box.bytes+1,&p,sizeof(p));plan=(const struct pt_render_plan *)(box.bytes+1);}
        else{memcpy(box.bytes+1,&s,sizeof(s));scope=(const struct pt_paula_segment_boundary_scope *)(box.bytes+1);}
        memcpy(before,&box,sizeof(box));assert(pt_paula_segment_boundary_admit(scope,0,s.frame[0],plan,o,actual,a)==PT_PAULA_BOUNDARY_ALIAS);
        assert(!memcmp(before,&box,sizeof(box))&&calls==f.fast.calls);++boundary_cases;
    }
    close_fixture(&f);
}
/* These storage guards use genuine owned append/current-pin lifetimes. No
 * enlarged version observation, fabricated allocation or source token is used.
 * Their plans use the genuine voice API; the original53 renderer cases above
 * remain unchanged and are counted separately before these additions. */
struct boundary_master {
    struct memory fast;struct pt_allocator allocator;struct pt_document document;
    struct pt_sampler sampler;struct pt_pattern_history history;
    struct pt_pattern_command commands[4];struct pt_event_change changes[4];
    struct pt_pcm pcm;struct pt_sample_version *pin;unsigned sample;int32_t before[256];
};
static void master_init(struct boundary_master *m,unsigned bits,unsigned channels)
{
    struct pt_pcm owned;unsigned i;memset(m,0,sizeof(*m));m->allocator=(struct pt_allocator){&m->fast,allocate,release};
    pt_document_init(&m->document,&m->allocator);assert(pt_document_new(&m->document,4,SIZE_MAX)==PT_PROJECT_OK);
    pt_sampler_init(&m->sampler,&m->allocator,1024*1024);
    assert(pt_pattern_history_init(&m->history,&m->document.project,m->commands,4,m->changes,4)==PT_EDIT_OK);
    owned=(struct pt_pcm){allocate(&m->fast,256U*sizeof(int32_t)),256,32,8000,(uint8_t)channels,(uint8_t)bits};assert(owned.data);
    for(i=0;i<256;++i)owned.data[i]=m->before[i]=(int32_t)(i%32)-16;
    m->sample=m->document.project.sample_count;
    assert(pt_sampler_append_owned(&m->sampler,&m->document.project,&m->history,&owned,&m->allocator,"guard")==PT_EDIT_OK&&!owned.data&&!owned.capacity);
    assert(pt_sampler_pin(&m->sampler,&m->document.project,m->sample,m->sampler.generation,&m->pcm,&m->pin)==PT_EDIT_OK);
    assert(m->pcm.capacity==256&&m->pcm.frames==32&&m->pcm.channels==channels&&m->pcm.bits==bits);
    assert(m->document.project.samples[m->sample].pcm.capacity==256);
}
static void master_contents(struct boundary_master *m)
{assert(!memcmp(m->pcm.data,m->before,sizeof(m->before)));}
static void master_close(struct boundary_master *m)
{
    master_contents(m);pt_pattern_history_release(&m->history);pt_sampler_release(&m->sampler);
    /* The actual retained pin independently keeps active samples AND real tail. */
    master_contents(m);pt_sampler_unpin(m->pin);pt_document_release(&m->document);assert(!m->fast.live);
}
static void master_plan(struct boundary_master *m,struct pt_paula_segment_boundary_scope *scope,struct pt_render_plan *p)
{
    memset(scope,0,sizeof(*scope));scope->dependency_count=1;scope->dependency[0]=&m->document.project.samples[m->sample].pcm;
    scope->frame[0]=960;scope->frame[1]=1920;scope->frame[2]=2880;scope->frame[3]=3840;
    memset(p,0,sizeof(*p));p->count=2;p->action[0].kind=PT_RENDER_SEGMENT;
    assert(pt_voice_init_segment(&p->action[0].voice,scope->dependency[0],0,16,4,12,STEP,0)==PT_PCM_OK);
    p->action[1]=p->action[0];p->action[1].kind=PT_RENDER_CONTROL;p->action[1].gain[0]=65536;p->action[1].voice.step*=2;
}
static void master_alias(struct boundary_master *m,const struct pt_paula_segment_boundary_scope *s,const struct pt_render_plan *p,unsigned element)
{
    struct pt_render_action *out=(struct pt_render_action *)(m->pcm.data+element);unsigned calls=m->fast.calls;
    assert(element<m->pcm.capacity&&sizeof(*out)<=(m->pcm.capacity-element)*sizeof(int32_t));
    assert(!((uintptr_t)out%offsetof(struct {char c;struct pt_render_action a;},a)));
    assert(pt_paula_segment_boundary_admit(s,0,960,p,NULL,NULL,out)==PT_PAULA_BOUNDARY_ALIAS);
    assert(calls==m->fast.calls);master_contents(m);++boundary_cases;
}
static void supported_master_aliases(unsigned bits)
{
    struct boundary_master m;struct pt_paula_segment_boundary_scope s;struct pt_render_plan p;struct pt_render_action out;
    master_init(&m,bits,1);master_plan(&m,&s,&p);
    assert(pt_paula_segment_boundary_admit(&s,0,960,&p,NULL,NULL,&out)==PT_PAULA_BOUNDARY_OK);
    assert(sizeof(out)<=m.pcm.frames*sizeof(int32_t));master_alias(&m,&s,&p,0);
    assert(128U>=m.pcm.frames*m.pcm.channels);master_alias(&m,&s,&p,128);master_close(&m);
}
static void stereo_element_capacity(void)
{
    struct boundary_master m;struct pt_paula_segment_boundary_scope s;struct pt_render_plan p;
    master_init(&m,24,2);master_plan(&m,&s,&p);assert(m.pcm.capacity==256&&m.pcm.frames*m.pcm.channels==64);
    /* Storage-only guard: stereo is not finite Paula geometry capability. The
     * actual capacity is256 elements, not512; unused element128 is protected. */
    master_alias(&m,&s,&p,128);master_close(&m);
}
static void master_metadata_refusers(void)
{
    struct boundary_master m;struct pt_paula_segment_boundary_scope scope,s;struct pt_render_plan plan,p;
    struct pt_pcm bad,metadata_before;struct pt_render_action out,before;unsigned mode,calls;
    master_init(&m,24,1);master_plan(&m,&scope,&plan);
    for(mode=0;mode<4;++mode){s=scope;p=plan;bad=m.pcm;s.dependency[0]=&bad;
        p.action[0].voice.pcm=p.action[1].voice.pcm=&bad;
        if(mode==0)bad.capacity=SIZE_MAX/sizeof(int32_t)+1U;
        else if(mode==1)bad.data=NULL;
        else if(mode==2)bad.data=(int32_t *)((unsigned char *)m.pcm.data+1);
        else bad.data=(int32_t *)(UINTPTR_MAX-UINTPTR_MAX%offsetof(struct {char c;int32_t value;},value));
        memcpy(&metadata_before,&bad,sizeof(metadata_before));memset(&out,0x4a,sizeof(out));memcpy(&before,&out,sizeof(before));calls=m.fast.calls;
        assert(pt_paula_segment_boundary_admit(&s,0,960,&p,NULL,NULL,&out)==PT_PAULA_BOUNDARY_INVALID);
        assert(!memcmp(&out,&before,sizeof(out))&&!memcmp(&bad,&metadata_before,sizeof(bad))&&calls==m.fast.calls);
        master_contents(&m);++boundary_cases;
    }
    master_close(&m);
}
static void unused_dependency_and_descriptor_aliases(void)
{
    struct fixture f;struct boundary_renderer r;struct pt_paula_segment_boundary_scope s;struct pt_render_plan p;
    struct pt_sample samples[4];unsigned i,calls;
    boundary_init(&f);s=boundary_scope(&f);renderer_init(&r,&f);renderer_plan(&r,0,&p);calls=f.fast.calls;
    for(i=1;i<3;++i){struct pt_render_action *out=(struct pt_render_action *)f.pcm[i].data;
        assert(sizeof(*out)<=f.pcm[i].capacity*sizeof(int32_t));assert(!((uintptr_t)out%offsetof(struct {char c;struct pt_render_action a;},a)));
        assert(p.action[0].voice.pcm!=s.dependency[i]&&!p.action[0].voice.repeat_pcm);
        assert(pt_paula_segment_boundary_admit(&s,0,960,&p,NULL,NULL,out)==PT_PAULA_BOUNDARY_ALIAS&&calls==f.fast.calls);
        storage(&f);++boundary_cases;}
    memcpy(samples,f.document.project.samples,sizeof(samples));
    assert(sizeof(struct pt_render_action)<=(f.document.project.sample_count-2U)*sizeof(struct pt_sample)-offsetof(struct pt_sample,pcm));
    assert(pt_paula_segment_boundary_admit(&s,0,960,&p,NULL,NULL,(struct pt_render_action *)&f.document.project.samples[2].pcm)==PT_PAULA_BOUNDARY_ALIAS);
    assert(!memcmp(samples,f.document.project.samples,sizeof(samples))&&calls==f.fast.calls);storage(&f);++boundary_cases;
    close_fixture(&f);
}

/* Zero capacity carries no writable span. Pointer-bearing or nonempty
 * descriptors therefore refuse before publication. These local metadata
 * negatives never alter the genuine retained version or its full storage. */
static void zero_capacity_refusers(void)
{
    struct boundary_master m;struct pt_paula_segment_boundary_scope scope,s,saved_scope;
    struct pt_render_plan plan,p,saved_plan;struct pt_pcm bad,saved_bad;
    struct pt_render_action ordinary,before,*out;unsigned mode,calls;struct pt_sample_version *pin;
    master_init(&m,24,1);master_plan(&m,&scope,&plan);pin=m.pin;
    for(mode=0;mode<4;++mode){s=scope;p=plan;bad=m.pcm;bad.capacity=0;out=&ordinary;
        memset(&ordinary,0x6a,sizeof(ordinary));
        if(mode==1){bad.frames=0;out=(struct pt_render_action *)(m.pcm.data+128);}
        else if(mode==2)bad.data=NULL;
        else if(mode==3){bad.frames=0;bad.data=m.pcm.data+128;}
        else out=(struct pt_render_action *)m.pcm.data;
        if(mode==3){s.dependency_count=2;s.dependency[1]=&bad;}
        else{s.dependency[0]=&bad;p.action[0].voice.pcm=p.action[1].voice.pcm=&bad;}
        assert(!((uintptr_t)out%offsetof(struct {char c;struct pt_render_action action;},action)));
        if(mode<2)assert(sizeof(*out)<=(m.pcm.capacity-(mode?128U:0U))*sizeof(int32_t));
        memcpy(&before,out,sizeof(before));memcpy(&saved_bad,&bad,sizeof(saved_bad));memcpy(&saved_scope,&s,sizeof(saved_scope));memcpy(&saved_plan,&p,sizeof(saved_plan));calls=m.fast.calls;
        assert(pt_paula_segment_boundary_admit(&s,0,960,&p,NULL,NULL,out)==PT_PAULA_BOUNDARY_INVALID);
        assert(!memcmp(out,&before,sizeof(before))&&!memcmp(&bad,&saved_bad,sizeof(bad)));
        assert(!memcmp(&s,&saved_scope,sizeof(s))&&!memcmp(&p,&saved_plan,sizeof(p)));
        assert(m.fast.calls==calls&&m.pin==pin);master_contents(&m);++boundary_cases;
    }
    master_close(&m);
}
static void genuine_empty_unused_dependency(void)
{
    struct boundary_master m;struct pt_paula_segment_boundary_scope s,before_scope;
    struct pt_render_plan p,before_plan;struct pt_render_action out,expected;
    const struct pt_pcm *empty;struct pt_pcm before_empty;unsigned calls;struct pt_sample_version *pin;
    master_init(&m,24,1);master_plan(&m,&s,&p);assert(m.sample!=0);
    /* This is actual document-created unused storage metadata, never a
     * fabricated current version or a certificate for an empty voice. */
    empty=&m.document.project.samples[0].pcm;
    assert(!empty->data&&!empty->capacity&&!empty->frames);memcpy(&before_empty,empty,sizeof(before_empty));
    assert(p.action[0].voice.pcm!=empty&&!p.action[0].voice.repeat_pcm);
    s.dependency_count=2;s.dependency[1]=empty;memcpy(&before_scope,&s,sizeof(before_scope));memcpy(&before_plan,&p,sizeof(before_plan));
    memcpy(&expected,p.action,sizeof(expected));expected.voice.step=p.action[1].voice.step;
    expected.gain[0]=p.action[1].gain[0];expected.gain[1]=p.action[1].gain[1];
    memset(&out,0x6b,sizeof(out));calls=m.fast.calls;pin=m.pin;
    assert(pt_paula_segment_boundary_admit(&s,0,960,&p,NULL,NULL,&out)==PT_PAULA_BOUNDARY_OK);
    assert(!memcmp(&out,&expected,sizeof(out))&&!memcmp(empty,&before_empty,sizeof(before_empty)));
    assert(!memcmp(&s,&before_scope,sizeof(s))&&!memcmp(&p,&before_plan,sizeof(p)));
    assert(m.fast.calls==calls&&m.pin==pin);master_contents(&m);++boundary_cases;master_close(&m);
}
int main(void)
{boundary_positive(0);boundary_positive(1);boundary_refusers();boundary_aliases();assert(boundary_cases==53);
 supported_master_aliases(8);supported_master_aliases(16);supported_master_aliases(24);stereo_element_capacity();
 master_metadata_refusers();unused_dependency_and_descriptor_aliases();assert(boundary_cases==67);
 zero_capacity_refusers();genuine_empty_unused_dependency();assert(boundary_cases==72);
 puts("PAULA SEGMENT BOUNDARY HOST FIXTURE PASS:72 cases;genuine renderer primary-CONTROL plans;final step-gain normalization;all voice geometry and pending dependencies preserved;exact960-1920-2880-3840;full original-reader key comparisons;genuine3 master pins and selective Chip leases through both retirement orders;scope-ordered-plan-geometry-key-alias refusers;full int32 element capacities and genuine unused-tail protection;mono8-16-24 master and stereo-capacity aliases;metadata overflow-missing-misaligned-wrapped refusers;zero-capacity pointer-frame refusers;genuine empty unused dependency storage shape only;ordinary song and preflight rejection unchanged;HOST_ONLY_NATIVE_OUTPUT_NOT_RUN");return 0;}
