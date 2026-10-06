#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/editor/mixed_readers_plan_normalize.h"
#include "../src/editor/mixed_preflight.h"
#include "../src/editor/sampler_internal.h"
#include "../src/core/amigus_render_voice.h"
#include "../src/core/render_lookahead.h"

struct allocations {unsigned calls,releases,live;};
static void *new_memory(void *context,size_t n)
{
    struct allocations *a=context;void *p=malloc(n);
    assert(p);++a->calls;++a->live;return p;
}
static void free_memory(void *context,void *p)
{
    struct allocations *a=context;assert(p&&a->live);++a->releases;--a->live;free(p);
}
struct fixture {
    struct pt_project project;struct pt_sample samples[3];
    struct pt_event events[64*16];uint16_t order;
    int32_t pcm[3][8192];uint32_t slices[3][2];
    struct pt_extension extension;uint8_t payload[4096];
    struct pt_render_plan plan;struct pt_mixed_plan_origin origins[16];
    struct pt_paula_render_caps caps;struct pt_playback_format format;
    struct pt_mixed_plan_span contexts[32];struct pt_mixed_plan_inputs inputs;
    struct pt_mixed_plan_normalizer **owner;size_t owner_capacity;
    void *workspace;size_t capacity;
    struct pt_mixed_plan_batch *output;
    struct allocations allocations;struct pt_allocator allocator;
    struct pt_sampler sampler;struct pt_sample_version *pins[3];
};
static struct fixture *make(unsigned bits,unsigned cache_bits,unsigned endian,unsigned paula)
{
    struct fixture *f=calloc(1,sizeof(*f));unsigned i,j;
    assert(f);f->owner_capacity=sizeof(*f->output);
    f->owner=calloc(1,f->owner_capacity);assert(f->owner);
    assert(f->owner_capacity>=sizeof(struct pt_mixed_plan_report)&&
        (uintptr_t)f->owner%_Alignof(struct pt_mixed_plan_batch)==0&&
        (uintptr_t)f->owner%_Alignof(struct pt_mixed_plan_report)==0);
    f->capacity=pt_mixed_plan_normalizer_workspace_size()+sizeof(*f->output)+64;
    f->workspace=calloc(1,f->capacity);f->output=malloc(sizeof(*f->output));assert(f->workspace&&f->output);
    memset((uint8_t *)f->workspace+pt_mixed_plan_normalizer_workspace_size(),0xa7,
        f->capacity-pt_mixed_plan_normalizer_workspace_size());
    assert((uintptr_t)f->workspace%pt_mixed_plan_normalizer_workspace_alignment()==0);
    pt_channels_init(&f->project.channels);f->project.channels.count=16;
    for(i=0;i<16;++i)f->project.channels.track[i].route=(uint8_t)(i<paula?PT_PAULA:PT_AMIGUS);
    f->project.samples=f->samples;f->project.sample_count=3;f->project.orders=&f->order;
    f->project.order_count=f->project.pattern_count=1;f->project.events=f->events;
    f->project.speed=1;f->project.bpm=125;f->project.extensions=&f->extension;f->project.extension_count=1;
    f->extension.id=0x706c616e;f->extension.version=1;f->extension.data=f->payload;f->extension.length=sizeof(f->payload);
    for(i=0;i<3;++i){
        for(j=0;j<8192;++j)f->pcm[i][j]=((int32_t)(j%11)-5)*(int32_t)(1U<<(bits-8));
        if(bits==24){f->pcm[i][0]=8388607;f->pcm[i][1]=-8388607;f->pcm[i][2]=257;f->pcm[i][3]=-257;}
        f->samples[i].pcm=(struct pt_pcm){f->pcm[i],8192,4096,8000,1,(uint8_t)bits};
        f->samples[i].volume=1;
    }
    f->caps=(struct pt_paula_render_caps){3546895,124,65535};
    f->format.bits=cache_bits;f->format.little_endian=endian;
    f->contexts[0]=(struct pt_mixed_plan_span){f,sizeof(*f)};
    f->inputs=(struct pt_mixed_plan_inputs){&f->project,&f->plan,f->origins,&f->caps,&f->format,
        f->contexts,1,48000,960,31,7};
    f->allocator=(struct pt_allocator){&f->allocations,new_memory,free_memory};
    assert(pt_project_validate(&f->project,NULL)==PT_PROJECT_OK);
    return f;
}
static void drop(struct fixture *f)
{
    unsigned i;size_t byte;assert(pt_mixed_plan_normalizer_close(f->owner)&&!*f->owner);
    assert(pt_mixed_plan_normalizer_close(f->owner));
    for(byte=sizeof(*f->owner);byte<f->owner_capacity;++byte)
        assert(!((uint8_t *)f->owner)[byte]);
    for(byte=0;byte<pt_mixed_plan_normalizer_workspace_size();++byte)
        assert(!((uint8_t *)f->workspace)[byte]);
    for(;byte<f->capacity;++byte)assert(((uint8_t *)f->workspace)[byte]==0xa7);
    for(i=0;i<3;++i)if(f->pins[i])pt_sampler_unpin(f->pins[i]);
    if(f->sampler.allocator.allocate)pt_sampler_release(&f->sampler);
    assert(!f->allocations.live);free(f->owner);free(f->output);free(f->workspace);free(f);
}
static struct pt_voice initial(const struct fixture *f,unsigned sample)
{
    struct pt_voice v={0};v.pcm=&f->project.samples[sample].pcm;
    v.step=((uint64_t)8000<<32)/f->inputs.rate;v.end=v.pcm->frames;v.active=1;return v;
}
static void singleton(struct fixture *f,unsigned track,unsigned sample)
{
    struct pt_render_action *a=f->plan.action;
    memset(&f->plan,0,sizeof(f->plan));f->plan.count=1;a->kind=PT_RENDER_TRIGGER;a->channel=track;
    a->voice=initial(f,sample);
    a->gain[(f->project.channels.track[track].route==PT_PAULA&&(track==1||track==2))?1:0]=1024;
}
static void begin(struct fixture *f)
{
    assert(!*f->owner);assert(pt_mixed_plan_normalizer_begin_in_workspace(f->workspace,f->capacity,&f->inputs,f->owner)==PT_MIXED_PLAN_PENDING);
    assert(*f->owner);memset(f->output,0xa5,sizeof(*f->output));
}
static enum pt_mixed_plan_result finish(struct fixture *f,unsigned work)
{
    enum pt_mixed_plan_result result;struct pt_mixed_plan_report before,after;unsigned loops=0;
    assert(pt_mixed_plan_normalizer_report(*f->owner,&before)==PT_MIXED_PLAN_PENDING);
    do{
        result=pt_mixed_plan_normalizer_step(*f->owner,f->inputs.revision,f->inputs.generation,work);
        assert(pt_mixed_plan_normalizer_report(*f->owner,&after)==result);
        assert(after.last_work<=work&&after.work-before.work<=work&&after.work>before.work);
        before=after;assert(++loops<100000);
    }while(result==PT_MIXED_PLAN_PENDING);
    return result;
}
static void expect(struct fixture *f,enum pt_mixed_plan_result result,enum pt_mixed_plan_reason reason)
{
    struct pt_mixed_plan_batch *old=malloc(sizeof(*old));struct pt_mixed_plan_report report;
    struct fixture *original=malloc(sizeof(*original));
    assert(old&&original);memcpy(original,f,sizeof(*original));begin(f);memcpy(old,f->output,sizeof(*old));
    assert(pt_mixed_plan_normalizer_get(*f->owner,31,7,f->output)==PT_MIXED_PLAN_PENDING&&!memcmp(old,f->output,sizeof(*old)));
    assert(finish(f,17)==result);assert(pt_mixed_plan_normalizer_get(*f->owner,31,7,f->output)==result);
    assert(pt_mixed_plan_normalizer_report(*f->owner,&report)==result&&report.reason==reason);
    if(result!=PT_MIXED_PLAN_READY)assert(!memcmp(old,f->output,sizeof(*old)));
    assert(pt_mixed_plan_normalizer_close(f->owner)&&!*f->owner);
    assert(!memcmp(original,f,sizeof(*original)));free(original);free(old);
}
static int image_same(const struct pt_amigus_voice_plan *a,const struct pt_amigus_voice_plan *b)
{
    return a->start==b->start&&a->loop==b->loop&&a->end_exclusive==b->end_exclusive&&
        a->rate==b->rate&&a->control==b->control&&a->left==b->left&&a->right==b->right;
}
static void shape_cases(void)
{
    struct fixture *f=make(24,16,1,4);unsigned i;struct pt_mixed_plan_report report;
    for(i=0;i<16;++i){struct pt_render_action *a=f->plan.action+i;
        a->kind=PT_RENDER_TRIGGER;a->channel=i;a->voice=initial(f,i%3);
        a->gain[(i==1||i==2)?1:0]=1024;f->plan.action[16+i]=*a;
        f->plan.action[16+i].kind=PT_RENDER_CONTROL;f->plan.action[16+i].voice.step+=1;
    }
    f->plan.count=32;expect(f,PT_MIXED_PLAN_READY,PT_MIXED_PLAN_REASON_NONE);
    assert(f->output->count==16&&f->output->frame==960);
    for(i=0;i<16;++i){const struct pt_mixed_plan_record *r=f->output->record+i;
        assert(r->track==i&&r->first_action==i&&r->control_action==16+i&&r->sample==i%3&&!r->channel);
        assert(r->slot==(i<4?i:i-4)&&r->route==(i<4?PT_PAULA:PT_AMIGUS));
        assert(f->output->next[i].present&&f->output->next[i].frame==960);
    }
    /* Every filled late slot is inspected until the first unsupported duplicate;
     * no partial normalized record or source mask can escape. */
    for(i=32;i<64;++i)f->plan.action[i]=f->plan.action[i%16];
    f->plan.count=64;begin(f);assert(finish(f,1)==PT_MIXED_PLAN_REFUSED);
    assert(pt_mixed_plan_normalizer_report(*f->owner,&report)==PT_MIXED_PLAN_REFUSED);
    assert(report.action==32&&report.reason==PT_MIXED_PLAN_REASON_DUPLICATE&&report.actions_scanned==33);
    assert(pt_mixed_plan_normalizer_close(f->owner));f->plan.count=65;
    expect(f,PT_MIXED_PLAN_REFUSED,PT_MIXED_PLAN_REASON_COUNT);
    singleton(f,4,0);
    f->plan.action[1]=f->plan.action[0];f->plan.count=2;
    {struct pt_wavetable_preflight_report w;uint16_t held=0;
        assert(pt_wavetable_check_plan(&f->project,48000,&f->plan,&f->format,1,&held,&w)==PT_WAVETABLE_COMPATIBLE);
        assert(held==(1U<<4));}
    expect(f,PT_MIXED_PLAN_REFUSED,PT_MIXED_PLAN_REASON_DUPLICATE);
    f->plan.action[1].kind=PT_RENDER_STOP;
    {struct pt_wavetable_preflight_report w;uint16_t held=0;
        assert(pt_wavetable_check_plan(&f->project,48000,&f->plan,&f->format,1,&held,&w)==PT_WAVETABLE_COMPATIBLE&&!held);}
    expect(f,PT_MIXED_PLAN_REFUSED,PT_MIXED_PLAN_REASON_DUPLICATE);
    f->plan.action[0].kind=f->plan.action[1].kind=PT_RENDER_CONTROL;
    {struct pt_wavetable_preflight_report w;uint16_t held=(1U<<4);
        assert(pt_wavetable_check_plan(&f->project,48000,&f->plan,&f->format,1,&held,&w)==PT_WAVETABLE_COMPATIBLE);}
    expect(f,PT_MIXED_PLAN_REFUSED,PT_MIXED_PLAN_REASON_DUPLICATE);
    f->plan.action[1].kind=PT_RENDER_TRIGGER;
    expect(f,PT_MIXED_PLAN_REFUSED,PT_MIXED_PLAN_REASON_DUPLICATE);
    singleton(f,4,0);f->plan.count=2;f->plan.action[1]=f->plan.action[0];f->plan.action[1].kind=PT_RENDER_CONTROL;
    for(i=0;i<14;++i){struct pt_voice old=f->plan.action[1].voice;
        switch(i){
        case 0:f->plan.action[1].voice.pcm=&f->samples[1].pcm;break;
        case 1:f->plan.action[1].voice.repeat_pcm=&f->samples[1].pcm;break;
        case 2:++f->plan.action[1].voice.phase;break;
        case 3:++f->plan.action[1].voice.cycle;break;
        case 4:++f->plan.action[1].voice.start;break;
        case 5:--f->plan.action[1].voice.end;break;
        case 6:++f->plan.action[1].voice.loop_start;break;
        case 7:++f->plan.action[1].voice.loop_end;break;
        case 8:++f->plan.action[1].voice.loop;break;
        case 9:++f->plan.action[1].voice.looped;break;
        case 10:++f->plan.action[1].voice.linear;break;
        case 11:f->plan.action[1].voice.active=0;break;
        case 12:++f->plan.action[1].voice.segment;break;
        default:f->plan.action[1].voice.step+=123;break;
        }
        expect(f,i==13?PT_MIXED_PLAN_READY:PT_MIXED_PLAN_REFUSED,
            i==13?PT_MIXED_PLAN_REASON_NONE:PT_MIXED_PLAN_REASON_FOLD);
        f->plan.action[1].voice=old;
    }
    singleton(f,4,0);f->plan.action[0].kind=PT_RENDER_SEGMENT;
    expect(f,PT_MIXED_PLAN_REFUSED,PT_MIXED_PLAN_REASON_KIND);
    f->plan.action[0].kind=PT_RENDER_REPEAT;expect(f,PT_MIXED_PLAN_REFUSED,PT_MIXED_PLAN_REASON_KIND);
    f->plan.action[0].kind=PT_RENDER_TRIGGER;f->plan.action[0].channel=16;
    expect(f,PT_MIXED_PLAN_REFUSED,PT_MIXED_PLAN_REASON_ROUTE);
    f->plan.action[0].channel=4;f->project.channels.track[4].route=PT_MIDI;
    expect(f,PT_MIXED_PLAN_REFUSED,PT_MIXED_PLAN_REASON_ROUTE);drop(f);
    f=make(8,8,0,0);
    for(i=0;i<16;++i){f->plan.action[i]=(struct pt_render_action){PT_RENDER_TRIGGER,i,initial(f,0),{0,0}};}
    f->plan.count=16;expect(f,PT_MIXED_PLAN_READY,PT_MIXED_PLAN_REASON_NONE);
    for(i=0;i<16;++i)assert(f->output->record[i].slot==i&&f->output->record[i].route==PT_AMIGUS);
    memset(&f->plan,0,sizeof(f->plan));expect(f,PT_MIXED_PLAN_READY,PT_MIXED_PLAN_REASON_NONE);
    assert(!f->output->count);drop(f);
    puts("PAIRED NORMALIZER SHAPES PASS: whole64 input,32 to16 exact fold, fresh4/12 and0/16 identities, atomic duplicate/kind/fold refusal");
}
static void geometry_cases(void)
{
    const unsigned bits[]={8,16,24};unsigned b,cache,endian;
    for(b=0;b<3;++b)for(cache=8;cache<=16;cache+=8)for(endian=0;endian<2;++endian){
        struct fixture *f=make(bits[b],cache,endian,4);struct pt_paula_render_plan p;
        struct pt_amigus_voice_plan canonical,actual;const struct pt_mixed_plan_record *r;
        singleton(f,0,0);expect(f,PT_MIXED_PLAN_READY,PT_MIXED_PLAN_REASON_NONE);
        assert(pt_paula_render_voice(&f->plan.action[0].voice,48000,f->plan.action[0].gain,0,&f->caps,&p));
        assert(p.offset==0&&p.length==4096&&p.period==443&&p.volume==1&&
            f->output->record[0].geometry.paula.period==p.period&&
            f->output->record[0].geometry.paula.volume==p.volume);
        f->plan.action[0].voice.start=2;f->plan.action[0].voice.phase=(uint64_t)2<<32;
        assert(pt_paula_render_voice(&f->plan.action[0].voice,48000,f->plan.action[0].gain,0,&f->caps,&p));
        expect(f,PT_MIXED_PLAN_REFUSED,PT_MIXED_PLAN_REASON_PAULA);
        singleton(f,0,0);f->plan.action[0].voice.end=4094;
        assert(pt_paula_render_voice(&f->plan.action[0].voice,48000,f->plan.action[0].gain,0,&f->caps,&p));
        expect(f,PT_MIXED_PLAN_REFUSED,PT_MIXED_PLAN_REASON_PAULA);
        singleton(f,0,0);f->plan.action[0].gain[0]=1025;
        expect(f,PT_MIXED_PLAN_REFUSED,PT_MIXED_PLAN_REASON_PAULA);
        singleton(f,0,0);f->plan.action[0].voice.phase=1;
        expect(f,PT_MIXED_PLAN_REFUSED,PT_MIXED_PLAN_REASON_PAULA);
        f->samples[0].pcm.frames=4095;singleton(f,0,0);
        expect(f,PT_MIXED_PLAN_REFUSED,PT_MIXED_PLAN_REASON_PAULA);f->samples[0].pcm.frames=4096;
        f->samples[0].pcm.channels=2;singleton(f,0,0);
        expect(f,PT_MIXED_PLAN_REFUSED,PT_MIXED_PLAN_REASON_PAULA);f->samples[0].pcm.channels=1;
        f->samples[0].loop=PT_LOOP_FORWARD;f->samples[0].loop_start=2;f->samples[0].loop_end=4096;
        singleton(f,0,0);expect(f,PT_MIXED_PLAN_REFUSED,PT_MIXED_PLAN_REASON_PAULA);
        f->samples[0].loop=PT_LOOP_NONE;f->samples[0].loop_start=f->samples[0].loop_end=0;
        f->samples[0].interpolation=1;singleton(f,0,0);
        expect(f,PT_MIXED_PLAN_REFUSED,PT_MIXED_PLAN_REASON_PAULA);f->samples[0].interpolation=0;
        singleton(f,4,1);expect(f,PT_MIXED_PLAN_READY,PT_MIXED_PLAN_REASON_NONE);r=f->output->record;
        assert(pt_amigus_render_voice(&f->plan.action[0].voice,48000,f->plan.action[0].gain,&f->format,0,4096*(cache/8),&canonical));
        assert(pt_amigus_voice_plan_prepare(&f->samples[1],&f->format,&r->geometry.amigus.trigger,0,4096*(cache/8),&actual));
        assert(image_same(&canonical,&actual)&&image_same(&canonical,&r->image));
        assert(r->geometry.amigus.bits==cache&&r->geometry.amigus.little_endian==endian);
        f->plan.action[0].voice.start=1;f->plan.action[0].voice.phase=(uint64_t)1<<32;
        expect(f,cache==8?PT_MIXED_PLAN_REFUSED:PT_MIXED_PLAN_READY,
            cache==8?PT_MIXED_PLAN_REASON_AMIGUS:PT_MIXED_PLAN_REASON_NONE);
        f->plan.action[0].voice.start=2;f->plan.action[0].voice.phase=(uint64_t)2<<32;
        expect(f,PT_MIXED_PLAN_READY,PT_MIXED_PLAN_REASON_NONE);assert(f->output->record[0].geometry.amigus.trigger.offset==2);
        {
            const uint32_t origins[]={64,24576};unsigned fragment;
            for(fragment=0;fragment<2;++fragment){
                assert(pt_amigus_render_voice(&f->plan.action[0].voice,48000,f->plan.action[0].gain,
                    &f->format,origins[fragment],4096*(cache/8),&canonical));
                assert(pt_amigus_voice_plan_prepare(&f->samples[1],&f->format,
                    &f->output->record[0].geometry.amigus.trigger,origins[fragment],4096*(cache/8),&actual));
                assert(image_same(&canonical,&actual)&&actual.start==origins[fragment]+2*(cache/8)&&
                    actual.end_exclusive==origins[fragment]+4096*(cache/8));
            }
        }
        f->plan.action[0].voice.end=4094;
        assert(pt_amigus_render_voice(&f->plan.action[0].voice,48000,f->plan.action[0].gain,&f->format,0,4096*(cache/8),&canonical));
        expect(f,PT_MIXED_PLAN_REFUSED,PT_MIXED_PLAN_REASON_AMIGUS);
        singleton(f,4,1);f->plan.action[0].voice.phase=1;
        expect(f,PT_MIXED_PLAN_REFUSED,PT_MIXED_PLAN_REASON_AMIGUS);
        singleton(f,4,1);f->samples[1].pcm.channels=2;
        expect(f,PT_MIXED_PLAN_REFUSED,PT_MIXED_PLAN_REASON_AMIGUS);f->samples[1].pcm.channels=1;
        singleton(f,4,1);f->samples[1].interpolation=1;
        expect(f,PT_MIXED_PLAN_REFUSED,PT_MIXED_PLAN_REASON_AMIGUS);f->samples[1].interpolation=0;
        f->samples[1].loop=PT_LOOP_FORWARD;f->samples[1].loop_start=2;f->samples[1].loop_end=4096;
        f->plan.action[0].voice.loop=PT_VOICE_FORWARD;f->plan.action[0].voice.loop_start=2;f->plan.action[0].voice.loop_end=4096;
        f->plan.action[0].voice.cycle=(uint64_t)4094<<32;
        expect(f,PT_MIXED_PLAN_READY,PT_MIXED_PLAN_REASON_NONE);
        f->plan.action[0].voice.loop_end=4094;
        expect(f,PT_MIXED_PLAN_REFUSED,PT_MIXED_PLAN_REASON_AMIGUS);
        f->plan.action[0].voice.pcm=(const struct pt_pcm *)(uintptr_t)1;
        expect(f,PT_MIXED_PLAN_REFUSED,PT_MIXED_PLAN_REASON_SOURCE);drop(f);
    }
    {
        struct fixture *f=make(24,16,1,4);int32_t *full=calloc(131087,sizeof(*full));
        assert(full);f->samples[0].pcm.data=full;f->samples[0].pcm.capacity=131087;
        f->samples[0].pcm.frames=2;singleton(f,0,0);
        expect(f,PT_MIXED_PLAN_READY,PT_MIXED_PLAN_REASON_NONE);
        f->samples[0].pcm.frames=131070;singleton(f,0,0);
        expect(f,PT_MIXED_PLAN_READY,PT_MIXED_PLAN_REASON_NONE);
        f->samples[0].pcm.frames=131072;singleton(f,0,0);
        expect(f,PT_MIXED_PLAN_REFUSED,PT_MIXED_PLAN_REASON_PAULA);
        f->samples[0].pcm.frames=2;singleton(f,0,0);f->caps.maximum_period=124;
        expect(f,PT_MIXED_PLAN_REFUSED,PT_MIXED_PLAN_REASON_PAULA);
        drop(f);free(full);
    }
    puts("PAIRED NORMALIZER GEOMETRY PASS: exact canonical/factory oracle,8/16/24 masters,8/16 endian caches, whole Paula and strict card bounds/interpolation refusal");
}
static void rates_gains_and_origins(void)
{
    struct fixture *f=make(24,16,1,4);struct pt_mixed_plan_report report;
    const uint64_t steps[]={16,((uint64_t)8000<<32)/48000,((uint64_t)192000<<32)/48000};unsigned i;
    singleton(f,4,0);
    for(i=0;i<3;++i){struct pt_amigus_voice_plan literal;
        f->plan.action[0].voice.step=steps[i];f->plan.action[0].gain[0]=0;
        expect(f,PT_MIXED_PLAN_READY,PT_MIXED_PLAN_REASON_NONE);
        assert(pt_amigus_voice_plan_prepare(&f->samples[0],&f->format,&f->output->record[0].geometry.amigus.trigger,0,8192,&literal));
        assert(literal.rate==(i==0?1:i==2?0x40000000UL:f->output->record[0].image.rate));
        assert(literal.rate==f->output->record[0].image.rate);
        if(i==2)assert(f->output->record[0].geometry.amigus.trigger.rate_numerator==3145728000UL);
    }
    f->plan.action[0].voice.step=steps[2]+16;expect(f,PT_MIXED_PLAN_REFUSED,PT_MIXED_PLAN_REASON_AMIGUS);
    singleton(f,4,0);f->plan.action[0].gain[0]=65536;
    expect(f,PT_MIXED_PLAN_READY,PT_MIXED_PLAN_REASON_NONE);assert(f->output->record[0].geometry.amigus.trigger.volume==64&&!f->output->record[0].geometry.amigus.trigger.pan);
    f->plan.action[0].gain[0]=0;f->plan.action[0].gain[1]=65536;
    expect(f,PT_MIXED_PLAN_READY,PT_MIXED_PLAN_REASON_NONE);assert(f->output->record[0].geometry.amigus.trigger.pan==256);
    singleton(f,4,0);f->plan.action[0].gain[0]=32639;f->plan.action[0].gain[1]=32896;
    begin(f);assert(finish(f,256)==PT_MIXED_PLAN_REFUSED);
    assert(pt_mixed_plan_normalizer_report(*f->owner,&report)==PT_MIXED_PLAN_REFUSED&&report.reason==PT_MIXED_PLAN_REASON_GAINS);
    assert(report.candidates==PT_MIXED_PLAN_GAIN_CANDIDATES);assert(pt_mixed_plan_normalizer_close(f->owner));
    {
        struct pt_amigus_voice_plan literal;
        struct pt_amigus_voice_request request={8000,1,0,37,91};
        assert(pt_amigus_voice_plan_prepare(&f->samples[0],&f->format,&request,0,8192,&literal));
        singleton(f,4,0);
        f->plan.action[0].gain[0]=(uint32_t)(((uint64_t)literal.left*65536+32767)/65535);
        f->plan.action[0].gain[1]=(uint32_t)(((uint64_t)literal.right*65536+32767)/65535);
        expect(f,PT_MIXED_PLAN_READY,PT_MIXED_PLAN_REASON_NONE);
        assert(f->output->record[0].geometry.amigus.trigger.volume==37&&
            f->output->record[0].geometry.amigus.trigger.pan==91&&
            f->output->record[0].image.left==literal.left&&f->output->record[0].image.right==literal.right);
    }
    singleton(f,4,1);f->origins[4]=(struct pt_mixed_plan_origin){1,1,0,0};f->plan.action[0].kind=PT_RENDER_CONTROL;
    expect(f,PT_MIXED_PLAN_READY,PT_MIXED_PLAN_REASON_NONE);
    assert(f->output->record[0].kind==PT_MIXED_PLAN_CONTROL&&!f->output->record[0].geometry.amigus.bits&&!f->output->record[0].geometry.amigus.trigger.rate_numerator);
    assert(f->output->next[4].sample==1&&f->output->next[4].frame==0);
    f->plan.action[0].voice.pcm=&f->samples[0].pcm;expect(f,PT_MIXED_PLAN_REFUSED,PT_MIXED_PLAN_REASON_SOURCE);
    f->plan.action[0].kind=PT_RENDER_STOP;f->plan.action[0].voice=(struct pt_voice){0};
    expect(f,PT_MIXED_PLAN_READY,PT_MIXED_PLAN_REASON_NONE);assert(!f->output->next[4].present&&!f->output->record[0].geometry.amigus.rate);
    f->origins[4].frame=960;expect(f,PT_MIXED_PLAN_REFUSED,PT_MIXED_PLAN_REASON_ORIGIN);
    memset(f->origins,0,sizeof(f->origins));expect(f,PT_MIXED_PLAN_REFUSED,PT_MIXED_PLAN_REASON_ORIGIN);
    f->inputs.frame=UINT64_MAX;
    assert(pt_mixed_plan_normalizer_begin_in_workspace(f->workspace,f->capacity,&f->inputs,
        f->owner)==PT_MIXED_PLAN_INVALID&&!*f->owner);f->inputs.frame=960;
    drop(f);puts("PAIRED NORMALIZER RATES PASS: literal min/max rational equality, finite exact gains and nonrepresentable pan refusal, logical origins without ACTIVE authority");
}
static void owner_output_refusals(struct fixture *f,enum pt_mixed_plan_result state)
{
    struct pt_mixed_plan_normalizer *copied=*f->owner;
    struct pt_mixed_plan_report before,after;uint8_t *saved=malloc(f->owner_capacity);
    assert(saved);memcpy(saved,f->owner,f->owner_capacity);
    assert(pt_mixed_plan_normalizer_report(*f->owner,&before)==state);
    /* Real aligned owner backing is large enough for BOTH complete results.
     * The known pointer extent alone must refuse every overlapping write. */
    assert(pt_mixed_plan_normalizer_get(*f->owner,31,7,(void *)f->owner)==PT_MIXED_PLAN_ALIAS);
    assert(pt_mixed_plan_normalizer_report(*f->owner,(void *)f->owner)==PT_MIXED_PLAN_ALIAS);
    assert(pt_mixed_plan_normalizer_begin_in_workspace(f->workspace,f->capacity,
        &f->inputs,f->owner)==PT_MIXED_PLAN_INVALID);
    assert(!memcmp(saved,f->owner,f->owner_capacity)&&*f->owner==copied);
    assert(!pt_mixed_plan_normalizer_close(&copied)&&copied==*f->owner);
    assert(!memcmp(saved,f->owner,f->owner_capacity));
    assert(pt_mixed_plan_normalizer_report(*f->owner,&after)==state&&!memcmp(&before,&after,sizeof(before)));
    free(saved);
}
static void alias_stale_and_cancel(void)
{
    struct fixture *f=make(24,16,1,4);struct pt_mixed_plan_batch *old=malloc(sizeof(*old));
    struct pt_mixed_plan_report report;struct pt_project header;unsigned i,aligned=0;
    void *aliases[9];void *extra=calloc(1,4096);assert(old&&extra);
    singleton(f,4,0);f->inputs.context_count=0;
    f->samples[2].slices=f->slices[2];f->samples[2].slice_count=2;
    f->slices[2][0]=0;f->slices[2][1]=2048;
    aliases[0]=&f->plan.action[63];aliases[1]=&f->origins[15];aliases[2]=f->pcm[2]+5000;
    aliases[3]=f->samples+2;aliases[4]=f->payload+1024;aliases[5]=f->slices[2];
    aliases[6]=&f->caps;aliases[7]=&f->format;aliases[8]=&f->inputs;
    for(i=0;i<9;++i){void *slot=aliases[i];uint8_t before[sizeof(void *)];
        enum pt_mixed_plan_result refusal=(uintptr_t)slot%_Alignof(struct pt_mixed_plan_normalizer *)?
            PT_MIXED_PLAN_INVALID:PT_MIXED_PLAN_ALIAS;
        /* Snapshot bytes without claiming a misaligned address is a typed
         * owner slot. Initial alignment refusal precedes overlap admission. */
        memcpy(before,slot,sizeof(before));
        assert(pt_mixed_plan_normalizer_begin_in_workspace(f->workspace,f->capacity,&f->inputs,slot)==refusal);
        assert(!memcmp(before,slot,sizeof(before))&&!*f->owner);
        if(refusal==PT_MIXED_PLAN_ALIAS)++aligned;
    }
    assert(aligned>=4); /* Actual aligned captured spans exercise overlap. */
    {
        int32_t before=f->pcm[2][5000];
        size_t alignment=pt_mixed_plan_normalizer_workspace_alignment();
        uint8_t *raw=(void *)(f->pcm[2]+5000);
        uint8_t *proper=raw+(alignment-(uintptr_t)raw%alignment)%alignment;
        uint8_t saved[sizeof(int32_t)];
        enum pt_mixed_plan_result refusal=(uintptr_t)raw%alignment?
            PT_MIXED_PLAN_INVALID:PT_MIXED_PLAN_ALIAS;
        assert(pt_mixed_plan_normalizer_begin_in_workspace(f->pcm[2]+5000,f->capacity,
            &f->inputs,f->owner)==refusal);
        assert(!*f->owner&&f->pcm[2][5000]==before);
        memcpy(saved,proper,sizeof(saved));
        assert(pt_mixed_plan_normalizer_begin_in_workspace(proper,f->capacity,
            &f->inputs,f->owner)==PT_MIXED_PLAN_ALIAS);
        assert(!*f->owner&&!memcmp(saved,proper,sizeof(saved)));
    }
    begin(f);owner_output_refusals(f,PT_MIXED_PLAN_PENDING);
    assert(finish(f,17)==PT_MIXED_PLAN_READY);owner_output_refusals(f,PT_MIXED_PLAN_READY);
    assert(pt_mixed_plan_normalizer_close(f->owner)&&!*f->owner);
    begin(f);memcpy(old,f->output,sizeof(*old));
    for(i=0;i<9;++i){
        uint8_t before[sizeof(void *)];memcpy(before,aliases[i],sizeof(before));
        enum pt_mixed_plan_result refusal=(uintptr_t)aliases[i]%_Alignof(struct pt_mixed_plan_normalizer *)?
            PT_MIXED_PLAN_INVALID:PT_MIXED_PLAN_ALIAS;
        assert(pt_mixed_plan_normalizer_get(*f->owner,31,7,aliases[i])==PT_MIXED_PLAN_ALIAS);
        assert(pt_mixed_plan_normalizer_report(*f->owner,aliases[i])==PT_MIXED_PLAN_ALIAS);
        assert(pt_mixed_plan_normalizer_begin_in_workspace(f->workspace,f->capacity,
            &f->inputs,aliases[i])==refusal);
        assert(!memcmp(before,aliases[i],sizeof(before)));
    }
    assert(pt_mixed_plan_normalizer_get(*f->owner,31,7,(void *)((uint8_t *)f->workspace+pt_mixed_plan_normalizer_workspace_size()+8))==PT_MIXED_PLAN_ALIAS);
    assert(pt_mixed_plan_normalizer_begin_in_workspace(f->workspace,f->capacity,&f->inputs,f->owner)==PT_MIXED_PLAN_INVALID);
    assert(pt_mixed_plan_normalizer_step(*f->owner,31,7,0)==PT_MIXED_PLAN_INVALID);
    assert(pt_mixed_plan_normalizer_step(*f->owner,31,7,257)==PT_MIXED_PLAN_INVALID);
    f->project.channels.selected=15;assert(pt_mixed_plan_normalizer_step(*f->owner,31,7,1)==PT_MIXED_PLAN_PENDING);
    header=f->project;f->project.samples=(void *)(uintptr_t)1;f->project.events=(void *)(uintptr_t)1;
    assert(pt_mixed_plan_normalizer_step(*f->owner,32,7,1)==PT_MIXED_PLAN_STALE);
    assert(pt_mixed_plan_normalizer_get(*f->owner,31,7,f->output)==PT_MIXED_PLAN_STALE&&!memcmp(old,f->output,sizeof(*old)));
    assert(pt_mixed_plan_normalizer_report(*f->owner,&report)==PT_MIXED_PLAN_STALE);
    assert(pt_mixed_plan_normalizer_close(f->owner)&&!*f->owner);f->project=header;
    begin(f);header=f->project;f->project.samples=(void *)(uintptr_t)1;
    assert(pt_mixed_plan_normalizer_step(*f->owner,31,7,1)==PT_MIXED_PLAN_STALE);
    assert(pt_mixed_plan_normalizer_close(f->owner));f->project=header;
    f->inputs.context_count=2;f->contexts[1]=(struct pt_mixed_plan_span){extra,4096};
    {
        struct pt_mixed_plan_normalizer **slot=(void *)((uint8_t *)extra+4000);
        assert(pt_mixed_plan_normalizer_begin_in_workspace(f->workspace,f->capacity,&f->inputs,
            slot)==PT_MIXED_PLAN_ALIAS&&!memcmp(slot,(uint8_t[sizeof(void *)]){0},sizeof(void *)));
        begin(f);*slot=*f->owner;memcpy(old,f->output,sizeof(*old));header=f->project;
        f->project.samples=(void *)(uintptr_t)1;
        assert(pt_mixed_plan_normalizer_get(*f->owner,32,7,(void *)((uint8_t *)extra+4000))==PT_MIXED_PLAN_ALIAS);
        assert(pt_mixed_plan_normalizer_report(*f->owner,&report)==PT_MIXED_PLAN_PENDING);
        assert(pt_mixed_plan_normalizer_get(*f->owner,32,7,f->output)==PT_MIXED_PLAN_STALE&&!memcmp(old,f->output,sizeof(*old)));
        assert(!pt_mixed_plan_normalizer_close(slot));
        assert(pt_mixed_plan_normalizer_close(f->owner));*slot=NULL;f->project=header;
    }
    f->inputs.context_count=0;
    begin(f);assert(pt_mixed_plan_normalizer_get(*f->owner,31,8,f->output)==PT_MIXED_PLAN_STALE);
    assert(pt_mixed_plan_normalizer_close(f->owner));
    /* No stale headers are needed to invalidate immutable descriptor replacement. */
    begin(f);f->samples[0].pcm.capacity--;
    assert(pt_mixed_plan_normalizer_step(*f->owner,31,7,1)==PT_MIXED_PLAN_STALE);
    assert(pt_mixed_plan_normalizer_close(f->owner));f->samples[0].pcm.capacity++;
    /* Cancel each distinct construction phase plus first/mid/last finite search. */
    singleton(f,4,0);f->plan.action[0].gain[0]=32639;f->plan.action[0].gain[1]=32896;
    for(i=0;i<9;++i){unsigned target=i<6?0:i==6?1:i==7?8000:16449;
        begin(f);
        for(;;){assert(pt_mixed_plan_normalizer_report(*f->owner,&report)==PT_MIXED_PLAN_PENDING);
            if((i<6&&report.phase==(enum pt_mixed_plan_phase)i)||(i>=6&&report.candidates>=target))break;
            assert(pt_mixed_plan_normalizer_step(*f->owner,31,7,1)==PT_MIXED_PLAN_PENDING);
        }
        header=f->project;f->project.samples=(void *)(uintptr_t)1;
        f->project.extensions=(void *)(uintptr_t)1;
        assert(pt_mixed_plan_normalizer_close(f->owner)&&!*f->owner);f->project=header;
    }
    singleton(f,4,0);begin(f);assert(finish(f,256)==PT_MIXED_PLAN_READY);
    header=f->project;f->project.samples=(void *)(uintptr_t)1;
    assert(pt_mixed_plan_normalizer_close(f->owner)&&!*f->owner);f->project=header;
    free(extra);free(old);drop(f);puts("PAIRED NORMALIZER GUARDS PASS: complete unused/spare/context/workspace spans, poisoned former sources, selection/tags, every phase and bounded candidate cancellation");
}
static uint8_t *encoded(struct fixture *f,size_t *bytes)
{
    size_t used;uint8_t *p;
    assert(pt_project_size(&f->project,bytes)==PT_PROJECT_OK);p=malloc(*bytes);assert(p);
    assert(pt_project_encode(&f->project,p,*bytes,&used)==PT_PROJECT_OK&&used==*bytes);return p;
}
static void genuine_renderer_case(void)
{
    struct fixture *f=make(24,16,1,1);struct pt_render_options options={0};
    struct pt_render_sequence *sequence=NULL,*oracle=NULL;struct pt_mixed_preflight_setup *setup=NULL;
    struct pt_mixed_preflight *audit=NULL;struct pt_mixed_report gate;
    struct pt_render_lookahead *lookahead=calloc(1,sizeof(*lookahead));
    struct pt_render_plan *expected=calloc(1,sizeof(*expected));
    struct pt_render_interval interval,reference;enum pt_render_setup_result sr;enum pt_mixed_result mr;
    uint8_t *save,*after;size_t bytes,after_bytes;uint64_t absolute=0;unsigned i,ready=0,loops=0,windows=0;
    assert(lookahead&&expected);f->project.channels.count=2;
    f->events[0]=(struct pt_event){428,0,PT_NOTE_PERIOD,1,0,0,0,0};
    f->events[1]=(struct pt_event){428,0,PT_NOTE_PERIOD,2,0,0,0,0};
    f->events[2].effect=15;f->events[2].parameter=250;
    f->events[6].effect=15;f->events[6].parameter=0;
    options.rate=48000;options.bits=24;options.tracks=3;options.gain_q16=65536;
    options.tick_limit=100;options.frame_limit=100000;
    save=encoded(f,&bytes);pt_sampler_init(&f->sampler,&f->allocator,4*1024*1024);
    /* Establish before ANY sequence borrow, then enumerate real held extents. */
    for(i=0;i<3;++i){struct pt_pcm pcm;struct pt_sampler_storage_span spans[6];unsigned n,j;
        assert(pt_sampler_pin(&f->sampler,&f->project,i,f->sampler.generation,&pcm,&f->pins[i])==PT_EDIT_OK);
        assert(pcm.bits==24&&pcm.data[2]==257&&pcm.data[3]==-257);
        assert(pt_sampler_version_spans(f->pins[i],spans,6,&n));
        for(j=0;j<n;++j){unsigned index=f->inputs.context_count++;
            assert(index<32);f->contexts[index]=(struct pt_mixed_plan_span){spans[j].data,spans[j].bytes};}
    }
    f->inputs.generation=f->sampler.generation;
    assert(pt_mixed_preflight_setup_begin(&f->project,&options,NULL,&f->caps,&f->format,1,1,&f->allocator,
        f->inputs.revision,f->inputs.generation,&setup)==PT_RENDER_SETUP_PENDING);
    do{sr=pt_mixed_preflight_setup_step(setup,f->inputs.revision,f->inputs.generation,31);assert(++loops<10000);}while(sr==PT_RENDER_SETUP_PENDING);
    assert(sr==PT_RENDER_SETUP_READY);
    assert(pt_mixed_preflight_setup_transfer(&setup,f->inputs.revision,f->inputs.generation,&audit)==PT_RENDER_SETUP_READY&&!setup);
    loops=0;do{mr=pt_mixed_preflight_step(audit,&gate);assert(++loops<10000);}while(mr==PT_MIXED_PENDING);
    assert(mr==PT_MIXED_OK&&pt_mixed_preflight_take(audit,&sequence));pt_mixed_preflight_close(&audit);assert(!audit);
    assert(pt_render_sequence_open(&f->project,&options,&f->allocator,&oracle)==PT_RENDER_OK);
    /* Fixture-only parity: no production strict whole-song gate or producer. */
    do{
        uint32_t remaining;
        assert(pt_render_sequence_next(sequence,&interval)==PT_RENDER_OK);
        assert(pt_render_sequence_next(oracle,&reference)==PT_RENDER_OK&&!memcmp(&interval,&reference,sizeof(interval)));
        assert(pt_render_lookahead_begin(lookahead,sequence)==PT_RENDER_OK);
        ready=0;while(!ready)assert(pt_render_lookahead_step(lookahead,127,&f->plan,&ready)==PT_RENDER_OK);
        remaining=interval.frames;
        while(remaining){unsigned n=remaining>256?256:remaining;
            assert(pt_render_sequence_consume(sequence,n)==PT_RENDER_OK);
            assert(pt_render_sequence_consume(oracle,n)==PT_RENDER_OK);remaining-=n;}
        assert(pt_render_sequence_complete(oracle,expected)==PT_RENDER_OK);
        assert(f->plan.count==expected->count&&!memcmp(f->plan.action,expected->action,f->plan.count*sizeof(*expected->action)));
        f->inputs.frame=absolute+interval.frames;begin(f);
        if(f->inputs.frame==960)windows|=1;
        if(f->inputs.frame==1440)windows|=2;
        if(f->inputs.frame==1920)windows|=4;
        assert(finish(f,31)==PT_MIXED_PLAN_READY);
        assert(pt_mixed_plan_normalizer_get(*f->owner,f->inputs.revision,f->inputs.generation,f->output)==PT_MIXED_PLAN_READY);
        assert(f->output->frame==absolute+interval.frames);
        assert(pt_mixed_plan_normalizer_close(f->owner));memcpy(f->origins,f->output->next,sizeof(f->origins));
        assert(pt_render_lookahead_commit(lookahead)==PT_RENDER_OK);absolute+=interval.frames;
    }while(!interval.end);
    assert(absolute==1920&&absolute==gate.frames&&windows==7);
    assert(pt_render_sequence_rewind(sequence)==PT_RENDER_OK);pt_render_lookahead_cancel(lookahead);
    pt_render_sequence_close(sequence);pt_render_sequence_close(oracle);
    after=encoded(f,&after_bytes);assert(after_bytes==bytes&&!memcmp(save,after,bytes));
    free(after);free(save);free(expected);free(lookahead);drop(f);
    puts("PAIRED NORMALIZER RENDERER PASS: genuine established24-bit masters/setup/same audit sequence/lookahead parity, literal original frames and exact master-save bytes; fixture only");
}
int main(void)
{
    shape_cases();geometry_cases();rates_gains_and_origins();alias_stale_and_cancel();genuine_renderer_case();
    puts("PAIRED PLAN NORMALIZER PASS: geometry-only software, no pins/keys/queue/activation/device or timing authority");return 0;
}
