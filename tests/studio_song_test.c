#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "studio_internal.h"
static int32_t reference[300000];static unsigned used,owned,pins,refuse,attempt,fail_at,fail_pin;
static unsigned source_calls;
static struct {void *data;size_t bytes;} live[8];
static struct pt_pcm master;
static void *allocate(void *c,size_t n)
{void *p;unsigned i;(void)c;if(refuse || ++attempt==fail_at)return NULL;p=malloc(n);assert(p);
 for(i=0;i<8 && live[i].data;++i){}assert(i<8);live[i].data=p;live[i].bytes=n;++owned;return p;}
static void release(void *c,void *p)
{unsigned i;(void)c;assert(owned);for(i=0;i<8 && live[i].data!=p;++i){}assert(i<8);
 live[i].data=NULL;live[i].bytes=0;--owned;free(p);}
static int acquire(void *c,uint64_t key,uint64_t version,struct pt_pcm *p,void **token)
{(void)c;++source_calls;if(fail_pin || key!=1 || version!=1)return 0;*p=master;*token=&master;++pins;return 1;}
static void unpin(void *c,void *p) {(void)c;assert(p==&master && pins);--pins;}
static int capture(void *c,const struct pt_pcm *p,uint64_t offset)
{(void)c;assert(offset*2==used && used+p->frames*2<=300000);memcpy(reference+used,p->data,p->frames*2*sizeof(int32_t));used+=p->frames*2;return 1;}
#ifndef PT_SONG_FIRST_BLOCK
#define PT_SONG_FIRST_BLOCK 1
#endif
static void refused_outputs(struct pt_studio_song *song,void *alias)
{
    const struct pt_pcm *block=(const struct pt_pcm *)(uintptr_t)17;
    unsigned done=73,calls=source_calls;
    assert(pt_studio_song_prepare(song,(unsigned *)alias)==PT_RENDER_INVALID);
    assert(pt_studio_song_pull(song,17,(const struct pt_pcm **)alias,&done)==PT_RENDER_INVALID && done==73);
    assert(pt_studio_song_pull(song,17,&block,(unsigned *)alias)==PT_RENDER_INVALID && block==(const struct pt_pcm *)(uintptr_t)17);
    assert(source_calls==calls);
}
struct pending_sources {struct pt_pcm pcm[2];unsigned calls,pinned;};
static int pending_acquire(void *ctx,uint64_t key,uint64_t version,struct pt_pcm *pcm,void **token)
{
    struct pending_sources *source=ctx;++source->calls;
    if(!key || key>2 || version!=1)return 0;
    *pcm=source->pcm[key-1];*token=&source->pcm[key-1];++source->pinned;return 1;
}
static void pending_release(void *ctx,void *token)
{
    struct pending_sources *source=ctx;
    assert(source->pinned && (token==source->pcm || token==source->pcm+1));
    ++source->calls;--source->pinned;
}
static void pending_output_guard_case(const struct pt_allocator *allocator)
{
    static int32_t data[2][400],before[2][400];
    struct pending_sources source={{{0}},0,0};struct pt_studio_mix *mix;
    struct pt_studio_source provider={&source,pending_acquire,pending_release};
    struct pt_studio_note note={0};unsigned i,calls;
    for(i=0;i<400;++i) {data[0][i]=(int32_t)(65537+i);data[1][i]=(int32_t)(131073+i);}
    memcpy(before,data,sizeof(data));source.pcm[0]=(struct pt_pcm){data[0],400,8,48000,1,24};
    source.pcm[1]=(struct pt_pcm){data[1],400,8,48000,1,24};
    mix=pt_studio_open(allocator,&provider,1);assert(mix && owned==1);
    note.key=note.version=1;note.step=1ULL<<32;note.end=note.loop_end=8;note.loop=PT_VOICE_FORWARD;
    note.gain[0]=note.gain[1]=65536;
    assert(pt_studio_trigger(mix,0,&note)==PT_PCM_OK && source.pinned==1);
    assert(pt_studio_repeat(mix,0,2,1,0,8)==PT_PCM_OK && source.pinned==2);
    calls=source.calls;
    for(i=0;i<2;++i) {
        assert(!pt_studio_mix_output_disjoint(mix,data[i],sizeof(unsigned)));
        assert(!pt_studio_mix_output_disjoint(mix,data[i]+12,sizeof(void *)));
        assert(!pt_studio_mix_output_disjoint(mix,(uint8_t *)data[i]+sizeof(data[i])-1,sizeof(void *)));
    }
    assert(!pt_studio_mix_output_disjoint(mix,mix,sizeof(unsigned)));
    assert(pt_studio_mix_output_disjoint(mix,&calls,sizeof(calls)));
    assert(calls==source.calls && !memcmp(before,data,sizeof(data)));
    pt_studio_close(mix);assert(!source.pinned && !owned);
}
static void output_guard_cases(const struct pt_allocator *allocator,const struct pt_studio_source *provider)
{
    static struct pt_project p,saved_project;static struct pt_sample sample[2],saved_samples[2];
    static struct pt_event events[64*4],saved_events[64*4];static uint16_t orders[1]={0};
    static uint32_t slices[2]={2,6};static struct pt_extension extension;
    static union {uint64_t alignment;uint8_t bytes[32];} extension_data;
    static int32_t data[400],unused[400],saved_data[400],saved_unused[400],provider_data[400],saved_provider[400];
    struct pt_render_options options={0},saved_options;struct pt_studio_binding bindings[2],saved_bindings[2];
    struct pt_allocator a=*allocator;struct pt_studio_source source=*provider;
    unsigned bits,i,j,mode;
    memset(&p,0,sizeof(p));memset(sample,0,sizeof(sample));memset(events,0,sizeof(events));
    memset(&extension,0,sizeof(extension));memset(&extension_data,0x5a,sizeof(extension_data));
    pt_channels_init(&p.channels);p.samples=sample;p.sample_count=2;p.events=events;p.orders=orders;
    p.order_count=p.pattern_count=1;p.speed=1;p.bpm=125;
    extension.id=0x5a5a5a5a;extension.length=sizeof(extension_data.bytes);extension.data=extension_data.bytes;
    p.extensions=&extension;p.extension_count=1;
    events[0].kind=PT_NOTE_PERIOD;events[0].pitch=428;events[0].instrument=1;
    events[4].effect=15;events[4].parameter=0;
    options.rate=48000;options.bits=24;options.tracks=1;options.gain_q16=65536;
    options.tick_limit=100;options.frame_limit=10000;
    for(bits=8;bits<=24;bits+=8) {
        void *alias[12];struct pt_studio_song *song=NULL;unsigned ready=0,steps=0,calls;
        const struct pt_pcm *block=NULL;unsigned done=0;uint8_t required[255];
        union {const struct pt_pcm *pcm;unsigned done;} shared;
        for(i=0;i<400;++i) {data[i]=(int32_t)(100001+i);unused[i]=(int32_t)(200003+i);}
        for(i=0;i<8;++i)data[i]=(int32_t)((bits==8?3:bits==16?257:65537)+i*2);
        unused[0]=31;sample[0].pcm=(struct pt_pcm){data,400,8,48000,1,(uint8_t)bits};
        sample[0].volume=64;sample[0].loop=PT_LOOP_FORWARD;sample[0].loop_end=8;
        sample[0].slices=slices;sample[0].slice_count=2;
        sample[1].pcm=(struct pt_pcm){unused,400,1,48000,1,(uint8_t)bits};sample[1].volume=64;
        master=sample[0].pcm;
        bindings[0]=(struct pt_studio_binding){&sample[0].pcm,1,1};bindings[1]=(struct pt_studio_binding){&sample[1].pcm,2,1};
        memcpy(saved_data,data,sizeof(data));memcpy(saved_unused,unused,sizeof(unused));
        saved_project=p;memcpy(saved_samples,sample,sizeof(sample));memcpy(saved_events,events,sizeof(events));
        saved_options=options;memcpy(saved_bindings,bindings,sizeof(bindings));
        alias[0]=data;alias[1]=data+12;alias[2]=unused;alias[3]=unused+12;
        alias[4]=&p;alias[5]=sample;alias[6]=events;alias[7]=orders;alias[8]=slices;
        alias[9]=&extension;alias[10]=extension_data.bytes;alias[11]=(uint8_t *)data+sizeof(data)-1;
        for(mode=0;mode<2;++mode) {
            for(i=0;i<12;++i) {
                enum pt_render_result result=mode?pt_studio_song_open(&p,&options,&a,&source,bindings,2,(struct pt_studio_song **)alias[i]):
                    pt_studio_song_begin(&p,&options,&a,&source,bindings,2,(struct pt_studio_song **)alias[i]);
                assert(result==PT_RENDER_INVALID && !owned && !pins);
            }
            {void *arguments[4]={&options,&a,&source,bindings};
                for(i=0;i<4;++i)assert((mode?pt_studio_song_open(&p,&options,&a,&source,bindings,2,(struct pt_studio_song **)arguments[i]):
                    pt_studio_song_begin(&p,&options,&a,&source,bindings,2,(struct pt_studio_song **)arguments[i]))==PT_RENDER_INVALID && !owned);
            }
        }
        assert(!memcmp(data,saved_data,sizeof(data)) && !memcmp(unused,saved_unused,sizeof(unused)));
        assert(!memcmp(&p,&saved_project,sizeof(p)) && !memcmp(sample,saved_samples,sizeof(sample)) && !memcmp(events,saved_events,sizeof(events)));
        assert(!memcmp(&options,&saved_options,sizeof(options)) && !memcmp(bindings,saved_bindings,sizeof(bindings)));
        assert(pt_studio_song_begin(&p,&options,&a,&source,bindings,2,&song)==PT_RENDER_OK && owned==3);
        calls=source_calls;
        for(i=0;i<12;++i)refused_outputs(song,alias[i]);
        for(i=0;i<8;++i)if(live[i].data) {
            uint8_t before[128],tail[128];size_t n=live[i].bytes<sizeof(before)?live[i].bytes:sizeof(before);
            memcpy(before,live[i].data,n);memcpy(tail,(uint8_t *)live[i].data+live[i].bytes-n,n);
            refused_outputs(song,live[i].data);
            refused_outputs(song,(uint8_t *)live[i].data+live[i].bytes-1);
            assert(!memcmp(before,live[i].data,n) && !memcmp(tail,(uint8_t *)live[i].data+live[i].bytes-n,n));
        }
        shared.pcm=(const struct pt_pcm *)(uintptr_t)17;
        assert(pt_studio_song_pull(song,17,&shared.pcm,&shared.done)==PT_RENDER_INVALID && shared.pcm==(const struct pt_pcm *)(uintptr_t)17);
        assert(source_calls==calls && !pins);
        assert(pt_studio_song_pull(song,17,&block,&done)==PT_RENDER_INVALID && !block && !done);
        for(j=0;!ready && j<128;++j)assert(pt_studio_song_prepare(song,&ready)==PT_RENDER_OK);
        assert(ready && j<128 && !pins);
        for(i=0;i<12;++i)assert(!pt_studio_song_required(song,(uint8_t *)alias[i]));
        for(i=0;i<8;++i)if(live[i].data)assert(!pt_studio_song_required(song,live[i].data));
        assert(pt_studio_song_required(song,required) && required[0]==1 && !required[1] && !required[254]);
        for(i=0;i<12;++i)refused_outputs(song,alias[i]);
        /* Public providers may pin valid storage different from project PCM. */
        memcpy(provider_data,data,sizeof(data));memcpy(saved_provider,provider_data,sizeof(provider_data));master.data=provider_data;
        for(i=0;i<20 && !block;++i)assert(pt_studio_song_pull(song,17,&block,&done)==PT_RENDER_OK && !done);
        assert(block && pins==1 && i<20);calls=source_calls;
        refused_outputs(song,provider_data);refused_outputs(song,provider_data+12);
        refused_outputs(song,(void *)block);refused_outputs(song,block->data);
        assert(!pt_studio_song_required(song,(uint8_t *)provider_data));
        assert(source_calls==calls && !memcmp(provider_data,saved_provider,sizeof(provider_data)));
        for(steps=0;!done && steps<128;++steps)assert(pt_studio_song_pull(song,256,&block,&done)==PT_RENDER_OK);
        assert(done && steps<128 && !pins && owned==1);
        assert(!memcmp(data,saved_data,sizeof(data)) && !memcmp(unused,saved_unused,sizeof(unused)));
        ready=91;assert(pt_studio_song_prepare(song,&ready)==PT_RENDER_INVALID && ready==91);
        /* Former owners can be gone after Stop; guard uses controller only. */
        p.samples=(struct pt_sample *)(uintptr_t)1;p.events=(struct pt_event *)(uintptr_t)1;
        assert(pt_studio_song_output_disjoint(song,&ready,sizeof(ready)));
        assert(pt_studio_song_prepare(song,&ready)==PT_RENDER_INVALID && ready==91);
        pt_studio_song_close(song);assert(!owned);p=saved_project;master=sample[0].pcm;
        assert(pt_studio_song_begin(&p,&options,&a,&source,bindings,2,&song)==PT_RENDER_OK);
        pt_studio_song_stop(song);ready=92;
        assert(pt_studio_song_prepare(song,&ready)==PT_RENDER_INVALID && ready==92);
        pt_studio_song_close(song);assert(!owned && !pins);
        assert(pt_studio_song_open(&p,&options,&a,&source,bindings,2,&song)==PT_RENDER_OK);
        fail_pin=1;
        for(i=0;i<20;++i)if(pt_studio_song_pull(song,17,&block,&done)!=PT_RENDER_OK)break;
        assert(i<20 && done && !pins);ready=93;
        assert(pt_studio_song_prepare(song,&ready)==PT_RENDER_SAMPLE && ready==93);
        pt_studio_song_close(song);fail_pin=0;assert(!owned);
    }
}
int main(void)
{
    struct pt_project p={0};struct pt_sample sample;struct pt_event events[64*4];uint16_t orders[1]={0};
    struct pt_render_options o={0};struct pt_allocator a={NULL,allocate,release};
    struct pt_studio_source provider={NULL,acquire,unpin};struct pt_studio_binding binding;
    int32_t pcm[8]={1,257,-513,799,123,991,-777,27};unsigned mode,partition;
    memset(&sample,0,sizeof(sample));memset(events,0,sizeof(events));pt_channels_init(&p.channels);
    p.samples=&sample;p.sample_count=1;p.events=events;p.orders=orders;p.order_count=p.pattern_count=1;p.speed=3;p.bpm=125;
    sample.pcm=(struct pt_pcm){pcm,8,8,48000,1,24};sample.volume=64;sample.loop=PT_LOOP_FORWARD;sample.loop_end=8;
    master=sample.pcm;binding=(struct pt_studio_binding){&sample.pcm,1,1};
    events[0].kind=PT_NOTE_PERIOD;events[0].pitch=428;events[0].instrument=1;
    events[4].effect=15;events[4].parameter=131;
    events[8].effect=14;events[8].parameter=0xe1;
    events[12].effect=10;events[12].parameter=1;
    events[16].kind=PT_NOTE_PERIOD;events[16].pitch=320;
    events[20].effect=15;events[20].parameter=0;
    o.rate=48000;o.bits=24;o.tracks=1;o.gain_q16=65536;o.tick_limit=1000;o.frame_limit=1000000;
    for(mode=0;mode<3;++mode)for(partition=PT_SONG_FIRST_BLOCK;partition<=256;partition=partition==1?17:partition==17?256:257) {
        struct pt_studio_song *s=NULL;struct pt_render_report report;unsigned emitted=0,done=0;
        const struct pt_pcm *block;
        o.include_lead_in=mode==1;o.pattern_only=o.row_range=mode==2;o.row_first=2;o.row_end=5;
        used=0;assert(pt_render_stream(&p,&o,capture,NULL,NULL,NULL,&report)==PT_RENDER_OK);
        if(partition==17) {
            unsigned ready=0,steps=0;uint8_t required[255];memset(required,0x5a,sizeof(required));
            assert(pt_studio_song_begin(&p,&o,&a,&provider,&binding,1,&s)==PT_RENDER_OK && owned==3);
            assert(!pt_studio_song_required(s,required) && required[0]==0x5a);
            assert(pt_studio_song_pull(s,256,&block,&done)==PT_RENDER_INVALID && !block && !done);
            while(!ready) {assert(pt_studio_song_prepare(s,&ready)==PT_RENDER_OK && !pins && owned==3);assert(++steps<10000);}
            assert(pt_studio_song_required(s,required) && required[0]==1 && !required[1] && !required[254]);
        }else assert(pt_studio_song_open(&p,&o,&a,&provider,&binding,1,&s)==PT_RENDER_OK && owned==3);
        assert(pt_studio_song_pull(s,257,&block,&done)==PT_RENDER_INVALID);
        while(!done) {
            assert(pt_studio_song_pull(s,partition,&block,&done)==PT_RENDER_OK);
            if(block) {assert(emitted+block->frames*2<=used);assert(!memcmp(block->data,reference+emitted,block->frames*2*sizeof(int32_t)));emitted+=block->frames*2;}
        }
        assert(emitted==used && emitted==report.frames*2 && !pins && owned==1);
        pt_studio_song_close(s);assert(!owned);
    }
    {struct pt_studio_song *s=NULL;const struct pt_pcm *block;unsigned done,i;
        for(i=1;i<=3;++i) {attempt=0;fail_at=i;
            assert(pt_studio_song_open(&p,&o,&a,&provider,&binding,1,&s)==PT_RENDER_MEMORY && !s && !owned && !pins);
        }
        fail_at=0;
        assert(pt_studio_song_open(&p,&o,&a,&provider,&binding,1,&s)==PT_RENDER_OK);
        for(i=0;i<30 && !pins;++i)assert(pt_studio_song_pull(s,17,&block,&done)==PT_RENDER_OK);
        assert(pins);pt_studio_song_stop(s);assert(!pins && owned==1);
        assert(pt_studio_song_pull(s,17,&block,&done)==PT_RENDER_OK && done && !block);
        pt_studio_song_stop(s);pt_studio_song_close(s);assert(!owned);
        assert(pt_studio_song_open(&p,&o,&a,&provider,&binding,1,&s)==PT_RENDER_OK);
        fail_pin=1;
        for(i=0;i<30;++i)if(pt_studio_song_pull(s,256,&block,&done)!=PT_RENDER_OK)break;
        assert(i<30 && done && !block && !pins && owned==1);
        assert(pt_studio_song_pull(s,256,&block,&done)==PT_RENDER_SAMPLE);
        pt_studio_song_close(s);assert(!owned);fail_pin=0;
        master.bits=8; /* Arbitrary public provider returns invalid sample values. */
        assert(pt_studio_song_open(&p,&o,&a,&provider,&binding,1,&s)==PT_RENDER_OK);
        for(i=0;i<30;++i)if(pt_studio_song_pull(s,256,&block,&done)!=PT_RENDER_OK)break;
        assert(i<30 && done && !block && !pins && owned==1);
        pt_studio_song_close(s);assert(!owned);master=sample.pcm;
    }
    output_guard_cases(&a,&provider);
    pending_output_guard_case(&a);
    puts("SONG SESSION PASS: reference audio, tempo/delay, pre-roll, partition invariance, protocol, allocation refusal and master/output alias guards");return 0;
}
