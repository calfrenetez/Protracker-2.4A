#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "../src/core/paula_segment_repeat_plan.h"
#include "../src/editor/sampler_paula.h"
#include "../src/editor/sampler_internal.h"

#define RATE 48000U
#define STEP (((uint64_t)8000<<32)/RATE)
static const struct pt_paula_render_caps caps={3546895,124,65535};
static unsigned cases;
struct memory {size_t live,calls,chip_bytes;};
static void *allocate(void *context,size_t bytes)
{struct memory *m=context;void *p=malloc(bytes);++m->calls;if(p)++m->live;return p;}
static void release(void *context,void *p)
{struct memory *m=context;if(p){assert(m->live);--m->live;free(p);}}
static void *chip_allocate(void *context,size_t bytes)
{struct memory *m=context;void *p=malloc(bytes);++m->calls;if(p){++m->live;m->chip_bytes+=bytes;}return p;}
static void chip_release(void *context,void *p,size_t bytes)
{struct memory *m=context;assert(p&&m->live&&m->chip_bytes>=bytes);--m->live;m->chip_bytes-=bytes;free(p);}
struct fixture {
    struct memory fast,chip;struct pt_allocator allocator;struct pt_document document;
    struct pt_sampler sampler;struct pt_sampler_paula owner;
    struct pt_pattern_history history;struct pt_pattern_command commands[4];struct pt_event_change changes[4];
    struct pt_sample_version *pin[2];struct pt_pcm pcm[2];struct pt_cache_lease lease[2];
    struct pt_paula_segment_source source[2];int32_t original[2][32];uint8_t cache[2][32];
};
static struct pt_paula_segment_source observation(struct fixture *f,unsigned sample,
    struct pt_pcm pcm,struct pt_sample_version *pin,struct pt_cache_lease lease)
{
    struct pt_paula_segment_source s;struct pt_sampler_storage_span spans[PT_SAMPLER_VERSION_SPANS];
    const uint8_t *data=NULL;size_t bytes=0;unsigned i,n=0;
    assert(pt_sampler_paula_location(&f->owner,0,lease,&data,&bytes));
    assert(pt_sampler_version_spans(pin,spans,PT_SAMPLER_VERSION_SPANS,&n)&&n);
    memset(&s,0,sizeof(s));s.descriptor=&f->document.project.samples[sample].pcm;s.master=pcm;s.version=pin;
    s.sampler_owner=&f->sampler;s.project_identity=&f->document.project;s.cache_owner=&f->owner;
    s.cache_version=f->owner.version;s.generation=f->sampler.generation;s.sample=sample;
    s.lease=lease;s.cache_data=data;s.cache_bytes=bytes;
    for(i=0;i<n;++i)s.guard[s.guard_count++]=(struct pt_paula_segment_span){spans[i].data,spans[i].bytes};
    s.guard[s.guard_count++]=(struct pt_paula_segment_span){&f->owner,sizeof(f->owner)};
    s.guard[s.guard_count++]=(struct pt_paula_segment_span){&f->sampler,sizeof(f->sampler)};
    s.guard[s.guard_count++]=(struct pt_paula_segment_span){&f->document.project,sizeof(f->document.project)};
    assert(s.guard_count<=PT_PAULA_SEGMENT_SOURCE_GUARDS);return s;
}
static void init(struct fixture *f,unsigned bits)
{
    unsigned i,j;memset(f,0,sizeof(*f));f->allocator=(struct pt_allocator){&f->fast,allocate,release};
    pt_document_init(&f->document,&f->allocator);assert(pt_document_new(&f->document,4,SIZE_MAX)==PT_PROJECT_OK);
    for(j=0;j<2;++j){for(i=0;i<32;++i)f->original[j][i]=((int32_t)i-16+j*3)*(1L<<(bits-8));
        f->document.project.samples[j].pcm=(struct pt_pcm){f->original[j],32,32,8000,1,(uint8_t)bits};}
    pt_sampler_init(&f->sampler,&f->allocator,1024*1024);
    assert(pt_pattern_history_init(&f->history,&f->document.project,f->commands,4,f->changes,4)==PT_EDIT_OK);
    for(j=0;j<2;++j)assert(pt_sampler_pin(&f->sampler,&f->document.project,j,f->sampler.generation,
        f->pcm+j,f->pin+j)==PT_EDIT_OK);
    assert(pt_sampler_paula_bind(&f->owner,&f->sampler,&f->document.project,&f->chip,chip_allocate,chip_release,256));
    for(j=0;j<2;++j){assert(pt_sampler_paula_acquire(&f->owner,0,j,0,f->lease+j)==PT_CACHE_LOAD);
        f->source[j]=observation(f,j,f->pcm[j],f->pin[j],f->lease[j]);
        assert(f->source[j].cache_bytes==32);memcpy(f->cache[j],f->source[j].cache_data,32);}
    assert(f->chip.chip_bytes==64);
}
static void check_storage(struct fixture *f)
{unsigned i;for(i=0;i<2;++i){assert(!memcmp(f->pcm[i].data,f->original[i],sizeof(f->original[i])));
 assert(!memcmp(f->source[i].cache_data,f->cache[i],32));}}
static void close_fixture(struct fixture *f)
{
    unsigned i;check_storage(f);for(i=0;i<2;++i)assert(pt_sampler_paula_unpin(&f->owner,f->lease[i]));
    assert(pt_sampler_paula_close(&f->owner)&&!f->chip.live&&!f->chip.chip_bytes);
    pt_pattern_history_release(&f->history);pt_sampler_release(&f->sampler);
    /* Genuine old pins survive document/history/current release, never planner ownership. */
    for(i=0;i<2;++i){assert(!memcmp(f->pcm[i].data,f->original[i],sizeof(f->original[i])));pt_sampler_unpin(f->pin[i]);}
    pt_document_release(&f->document);assert(!f->fast.live);
}
static struct pt_render_action segment_action(struct fixture *f,unsigned slot)
{
    struct pt_render_action a;memset(&a,0,sizeof(a));a.kind=PT_RENDER_SEGMENT;a.channel=0;
    assert(pt_voice_init_segment(&a.voice,f->source[0].descriptor,2,18,4,12,STEP,0)==PT_PCM_OK);
    a.gain[slot==0||slot==3?0:1]=65536;return a;
}
static void segments(unsigned bits)
{
    struct fixture f;struct pt_render_action a;struct pt_paula_segment_plan p,stop;unsigned slot;
    size_t fast_calls,chip_calls;init(&f,bits);fast_calls=f.fast.calls;chip_calls=f.chip.calls;
    for(slot=0;slot<4;++slot){a=segment_action(&f,slot);
        assert(pt_paula_segment_plan_build(&a,f.source,f.source,960,RATE,slot,&caps,&p)==PT_PAULA_SEGMENT_OK);
        assert(p.frame==960&&p.kind==PT_PAULA_SEGMENT_INITIAL&&p.source_count==1&&p.write_initial&&p.write_repeat);
        assert(p.initial.frame_offset==2&&p.initial.frame_length==16&&p.initial.byte_offset==2&&p.initial.byte_length==16&&p.initial.words==8);
        assert(p.repeat.frame_offset==4&&p.repeat.frame_length==8&&p.repeat.byte_offset==4&&p.repeat.byte_length==8&&p.repeat.words==4);
        assert(p.period==443&&p.volume==64&&p.slot==slot&&p.phase_q32==((uint64_t)2<<32));
        assert(pt_paula_segment_plan_identity_equal(&p,f.source,f.source));
    }
    a=segment_action(&f,0);assert(pt_voice_init_segment(&a.voice,f.source[0].descriptor,20,32,0,2,STEP,0)==PT_PCM_OK);
    assert(pt_paula_segment_plan_build(&a,f.source,f.source,1920,RATE,0,&caps,&p)==PT_PAULA_SEGMENT_OK);
    /* No reading/inferring a silent first word: these actual first bytes are nonzero. */
    assert(f.source[0].cache_data[0]&&p.repeat.words==1&&p.repeat.frame_offset==0);
    assert(pt_paula_segment_plan_stop(&p,3840,&stop)==PT_PAULA_SEGMENT_OK&&stop.source_count==1);
    assert(!stop.period&&!stop.volume&&!stop.write_initial&&!stop.write_repeat&&!stop.initial.words&&!stop.repeat.words);
    assert(pt_paula_segment_plan_identity_equal(&stop,f.source,f.source));
    assert(f.fast.calls==fast_calls&&f.chip.calls==chip_calls&&f.chip.chip_bytes==64);check_storage(&f);
    close_fixture(&f);++cases;
}
static void repeats(unsigned other,unsigned entered)
{
    struct fixture f;struct pt_render_action a;struct pt_paula_segment_plan p,stop;
    uint64_t phase;size_t fast_calls,chip_calls;init(&f,8);a=segment_action(&f,0);
    assert(pt_voice_advance(&a.voice,1,entered?100:3)==PT_PCM_OK);
    assert(!!a.voice.looped==!!entered);
    assert(pt_voice_set_repeat_source(&a.voice,f.source[other].descriptor,6,14)==PT_PCM_OK);
    a.kind=PT_RENDER_REPEAT;phase=a.voice.phase;assert(phase&UINT32_MAX);
    fast_calls=f.fast.calls;chip_calls=f.chip.calls;
    assert(pt_paula_segment_plan_build(&a,f.source,f.source+other,1920,RATE,0,&caps,&p)==PT_PAULA_SEGMENT_OK);
    assert(p.kind==PT_PAULA_SEGMENT_REPROGRAM&&!p.write_initial&&p.write_repeat&&p.phase_q32==phase);
    assert(p.source_count==(other?2U:1U)&&p.repeat.frame_offset==6&&p.repeat.words==4);
    assert(pt_paula_segment_plan_identity_equal(&p,f.source,f.source+other));
    assert(pt_paula_segment_plan_stop(&p,2880,&stop)==PT_PAULA_SEGMENT_OK&&stop.source_count==p.source_count);
    assert(pt_paula_segment_plan_stop(&stop,2880,&p)==PT_PAULA_SEGMENT_OK);
    /* Genuine software voice transition, independent of the descriptive planner. */
    assert(pt_voice_advance(&a.voice,1,256)==PT_PCM_OK&&a.voice.pcm==f.source[other].descriptor&&!a.voice.repeat_pcm);
    assert(f.fast.calls==fast_calls&&f.chip.calls==chip_calls&&f.chip.chip_bytes==64);check_storage(&f);
    close_fixture(&f);++cases;
}
static void refusal(const struct pt_render_action *a,const struct pt_paula_segment_source *c,
    const struct pt_paula_segment_source *n,uint64_t frame,unsigned rate,unsigned slot,
    const struct pt_paula_render_caps *cap,enum pt_paula_segment_result expected)
{
    struct pt_paula_segment_plan out,saved;memset(&out,0x5a,sizeof(out));memcpy(&saved,&out,sizeof(saved));
    assert(pt_paula_segment_plan_build(a,c,n,frame,rate,slot,cap,&out)==expected);
    assert(!memcmp(&out,&saved,sizeof(out)));++cases;
}
static void refusers(void)
{
    struct fixture f;struct pt_render_action base,a;struct pt_paula_segment_source s;
    struct pt_paula_render_caps cap;unsigned i;init(&f,8);base=segment_action(&f,0);
    for(i=0;i<12;++i){a=base;
        switch(i){case 0:a.kind=PT_RENDER_TRIGGER;break;case 1:a.voice.loop=PT_VOICE_PINGPONG;break;
        case 2:a.voice.linear=1;break;case 3:a.voice.looped=1;break;case 4:a.voice.phase+=1;break;
        case 5:a.voice.start=3;break;case 6:a.voice.end=17;break;case 7:a.voice.loop_start=5;break;
        case 8:a.voice.loop_end=13;a.voice.cycle=(uint64_t)9<<32;break;case 9:a.voice.cycle+=1;break;
        case 10:a.voice.active=0;break;default:a.voice.repeat_pcm=f.source[1].descriptor;break;}
        refusal(&a,f.source,f.source,960,RATE,0,&caps,PT_PAULA_SEGMENT_UNSUPPORTED);
    }
    refusal(&base,f.source,f.source,UINT64_MAX,RATE,0,&caps,PT_PAULA_SEGMENT_INVALID);
    refusal(&base,f.source,f.source,960,8000,0,&caps,PT_PAULA_SEGMENT_INVALID);
    refusal(&base,f.source,f.source,960,RATE,4,&caps,PT_PAULA_SEGMENT_INVALID);
    a=base;a.gain[1]=1;refusal(&a,f.source,f.source,960,RATE,0,&caps,PT_PAULA_SEGMENT_UNSUPPORTED);
    a=base;a.gain[0]=1025;refusal(&a,f.source,f.source,960,RATE,0,&caps,PT_PAULA_SEGMENT_UNSUPPORTED);
    cap=caps;cap.maximum_period=200;refusal(&base,f.source,f.source,960,RATE,0,&cap,PT_PAULA_SEGMENT_UNSUPPORTED);
    s=f.source[0];--s.cache_bytes;refusal(&base,&s,&s,960,RATE,0,&caps,PT_PAULA_SEGMENT_INVALID);
    s=f.source[0];++s.master.rate;refusal(&base,&s,&s,960,RATE,0,&caps,PT_PAULA_SEGMENT_STALE);
    s=f.source[0];s.guard[0].data=(void *)(UINTPTR_MAX-1);s.guard[0].bytes=4;
    refusal(&base,&s,&s,960,RATE,0,&caps,PT_PAULA_SEGMENT_INVALID);
    s=f.source[0];s.master.capacity=SIZE_MAX;refusal(&base,&s,&s,960,RATE,0,&caps,PT_PAULA_SEGMENT_INVALID);
    s=f.source[0];++s.generation;refusal(&base,f.source,&s,960,RATE,0,&caps,PT_PAULA_SEGMENT_STALE);
    s=f.source[0];++s.lease.serial;refusal(&base,f.source,&s,960,RATE,0,&caps,PT_PAULA_SEGMENT_UNSUPPORTED);
    check_storage(&f);close_fixture(&f);
}
static void identity_changes(void)
{
    struct fixture f;struct pt_render_action a;struct pt_paula_segment_plan p;
    struct pt_paula_segment_source s;unsigned i;init(&f,8);a=segment_action(&f,0);
    assert(pt_paula_segment_plan_build(&a,f.source,f.source,960,RATE,0,&caps,&p)==PT_PAULA_SEGMENT_OK);
    for(i=0;i<10;++i){s=f.source[0];
        switch(i){case 0:++s.generation;break;case 1:++s.lease.serial;break;case 2:++s.sample;break;
        case 3:s.version=f.pin[1];break;case 4:s.descriptor=f.source[1].descriptor;break;
        case 5:s.cache_data=f.source[1].cache_data;break;case 6:++s.master.capacity;break;
        case 7:++s.cache_version;break;case 8:s.cache_owner=&f.sampler;break;default:--s.guard_count;break;}
        assert(!pt_paula_segment_plan_identity_equal(&p,&s,&s));
    }
    check_storage(&f);close_fixture(&f);++cases;
}
static void unsupported_cross(unsigned bits)
{
    struct fixture f;struct pt_render_action a;init(&f,bits);a=segment_action(&f,0);
    /* The generic software voice API permits matching formats. The opt-in
     * classic cross-source planner preserves the renderer's mono8 restriction. */
    assert(pt_voice_set_repeat_source(&a.voice,f.source[1].descriptor,6,14)==PT_PCM_OK);a.kind=PT_RENDER_REPEAT;
    refusal(&a,f.source,f.source+1,1920,RATE,0,&caps,PT_PAULA_SEGMENT_UNSUPPORTED);
    close_fixture(&f);
}
static void alias_refusers(void)
{
    struct fixture f;struct pt_render_action a;struct pt_paula_segment_plan p;
    union aligned {long double alignment;unsigned char bytes[4096];} storage,saved;
    struct pt_paula_segment_source s;unsigned i;init(&f,8);a=segment_action(&f,0);
    memset(&storage,0x35,sizeof(storage));memcpy(&saved,&storage,sizeof(saved));
    for(i=0;i<4;++i){assert(pt_paula_segment_plan_build(
        i==0?(const struct pt_render_action *)(storage.bytes+1):&a,
        i==1?(const struct pt_paula_segment_source *)(storage.bytes+1):f.source,f.source,
        960,RATE,0,i==2?(const struct pt_paula_render_caps *)(storage.bytes+1):&caps,
        i==3?(struct pt_paula_segment_plan *)(storage.bytes+1):&p)==PT_PAULA_SEGMENT_ALIAS);
        assert(!memcmp(&storage,&saved,sizeof(storage)));++cases;
    }
    assert(pt_paula_segment_plan_build(&a,f.source,f.source,960,RATE,0,&caps,
        (struct pt_paula_segment_plan *)&a)==PT_PAULA_SEGMENT_ALIAS);++cases;
    /* Complete guarded owner/version extent, not just selected PCM bytes. */
    s=f.source[0];s.guard[s.guard_count++]=(struct pt_paula_segment_span){&p,sizeof(p)};
    memset(&p,0x35,sizeof(p));
    assert(pt_paula_segment_plan_build(&a,&s,&s,960,RATE,0,&caps,&p)==PT_PAULA_SEGMENT_ALIAS);
    for(i=0;i<sizeof(p);++i)assert(((unsigned char *)&p)[i]==0x35);++cases;
    assert(pt_paula_segment_plan_build(&a,f.source,f.source,960,RATE,0,&caps,
        (struct pt_paula_segment_plan *)f.pin[0])==PT_PAULA_SEGMENT_ALIAS);++cases;
    s=f.source[0];s.descriptor=(const struct pt_pcm *)(storage.bytes+1);
    refusal(&a,&s,&s,960,RATE,0,&caps,PT_PAULA_SEGMENT_INVALID);
    close_fixture(&f);
}
static void stop_refusers(void)
{
    struct fixture f;struct pt_render_action a;struct pt_paula_segment_plan p,out,saved,original;
    init(&f,8);a=segment_action(&f,0);
    assert(pt_paula_segment_plan_build(&a,f.source,f.source,1920,RATE,0,&caps,&p)==PT_PAULA_SEGMENT_OK);
    memcpy(&original,&p,sizeof(p));memset(&out,0x6c,sizeof(out));memcpy(&saved,&out,sizeof(out));
    assert(pt_paula_segment_plan_stop(&p,960,&out)==PT_PAULA_SEGMENT_INVALID&&!memcmp(&out,&saved,sizeof(out)));++cases;
    assert(pt_paula_segment_plan_stop(&p,UINT64_MAX,&out)==PT_PAULA_SEGMENT_INVALID&&!memcmp(&out,&saved,sizeof(out)));++cases;
    assert(pt_paula_segment_plan_stop(&p,3840,&p)==PT_PAULA_SEGMENT_ALIAS&&!memcmp(&p,&original,sizeof(p)));++cases;
    close_fixture(&f);
}
static void genuine_retention(void)
{
    struct fixture f;struct pt_render_action a;struct pt_paula_segment_plan p,stop;
    struct pt_pcm fresh;struct pt_sample_version *new_pin=NULL;struct pt_cache_lease new_lease;
    struct pt_paula_segment_source new_source;const uint8_t *data=NULL;size_t bytes=0;init(&f,8);
    a=segment_action(&f,0);assert(pt_voice_set_repeat_source(&a.voice,f.source[1].descriptor,6,14)==PT_PCM_OK);a.kind=PT_RENDER_REPEAT;
    assert(pt_paula_segment_plan_build(&a,f.source,f.source+1,1920,RATE,0,&caps,&p)==PT_PAULA_SEGMENT_OK&&p.source_count==2);
    assert(pt_sampler_edit(&f.sampler,&f.document.project,&f.history,0,PT_PCM_REVERSE,0,32,0)==PT_EDIT_OK);
    assert(pt_sampler_pin_current(&f.sampler,&f.document.project,0,f.sampler.generation,f.pin[0],&fresh,&new_pin)==PT_EDIT_CONFLICT);
    assert(!new_pin&&!pt_sampler_paula_location(&f.owner,0,f.lease[0],&data,&bytes));
    assert(f.chip.chip_bytes==64);check_storage(&f);
    assert(pt_sampler_pin(&f.sampler,&f.document.project,0,f.sampler.generation,&fresh,&new_pin)==PT_EDIT_OK);
    assert(pt_sampler_paula_acquire(&f.owner,0,0,0,&new_lease)==PT_CACHE_LOAD);
    new_source=observation(&f,0,fresh,new_pin,new_lease);
    assert(!pt_paula_segment_plan_identity_equal(&p,&new_source,f.source+1));
    assert(pt_sampler_paula_unpin(&f.owner,new_lease));pt_sampler_unpin(new_pin);
    /* Terminal planning follows no former descriptor and retains both old identities. */
    assert(pt_paula_segment_plan_stop(&p,3840,&stop)==PT_PAULA_SEGMENT_OK&&stop.source_count==2);
    assert(stop.source[0].version==f.pin[0]&&stop.source[1].version==f.pin[1]);
    assert(!pt_sampler_paula_close(&f.owner)&&f.chip.chip_bytes==64);check_storage(&f);
    /* Never dispatched: no DMA/reader existed. Explicit genuine lease disposal
     * is permitted here; this does NOT fabricate an active-reader STOP proof. */
    close_fixture(&f);++cases;
}
static void large_ranges(void)
{
    struct fixture f;struct pt_render_action a;struct pt_paula_segment_plan p;
    int32_t *original=calloc(132096,sizeof(*original));struct pt_pcm owned;unsigned i,sample;const uint32_t frames=131073;
    assert(original);memset(&f,0,sizeof(f));f.allocator=(struct pt_allocator){&f.fast,allocate,release};
    pt_document_init(&f.document,&f.allocator);assert(pt_document_new(&f.document,4,SIZE_MAX)==PT_PROJECT_OK);
    f.document.project.samples[0].pcm=(struct pt_pcm){original,132096,frames,8000,1,8};
    pt_sampler_init(&f.sampler,&f.allocator,1024*1024);
    assert(pt_sampler_pin(&f.sampler,&f.document.project,0,f.sampler.generation,f.pcm,f.pin)==PT_EDIT_OK);
    /* Ordinary promotion owns only active mono frames, never the donor tail. */
    assert(f.pcm[0].capacity==frames&&f.document.project.samples[0].pcm.capacity==frames);
    assert(pt_sampler_paula_bind(&f.owner,&f.sampler,&f.document.project,&f.chip,chip_allocate,chip_release,131074));
    assert(pt_sampler_paula_acquire(&f.owner,0,0,0,f.lease)==PT_CACHE_LOAD);
    f.source[0]=observation(&f,0,f.pcm[0],f.pin[0],f.lease[0]);assert(f.source[0].cache_bytes==131074);
    memset(&a,0,sizeof(a));a.kind=PT_RENDER_SEGMENT;a.gain[0]=65536;
    assert(pt_voice_init_segment(&a.voice,f.source[0].descriptor,0,2,4,6,STEP,0)==PT_PCM_OK);
    assert(pt_paula_segment_plan_build(&a,f.source,f.source,960,RATE,0,&caps,&p)==PT_PAULA_SEGMENT_OK);
    assert(p.initial.words==1&&p.repeat.words==1);++cases;
    assert(pt_voice_init_segment(&a.voice,f.source[0].descriptor,2,131072,0,131070,STEP,0)==PT_PCM_OK);
    assert(pt_paula_segment_plan_build(&a,f.source,f.source,960,RATE,0,&caps,&p)==PT_PAULA_SEGMENT_OK);
    assert(p.initial.words==65535&&p.repeat.words==65535&&p.initial.byte_offset==2);++cases;
    assert(pt_voice_init_segment(&a.voice,f.source[0].descriptor,0,131072,4,6,STEP,0)==PT_PCM_OK);
    refusal(&a,f.source,f.source,960,RATE,0,&caps,PT_PAULA_SEGMENT_UNSUPPORTED);
    assert(pt_voice_init_segment(&a.voice,f.source[0].descriptor,0,2,0,131072,STEP,0)==PT_PCM_OK);
    refusal(&a,f.source,f.source,960,RATE,0,&caps,PT_PAULA_SEGMENT_UNSUPPORTED);
    a.voice.loop_start=131072;a.voice.loop_end=131074;a.voice.cycle=(uint64_t)2<<32;
    refusal(&a,f.source,f.source,960,RATE,0,&caps,PT_PAULA_SEGMENT_UNSUPPORTED); /* The padded byte is not a master frame. */
    a.voice.loop_start=4;a.voice.loop_end=6;
    /* A complete aligned output lies inside the actual compacted allocation. */
    assert(130800U<f.pcm[0].capacity&&sizeof(p)<=(f.pcm[0].capacity-130800U)*sizeof(int32_t));
    assert(!((uintptr_t)(f.pcm[0].data+130800U)%offsetof(struct {char c;struct pt_paula_segment_plan p;},p)));
    assert(pt_paula_segment_plan_build(&a,f.source,f.source,960,RATE,0,&caps,
        (struct pt_paula_segment_plan *)(f.pcm[0].data+130800U))==PT_PAULA_SEGMENT_ALIAS);
    for(i=0;i<frames;++i)assert(f.pcm[0].data[i]==0);
    for(i=frames;i<132096;++i)assert(original[i]==0);
    assert(pt_sampler_paula_unpin(&f.owner,f.lease[0])&&pt_sampler_paula_close(&f.owner));
    pt_sampler_release(&f.sampler);pt_sampler_unpin(f.pin[0]);pt_document_release(&f.document);free(original);
    assert(!f.fast.live&&!f.chip.live&&!f.chip.chip_bytes);

    /* A distinct genuine owned append preserves its real unused capacity.
     * The allocator transfers this separate PCM allocation; no enlarged pin
     * observation or fabricated version/storage extent is constructed. */
    memset(&f,0,sizeof(f));f.allocator=(struct pt_allocator){&f.fast,allocate,release};
    pt_document_init(&f.document,&f.allocator);assert(pt_document_new(&f.document,4,SIZE_MAX)==PT_PROJECT_OK);
    pt_sampler_init(&f.sampler,&f.allocator,1024*1024);
    assert(pt_pattern_history_init(&f.history,&f.document.project,f.commands,4,f.changes,4)==PT_EDIT_OK);
    owned=(struct pt_pcm){allocate(&f.fast,132096U*sizeof(int32_t)),132096,frames,8000,1,8};
    assert(owned.data);memset(owned.data,0,owned.capacity*sizeof(int32_t));sample=f.document.project.sample_count;
    assert(pt_sampler_append_owned(&f.sampler,&f.document.project,&f.history,&owned,&f.allocator,"capacity")==PT_EDIT_OK);
    assert(!owned.data&&!owned.capacity&&f.document.project.sample_count==sample+1U);
    assert(pt_sampler_pin(&f.sampler,&f.document.project,sample,f.sampler.generation,f.pcm,f.pin)==PT_EDIT_OK);
    assert(f.pcm[0].capacity==132096U&&f.pcm[0].frames==frames);
    assert(f.document.project.samples[sample].pcm.capacity==132096U);
    assert(pt_sampler_paula_bind(&f.owner,&f.sampler,&f.document.project,&f.chip,chip_allocate,chip_release,131074));
    assert(pt_sampler_paula_acquire(&f.owner,0,sample,0,f.lease)==PT_CACHE_LOAD);
    f.source[0]=observation(&f,sample,f.pcm[0],f.pin[0],f.lease[0]);
    assert(f.source[0].master.capacity==132096U&&f.source[0].cache_bytes==131074U);
    memset(&a,0,sizeof(a));a.kind=PT_RENDER_SEGMENT;a.gain[0]=65536;
    assert(pt_voice_init_segment(&a.voice,f.source[0].descriptor,0,2,4,6,STEP,0)==PT_PCM_OK);
    assert(131074U>=frames&&sizeof(p)<=(f.pcm[0].capacity-131074U)*sizeof(int32_t));
    assert(!((uintptr_t)(f.pcm[0].data+131074U)%offsetof(struct {char c;struct pt_paula_segment_plan p;},p)));
    assert(pt_paula_segment_plan_build(&a,f.source,f.source,960,RATE,0,&caps,
        (struct pt_paula_segment_plan *)(f.pcm[0].data+131074U))==PT_PAULA_SEGMENT_ALIAS);
    for(i=0;i<frames;++i)assert(f.pcm[0].data[i]==0);
    for(i=frames;i<132096;++i)assert(f.pcm[0].data[i]==0);++cases;
    assert(pt_sampler_paula_unpin(&f.owner,f.lease[0])&&pt_sampler_paula_close(&f.owner));
    pt_pattern_history_release(&f.history);pt_sampler_release(&f.sampler);
    /* Current/history release retains this genuine pin and its unused tail. */
    for(i=0;i<132096;++i)assert(f.pcm[0].data[i]==0);
    pt_sampler_unpin(f.pin[0]);pt_document_release(&f.document);
    assert(!f.fast.live&&!f.chip.live&&!f.chip.chip_bytes);
}
int main(void)
{
    segments(8);segments(16);segments(24);
    repeats(0,0);repeats(1,0);repeats(0,1);repeats(1,1);
    refusers();identity_changes();unsupported_cross(16);unsupported_cross(24);alias_refusers();stop_refusers();genuine_retention();large_ranges();
    assert(cases==52);
    puts("PAULA SEGMENT REPEAT PLAN HOST FIXTURE PASS:52 cases;exact mono8-16-24 initial geometry;even frame-byte-word ranges1-65535;original960-1920-2880-3840;overlap-disjoint loops;genuine voice pending-source handoff;fractional current phase retained;no restart or silent-word inference;full identity and alias refusals;genuine pins and Chip leases retained across stale edits and declarative STOP;OWNERSHIP_READER_NATIVE_INTEGRATION_NOT_RUN");return 0;
}
