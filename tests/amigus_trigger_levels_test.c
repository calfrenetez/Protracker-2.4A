#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "amigus_trigger_levels.h"
#include "amigus_render_voice.h"
#define PT_VOICE_PLAN_NATIVE
#include "amigus_voice_plan_test.c"

struct inputs {
    struct pt_sample sample;
    struct pt_playback_format format;
    struct pt_amigus_trigger_levels_request request;
    int32_t pcm[64];uint32_t slices[8];uint8_t spare[64];
};
static void defaults(struct inputs *x)
{
    memset(x,0,sizeof(*x));x->sample.pcm.data=x->pcm;x->sample.pcm.capacity=64;
    x->sample.pcm.frames=16;x->sample.pcm.rate=48000;x->sample.pcm.channels=1;
    x->sample.pcm.bits=24;x->sample.slices=x->slices;x->sample.slice_count=8;
    x->format.bits=16;x->request.geometry.rate_numerator=48000;
    x->request.geometry.rate_denominator=1;x->request.left=32639;x->request.right=32895;
}
static int same_five(const struct pt_amigus_voice_plan *a,const struct pt_amigus_voice_plan *b)
{return a->start==b->start&&a->loop==b->loop&&a->end_exclusive==b->end_exclusive&&a->rate==b->rate&&a->control==b->control;}
static int same_seven(const struct pt_amigus_voice_plan *a,const struct pt_amigus_voice_plan *b)
{return same_five(a,b)&&a->left==b->left&&a->right==b->right;}
static void refusal(struct inputs *x,uint32_t address,uint32_t bytes)
{
    struct pt_amigus_voice_plan out;uint8_t old[sizeof(out)];struct inputs before;
    memset(&out,0xa5,sizeof(out));memcpy(old,&out,sizeof(out));memcpy(&before,x,sizeof(before));
    assert(!pt_amigus_trigger_levels_prepare(&x->sample,&x->format,&x->request,address,bytes,&out));
    assert(!memcmp(old,&out,sizeof(out))&&!memcmp(&before,x,sizeof(before)));
}
static void legacy_parity(void)
{
    struct inputs x;struct pt_amigus_voice_request r;
    struct pt_amigus_voice_plan legacy,exact;unsigned volume,pan,bits,cache,endian,selection,loop,linear,count=0;
    const unsigned source_bits[]={8,16,24};defaults(&x);r=x.request.geometry;
    for(volume=0;volume<=64;++volume)for(pan=0;pan<=256;++pan){
        uint16_t left=(uint16_t)(((uint64_t)65535*volume*(256-pan)+8192)/16384);
        uint16_t right=(uint16_t)(((uint64_t)65535*volume*pan+8192)/16384);
        r.volume=volume;r.pan=pan;
        assert(pt_amigus_voice_plan_prepare(&x.sample,&x.format,&r,256,32,&legacy));
        assert(legacy.left==left&&legacy.right==right);
        assert(left!=32639||right!=32895); /* No legacy pair is canonical centre. */
        x.request.left=left;x.request.right=right;
        assert(pt_amigus_trigger_levels_prepare(&x.sample,&x.format,&x.request,256,32,&exact));
        assert(same_seven(&legacy,&exact));++count;
    }
    assert(count==16705);count=0;
    for(bits=0;bits<3;++bits)for(cache=8;cache<=16;cache+=8)for(endian=0;endian<2;++endian)
    for(selection=0;selection<3;++selection)for(loop=0;loop<2;++loop)for(linear=0;linear<2;++linear){
        defaults(&x);x.sample.pcm.bits=(uint8_t)source_bits[bits];
        x.sample.pcm.channels=(uint8_t)(selection?2:1);x.format.channel=selection==2?1:0;
        x.format.bits=cache;x.format.little_endian=endian;x.sample.interpolation=(uint8_t)linear;
        x.sample.loop=(uint8_t)(loop?PT_LOOP_FORWARD:PT_LOOP_NONE);
        x.sample.loop_start=loop?4:0;x.sample.loop_end=loop?12:0;x.request.geometry.offset=2;
        r=x.request.geometry;r.volume=64;r.pan=128;
        assert(pt_amigus_voice_plan_prepare(&x.sample,&x.format,&r,256,16*(cache/8),&legacy));
        assert(pt_amigus_trigger_levels_prepare(&x.sample,&x.format,&x.request,256,16*(cache/8),&exact));
        assert(same_five(&legacy,&exact)&&exact.left==32639&&exact.right==32895);++count;
    }
    assert(count==144);
    puts("AMIGUS DIRECT LEVELS LEGACY PASS: 16705 volume/pan pairs and144 source/cache/endian/channel/loop/interpolation geometries retain unchanged legacy fields");
}
static void canonical(void)
{
    struct inputs x;struct pt_project project;struct pt_render_options options;
    struct pt_render_command_state commands;struct pt_amigus_voice_request old;
    struct pt_amigus_voice_plan legacy,canonical,exact;
    const uint32_t rates[]={1,2,3,17,0x10000000UL,0x3ffffffeUL,0x3fffffffUL,0x40000000UL};
    unsigned i,origin,loop,linear,bits,cache,endian;
    const unsigned source_bits[]={8,16,24};
    defaults(&x);memset(&project,0,sizeof(project));memset(&options,0,sizeof(options));
    pt_channels_init(&project.channels);assert(pt_channels_route(&project.channels,0,PT_AMIGUS)==PT_CHANNEL_OK);
    project.channels.track[0].pan=128;project.samples=&x.sample;project.sample_count=1;
    options.rate=48000;options.gain_q16=65536;options.tracks=1;
    pt_render_commands_init(&commands);commands.output_volume[0]=64;
    assert(pt_voice_init(&commands.voice[0],&x.sample.pcm,0,16,PT_VOICE_ONCE,0,0,UINT64_C(1)<<32,0)==PT_PCM_OK);
    pt_render_commands_gains(&project,&options,&commands);
    assert(commands.gain[0][0]==32639&&commands.gain[0][1]==32896);
    assert(pt_amigus_render_voice(&commands.voice[0],48000,commands.gain[0],&x.format,0,32,&canonical));
    assert(canonical.left==32639&&canonical.right==32895);
    old=x.request.geometry;old.volume=64;old.pan=128;
    assert(pt_amigus_voice_plan_prepare(&x.sample,&x.format,&old,0,32,&legacy));
    assert(legacy.left==32768&&legacy.right==32768&&!same_seven(&legacy,&canonical));
    assert(pt_amigus_trigger_levels_prepare(&x.sample,&x.format,&x.request,0,32,&exact)&&same_seven(&exact,&canonical));
    /* Real canonical mono converter and unchanged rational helper agree in all
     * seven fields at both numeric origins and min/max register-rate literals. */
    for(bits=0;bits<3;++bits)for(cache=8;cache<=16;cache+=8)for(endian=0;endian<2;++endian)
    for(i=0;i<sizeof(rates)/sizeof(rates[0]);++i)for(origin=0;origin<2;++origin)
    for(loop=0;loop<2;++loop)for(linear=0;linear<2;++linear){
        uint64_t n=((uint64_t)rates[i]*375+127)/128;uint32_t address=origin?256:0;
        x.sample.pcm.bits=(uint8_t)source_bits[bits];x.format.bits=cache;x.format.little_endian=endian;
        x.sample.loop=(uint8_t)(loop?PT_LOOP_FORWARD:PT_LOOP_NONE);x.sample.loop_start=loop?4:0;
        x.sample.loop_end=loop?12:0;x.sample.interpolation=(uint8_t)linear;x.request.geometry.offset=2;
        x.request.geometry.rate_numerator=(uint32_t)n;x.request.geometry.rate_denominator=16384;
        assert(n<=UINT32_MAX);
        assert(pt_voice_init(&commands.voice[0],&x.sample.pcm,2,16,loop?PT_VOICE_FORWARD:PT_VOICE_ONCE,
            loop?4:0,loop?12:0,(uint64_t)rates[i]*16,linear)==PT_PCM_OK);
        assert(pt_amigus_render_voice(&commands.voice[0],48000,commands.gain[0],&x.format,address,16*(cache/8),&canonical));
        assert(pt_amigus_trigger_levels_prepare(&x.sample,&x.format,&x.request,address,16*(cache/8),&exact));
        assert(exact.rate==rates[i]&&same_seven(&canonical,&exact));
    }
    assert(x.request.geometry.rate_numerator==UINT32_C(3145728000));
    puts("AMIGUS DIRECT LEVELS CANONICAL PASS: actual mono renderer centre32639/32895 equals all seven explicit fields; legacy64/128 remains32768/32768; min/max rate and relocation exact");
}
static void quantized_pairs(void)
{
    struct inputs x;struct pt_amigus_voice_plan p;unsigned value,i;
    const uint16_t pairs[][2]={{0,0},{1,0},{0,1},{1,1},{32767,32768},{32768,32767},
        {65534,65535},{65535,65534},{65535,65535},{32639,32895}};
    defaults(&x);
    for(value=0;value<=65535;++value){
        x.request.left=(uint16_t)value;x.request.right=(uint16_t)(65535-value);
        assert(pt_amigus_trigger_levels_prepare(&x.sample,&x.format,&x.request,256,32,&p));
        assert(p.left==value&&p.right==65535-value);
    }
    for(i=0;i<sizeof(pairs)/sizeof(pairs[0]);++i){
        x.request.left=pairs[i][0];x.request.right=pairs[i][1];
        assert(pt_amigus_trigger_levels_prepare(&x.sample,&x.format,&x.request,256,32,&p));
        assert(p.left==pairs[i][0]&&p.right==pairs[i][1]);
    }
    /* The helper validates metadata/spans only, never PCM or slice values. */
    for(i=0;i<64;++i)x.pcm[i]=INT32_MAX;
    for(i=0;i<8;++i)x.slices[i]=UINT32_MAX;
    x.sample.pcm.bits=8;
    assert(pt_amigus_trigger_levels_prepare(&x.sample,&x.format,&x.request,256,32,&p));
    puts("AMIGUS DIRECT LEVELS DOMAIN PASS: all65536 individual register values, silent/asymmetric/independent maxima, no gain defaults/clamps or PCM/slice value reads");
}
static void geometry_refusals(void)
{
    struct inputs x;struct pt_amigus_voice_plan p;
#define D1_RESET() defaults(&x)
#define D1_REFUSE() refusal(&x,256,32)
    D1_RESET();x.request.geometry.volume=1;D1_REFUSE();D1_RESET();x.request.geometry.pan=1;D1_REFUSE();
    D1_RESET();x.request.geometry.rate_numerator=0;D1_REFUSE();D1_RESET();x.request.geometry.rate_denominator=0;D1_REFUSE();
    D1_RESET();x.request.geometry.rate_numerator=192001;D1_REFUSE();D1_RESET();x.request.geometry.rate_numerator=1;x.request.geometry.rate_denominator=UINT32_MAX;D1_REFUSE();
    D1_RESET();x.request.geometry.rate_numerator=UINT32_MAX;D1_REFUSE();D1_RESET();x.request.geometry.rate_numerator=3145728001UL;x.request.geometry.rate_denominator=16384;D1_REFUSE();
    D1_RESET();x.request.geometry.offset=16;D1_REFUSE();D1_RESET();x.sample.pcm.frames=0;D1_REFUSE();
    D1_RESET();x.sample.pcm.bits=7;D1_REFUSE();D1_RESET();x.sample.pcm.channels=3;D1_REFUSE();D1_RESET();x.format.channel=1;D1_REFUSE();
    D1_RESET();x.format.bits=24;D1_REFUSE();D1_RESET();x.format.little_endian=2;D1_REFUSE();D1_RESET();x.format.word_pad=1;D1_REFUSE();
    D1_RESET();x.sample.interpolation=2;D1_REFUSE();D1_RESET();x.sample.loop=PT_LOOP_PINGPONG;D1_REFUSE();D1_RESET();x.sample.loop=PT_LOOP_CROSSFADE;D1_REFUSE();
    D1_RESET();x.sample.loop_start=2;D1_REFUSE();D1_RESET();x.sample.loop_end=16;D1_REFUSE();D1_RESET();x.sample.crossfade=1;D1_REFUSE();
    D1_RESET();x.sample.loop=PT_LOOP_FORWARD;x.sample.loop_start=12;x.sample.loop_end=12;D1_REFUSE();
    D1_RESET();x.sample.loop=PT_LOOP_FORWARD;x.sample.loop_start=4;x.sample.loop_end=17;D1_REFUSE();
    D1_RESET();x.sample.loop=PT_LOOP_FORWARD;x.sample.loop_start=4;x.sample.loop_end=12;x.sample.crossfade=1;D1_REFUSE();
    D1_RESET();refusal(&x,256,31);refusal(&x,256,33);refusal(&x,257,32);
    refusal(&x,PT_AMIGUS_RAM_ADDRESS_SPACE,32);refusal(&x,UINT32_MAX-16,32);
    refusal(&x,PT_AMIGUS_RAM_ADDRESS_SPACE-32,32);
    assert(pt_amigus_trigger_levels_prepare(&x.sample,&x.format,&x.request,PT_AMIGUS_RAM_ADDRESS_SPACE-34,32,&p));
    assert(p.end_exclusive==PT_AMIGUS_RAM_ADDRESS_SPACE-2);
    /* Voice pointers retain legacy2-alignment; genuine upload owner separately
     * imposes4-alignment. Numeric address258 is intentionally accepted. */
    assert(pt_amigus_trigger_levels_prepare(&x.sample,&x.format,&x.request,258,32,&p));
    D1_RESET();x.format.bits=8;x.request.geometry.offset=1;refusal(&x,256,16);
    D1_RESET();x.format.bits=8;x.sample.loop=PT_LOOP_FORWARD;x.sample.loop_start=3;x.sample.loop_end=12;refusal(&x,256,16);
    D1_RESET();x.format.bits=8;x.sample.loop=PT_LOOP_FORWARD;x.sample.loop_start=4;x.sample.loop_end=11;refusal(&x,256,16);
    D1_RESET();x.format.bits=8;x.sample.pcm.frames=15;refusal(&x,256,15);
    D1_RESET();x.sample.pcm.frames=UINT32_MAX;x.sample.pcm.capacity=UINT32_MAX;refusal(&x,0,UINT32_MAX);
#undef D1_REFUSE
#undef D1_RESET
}
static void alias_and_malformed(void)
{
    struct inputs x,before;struct pt_amigus_voice_plan out;uint8_t old[sizeof(out)];unsigned extent;
    uintptr_t address[5];size_t bytes[5],offset;
    defaults(&x);address[0]=(uintptr_t)&x.sample;bytes[0]=sizeof(x.sample);
    address[1]=(uintptr_t)&x.format;bytes[1]=sizeof(x.format);
    address[2]=(uintptr_t)&x.request;bytes[2]=sizeof(x.request);
    address[3]=(uintptr_t)x.pcm;bytes[3]=sizeof(x.pcm);address[4]=(uintptr_t)x.slices;bytes[4]=sizeof(x.slices);
    memcpy(&before,&x,sizeof(x));
    for(extent=0;extent<5;++extent)for(offset=0;offset<bytes[extent];offset+=_Alignof(struct pt_amigus_voice_plan)){
        struct pt_amigus_voice_plan *alias=(struct pt_amigus_voice_plan *)(address[extent]+offset);
        assert(!pt_amigus_trigger_levels_prepare(&x.sample,&x.format,&x.request,256,32,alias));
        assert(!memcmp(&before,&x,sizeof(x)));
    }
    memset(&out,0xa5,sizeof(out));memcpy(old,&out,sizeof(out));
    assert(!pt_amigus_trigger_levels_prepare(NULL,&x.format,&x.request,256,32,&out));
    assert(!pt_amigus_trigger_levels_prepare(&x.sample,NULL,&x.request,256,32,&out));
    assert(!pt_amigus_trigger_levels_prepare(&x.sample,&x.format,NULL,256,32,&out));
    assert(!pt_amigus_trigger_levels_prepare(&x.sample,(const struct pt_playback_format *)&x.sample,&x.request,256,32,&out));
    assert(!pt_amigus_trigger_levels_prepare(&x.sample,&x.format,(const struct pt_amigus_trigger_levels_request *)&x.format,256,32,&out));
    assert(!pt_amigus_trigger_levels_prepare((const struct pt_sample *)((const uint8_t *)&x.sample+1),&x.format,&x.request,256,32,&out));
    assert(!pt_amigus_trigger_levels_prepare(&x.sample,(const struct pt_playback_format *)((const uint8_t *)&x.format+1),&x.request,256,32,&out));
    assert(!pt_amigus_trigger_levels_prepare(&x.sample,&x.format,(const struct pt_amigus_trigger_levels_request *)((const uint8_t *)&x.request+1),256,32,&out));
    assert(!pt_amigus_trigger_levels_prepare(&x.sample,&x.format,&x.request,256,32,(struct pt_amigus_voice_plan *)((uint8_t *)&out+1)));
    assert(!pt_amigus_trigger_levels_prepare(&x.sample,&x.format,&x.request,256,32,NULL));
    assert(!pt_amigus_trigger_levels_prepare((const struct pt_sample *)(UINTPTR_MAX-3),&x.format,&x.request,256,32,&out));
    assert(!pt_amigus_trigger_levels_prepare(&x.sample,(const struct pt_playback_format *)(UINTPTR_MAX-3),&x.request,256,32,&out));
    assert(!pt_amigus_trigger_levels_prepare(&x.sample,&x.format,(const struct pt_amigus_trigger_levels_request *)(UINTPTR_MAX-3),256,32,&out));
    assert(!pt_amigus_trigger_levels_prepare(&x.sample,&x.format,&x.request,256,32,(struct pt_amigus_voice_plan *)(UINTPTR_MAX-3)));
    assert(!pt_amigus_trigger_levels_prepare(&x.sample,&x.format,(const struct pt_amigus_trigger_levels_request *)x.pcm,256,32,&out));
    assert(!pt_amigus_trigger_levels_prepare(&x.sample,(const struct pt_playback_format *)x.slices,&x.request,256,32,&out));
    assert(!memcmp(old,&out,sizeof(out))&&!memcmp(&before,&x,sizeof(x)));
#define D1_RESET() defaults(&x)
#define D1_REFUSE() refusal(&x,256,32)
    D1_RESET();x.sample.pcm.capacity=SIZE_MAX/sizeof(int32_t)+1;D1_REFUSE();
    /* The unchanged legacy function is a metadata-only numeric oracle and
     * intentionally accepts missing/undersized master storage. D1's stronger
     * documented complete-span preconditions refuse those inputs atomically. */
    D1_RESET();x.sample.pcm.data=NULL;
    {struct pt_amigus_voice_plan legacy;
        assert(pt_amigus_voice_plan_prepare(&x.sample,&x.format,&x.request.geometry,256,32,&legacy));}
    D1_REFUSE();D1_RESET();x.sample.pcm.capacity=15;
    {struct pt_amigus_voice_plan legacy;
        assert(pt_amigus_voice_plan_prepare(&x.sample,&x.format,&x.request.geometry,256,32,&legacy));}
    D1_REFUSE();
    D1_RESET();x.sample.pcm.data=(int32_t *)(UINTPTR_MAX-3);D1_REFUSE();
    D1_RESET();x.sample.pcm.data=(int32_t *)((uint8_t *)x.pcm+1);D1_REFUSE();
    D1_RESET();x.sample.slice_count=PT_PROJECT_SLICES+1;D1_REFUSE();D1_RESET();x.sample.slices=NULL;D1_REFUSE();
    D1_RESET();x.sample.slices=(uint32_t *)(UINTPTR_MAX-3);D1_REFUSE();
    D1_RESET();x.sample.slices=(uint32_t *)((uint8_t *)x.slices+1);D1_REFUSE();
    D1_RESET();x.sample.pcm.data=(int32_t *)&x.request;D1_REFUSE();
    D1_RESET();x.sample.slices=(uint32_t *)x.pcm;D1_REFUSE();
    D1_RESET();x.sample.slices=(uint32_t *)&x.sample;D1_REFUSE();
#undef D1_REFUSE
#undef D1_RESET
    geometry_refusals();
    puts("AMIGUS DIRECT LEVELS GUARDS PASS: complete request/descriptors/PCM spare/slices, wrapped/missing/misaligned storage and delegated geometry refuse with exact byte preservation");
}
int main(void)
{
    assert(voice_plan_fixture_main()==0);legacy_parity();canonical();quantized_pairs();alias_and_malformed();
    puts("AMIGUS DIRECT LEVELS PASS: D1 pure software only; no factory/controller/normalizer integration, native/card capacity/stop/timing/audio authority");
    return 0;
}
