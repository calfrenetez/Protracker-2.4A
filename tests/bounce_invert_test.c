#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/editor/bounce_invert.h"
#ifdef __amigaos__
static void test_failure(const char *condition,unsigned line)
{fprintf(stderr,"INVERT BOUNCE FAIL: line=%u condition=%s\n",line,condition);exit(20);}
#undef assert
#define assert(condition) ((condition)?(void)0:test_failure(#condition,__LINE__))
#endif
static unsigned calls,fail,live;
static void *allocate(void *c,size_t n){void *p;(void)c;if(++calls==fail)return NULL;p=malloc(n);if(p)++live;return p;}
static void release(void *c,void *p){(void)c;assert(live);--live;free(p);}
static struct pt_pattern_command commands[16];
static struct pt_event_change changes[32];
static int cancel(void *c,enum pt_render_phase phase,uint32_t t,uint64_t f)
{unsigned mode=*(unsigned *)c;(void)t;return mode==1?phase!=PT_RENDER_ANALYSE:!(phase==PT_RENDER_MIX && f>=256);}
static void check(const struct pt_sample *s)
{
    unsigned i;assert(s->pcm.bits==24 && s->pcm.channels==2 && s->pcm.frames==2880 && !s->loop);
    for(i=0;i<2880;++i){unsigned index=i%4,tick=i/960;int value=(int)(index+1)*10;if(index && index<=tick+1)value=-1-value;
        assert(s->pcm.data[i*2]==value*65536 && !s->pcm.data[i*2+1]);}
}
int main(void)
{
    struct pt_allocator a={NULL,allocate,release};struct pt_document d,copy;struct pt_sampler s;struct pt_pattern_history h;
    struct pt_project *p;struct pt_sample *original;struct pt_render_options o={0};struct pt_render_report r,before;
    enum pt_render_result detail;unsigned i,base,count,mode;int32_t pcm[4]={10,20,30,40},saved[4];size_t n,w;uint8_t *encoded;
    pt_document_init(&d,&a);pt_document_init(&copy,&a);assert(pt_document_new(&d,4,SIZE_MAX)==PT_PROJECT_OK);p=&d.project;
    p->samples[0].pcm=(struct pt_pcm){pcm,4,4,48000,1,8};p->samples[0].volume=64;p->samples[0].loop=PT_LOOP_FORWARD;p->samples[0].loop_end=4;p->speed=3;
    p->events[0].kind=PT_NOTE_PERIOD;p->events[0].pitch=428;p->events[0].instrument=1;
    p->events[1].instrument=1;p->events[1].effect=14;p->events[1].parameter=255;p->events[4].effect=15;
    o.rate=48000;o.bits=24;o.gain_q16=65536;o.tracks=1;o.frame_limit=100000;o.tick_limit=100;
    pt_sampler_init(&s,&a,1024*1024);assert(pt_pattern_history_init(&h,p,commands,16,changes,32)==PT_EDIT_OK);
    memcpy(saved,pcm,sizeof(pcm));original=p->samples;memset(&before,0x55,sizeof(before));r=before;base=live;
    /* All private planning and filling allocations, including failure after
       output/undo staging, must roll back without discarding master PCM. */
    for(i=1;i<=11;++i){fail=calls+i;
        assert(pt_sampler_bounce_invert(&s,p,&h,&o,"EF BOUNCE",NULL,NULL,&r,&detail,100000)==PT_EDIT_CAPACITY);
        assert(live==base && !s.bytes && !s.table && !h.count && p->samples==original && p->sample_count==31 && !memcmp(&r,&before,sizeof(r)) && !memcmp(pcm,saved,sizeof(pcm)));
    }
    fail=0;assert(pt_sampler_bounce_invert(&s,p,&h,&o,"EF BOUNCE",NULL,NULL,&r,&detail,1)==PT_EDIT_CAPACITY && detail==PT_RENDER_MEMORY && live==base);
    for(mode=1;mode<=2;++mode)assert(pt_sampler_bounce_invert(&s,p,&h,&o,"CANCEL",cancel,&mode,&r,&detail,100000)==PT_EDIT_CANCELLED && live==base && !h.count);
    assert(pt_sampler_bounce_invert(&s,p,&h,&o,"EF BOUNCE",NULL,NULL,&r,&detail,100000)==PT_EDIT_OK);
    assert(r.frames==2880 && p->sample_count==32 && h.count==1 && h.cursor==1);check(p->samples+31);
    assert(!memcmp(pcm,saved,sizeof(pcm)) && p->samples[0].pcm.data==pcm);
    assert(pt_pattern_undo(p,&h,-1)==PT_EDIT_OK && p->samples==original && p->sample_count==31);base=live;
    assert(pt_sampler_bounce_invert(&s,p,&h,&o,"REFUSED",NULL,NULL,&r,&detail,1)==PT_EDIT_CAPACITY && h.count==1 && h.cursor==0 && live==base);
    assert(pt_pattern_undo(p,&h,1)==PT_EDIT_OK);check(p->samples+31);
    /* Save/load keeps both unchanged source and generated true24 master. */
    assert(pt_project_size(p,&n)==PT_PROJECT_OK);encoded=malloc(n);assert(encoded);
    assert(pt_project_encode(p,encoded,n,&w)==PT_PROJECT_OK && pt_document_load(&copy,encoded,n,SIZE_MAX)==PT_PROJECT_OK);
    assert(!memcmp(copy.project.samples[0].pcm.data,saved,sizeof(saved)));check(copy.project.samples+31);free(encoded);
    assert(pt_pattern_undo(p,&h,-1)==PT_EDIT_OK);base=live;count=calls;
    assert(pt_sampler_bounce_invert(&s,p,&h,&o,"REPLACE REDO",NULL,NULL,&r,&detail,100000)==PT_EDIT_OK);count=calls-count;check(p->samples+31);
    assert(pt_pattern_undo(p,&h,-1)==PT_EDIT_OK);base=live;
    for(i=1;i<=count;++i){size_t bytes=s.bytes;fail=calls+i;
        assert(pt_sampler_bounce_invert(&s,p,&h,&o,"NO MEMORY",NULL,NULL,&r,&detail,100000)==PT_EDIT_CAPACITY);
        assert(live==base && s.bytes==bytes && h.count==1 && !h.cursor && p->sample_count==31 && p->samples==original && !memcmp(pcm,saved,sizeof(pcm)));
    }
    fail=0;assert(pt_pattern_undo(p,&h,1)==PT_EDIT_OK);check(p->samples+31);
    pt_pattern_history_release(&h);pt_sampler_release(&s);pt_document_release(&d);pt_document_release(&copy);assert(!live);
    puts("INVERT BOUNCE PASS: shared mutation, exact24 PCM, immutable masters, allocation/budget/cancellation rollback, redo, undo and project save");return 0;
}
