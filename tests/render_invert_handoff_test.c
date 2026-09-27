#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "document.h"
#include "render_invert.h"
static void *allocate(void *c,size_t n){(void)c;return malloc(n);}
static void release(void *c,void *p){(void)c;free(p);}
static unsigned word(const unsigned char *p){return p[0]*256U+p[1];}
static uint32_t lng(const unsigned char *p){return (uint32_t)word(p)*65536+word(p+2);}
struct oracle {
    unsigned char *data,*initial;size_t size;
    unsigned char record[100][164];uint64_t tick_end[100];unsigned ticks;
    unsigned tick,applied,end,last_trigger,repeat_loop,changes,delayed_changes;uint64_t phase,frames;
};
static void reset(struct oracle *o)
{
    memcpy(o->data,o->initial,o->size);o->tick=0;o->applied=0;o->end=0;
    o->last_trigger=0;o->repeat_loop=0;o->changes=0;o->delayed_changes=0;o->phase=0;o->frames=0;
}
static int receive(void *ctx,const struct pt_pcm *pcm,uint64_t offset)
{
    struct oracle *o=ctx;unsigned i;
    assert(offset==o->frames && pcm->channels==2 && pcm->bits==24);
    for(i=0;i<pcm->frames;++i) {
        const unsigned char *r;unsigned loop,length;int value;
        while(o->tick<o->ticks && offset+i>=o->tick_end[o->tick])++o->tick;
        assert(o->tick<o->ticks);r=o->record[o->tick];loop=lng(r+58);length=word(r+62)*2;
        assert((length==2 || length==16) && loop+length<=o->size);
        if(o->applied!=o->tick+1) {
            /* Independent reference snapshots: do not call production EFx code. */
            memcpy(o->data+loop,r+148,length);o->applied=o->tick+1;
        }
        if(word(r+72)!=o->last_trigger) {
            o->phase=(uint64_t)lng(r+66)<<32;o->end=lng(r+66)+word(r+70)*2;
            o->last_trigger=word(r+72);o->repeat_loop=loop;
        }
        assert((o->phase>>32)<o->size);value=o->data[o->phase>>32];if(value>127)value-=256;
        if(pcm->data[i*2]!=value*1024*(int)r[32] || pcm->data[i*2+1]!=0) {
            fprintf(stderr,"EFx handoff mismatch frame=%llu tick=%u phase=%llu got=%ld expected=%d\n",
                (unsigned long long)(offset+i),o->tick,(unsigned long long)o->phase,(long)pcm->data[i*2],value*1024*(int)r[32]);abort();
        }
        assert(word(r+44));o->phase+=(428ULL<<32)/word(r+44);
        if((o->phase>>32)>=o->end) {
            if(loop!=o->repeat_loop) {
                /* Demonstrate the transfer waits for a repeat boundary inside
                   the tick, retaining the fractional phase remainder. */
                assert(o->tick && offset+i>=o->tick_end[o->tick-1]);++o->changes;
                if(offset+i>o->tick_end[o->tick-1])++o->delayed_changes;
            }
            o->phase-=(uint64_t)o->end<<32;o->phase%=((uint64_t)length<<32);
            o->phase+=(uint64_t)loop<<32;o->end=loop+length;o->repeat_loop=loop;
        }
    }
    o->frames+=pcm->frames;return 1;
}
static int count(void *ctx,const struct pt_pcm *p,uint64_t offset){(void)p;(void)offset;++*(unsigned *)ctx;return 1;}
static void refused(const struct pt_project *p,const struct pt_render_options *options,const struct pt_allocator *a)
{
    struct pt_render_report report,before;unsigned calls=0;enum pt_render_result result;
    struct pt_render_invert_session *session=NULL;
    memset(&report,0xa5,sizeof(report));before=report;
    result=pt_render_invert_stream(p,options,count,&calls,NULL,NULL,&report,SIZE_MAX,a);
    assert((result==PT_RENDER_EFFECT || result==PT_RENDER_SAMPLE) && !calls && !memcmp(&report,&before,sizeof(report)));
    result=pt_render_invert_open(p,options,SIZE_MAX,a,&session);
    assert((result==PT_RENDER_EFFECT || result==PT_RENDER_SAMPLE) && session==NULL);
}
int main(int argc,char **argv)
{
    struct pt_allocator a={NULL,allocate,release};struct pt_document d;struct oracle o;
    struct pt_render_options options;struct pt_render_report report;FILE *f;long size;
    unsigned i,offset=2108,partitions[]={1,17,256},calls=0;uint64_t time=0;
    int32_t *masters[31]={0};unsigned char r[164];
    assert(argc==3);memset(&o,0,sizeof(o));memset(&options,0,sizeof(options));
    f=fopen(argv[1],"rb");assert(f && !fseek(f,0,SEEK_END));size=ftell(f);assert(size>0);rewind(f);
    o.size=(size_t)size;o.initial=malloc(o.size);o.data=malloc(o.size);
    assert(o.initial && o.data && fread(o.initial,1,o.size,f)==o.size && !fclose(f));
    pt_document_init(&d,&a);assert(pt_document_load(&d,o.initial,o.size,SIZE_MAX)==PT_PROJECT_OK);
    for(i=0;i<31;++i) {
        struct pt_sample *s=d.project.samples+i;s->pcm.rate=48000;
        if(s->pcm.frames) {
            masters[i]=malloc(s->pcm.frames*sizeof(int32_t));assert(masters[i]);
            memcpy(masters[i],s->pcm.data,s->pcm.frames*sizeof(int32_t));
            if(!s->loop)o.initial[offset]=o.initial[offset+1]=0;
        }
        offset+=s->pcm.frames;
    }
    assert(offset==o.size);f=fopen(argv[2],"rb");assert(f);
    while(fread(r,1,164,f)==164) {
        if(!r[14] || !word(r+28))continue;
        assert(o.ticks<100 && word(r+12)>=32);memcpy(o.record[o.ticks],r,164);
        time+=(120000ULL<<32)/word(r+12);o.tick_end[o.ticks++]=time>>32;
    }
    assert(feof(f) && !ferror(f) && !fclose(f) && o.ticks==18);
    d.project.channels.track[0].pan=0;options.rate=48000;options.bits=24;options.gain_q16=65536;
    options.tracks=1;options.tick_limit=100;options.frame_limit=100000;
    assert(pt_render_stream(&d.project,&options,count,&calls,NULL,NULL,&report)==PT_RENDER_EFFECT && !calls);
    reset(&o);
    assert(pt_render_invert_stream(&d.project,&options,receive,&o,NULL,NULL,&report,SIZE_MAX,&a)==PT_RENDER_OK);
    assert(report.frames==o.frames && o.frames==o.tick_end[o.ticks-1] && o.changes==2 && o.delayed_changes>=1);
    for(i=0;i<3;++i) {
        struct pt_render_invert_session *session=NULL;unsigned done=0,pulls=0;const struct pt_pcm *pcm;
        reset(&o);assert(pt_render_invert_open(&d.project,&options,SIZE_MAX,&a,&session)==PT_RENDER_OK);
        while(!done) {
            assert(++pulls<20000 && pt_render_invert_pull(session,partitions[i],&pcm,&done)==PT_RENDER_OK);
            if(pcm)assert(receive(&o,pcm,o.frames));
        }
        assert(o.frames==o.tick_end[o.ticks-1] && o.changes==2 && o.delayed_changes>=1);pt_render_invert_close(session);
    }
    for(i=0;i<31;++i)if(masters[i])assert(!memcmp(masters[i],d.project.samples[i].pcm.data,d.project.samples[i].pcm.frames*sizeof(int32_t)));
    d.project.samples[1].interpolation=1;refused(&d.project,&options,&a);d.project.samples[1].interpolation=0;
    d.project.samples[1].pcm.bits=16;refused(&d.project,&options,&a);d.project.samples[1].pcm.bits=8;
    d.project.samples[1].pcm.bits=24;refused(&d.project,&options,&a);d.project.samples[1].pcm.bits=8;
    d.project.samples[1].pcm.rate=44100;refused(&d.project,&options,&a);d.project.samples[1].pcm.rate=48000;
    for(i=0;i<31;++i)free(masters[i]);pt_document_release(&d);free(o.data);free(o.initial);
    puts("EFx handoff reference PCM PASS: offline and pull1/17/256, preserved masters, refusal boundaries");return 0;
}
