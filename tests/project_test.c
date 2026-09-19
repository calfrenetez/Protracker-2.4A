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
    puts("PROJECT PASS: exact mixed-route round trip, 24-bit stereo, OFF, loops, slices, MIDI endpoints, extensions, CRC, bounds, transactional decode");
    return 0;
}
