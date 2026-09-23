#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/editor/bounce.h"
static unsigned calls,fail,live;
static void *allocate(void *c,size_t n) {(void)c;void *p;if(++calls==fail)return NULL;p=malloc(n);if(p)++live;return p;}
static void release(void *c,void *p) {(void)c;if(p) {assert(live);--live;free(p);}}
static struct pt_pattern_command commands[128];
static struct pt_event_change changes[2048];
static int cancel(void *ctx,enum pt_render_phase phase,uint32_t ticks,uint64_t frames)
{
    unsigned mode=*(unsigned *)ctx;(void)ticks;
    if(mode==1)return phase!=PT_RENDER_ANALYSE;
    if(mode==2)return phase!=PT_RENDER_MIX || frames<256;
    return phase!=PT_RENDER_MIX || frames<960;
}
static void check_pcm(const struct pt_sample *s)
{
    unsigned i;assert(s->pcm.frames==960 && s->pcm.bits==24 && s->pcm.channels==2 && s->pcm.rate==48000);
    assert(s->volume==64 && !s->finetune && !s->loop && !s->loop_start && !s->loop_end && !s->slice_count);
    for(i=0;i<960;++i)assert(s->pcm.data[i*2]==0x123457 && s->pcm.data[i*2+1]==-0x345679);
}
static enum pt_edit_result bad_fill(void *ctx,struct pt_pcm *pcm)
{(void)ctx;pcm->frames=0;return PT_EDIT_OK;}
int main(void)
{
    struct pt_allocator a={NULL,allocate,release};struct pt_document d,copy;struct pt_sampler s;struct pt_pattern_history h;
    struct pt_project *p;struct pt_sample *original;struct pt_event_update note;
    struct pt_render_options o;struct pt_render_report report,before;enum pt_render_result detail;
    unsigned i,base,mode,count,revision;size_t n,w,budget;uint8_t *encoded,*again;int32_t pcm[2]={0x123457,-0x345679};
    pt_document_init(&d,&a);pt_document_init(&copy,&a);assert(pt_document_new(&d,4,SIZE_MAX)==PT_PROJECT_OK);p=&d.project;
    p->samples[0].pcm=(struct pt_pcm){pcm,2,1,48000,2,24};p->samples[0].volume=64;
    p->samples[0].loop=PT_LOOP_FORWARD;p->samples[0].loop_end=1;p->channels.track[0].pan=128;p->speed=1;
    p->events[0].kind=PT_NOTE_PERIOD;p->events[0].pitch=428;p->events[0].instrument=1;p->events[4].effect=15;
    memset(&o,0,sizeof(o));o.rate=48000;o.bits=24;o.gain_q16=65536;o.tracks=1;o.frame_limit=100000;o.tick_limit=100;o.pattern_only=1;
    memset(&report,0x5a,sizeof(report));before=report;original=p->samples;
    pt_sampler_init(&s,&a,1024*1024);assert(pt_pattern_history_init(&h,p,commands,128,changes,2048)==PT_EDIT_OK);base=live;
    /* Fail measure, append record, sample table, PCM version, then mixer workspace. */
    for(i=1;i<=5;++i) {
        fail=calls+i;assert(pt_sampler_bounce(&s,p,&h,&o,"BOUNCE",NULL,NULL,&report,&detail)==PT_EDIT_CAPACITY);
        if(i==1 || i==5)assert(detail==PT_RENDER_MEMORY);
        assert(pcm[0]==0x123457 && pcm[1]==-0x345679);
        assert(!s.bytes && !s.table && live==base && p->samples==original && p->sample_count==31 && !h.count && !memcmp(&before,&report,sizeof(report)));
    }
    fail=0;budget=s.budget;s.budget=20000;
    /* This low budget refuses staging on both 32-bit and 64-bit hosts. */
    o.include_lead_in=1;o.frame_limit=100000;o.rate=48000;p->speed=6;
    assert(pt_sampler_bounce(&s,p,&h,&o,"BOUNCE",NULL,NULL,&report,&detail)==PT_EDIT_CAPACITY && !s.bytes && live==base);
    s.budget=budget;o.include_lead_in=0;p->speed=1;
    for(mode=1;mode<=3;++mode) {
        assert(pt_sampler_bounce(&s,p,&h,&o,"BOUNCE",cancel,&mode,&report,&detail)==PT_EDIT_CANCELLED && detail==PT_RENDER_CANCELLED);
        assert(!s.bytes && !s.table && live==base && !h.count && p->samples==original && p->sample_count==31 && !memcmp(&before,&report,sizeof(report)));
    }
    p->events[0].effect=14;p->events[0].parameter=0xf1;
    assert(pt_sampler_bounce(&s,p,&h,&o,"BOUNCE",NULL,NULL,&report,&detail)==PT_EDIT_UNSUPPORTED && detail==PT_RENDER_EFFECT && live==base);p->events[0].effect=0;p->events[0].parameter=0;
    h.next_revision=UINT32_MAX;
    assert(pt_sampler_bounce(&s,p,&h,&o,"BOUNCE",NULL,NULL,&report,&detail)==PT_EDIT_CAPACITY && !s.bytes && !s.table && live==base);h.next_revision=1;
    assert(pt_sampler_append_generated(&s,p,&h,&p->samples[0].pcm,"BAD",bad_fill,NULL)==PT_EDIT_INVALID && !s.bytes && live==base);
    assert(pt_sampler_bounce(&s,p,&h,&o,"BOUNCE",NULL,NULL,&report,&detail)==PT_EDIT_OK && detail==PT_RENDER_OK);
    assert(report.frames==960 && report.ticks==2 && !report.clipped && p->sample_count==32 && h.count==1 && h.cursor==1 && pt_pattern_dirty(&h));
    assert(p->samples[0].pcm.data==pcm && p->samples[31].pcm.data!=pcm && !strcmp(p->samples[31].name,"BOUNCE"));check_pcm(&p->samples[31]);
    p->events[8].instrument=32;assert(pt_pattern_undo(p,&h,-1)==PT_EDIT_CONFLICT);p->events[8].instrument=0;
    assert(pt_sampler_attributes(&s,p,&h,31,"RENAMED",32,0)==PT_EDIT_OK);
    assert(pt_sampler_edit(&s,p,&h,31,PT_PCM_GAIN,0,960,500)==PT_EDIT_OK);
    note=(struct pt_event_update){8,{428,0,PT_NOTE_PERIOD,32,0,0,0,0}};
    assert(pt_pattern_apply(p,&h,&note,1)==PT_EDIT_OK);
    for(i=0;i<4;++i)assert(pt_pattern_undo(p,&h,-1)==PT_EDIT_OK);
    assert(p->sample_count==31 && p->samples==original && !s.current[31] && !pt_pattern_dirty(&h));
    count=(unsigned)h.count;revision=h.revision;budget=s.bytes;base=live;mode=2;
    assert(pt_sampler_bounce(&s,p,&h,&o,"CANCEL",cancel,&mode,&report,&detail)==PT_EDIT_CANCELLED && s.bytes==budget && live==base && h.count==count && h.revision==revision && !h.cursor);
    /* Existing sample table and redo resources must survive late workspace failure. */
    before=report;
    for(i=1;i<=4;++i) {
        fail=calls+i;
        assert(pt_sampler_bounce(&s,p,&h,&o,"NO MEMORY",NULL,NULL,&report,&detail)==PT_EDIT_CAPACITY);
        assert(s.bytes==budget && live==base && h.count==count && h.revision==revision && !h.cursor);
        assert(p->samples==original && p->sample_count==31 && !memcmp(&before,&report,sizeof(report)));
        assert(pcm[0]==0x123457 && pcm[1]==-0x345679);
        if(i==1 || i==4)assert(detail==PT_RENDER_MEMORY);
    }
    fail=0;
    for(i=0;i<4;++i)assert(pt_pattern_undo(p,&h,1)==PT_EDIT_OK);
    assert(p->sample_count==32 && p->events[8].instrument==32 && p->samples[31].volume==32 && !strcmp(p->samples[31].name,"RENAMED"));
    assert(pt_project_size(p,&n)==PT_PROJECT_OK);encoded=malloc(n);again=malloc(n);assert(encoded && again);
    assert(pt_project_encode(p,encoded,n,&w)==PT_PROJECT_OK && w==n);
    assert(pt_document_load(&copy,encoded,n,SIZE_MAX)==PT_PROJECT_OK && copy.project.sample_count==32);
    assert(pt_project_encode(&copy.project,again,n,&w)==PT_PROJECT_OK && !memcmp(encoded,again,n));free(encoded);free(again);
    for(i=0;i<4;++i)assert(pt_pattern_undo(p,&h,-1)==PT_EDIT_OK);
    /* Reuse an undone bounced index as an empty slot, edit, then branch back to a bounce. */
    assert(pt_sampler_add_slot(&s,p,&h)==PT_EDIT_OK && p->sample_count==32 && !p->samples[31].pcm.frames);
    assert(pt_sampler_attributes(&s,p,&h,31,"EMPTY",12,0)==PT_EDIT_OK);
    assert(pt_pattern_undo(p,&h,-1)==PT_EDIT_OK && pt_pattern_undo(p,&h,-1)==PT_EDIT_OK);
    assert(pt_sampler_bounce(&s,p,&h,&o,"NEW",NULL,NULL,&report,&detail)==PT_EDIT_OK);check_pcm(&p->samples[31]);
    /* Evict the append resource: the active sample must still own its PCM. */
    for(i=0;i<130;++i)assert(pt_pattern_title_apply(p,&h,i&1?"A":"B")==PT_EDIT_OK);
    check_pcm(&p->samples[31]);pt_pattern_history_release(&h);check_pcm(&p->samples[31]);
    pt_sampler_release(&s);assert(!s.bytes);pt_document_release(&d);pt_document_release(&copy);assert(!live);
    puts("BOUNCE PASS: exact true24 PCM, atomic new slot, allocation/budget/cancellation rollback, redo preservation, mixed undo, references, table reuse, eviction and PTG identity");return 0;
}
