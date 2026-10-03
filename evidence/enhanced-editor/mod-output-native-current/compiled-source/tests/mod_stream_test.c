#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../src/platform/mod_file.h"
#include "document.h"
static struct pt_event events[65*256];
static int32_t pcm[3][2050],before[3][2050];
static uint8_t gold[80000],output[80000],header[1084];
struct sink_state {size_t pos,fail;};
static int collect(void *context,const uint8_t *data,size_t n)
{struct sink_state *s=context;assert(n<=1084);if(s->pos>=s->fail)return 0;assert(s->pos+n<=sizeof(output));memcpy(output+s->pos,data,n);s->pos+=n;return 1;}
struct budget {size_t live,peak;unsigned refuse;};
static void *allocate(void *context,size_t n)
{struct budget *b=context;void *p;if(b->refuse)return NULL;assert(!b->live && n<16384);p=malloc(n);assert(p);b->live=n;b->peak=n;return p;}
static void release(void *context,void *p)
{struct budget *b=context;assert(b->live);b->live=0;free(p);}
/* Shared by host and the existing native Exec MOD stream entry. All ordinary
 * alias probes use aligned storage inside real allocations, including padding. */
typedef enum pt_project_result (*mod_analyse_fn)(const struct pt_project *,struct pt_mod_export_report *);
typedef enum pt_project_result (*mod_encode_fn)(const struct pt_project *,uint8_t *,size_t,size_t *);
struct mod_alias_source {
    struct pt_project project;
    struct pt_sample samples[2];
    union {size_t align;uint16_t values[16];} orders;
    union {size_t align;struct pt_event values[256];} events;
    union {size_t align;uint32_t values[4];} slices;
    struct pt_extension extensions[2];
    union {size_t align;uint8_t bytes[4096];} payload;
    union {size_t align;int32_t values[2][4096];} masters;
    uint8_t tail[4096];
};
static unsigned char mod_source_before[sizeof(struct mod_alias_source)];
static union {size_t align;uint8_t bytes[4096];} mod_encoded,mod_encoded_before;
struct mod_adjacent_output {uint8_t emitted[2112];size_t written;};
static struct mod_adjacent_output mod_adjacent;
static void mod_output_alias_fixture(void)
{
    const mod_analyse_fn analyses[]={pt_mod_export_analyse,pt_mod_export_analyse_round8,pt_mod_playback_analyse};
    const mod_encode_fn encoders[]={pt_mod_export_direct,pt_mod_export_round8,pt_mod_export_tpdf8,pt_mod_playback_encode};
    struct mod_alias_source *f=malloc(sizeof(*f));struct pt_mod_export_report report,saved;
    struct pt_project *p;void *targets[10];size_t written,bytes=2112;unsigned bits,a,e,t;
    struct sink_state sink;
    assert(f);memset(f,0,sizeof(*f));p=&f->project;pt_channels_init(&p->channels);
    p->bpm=125;p->speed=6;p->order_count=16;p->pattern_count=1;p->sample_count=2;
    p->orders=f->orders.values;p->events=f->events.values;p->samples=f->samples;
    p->extensions=f->extensions;p->extension_count=1;
    f->extensions[0]=(struct pt_extension){PT_CLASSIC_HEADER_TAG,1084,1,f->payload.bytes};
    f->payload.bytes[950]=16;memcpy(f->payload.bytes+1080,"M.K.",4);
    f->samples[0].volume=f->samples[1].volume=64;
    memset(f->masters.values,0x5a,sizeof(f->masters.values));
    f->samples[1].pcm=(struct pt_pcm){f->masters.values[1],4096,0,PT_CLASSIC_RATE,1,8};
    targets[0]=p;targets[1]=f->orders.values;targets[2]=f->events.values;targets[3]=f->samples;
    targets[4]=f->extensions;targets[5]=f->payload.bytes;targets[6]=f->masters.values[0];
    targets[7]=f->masters.values[0]+8;targets[8]=f->masters.values[1];
    targets[9]=(uint8_t *)f->masters.values[1]+sizeof(f->masters.values[1])-sizeof(report);
    for(bits=8;bits<=24;bits+=8) {
        int32_t factor=(int32_t)1<<(bits-8);
        f->samples[0].pcm=(struct pt_pcm){f->masters.values[0],4096,4,PT_CLASSIC_RATE,1,(uint8_t)bits};
        f->masters.values[0][0]=-17*factor;f->masters.values[0][1]=23*factor;
        f->masters.values[0][2]=-41*factor;f->masters.values[0][3]=61*factor;
        assert(pt_project_validate(p,NULL)==PT_PROJECT_OK);
        memcpy(mod_source_before,f,sizeof(*f));
        for(a=0;a<3;++a) {
            assert(analyses[a](p,&report)==PT_PROJECT_OK);
            assert(report.issues==(bits==8?0U:PT_EXPORT_PRECISION));
            assert(report.bytes==(a==0 && bits>8?0:bytes));
            for(t=0;t<10;++t) {
                assert(analyses[a](p,(struct pt_mod_export_report *)targets[t])==PT_PROJECT_ALIAS);
                assert(!memcmp(f,mod_source_before,sizeof(*f)));
            }
            assert(analyses[a](p,NULL)==PT_PROJECT_INVALID);
        }
        for(e=0;e<4;++e) {
            enum pt_project_result expected=e==0 && bits>8?PT_PROJECT_UNSUPPORTED:PT_PROJECT_ALIAS;
            memset(mod_encoded.bytes,0xa5,sizeof(mod_encoded.bytes));mod_encoded_before=mod_encoded;
            for(t=0;t<10;++t) {
                assert(encoders[e](p,mod_encoded.bytes,sizeof(mod_encoded.bytes),(size_t *)targets[t])==expected);
                assert(!memcmp(f,mod_source_before,sizeof(*f)) && !memcmp(&mod_encoded,&mod_encoded_before,sizeof(mod_encoded)));
                written=99;
                assert(encoders[e](p,(uint8_t *)targets[t],bytes,&written)==expected && written==99);
                assert(!memcmp(f,mod_source_before,sizeof(*f)));
            }
            assert(encoders[e](p,NULL,bytes,&written)==(expected==PT_PROJECT_UNSUPPORTED?expected:PT_PROJECT_INVALID));
            assert(encoders[e](p,mod_encoded.bytes,bytes,NULL)==(expected==PT_PROJECT_UNSUPPORTED?expected:PT_PROJECT_INVALID));
            written=99;
            assert(encoders[e](p,mod_encoded.bytes,bytes-1,(size_t *)targets[7])==
                   (expected==PT_PROJECT_UNSUPPORTED?expected:PT_PROJECT_CAPACITY));
            assert(!memcmp(f,mod_source_before,sizeof(*f)) && !memcmp(&mod_encoded,&mod_encoded_before,sizeof(mod_encoded)));
            assert(encoders[e](p,mod_encoded.bytes,sizeof(mod_encoded.bytes),(size_t *)(void *)(mod_encoded.bytes+8))==expected);
            assert(!memcmp(&mod_encoded,&mod_encoded_before,sizeof(mod_encoded)));
            if(expected==PT_PROJECT_UNSUPPORTED)continue;
            assert(encoders[e](p,mod_encoded.bytes,sizeof(mod_encoded.bytes),&written)==PT_PROJECT_OK && written==bytes);
            sink=(struct sink_state){0,SIZE_MAX};
            assert((e==3?pt_mod_playback_stream(p,collect,&sink):pt_mod_export_stream(p,e,collect,&sink))==PT_PROJECT_OK);
            assert(sink.pos==written && !memcmp(mod_encoded.bytes,output,written) && !memcmp(f,mod_source_before,sizeof(*f)));
            /* A scalar immediately after the emitted bytes is disjoint, even
             * when it lies inside the caller's larger output capacity. */
            assert(offsetof(struct mod_adjacent_output,written)==bytes);
            assert(encoders[e](p,(uint8_t *)(void *)&mod_adjacent,sizeof(mod_adjacent),&mod_adjacent.written)==PT_PROJECT_OK);
            assert(mod_adjacent.written==bytes && !memcmp(mod_adjacent.emitted,mod_encoded.bytes,bytes));
        }
        /* Playback's explicit odd-sample padding remains private and byte exact. */
        f->samples[0].pcm.frames=3;memcpy(mod_source_before,f,sizeof(*f));
        assert(pt_mod_playback_analyse(p,&report)==PT_PROJECT_OK && report.bytes==bytes &&
               report.issues==(PT_EXPORT_PADDING|(bits==8?0U:PT_EXPORT_PRECISION)));
        assert(pt_mod_playback_encode(p,mod_encoded.bytes,sizeof(mod_encoded.bytes),&written)==PT_PROJECT_OK &&
               written==bytes && !mod_encoded.bytes[bytes-1]);
        sink=(struct sink_state){0,SIZE_MAX};assert(pt_mod_playback_stream(p,collect,&sink)==PT_PROJECT_OK &&
               sink.pos==bytes && !memcmp(mod_encoded.bytes,output,bytes) && !memcmp(f,mod_source_before,sizeof(*f)));
        for(e=0;e<3;++e) {
            written=99;assert(encoders[e](p,(uint8_t *)targets[7],bytes,&written)==PT_PROJECT_UNSUPPORTED && written==99);
            assert(!memcmp(f,mod_source_before,sizeof(*f)));
        }
        f->samples[0].pcm.frames=4;
        /* Unsupported stereo geometry still takes precedence over output alias. */
        f->samples[0].pcm.channels=2;f->samples[0].pcm.frames=2;memcpy(mod_source_before,f,sizeof(*f));
        for(e=0;e<4;++e) {
            written=99;assert(encoders[e](p,(uint8_t *)targets[7],bytes,(size_t *)targets[8])==PT_PROJECT_UNSUPPORTED);
            assert(encoders[e](p,NULL,0,NULL)==PT_PROJECT_UNSUPPORTED && written==99 && !memcmp(f,mod_source_before,sizeof(*f)));
        }
        f->samples[0].pcm.channels=1;f->samples[0].pcm.frames=4;
    }
    /* Slices cannot be exported to classic MOD, but analysis must not publish a
     * report over their live table. Unsupported export remains transactional. */
    f->samples[0].slice_count=4;f->samples[0].slices=f->slices.values;
    for(t=0;t<4;++t)f->slices.values[t]=t;
    memcpy(mod_source_before,f,sizeof(*f));
    for(a=0;a<3;++a) {
        assert(analyses[a](p,(struct pt_mod_export_report *)(void *)f->slices.values)==PT_PROJECT_ALIAS);
        assert(!memcmp(f,mod_source_before,sizeof(*f)));
    }
    for(e=0;e<4;++e) {
        written=99;assert(encoders[e](p,(uint8_t *)f->slices.values,bytes,(size_t *)targets[7])==PT_PROJECT_UNSUPPORTED);
        assert(written==99 && !memcmp(f,mod_source_before,sizeof(*f)));
    }
    f->samples[0].slice_count=0;f->samples[0].slices=NULL;f->samples[0].pcm.bits=8;
    f->masters.values[0][0]=-17;f->masters.values[0][1]=23;f->masters.values[0][2]=-41;f->masters.values[0][3]=61;
    /* Geometry declares an unrepresentable inactive capacity; active validation
     * is safe, while publication guards must refuse without reading padding. */
    f->samples[1].pcm.capacity=SIZE_MAX/sizeof(int32_t)+1;
    assert(pt_project_validate(p,NULL)==PT_PROJECT_OK);memcpy(mod_source_before,f,sizeof(*f));
    memset(&report,0xa5,sizeof(report));memcpy(&saved,&report,sizeof(saved));
    for(a=0;a<3;++a)assert(analyses[a](p,&report)==PT_PROJECT_ALIAS && !memcmp(&report,&saved,sizeof(report)));
    memset(mod_encoded.bytes,0xa5,sizeof(mod_encoded.bytes));mod_encoded_before=mod_encoded;
    for(e=0;e<4;++e) {
        written=99;assert(encoders[e](p,mod_encoded.bytes,sizeof(mod_encoded.bytes),&written)==PT_PROJECT_ALIAS && written==99);
        assert(encoders[e](p,mod_encoded.bytes,bytes-1,&written)==PT_PROJECT_CAPACITY && written==99);
        assert(encoders[e](p,NULL,bytes,&written)==PT_PROJECT_INVALID && written==99);
        assert(!memcmp(f,mod_source_before,sizeof(*f)) && !memcmp(&mod_encoded,&mod_encoded_before,sizeof(mod_encoded)));
    }
    /* Distinct from byte-count overflow: a representable count whose source
     * address interval wraps also refuses without reading inactive capacity. */
    f->samples[1].pcm.capacity=(UINTPTR_MAX-(uintptr_t)f->samples[1].pcm.data)/sizeof(int32_t)+1;
    memcpy(mod_source_before,f,sizeof(*f));
    for(a=0;a<3;++a)assert(analyses[a](p,&report)==PT_PROJECT_ALIAS && !memcmp(&report,&saved,sizeof(report)));
    for(e=0;e<4;++e) {
        written=99;assert(encoders[e](p,mod_encoded.bytes,sizeof(mod_encoded.bytes),&written)==PT_PROJECT_ALIAS && written==99);
        assert(!memcmp(f,mod_source_before,sizeof(*f)) && !memcmp(&mod_encoded,&mod_encoded_before,sizeof(mod_encoded)));
    }
    f->samples[0].pcm.bits=16;memcpy(mod_source_before,f,sizeof(*f));
    assert(pt_mod_export_direct(p,mod_encoded.bytes,sizeof(mod_encoded.bytes),(size_t *)targets[7])==PT_PROJECT_UNSUPPORTED);
    assert(!memcmp(f,mod_source_before,sizeof(*f)));
    f->samples[1].pcm.capacity=4096;f->samples[0].pcm.bits=8;
    memcpy(mod_source_before,f,sizeof(*f));
    /* Deliberately unrepresentable destinations are never dereferenced. The
     * scalar address is aligned; this is a span-refusal test, not live storage. */
    for(a=0;a<3;++a)assert(analyses[a](p,(struct pt_mod_export_report *)(uintptr_t)(UINTPTR_MAX & ~(uintptr_t)(sizeof(size_t)-1)))==PT_PROJECT_ALIAS);
    for(e=0;e<4;++e) {
        written=99;
        assert(encoders[e](p,(uint8_t *)(uintptr_t)(UINTPTR_MAX-1023),bytes,&written)==PT_PROJECT_ALIAS && written==99);
        assert(encoders[e](p,mod_encoded.bytes,sizeof(mod_encoded.bytes),(size_t *)(uintptr_t)(UINTPTR_MAX & ~(uintptr_t)(sizeof(size_t)-1)))==PT_PROJECT_ALIAS);
        assert(!memcmp(f,mod_source_before,sizeof(*f)) && !memcmp(&mod_encoded,&mod_encoded_before,sizeof(mod_encoded)));
    }
    p->samples=NULL;memcpy(mod_source_before,f,sizeof(*f));
    for(a=0;a<3;++a)assert(analyses[a](p,&report)==PT_PROJECT_INVALID && !memcmp(&report,&saved,sizeof(report)));
    for(e=0;e<4;++e) {
        written=99;assert(encoders[e](p,mod_encoded.bytes,sizeof(mod_encoded.bytes),&written)==PT_PROJECT_INVALID && written==99);
        assert(!memcmp(f,mod_source_before,sizeof(*f)) && !memcmp(&mod_encoded,&mod_encoded_before,sizeof(mod_encoded)));
    }
    free(f);
    puts("MOD OUTPUT PRESERVATION PASS: analysis and direct/round8/TPDF/playback outputs, all source tables and full 8/16/24-bit capacities, mutual outputs, fail-closed capacity/address spans, refusal precedence and unchanged streamed/padded bytes");
}

static int mod_stream_fixture(int argc,char **argv)
{
    struct pt_project p;struct pt_sample samples[3];uint16_t orders[1]={0};
    struct pt_extension ext={PT_CLASSIC_HEADER_TAG,1084,1,header};
    struct budget b={0};struct pt_allocator a={&b,allocate,release};
    unsigned legacy,policy,i,j;size_t written;struct sink_state sink;FILE *f;
    assert(argc==2);mod_output_alias_fixture();memset(&p,0,sizeof(p));memset(samples,0,sizeof(samples));
    pt_channels_init(&p.channels);p.bpm=125;p.speed=6;p.mode=PT_MODE_WAVETABLE;
    p.order_count=1;p.pattern_count=65;p.orders=orders;p.events=events;p.samples=samples;p.sample_count=3;
    strcpy(p.title,"stream test");p.extensions=&ext;header[950]=1;header[951]=42;memcpy(header+1080,"M!K!",4);
    events[0].kind=PT_NOTE_PERIOD;events[0].pitch=428;events[0].instrument=1;
    events[64*256+255].effect=15;events[64*256+255].parameter=125;
    for(legacy=0;legacy<2;++legacy)for(policy=0;policy<3;++policy) {
        p.extension_count=(uint16_t)legacy;
        for(i=0;i<3;++i) {
            unsigned bits=policy?8+8*i:8;
            samples[i].pcm=(struct pt_pcm){pcm[i],2050,2050,PT_CLASSIC_RATE,1,(uint8_t)bits};samples[i].volume=64;
            for(j=0;j<2050;++j)pcm[i][j]=((int32_t)(j%256)-128)*(1L<<(bits-8))+(bits>8?(int32_t)(j%127):0);
            samples[i].loop=i==1?PT_LOOP_FORWARD:PT_LOOP_NONE;samples[i].loop_start=i==1?2:0;samples[i].loop_end=i==1?2048:0;
        }
        memcpy(before,pcm,sizeof(pcm));
        assert((policy==2?pt_mod_export_tpdf8(&p,gold,sizeof(gold),&written):policy?pt_mod_export_round8(&p,gold,sizeof(gold),&written):pt_mod_export_direct(&p,gold,sizeof(gold),&written))==PT_PROJECT_OK);
        sink=(struct sink_state){0,SIZE_MAX};assert(pt_mod_export_stream(&p,policy,collect,&sink)==PT_PROJECT_OK && sink.pos==written && !memcmp(gold,output,written));
        sink=(struct sink_state){0,1084};assert(pt_mod_export_stream(&p,policy,collect,&sink)==PT_PROJECT_INVALID && sink.pos==1084);
        b.refuse=1;assert(pt_mod_file_save(argv[1],&p,policy,&a)==PT_SAVE_MEMORY);b.refuse=0;
        assert(pt_mod_file_save(argv[1],&p,policy,&a)==PT_SAVE_OK && !b.live);
        assert(pt_mod_file_save(argv[1],&p,policy,&a)==PT_SAVE_PUBLISH && !b.live);
        f=fopen(argv[1],"rb");assert(f && fread(output,1,written,f)==written && fgetc(f)==EOF && !fclose(f));
        assert(!memcmp(gold,output,written) && !memcmp(before,pcm,sizeof(pcm)));assert(!unlink(argv[1]));
        sink=(struct sink_state){0,SIZE_MAX};p.samples[0].pcm.rate++;
        assert(pt_mod_export_stream(&p,policy,collect,&sink)==PT_PROJECT_UNSUPPORTED && !sink.pos);
        assert(pt_mod_file_save(argv[1],&p,policy,&a)==PT_SAVE_INVALID && !b.live && access(argv[1],F_OK));p.samples[0].pcm.rate--;
    }
    sink=(struct sink_state){0,SIZE_MAX};assert(pt_mod_export_stream(&p,3,collect,&sink)==PT_PROJECT_INVALID && !sink.pos);
    assert(pt_mod_export_stream(&p,0,collect,&sink)==PT_PROJECT_UNSUPPORTED && !sink.pos);
    printf("MOD STREAM PASS: direct/round8/TPDF exact multi-block bytes, legacy headers, loops, 65 patterns, source and destination protection; workspace=%lu\n",(unsigned long)b.peak);
    return 0;
}
#ifndef PT_MOD_STREAM_NATIVE
int main(int argc,char **argv) {return mod_stream_fixture(argc,argv);}
#endif
