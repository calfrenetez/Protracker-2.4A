#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "project.h"

#define VALUES 10032U
struct fixture {
    struct pt_project p;
    struct pt_sample sample;
    struct pt_event events[256];
    struct pt_extension extensions[2];
    union {uint16_t orders[2];uint32_t caps;} order;
    uint32_t slices[4];
    union {uint8_t bytes[16];uint32_t caps;} extra;
    union {
        int32_t values[VALUES];
        struct {int32_t prefix[VALUES-1];uint32_t caps;} tail;
        struct pt_project_validation job;
    } master;
};
static struct fixture f,before;
static struct pt_project_validation job,saved;
static const uint32_t revision=17,generation=29;

static void fixture(unsigned bits,unsigned channels,unsigned values)
{
    unsigned i;int32_t high=((int32_t)1<<(bits-1))-1;
    memset(&f,0,sizeof(f));pt_channels_init(&f.p.channels);
    f.p.orders=f.order.orders;f.p.order_count=2;f.p.pattern_count=1;
    f.p.events=f.events;f.p.samples=&f.sample;f.p.sample_count=1;
    f.p.bpm=125;f.p.speed=6;f.p.mode=PT_MODE_STUDIO;f.p.midi_flags=3;
    f.p.channels.track[3].route=PT_MIDI;
    f.p.channels.track[2].route=PT_AMIGUS;
    f.sample.pcm=(struct pt_pcm){f.master.values,VALUES,values/channels,48000,
        (uint8_t)channels,(uint8_t)bits};
    f.sample.volume=64;f.sample.finetune=-8;f.sample.interpolation=1;
    f.sample.loop=PT_LOOP_CROSSFADE;f.sample.loop_end=values/channels;
    f.sample.crossfade=2;f.sample.slices=f.slices;f.sample.slice_count=3;
    f.slices[0]=0;f.slices[1]=2;f.slices[2]=values/channels-1;
    for(i=0;i<values;++i)f.master.values[i]=i%4==0?high:i%4==1?-high-1:i%4==2?1:-1;
    /* Guard the whole declared capacity, but never validate padding as audio. */
    for(;i<VALUES;++i)f.master.values[i]=INT32_MAX;
    f.events[0]=(struct pt_event){428,3,PT_NOTE_PERIOD,1,14,0xc3,0,0};
    f.events[254]=(struct pt_event){127,0,PT_NOTE_MIDI,1,0,0,127,1};
    f.events[255].kind=PT_NOTE_OFF;
    f.p.extensions=f.extensions;f.p.extension_count=2;
    f.extensions[0]=(struct pt_extension){0x54455354UL,16,3,f.extra.bytes};
    f.extensions[1]=(struct pt_extension){0x54455332UL,0,7,NULL};
    for(i=0;i<16;++i)f.extra.bytes[i]=(uint8_t)(i+1);
}
static enum pt_project_result finish(unsigned work,size_t *total)
{
    enum pt_project_result r;unsigned calls=0;uint32_t caps=0xdeadbeef;
    *total=0;
    do {
        assert(pt_project_validation_get(&job,revision,generation,NULL)==PT_PROJECT_PENDING);
        assert(pt_project_validation_get(&job,revision,generation,&caps)==PT_PROJECT_PENDING);
        assert(caps==0xdeadbeef);
        r=pt_project_validation_step(&job,revision,generation,work);
        assert(job.last_work<=work);*total+=job.last_work;
        assert(++calls<=11000);
    } while(r==PT_PROJECT_PENDING);
    return r;
}
static void parity_and_bounds(void)
{
    const unsigned bits[]={8,16,24},work[]={1,7,4096};unsigned b,c,w;size_t total;
    assert(sizeof(job)<8192);
    for(b=0;b<3;++b)for(c=1;c<=2;++c)for(w=0;w<3;++w) {
        uint32_t oracle=0,actual=0;
        fixture(bits[b],c,10000);memcpy(&before,&f,sizeof(f));
        assert(pt_project_validate(&f.p,&oracle)==PT_PROJECT_OK);
        assert(pt_project_validation_begin(&job,&f.p,revision,generation)==PT_PROJECT_OK);
        assert(pt_project_validation_get(&job,revision,generation,NULL)==PT_PROJECT_PENDING);
        assert(finish(work[w],&total)==PT_PROJECT_OK);
        assert(total==4+2+1+10000+3+256+2);
        assert(pt_project_validation_get(&job,revision,generation,&actual)==PT_PROJECT_OK);
        assert(actual==oracle && (actual&PT_CAP_SLICES) && (actual&PT_CAP_CROSSFADE));
        assert(pt_project_validation_step(&job,revision,generation,1)==PT_PROJECT_OK);
        assert(job.last_work==0 && !memcmp(&f,&before,sizeof(f)));
        pt_project_validation_cancel(&job);
    }
}
static void invalid_case(unsigned which)
{
    size_t total;uint32_t caps=0x1234abcd;enum pt_project_result r;
    fixture(24,2,32);
    switch(which) {
    case 0:memset(f.p.title,'x',sizeof(f.p.title));break;
    case 1:f.p.channels.track[15].route=3;break;
    case 2:memset(f.p.midi_output[3],'x',64);break;
    case 3:f.order.orders[1]=1;break;
    case 4:f.sample.volume=65;break;
    case 5:f.sample.loop_start=f.sample.loop_end;break;
    case 6:f.sample.pcm.frames=VALUES;break;
    case 7:f.master.values[31]=8388608;break;
    case 8:f.slices[2]=f.sample.pcm.frames;break;
    case 9:f.slices[2]=f.slices[1];break;
    case 10:f.events[255].kind=PT_NOTE_PERIOD;f.events[255].pitch=0;break;
    case 11:f.events[255].instrument=2;break;
    case 12:f.events[255].slice=4;f.events[255].instrument=1;break;
    case 13:f.events[255].velocity=1;break;
    case 14:f.extensions[1].id=0x48454144UL;break;
    default:assert(0);break;
    }
    assert(pt_project_validate(&f.p,NULL)==PT_PROJECT_INVALID);
    memcpy(&before,&f,sizeof(f));memset(&job,0xa5,sizeof(job));memcpy(&saved,&job,sizeof(job));
    r=pt_project_validation_begin(&job,&f.p,revision,generation);
    if(r==PT_PROJECT_OK) {
        assert(finish(7,&total)==PT_PROJECT_INVALID);memcpy(&saved,&job,sizeof(job));
        assert(pt_project_validation_get(&job,revision,generation,&caps)==PT_PROJECT_INVALID);
        assert(pt_project_validation_step(&job,revision,generation,4096)==PT_PROJECT_INVALID);
        assert(!memcmp(&saved,&job,sizeof(job)));
    } else {
        assert(r==PT_PROJECT_INVALID && !memcmp(&saved,&job,sizeof(job)));
    }
    assert(caps==0x1234abcd && !memcmp(&f,&before,sizeof(f)));
    pt_project_validation_cancel(&job);
}
static void refusals_and_stale(void)
{
    uint32_t caps=0x55aa1234;size_t total;unsigned i;
    for(i=0;i<15;++i)invalid_case(i);
    fixture(16,1,32);
    assert(pt_project_validation_begin(&job,&f.p,revision,generation)==PT_PROJECT_OK);
    memcpy(&saved,&job,sizeof(job));
    assert(pt_project_validation_step(&job,revision,generation,0)==PT_PROJECT_INVALID);
    assert(pt_project_validation_step(&job,revision,generation,4097)==PT_PROJECT_INVALID);
    assert(!memcmp(&saved,&job,sizeof(job)));
    assert(pt_project_validation_step(&job,revision+1,generation,1)==PT_PROJECT_STALE);
    assert(pt_project_validation_get(&job,revision,generation+1,&caps)==PT_PROJECT_STALE);
    assert(caps==0x55aa1234 && !memcmp(&saved,&job,sizeof(job)));
    /* Changed table is refused by the fixed header before any former walk. */
    f.p.samples=NULL;f.p.events=NULL;f.p.extensions=NULL;
    assert(pt_project_validation_step(&job,revision,generation,1)==PT_PROJECT_STALE);
    assert(pt_project_validation_get(&job,revision,generation,&caps)==PT_PROJECT_STALE);
    assert(!memcmp(&saved,&job,sizeof(job)));
    pt_project_validation_cancel(&job);
    assert(pt_project_validation_get(&job,revision,generation,NULL)==PT_PROJECT_INVALID);
    assert(pt_project_validation_step(&job,revision,generation,1)==PT_PROJECT_INVALID);
    fixture(16,1,32);
    assert(pt_project_validation_begin(&job,&f.p,revision,generation)==PT_PROJECT_OK);
    assert(pt_project_validation_step(&job,revision,generation,7)==PT_PROJECT_PENDING);
    memcpy(&saved,&job,sizeof(job));f.sample.pcm.rate=22050;
    assert(pt_project_validation_step(&job,revision,generation,1)==PT_PROJECT_STALE);
    assert(!memcmp(&saved,&job,sizeof(job)));f.sample.pcm.rate=48000;
    f.p.channels.selected=3;
    assert(finish(4096,&total)==PT_PROJECT_OK);
    f.p.channels.selected=4;
    assert(pt_project_validation_get(&job,revision,generation,NULL)==PT_PROJECT_STALE);
    f.p.channels.selected=0;f.p.bpm=126;
    assert(pt_project_validation_get(&job,revision,generation,&caps)==PT_PROJECT_STALE);
    pt_project_validation_cancel(&job);
    /* Genuine former storage may be freed once tags/header have changed; stale
     * queries and cancellation must not revisit it. */
    {
        struct pt_sample *old=malloc(sizeof(*old));assert(old);
        fixture(16,1,32);memcpy(old,&f.sample,sizeof(*old));f.p.samples=old;
        assert(pt_project_validation_begin(&job,&f.p,revision,generation)==PT_PROJECT_OK);
        assert(pt_project_validation_step(&job,revision,generation,7)==PT_PROJECT_PENDING);
        memcpy(&saved,&job,sizeof(job));f.p.samples=NULL;free(old);
        assert(pt_project_validation_step(&job,revision,generation,1)==PT_PROJECT_STALE);
        assert(pt_project_validation_get(&job,revision,generation,&caps)==PT_PROJECT_STALE);
        assert(!memcmp(&job,&saved,sizeof(job)));pt_project_validation_cancel(&job);
    }
}
static void cancellation(void)
{
    /* Every body domain, plus complete-but-unpublished, is independently closed. */
    const unsigned cuts[]={0,1,5,7,20,40,43,100,298,300};unsigned k;
    for(k=0;k<sizeof(cuts)/sizeof(cuts[0]);++k) {
        unsigned i;size_t total;
        fixture(8,1,32);
        assert(pt_project_validation_begin(&job,&f.p,revision,generation)==PT_PROJECT_OK);
        for(i=0;i<cuts[k];++i) {
            enum pt_project_result r=pt_project_validation_step(&job,revision,generation,1);
            assert(r==PT_PROJECT_PENDING || r==PT_PROJECT_OK);
        }
        if(k+1==sizeof(cuts)/sizeof(cuts[0]))assert(finish(4096,&total)==PT_PROJECT_OK);
        memcpy(&before,&f,sizeof(f));f.p.samples=NULL;f.p.events=NULL;f.p.orders=NULL;
        pt_project_validation_cancel(&job);
        assert(!job.project && !job.phase);
        assert(pt_project_validation_get(&job,revision,generation,NULL)==PT_PROJECT_INVALID);
        assert(pt_project_validation_step(&job,revision,generation,1)==PT_PROJECT_INVALID);
        /* Only the deliberate header replacement differs. */
        before.p.samples=NULL;before.p.events=NULL;before.p.orders=NULL;
        assert(!memcmp(&f,&before,sizeof(f)));
    }
}
static void output_guards(void)
{
    uint32_t *out[8];size_t total;unsigned i;
    fixture(24,2,32);memcpy(&before,&f,sizeof(f));
    assert(pt_project_validation_begin(&f.master.job,&f.p,revision,generation)==PT_PROJECT_ALIAS);
    assert(!memcmp(&f,&before,sizeof(f)));
    memset(&job,0xa5,sizeof(job));memcpy(&saved,&job,sizeof(job));
    assert(pt_project_validation_begin(&job,(const struct pt_project *)(UINTPTR_MAX-7),revision,generation)==PT_PROJECT_ALIAS);
    assert(pt_project_validation_begin((struct pt_project_validation *)(UINTPTR_MAX-7),&f.p,revision,generation)==PT_PROJECT_ALIAS);
    assert(pt_project_validation_step((struct pt_project_validation *)(UINTPTR_MAX-7),revision,generation,1)==PT_PROJECT_ALIAS);
    assert(pt_project_validation_get((struct pt_project_validation *)(UINTPTR_MAX-7),revision,generation,NULL)==PT_PROJECT_ALIAS);
    pt_project_validation_cancel((struct pt_project_validation *)(UINTPTR_MAX-7));
    assert(!memcmp(&saved,&job,sizeof(job)));
    assert(pt_project_validation_begin(&job,&f.p,revision,generation)==PT_PROJECT_OK);
    assert(finish(4096,&total)==PT_PROJECT_OK);
    out[0]=&f.master.tail.caps;out[1]=&f.sample.pcm.rate;out[2]=&f.p.midi_flags;
    out[3]=f.slices+2;out[4]=&f.extra.caps;out[5]=&f.order.caps;
    out[6]=&f.extensions[1].id;out[7]=&job.capabilities;
    memcpy(&saved,&job,sizeof(job));memcpy(&before,&f,sizeof(f));
    for(i=0;i<8;++i) {
        assert(pt_project_validation_get(&job,revision,generation,out[i])==PT_PROJECT_ALIAS);
        assert(!memcmp(&f,&before,sizeof(f)) && !memcmp(&job,&saved,sizeof(job)));
    }
    assert(pt_project_validation_get(&job,revision,generation,(uint32_t *)(UINTPTR_MAX-1))==PT_PROJECT_ALIAS);
    assert(!memcmp(&f,&before,sizeof(f)) && !memcmp(&job,&saved,sizeof(job)));
    pt_project_validation_cancel(&job);
    fixture(8,1,32);f.sample.pcm.capacity=SIZE_MAX;
    memcpy(&before,&f,sizeof(f));memset(&job,0xa5,sizeof(job));memcpy(&saved,&job,sizeof(job));
    assert(pt_project_validation_begin(&job,&f.p,revision,generation)==PT_PROJECT_ALIAS);
    assert(!memcmp(&before,&f,sizeof(f)) && !memcmp(&job,&saved,sizeof(job)));
    fixture(8,1,32);f.sample.pcm.data=(int32_t *)(UINTPTR_MAX-7);
    assert(pt_project_validation_begin(&job,&f.p,revision,generation)==PT_PROJECT_ALIAS);
    assert(!memcmp(&job,&saved,sizeof(job)));
    fixture(8,1,32);f.p.samples=(struct pt_sample *)(UINTPTR_MAX-7);
    assert(pt_project_validation_begin(&job,&f.p,revision,generation)==PT_PROJECT_ALIAS);
    assert(!memcmp(&job,&saved,sizeof(job)));
    fixture(8,1,32);f.extensions[1].length=1;f.extensions[1].data=NULL;
    assert(pt_project_validation_begin(&job,&f.p,revision,generation)==PT_PROJECT_ALIAS);
    assert(!memcmp(&job,&saved,sizeof(job)));
}
static void empty_and_limits(void)
{
    uint32_t caps=0xabcdef01;size_t total;
    fixture(8,1,32);f.sample.pcm.data=NULL;f.sample.pcm.frames=0;f.sample.pcm.capacity=0;
    f.sample.loop=PT_LOOP_NONE;f.sample.loop_end=0;f.sample.crossfade=0;
    f.sample.slice_count=0;f.sample.slices=NULL;f.events[0].slice=0;
    assert(pt_project_validate(&f.p,NULL)==PT_PROJECT_OK);
    assert(pt_project_validation_begin(&job,&f.p,revision,generation)==PT_PROJECT_OK);
    assert(finish(4096,&total)==PT_PROJECT_OK);
    assert(pt_project_validation_get(&job,revision,generation,&caps)==PT_PROJECT_OK);
    pt_project_validation_cancel(&job);f.sample.pcm.capacity=1;
    memcpy(&before,&f,sizeof(f));memcpy(&saved,&job,sizeof(job));
    /* Public workspace publication adds the same full-storage boundary as caps. */
    assert(pt_project_validate(&f.p,NULL)==PT_PROJECT_OK);
    assert(pt_project_validate(&f.p,&caps)==PT_PROJECT_ALIAS);
    assert(pt_project_validation_begin(&job,&f.p,revision,generation)==PT_PROJECT_ALIAS);
    assert(!memcmp(&before,&f,sizeof(f)) && !memcmp(&saved,&job,sizeof(job)));
    f.p.extension_count=4091;
    assert(pt_project_validation_begin(&job,&f.p,revision,generation)==PT_PROJECT_INVALID);
    assert(!memcmp(&saved,&job,sizeof(job)));
}
static void maximum_metadata(void)
{
    static struct pt_sample samples[PT_PROJECT_SAMPLES];
    static struct pt_extension extensions[4090];
    static struct pt_event events[PT_PROJECT_PATTERNS*PT_PROJECT_ROWS*PT_CHANNEL_LIMIT];
    static uint16_t orders[PT_PROJECT_ORDERS];
    unsigned i;size_t total;uint32_t caps=0,oracle=0;
    fixture(8,1,32);memset(samples,0,sizeof(samples));memset(extensions,0,sizeof(extensions));
    memset(events,0,sizeof(events));memset(orders,0,sizeof(orders));
    assert(pt_channels_resize(&f.p.channels,16)==PT_CHANNEL_OK);
    f.p.samples=samples;f.p.sample_count=PT_PROJECT_SAMPLES;
    f.p.extensions=extensions;f.p.extension_count=4090;
    f.p.events=events;f.p.pattern_count=PT_PROJECT_PATTERNS;
    f.p.orders=orders;f.p.order_count=PT_PROJECT_ORDERS;
    for(i=0;i<PT_PROJECT_SAMPLES;++i) {
        samples[i].pcm.bits=8;samples[i].pcm.channels=1;samples[i].pcm.rate=48000;
    }
    assert(pt_project_validate(&f.p,&oracle)==PT_PROJECT_OK);
    assert(pt_project_validation_begin(&job,&f.p,revision,generation)==PT_PROJECT_OK);
    assert(finish(4096,&total)==PT_PROJECT_OK);
    assert(total==16+256+255+262144+4090);
    assert(pt_project_validation_get(&job,revision,generation,&caps)==PT_PROJECT_OK && caps==oracle);
    pt_project_validation_cancel(&job);
}
int main(void)
{
    parity_and_bounds();refusals_and_stale();cancellation();output_guards();empty_and_limits();maximum_metadata();
    printf("PROJECT VALIDATION WORKSPACE: %lu bytes; maximum work %u\n",(unsigned long)sizeof(job),PT_PROJECT_VALIDATION_WORK_MAX);
    puts("PROJECT VALIDATION PASS: bounded values, semantic parity, stale/alias/cancel preservation");
    return 0;
}
