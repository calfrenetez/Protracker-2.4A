#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "project.h"

static uint8_t encoded[65536], rewritten[65536], damaged[65536];
static struct pt_event events[2048], decoded_events[2048];
static int32_t pcm[24]={-128,0,1,127,-32768,1,32767,0,-8388608,1,-1,8388607}, decoded_pcm[24];
static uint32_t slices[2]={0,2}, decoded_slices[8];
static struct pt_sample samples[3], decoded_samples[3];
static uint16_t orders[2]={0,1}, decoded_orders[2];
static struct pt_extension ext[1], decoded_ext[1];
static uint8_t opaque[5]={1,9,24,255,0},decoded_opaque[8];

static void fixture(struct pt_project *p)
{
    unsigned i;
    memset(p,0,sizeof(*p));strcpy(p->title,"PT24G synthetic project");pt_channels_init(&p->channels);
    assert(pt_channels_resize(&p->channels,16)==PT_CHANNEL_OK);
    assert(pt_channels_route(&p->channels,15,PT_MIDI)==PT_CHANNEL_OK);
    p->channels.selected=15;strcpy(p->channels.track[15].name,"External");
    strcpy(p->midi_input,"Input cluster");strcpy(p->midi_output[15],"Output cluster");
    p->bpm=125;p->speed=6;p->mode=PT_MODE_STUDIO;p->order_count=2;p->pattern_count=2;p->sample_count=3;
    p->orders=orders;p->events=events;p->samples=samples;
    for(i=0;i<3;++i) {
        struct pt_sample *s=&samples[i];memset(s,0,sizeof(*s));strcpy(s->name,"Synthetic PCM");
        s->pcm.data=pcm+i*4;s->pcm.capacity=4;s->pcm.frames=i==2?2:4;s->pcm.rate=48000;
        s->pcm.channels=i==2?2:1;s->pcm.bits=(uint8_t)(8+i*8);s->volume=64;s->finetune=-8;
    }
    samples[0].slices=slices;samples[0].slice_count=2;samples[0].loop=PT_LOOP_CROSSFADE;
    samples[0].loop_end=4;samples[0].crossfade=2;samples[1].loop=PT_LOOP_PINGPONG;samples[1].loop_end=4;
    memset(events,0,sizeof(events));events[0].kind=PT_NOTE_PERIOD;events[0].pitch=428;events[0].instrument=1;events[0].slice=2;
    events[15].kind=PT_NOTE_MIDI;events[15].pitch=0;events[15].instrument=2;events[15].flags=1;events[15].velocity=127;
    events[31].kind=PT_NOTE_OFF;events[47].effect=14;events[47].parameter=0xc3;
    ext[0].id=0x54455354;ext[0].version=17;ext[0].length=5;ext[0].data=opaque;
    p->extensions=ext;p->extension_count=1;
}
static struct pt_project_storage storage(void)
{
    struct pt_project_storage s;
    memset(&s,0,sizeof(s));s.orders=decoded_orders;s.order_capacity=2;s.events=decoded_events;s.event_capacity=2048;
    s.samples=decoded_samples;s.sample_capacity=3;s.pcm=decoded_pcm;s.pcm_capacity=24;
    s.slices=decoded_slices;s.slice_capacity=8;s.extensions=decoded_ext;s.extension_capacity=1;
    s.extension_data=decoded_opaque;s.extension_bytes=8;return s;
}
static uint32_t get32(const uint8_t *p)
{ return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3]; }
static void put32(uint8_t *p,uint32_t n)
{ p[0]=(uint8_t)(n>>24);p[1]=(uint8_t)(n>>16);p[2]=(uint8_t)(n>>8);p[3]=(uint8_t)n; }
static void repair(uint8_t *p,size_t n)
{
    size_t i;unsigned j;uint32_t crc=0xffffffffUL;memset(p+20,0,4);
    for(i=0;i<n;++i) { crc^=p[i];for(j=0;j<8;++j)crc=(crc>>1)^((crc&1)?0xedb88320UL:0); }
    put32(p+20,crc^0xffffffffUL);
}
static size_t chunk(const uint8_t *p,uint32_t tag)
{
    size_t pos=32;unsigned i;
    for(i=0;i<get32(p+24);++i) {size_t n=get32(p+pos+8);if(get32(p+pos)==tag)return pos;pos+=12+n+((4-(n&3))&3);}
    assert(0);return 0;
}
/* Narrow shared coverage: native project-stream calls this without replaying
 * the historical full truncation matrix in project_baseline_main. */
struct project_alias_fixture {
    struct pt_project project;
    struct pt_event events[256];
    struct pt_sample sample;
    struct pt_extension extension;
    uint16_t orders[2];uint32_t markers[2];uint8_t opaque[16];
    union {size_t size;uint32_t caps;int32_t values[1024];} master;
};
static struct project_alias_fixture alias_source,alias_before;
static uint8_t alias_encoded[8192],alias_encoded_before[8192],alias_gold[8192],alias_streamed[8192];
struct alias_sink {size_t bytes;unsigned calls;};
static int alias_collect(void *context,const uint8_t *data,size_t n)
{
    struct alias_sink *s=context;
    assert(n<=1024 && n<=sizeof(alias_streamed)-s->bytes);
    memcpy(alias_streamed+s->bytes,data,n);s->bytes+=n;++s->calls;return 1;
}
static void alias_fixture(unsigned bits,unsigned channels)
{
    struct pt_project *p=&alias_source.project;struct pt_sample *s=&alias_source.sample;unsigned i;
    memset(&alias_source,0,sizeof(alias_source));pt_channels_init(&p->channels);
    p->orders=alias_source.orders;p->events=alias_source.events;p->samples=s;
    p->order_count=2;p->pattern_count=1;p->sample_count=1;p->bpm=125;p->speed=6;
    p->extensions=&alias_source.extension;p->extension_count=1;
    s->pcm=(struct pt_pcm){alias_source.master.values,1024,4/channels,48000,(uint8_t)channels,(uint8_t)bits};s->volume=64;
    s->slices=alias_source.markers;s->slice_count=2;alias_source.markers[1]=1;
    s->loop=PT_LOOP_FORWARD;s->loop_end=s->pcm.frames;
    alias_source.master.values[0]=bits==8?17:257;
    alias_source.master.values[1]=bits==8?-93:-513;
    alias_source.master.values[2]=bits==24?0x123457:30;
    alias_source.master.values[3]=bits==24?-0x234569:-77;
    /* Capacity padding deliberately exceeds every declared precision: guards
     * must protect its bytes without extending validation into those values. */
    for(i=4;i<1024;++i)alias_source.master.values[i]=(int32_t)(0x5ac30000UL+i);
    alias_source.extension=(struct pt_extension){0x54455354,16,17,alias_source.opaque};
    for(i=0;i<sizeof(alias_source.opaque);++i)alias_source.opaque[i]=(uint8_t)(17+i);
}
static void alias_outputs_refuse(void *location,size_t n)
{
    struct pt_project *p=&alias_source.project;struct alias_sink sink={0,0};size_t written=123;
    alias_before=alias_source;memset(alias_encoded,0xa5,sizeof(alias_encoded));
    memcpy(alias_encoded_before,alias_encoded,sizeof(alias_encoded));
    assert(pt_project_validate(p,(uint32_t *)location)==PT_PROJECT_ALIAS);
    assert(!memcmp(&alias_source,&alias_before,sizeof(alias_source)));
    assert(pt_project_size(p,(size_t *)location)==PT_PROJECT_ALIAS);
    assert(!memcmp(&alias_source,&alias_before,sizeof(alias_source)));
    assert(pt_project_encode(p,alias_encoded,sizeof(alias_encoded),(size_t *)location)==PT_PROJECT_ALIAS);
    assert(!memcmp(&alias_source,&alias_before,sizeof(alias_source)) && !memcmp(alias_encoded,alias_encoded_before,sizeof(alias_encoded)));
    assert(pt_project_encode(p,location,n,&written)==PT_PROJECT_ALIAS && written==123);
    assert(!memcmp(&alias_source,&alias_before,sizeof(alias_source)));
    assert(pt_project_stream(p,alias_collect,&sink,(size_t *)location)==PT_PROJECT_ALIAS && !sink.calls && !sink.bytes);
    assert(!memcmp(&alias_source,&alias_before,sizeof(alias_source)));
}
static void alias_bad_spans_refuse(void)
{
    struct pt_project *p=&alias_source.project;struct alias_sink sink={0,0};size_t n=123,w=456;uint32_t caps=789;
    alias_before=alias_source;memset(alias_encoded,0xa5,sizeof(alias_encoded));
    memcpy(alias_encoded_before,alias_encoded,sizeof(alias_encoded));
    /* NULL-caps validation still validates only the existing readable values. */
    assert(pt_project_validate(p,NULL)==PT_PROJECT_OK);
    assert(pt_project_validate(p,&caps)==PT_PROJECT_ALIAS && caps==789);
    assert(pt_project_size(p,&n)==PT_PROJECT_ALIAS && n==123);
    assert(pt_project_encode(p,alias_encoded,sizeof(alias_encoded),&w)==PT_PROJECT_ALIAS && w==456);
    assert(!memcmp(alias_encoded,alias_encoded_before,sizeof(alias_encoded)));
    assert(pt_project_stream(p,alias_collect,&sink,&w)==PT_PROJECT_ALIAS && w==456 && !sink.calls);
    assert(!memcmp(&alias_source,&alias_before,sizeof(alias_source)));
}
static void project_output_alias_cases(void)
{
    unsigned bits,channels,i;size_t n,w;uint32_t caps;struct alias_sink sink;
    for(bits=8;bits<=24;bits+=8)for(channels=1;channels<=2;++channels) {
        struct pt_project *p;void *locations[19];unsigned k=0;uint32_t expected;
        alias_fixture(bits,channels);p=&alias_source.project;
        expected=PT_CAP_SLICES|(bits==16?PT_CAP_16BIT:bits==24?PT_CAP_24BIT:0)|(channels==2?PT_CAP_STEREO:0);
        assert(pt_project_validate(p,&caps)==PT_PROJECT_OK && caps==expected);
        assert(pt_project_size(p,&n)==PT_PROJECT_OK && n<sizeof(alias_gold));
        assert(pt_project_encode(p,alias_gold,sizeof(alias_gold),&w)==PT_PROJECT_OK && w==n);
        locations[k++]=p;locations[k++]=(uint8_t *)p+sizeof(*p)-1;
        locations[k++]=alias_source.events;locations[k++]=(uint8_t *)alias_source.events+sizeof(alias_source.events)-1;
        locations[k++]=&alias_source.sample;locations[k++]=(uint8_t *)&alias_source.sample+sizeof(alias_source.sample)-1;
        locations[k++]=alias_source.orders;locations[k++]=(uint8_t *)alias_source.orders+sizeof(alias_source.orders)-1;
        locations[k++]=alias_source.markers;locations[k++]=(uint8_t *)alias_source.markers+sizeof(alias_source.markers)-1;
        locations[k++]=&alias_source.extension;locations[k++]=(uint8_t *)&alias_source.extension+sizeof(alias_source.extension)-1;
        locations[k++]=alias_source.opaque;locations[k++]=alias_source.opaque+sizeof(alias_source.opaque)-1;
        locations[k++]=alias_source.master.values;locations[k++]=(uint8_t *)alias_source.master.values+4*sizeof(int32_t)-1;
        locations[k++]=alias_source.master.values+8;locations[k++]=(uint8_t *)alias_source.master.values+sizeof(alias_source.master.values)-1;
        assert(k<=sizeof(locations)/sizeof(*locations));
        for(i=0;i<k;++i)alias_outputs_refuse(locations[i],n);
        /* Both directions of output/length overlap refuse before any bytes. */
        alias_outputs_refuse((void *)(uintptr_t)(UINTPTR_MAX-1),n);
        memset(alias_encoded,0xa5,sizeof(alias_encoded));memcpy(alias_encoded_before,alias_encoded,sizeof(alias_encoded));
        assert(pt_project_encode(p,alias_encoded,sizeof(alias_encoded),(size_t *)alias_encoded)==PT_PROJECT_ALIAS);
        assert(pt_project_encode(p,alias_encoded,sizeof(alias_encoded),(size_t *)(alias_encoded+n-1))==PT_PROJECT_ALIAS);
        assert(!memcmp(alias_encoded,alias_encoded_before,sizeof(alias_encoded)));
        w=123;assert(pt_project_encode(p,alias_encoded,n-1,&w)==PT_PROJECT_CAPACITY && w==123);
        assert(!memcmp(alias_encoded,alias_encoded_before,sizeof(alias_encoded)));
        alias_before=alias_source;
        assert(pt_project_encode(p,alias_encoded,sizeof(alias_encoded),&w)==PT_PROJECT_OK && w==n && !memcmp(alias_encoded,alias_gold,n));
        sink=(struct alias_sink){0,0};assert(pt_project_stream(p,alias_collect,&sink,&w)==PT_PROJECT_OK && w==n && sink.bytes==n);
        assert(!memcmp(alias_streamed,alias_gold,n) && !memcmp(&alias_source,&alias_before,sizeof(alias_source)));
        p->speed=0;alias_before=alias_source;n=123;w=456;caps=789;sink=(struct alias_sink){0,0};
        memcpy(alias_encoded_before,alias_encoded,sizeof(alias_encoded));
        assert(pt_project_validate(p,&caps)==PT_PROJECT_INVALID && caps==789);
        assert(pt_project_size(p,&n)==PT_PROJECT_INVALID && n==123);
        assert(pt_project_encode(p,alias_encoded,sizeof(alias_encoded),&w)==PT_PROJECT_INVALID && w==456);
        assert(pt_project_stream(p,alias_collect,&sink,&w)==PT_PROJECT_INVALID && w==456 && !sink.calls);
        assert(!memcmp(alias_encoded,alias_encoded_before,sizeof(alias_encoded)) && !memcmp(&alias_source,&alias_before,sizeof(alias_source)));
    }
    alias_fixture(24,1);alias_source.sample.pcm.capacity=SIZE_MAX/sizeof(int32_t)+1;alias_bad_spans_refuse();
    alias_fixture(24,1);alias_source.extension.data=(const uint8_t *)(uintptr_t)(UINTPTR_MAX-3);alias_bad_spans_refuse();
    alias_fixture(24,1);alias_source.sample.pcm.frames=0;alias_source.sample.slice_count=0;alias_source.sample.loop=PT_LOOP_NONE;alias_source.sample.loop_end=0;
    alias_source.sample.pcm.capacity=8;alias_source.sample.pcm.data=(int32_t *)(uintptr_t)(UINTPTR_MAX-7);alias_bad_spans_refuse();
    alias_source.sample.pcm.data=NULL;alias_bad_spans_refuse();
    /* Truly empty capacity has no backing span and remains serializable. */
    alias_source.sample.pcm.capacity=0;
    assert(pt_project_size(&alias_source.project,&n)==PT_PROJECT_OK);
    assert(pt_project_encode(&alias_source.project,alias_encoded,sizeof(alias_encoded),&w)==PT_PROJECT_OK && w==n);
}
int main(int argc,char **argv)
{
    struct pt_project p,decoded,before;struct pt_project_storage s=storage();
    struct pt_project_requirements need,sentinel;size_t n,w,i,pos;uint32_t caps;
    fixture(&p);assert(pt_project_validate(&p,&caps)==PT_PROJECT_OK);
    assert(caps==PT_CAP_KNOWN);
    assert(pt_project_size(&p,&n)==PT_PROJECT_OK && n<sizeof(encoded));
    assert(pt_project_encode(&p,encoded,sizeof(encoded),&w)==PT_PROJECT_OK && w==n);
    assert(pt_project_probe(encoded,n,&need)==PT_PROJECT_OK && need.events==2048 && need.pcm_values==12 && need.slices==2);
    assert(need.extensions==1 && need.extension_bytes==5 && need.capabilities==caps);
    assert(pt_project_decode(encoded,n,&s,&decoded)==PT_PROJECT_OK);
    assert(!memcmp(decoded_pcm,pcm,12*sizeof(int32_t)));
    assert(decoded.events[31].kind==PT_NOTE_OFF && decoded.events[47].effect==14 && decoded.events[47].parameter==0xc3);
    assert(decoded.events[15].kind==PT_NOTE_MIDI && decoded.events[15].pitch==0);
    assert(!strcmp(decoded.midi_output[15],"Output cluster"));
    assert(!memcmp(decoded.extensions[0].data,opaque,5));
    assert(pt_project_encode(&decoded,rewritten,sizeof(rewritten),&w)==PT_PROJECT_OK && w==n && !memcmp(encoded,rewritten,n));
    /* Input can disappear after decode; no strings/PCM/extensions point into it. */
    memset(damaged,0,n);memcpy(damaged,encoded,n);
    memset(&sentinel,0xa5,sizeof(sentinel));
    for(i=0;i<n;++i) {
        struct pt_project_requirements unchanged=sentinel;
        assert(pt_project_probe(encoded,i,&unchanged)!=PT_PROJECT_OK);
        assert(!memcmp(&unchanged,&sentinel,sizeof(sentinel)));
    }
    before=decoded;memset(decoded_events,0xa5,sizeof(decoded_events));s.event_capacity=2047;
    assert(pt_project_decode(encoded,n,&s,&decoded)==PT_PROJECT_CAPACITY);
    assert(!memcmp(&decoded,&before,sizeof(decoded)) && ((uint8_t *)decoded_events)[0]==0xa5);s.event_capacity=2048;
    s.pcm=(int32_t *)encoded;
    assert(pt_project_decode(encoded,n,&s,&decoded)==PT_PROJECT_ALIAS);s=storage();
    s.slices=(uint32_t *)s.pcm;
    assert(pt_project_decode(encoded,n,&s,&decoded)==PT_PROJECT_ALIAS);s=storage();
    assert(pt_project_encode(&p,(uint8_t *)events,n,&w)==PT_PROJECT_ALIAS);
    memset(rewritten,0x5a,sizeof(rewritten));
    assert(pt_project_encode(&p,rewritten,n-1,&w)==PT_PROJECT_CAPACITY && rewritten[0]==0x5a);
    damaged[n/2]^=1;assert(pt_project_probe(damaged,n,&need)==PT_PROJECT_CHECKSUM);
    memcpy(damaged,encoded,n);damaged[9]=2;assert(pt_project_probe(damaged,n,&need)==PT_PROJECT_UNSUPPORTED);
    memcpy(damaged,encoded,n);put32(damaged+16,0x80000000UL);assert(pt_project_probe(damaged,n,&need)==PT_PROJECT_UNSUPPORTED);
    memcpy(damaged,encoded,n);pos=chunk(damaged,0x54455354);damaged[pos+7]=1;repair(damaged,n);
    assert(pt_project_probe(damaged,n,&need)==PT_PROJECT_UNSUPPORTED);
    memcpy(damaged,encoded,n);pos=chunk(damaged,0x4f524452);damaged[pos+13]=2;repair(damaged,n);
    assert(pt_project_probe(damaged,n,&need)==PT_PROJECT_INVALID);
    memcpy(damaged,encoded,n);pos=chunk(damaged,0x50415454);damaged[pos+12]=4;repair(damaged,n);
    assert(pt_project_decode(damaged,n,&s,&decoded)==PT_PROJECT_INVALID && !memcmp(&decoded,&before,sizeof(decoded)));
    memcpy(damaged,encoded,n);pos=chunk(damaged,0x53414d50);put32(damaged+pos+12+48,4);repair(damaged,n);
    assert(pt_project_probe(damaged,n,&need)==PT_PROJECT_INVALID);
    memcpy(damaged,encoded,n);pos=chunk(damaged,0x4348414e);damaged[pos+12+4*22]=PT_PAULA;repair(damaged,n);
    assert(pt_project_probe(damaged,n,&need)==PT_PROJECT_INVALID);
    memcpy(damaged,encoded,n);put32(damaged+16,0);repair(damaged,n);
    assert(pt_project_probe(damaged,n,&need)==PT_PROJECT_INVALID);
    samples[0].crossfade=3;assert(pt_project_validate(&p,NULL)==PT_PROJECT_INVALID);samples[0].crossfade=2;
    slices[1]=0;assert(pt_project_validate(&p,NULL)==PT_PROJECT_INVALID);slices[1]=2;
    if(argc==2) {FILE *f=fopen(argv[1],"wb");assert(f);assert(fwrite(encoded,1,n,f)==n);assert(!fclose(f));}
    project_output_alias_cases();
    puts("PROJECT PASS: exact mixed-route round trip, 24-bit stereo, OFF, loops, slices, MIDI endpoints, extensions, CRC, bounds, transactional decode");
    return 0;
}
