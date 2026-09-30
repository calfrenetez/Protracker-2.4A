#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/editor/paula_voices.h"
struct driver {
    struct pt_paula_voice_plan plan[4];unsigned reading[4],starts,stops,controls,barriers;
    int start_result,stop_result[4],control_result,quiesce_result;
    size_t live,calls;unsigned fail,misaligned;
};
static void *chip_alloc(void *context,size_t bytes)
{
    struct driver *d=context;void *p;++d->calls;if(d->fail)return NULL;
#ifdef PT_TEST_NATIVE_CHIP
    p=pt_paula_chip_allocate(NULL,bytes);if(p)assert((TypeOfMem(p)&(MEMF_CHIP|MEMF_FAST))==MEMF_CHIP);
#else
    p=malloc(bytes+d->misaligned);if(p && d->misaligned)p=(uint8_t *)p+1;
#endif
    if(p)d->live+=bytes;
    return p;
}
static void chip_free(void *context,void *p,size_t bytes)
{
    struct driver *d=context;unsigned i;uintptr_t address=(uintptr_t)p;
    for(i=0;i<4;++i)if(d->reading[i])assert((uintptr_t)d->plan[i].data<address || (uintptr_t)d->plan[i].data>=address+bytes);
    assert(d->live>=bytes);d->live-=bytes;
#ifdef PT_TEST_NATIVE_CHIP
    pt_paula_chip_release(NULL,p,bytes);
#else
    free((uint8_t *)p-d->misaligned);
#endif
}
static int start(void *context,unsigned slot,const struct pt_paula_voice_plan *plan)
{
    struct driver *d=context;assert(slot<4 && !d->reading[slot]);
    assert(plan->data && !((uintptr_t)plan->data&1) && plan->words && plan->period && plan->volume<=64);
    d->plan[slot]=*plan;d->reading[slot]=1;++d->starts;return d->start_result;
}
static int stop(void *context,unsigned slot)
{
    struct driver *d=context;assert(slot<4);++d->stops;
    if(d->stop_result[slot]==1)d->reading[slot]=0;
    return d->stop_result[slot];
}
static int control(void *context,unsigned slot,uint16_t period,uint8_t volume)
{
    struct driver *d=context;assert(slot<4 && d->reading[slot] && period && volume<=64);
    ++d->controls;d->plan[slot].period=period;d->plan[slot].volume=volume;return d->control_result;
}
static int quiesce(void *context)
{struct driver *d=context;unsigned i;for(i=0;i<4;++i)assert(!d->reading[i]);++d->barriers;return d->quiesce_result;}
static void *fast_alloc(void *c,size_t n) {(void)c;return malloc(n);}
static void fast_free(void *c,void *p) {(void)c;free(p);}
static void fixture(unsigned bits)
{
    struct pt_allocator a={NULL,fast_alloc,fast_free};struct pt_document doc;struct pt_sampler sampler;
    struct pt_sampler_paula cache={0};struct pt_paula_voices owner={0};struct driver d={0};
    struct pt_paula_voice_api api={&d,start,stop,control};
    struct pt_paula_voice_request r={0,4,428,64},bad;
    struct pt_pattern_history history;struct pt_pattern_command commands[4];struct pt_event_change changes[4];
    int32_t samples[3]={-(1L<<(bits-1)),0,(1L<<(bits-1))-1},saved[3];
    int32_t stereo[6]={0,-(1L<<(bits-1)),0,0,0,(1L<<(bits-1))-1};
    const uint8_t *old;unsigned i,before;size_t calls;struct pt_cache_lease outside;
    memcpy(saved,samples,sizeof(saved));d.start_result=d.control_result=d.quiesce_result=1;
    for(i=0;i<4;++i)d.stop_result[i]=1;
    pt_document_init(&doc,&a);assert(pt_document_new(&doc,16,SIZE_MAX)==PT_PROJECT_OK);
    for(i=0;i<16;++i)doc.project.channels.track[i].route=PT_AMIGUS;
    doc.project.channels.track[0].route=PT_MIDI;
    for(i=4;i<11;i+=3)doc.project.channels.track[i].route=PT_PAULA;
    doc.project.channels.track[15].route=PT_PAULA;
    doc.project.samples[0].pcm=(struct pt_pcm){samples,3,3,48000,1,(uint8_t)bits};
    doc.project.samples[1].pcm=(struct pt_pcm){stereo,6,3,48000,2,(uint8_t)bits};
    pt_sampler_init(&sampler,&a,1024*1024);
    assert(pt_pattern_history_init(&history,&doc.project,commands,4,changes,4)==PT_EDIT_OK);
    assert(pt_sampler_paula_bind(&cache,&sampler,&doc.project,&d,chip_alloc,chip_free,8));
    assert(pt_sampler_paula_acquire(&cache,4,0,0,&outside)==PT_CACHE_LOAD);
    assert(!pt_paula_voices_bind(&owner,&cache,&api));
    assert(pt_sampler_paula_unpin(&cache,outside));
    assert(pt_paula_voices_bind(&owner,&cache,&api));
    assert(!pt_paula_voices_bind(&owner,&cache,&api));
    assert(pt_paula_voices_bind_quiesce(&owner,quiesce,&d));
    assert(!pt_paula_voices_bind_quiesce(&owner,quiesce,&d));
    assert(owner.map[4]==0 && owner.map[7]==1 && owner.map[10]==2 && owner.map[15]==3);
    calls=d.calls;
    assert(pt_paula_voices_trigger(&owner,0,0,0,&r)==PT_PAULA_VOICE_REFUSED);
    assert(pt_paula_voices_trigger(&owner,1,0,0,&r)==PT_PAULA_VOICE_REFUSED);
    assert(pt_paula_voices_trigger(&owner,16,0,0,&r)==PT_PAULA_VOICE_REFUSED);
    assert(pt_paula_voices_trigger(&owner,4,0,1,&r)==PT_PAULA_VOICE_REFUSED);
    assert(pt_paula_voices_trigger(&owner,4,99,0,&r)==PT_PAULA_VOICE_REFUSED);
    for(i=0;i<7;++i) {
        bad=r;
        switch(i){case 0:bad.offset=1;break;case 1:bad.length=3;break;case 2:bad.length=0;break;
            case 3:bad.length=131072;break;case 4:bad.length=6;break;case 5:bad.period=0;break;default:bad.volume=65;}
        assert(pt_paula_voices_trigger(&owner,4,0,0,&bad)==PT_PAULA_VOICE_REFUSED);
    }
    assert(d.calls==calls && !d.starts && !d.stops);
    assert(pt_paula_voices_trigger(&owner,4,0,0,&r)==PT_PAULA_VOICE_ACTIVE);
    old=d.plan[0].data;assert(old[0]==128 && old[2]==127 && old[3]==0 && d.plan[0].words==2);
    for(i=7;i<11;i+=3)assert(pt_paula_voices_trigger(&owner,i,0,0,&r)==PT_PAULA_VOICE_ACTIVE);
    assert(pt_paula_voices_trigger(&owner,15,0,0,&r)==PT_PAULA_VOICE_ACTIVE);
    assert(d.calls==calls && d.live==4); /* Four readers share one pinned copy. */
    before=d.stops;doc.project.channels.track[4].muted=1;doc.project.channels.track[15].solo=1;
    assert(pt_paula_voices_sync(&owner)==1 && d.stops==before && owner.map[15]==3);
    assert(pt_paula_voices_control(&owner,15,214,0)==PT_PAULA_VOICE_ACTIVE);
    before=d.controls;
    assert(pt_paula_voices_control(&owner,15,0,64)==PT_PAULA_VOICE_REFUSED);
    assert(pt_paula_voices_control(&owner,15,214,65)==PT_PAULA_VOICE_REFUSED && d.controls==before);
    d.control_result=0;assert(pt_paula_voices_control(&owner,15,214,32)==PT_PAULA_VOICE_UNCERTAIN);
    assert(pt_paula_voices_control(&owner,15,214,32)==PT_PAULA_VOICE_REFUSED && d.controls==before+1);
    d.stop_result[3]=0;assert(pt_paula_voices_stop(&owner,15)==0 && d.reading[3]);
    d.stop_result[3]=-1;assert(pt_paula_voices_stop(&owner,15)==-1 && d.reading[3]);
    d.stop_result[3]=2;assert(pt_paula_voices_stop(&owner,15)==-1 && d.reading[3]);
    d.stop_result[3]=1;assert(pt_paula_voices_stop(&owner,15)==1 && !d.reading[3]);
    assert(!memcmp(saved,samples,sizeof(saved)) && doc.project.samples[0].pcm.bits==bits);
    assert(pt_sampler_edit(&sampler,&doc.project,&history,0,PT_PCM_REVERSE,0,3,0)==PT_EDIT_OK);
    d.fail=1;before=d.stops;
    assert(pt_paula_voices_trigger(&owner,4,0,0,&r)==PT_PAULA_VOICE_REFUSED);
    assert(d.stops==before && d.reading[0] && old[0]==128 && d.live==4);d.fail=0;
    d.stop_result[0]=0;before=d.starts;
    assert(pt_paula_voices_trigger(&owner,4,0,0,&r)==PT_PAULA_VOICE_STOP_PENDING && d.starts==before);
    assert(d.live==8 && old[0]==128 && d.reading[0]); /* Candidate unpinned, old retained. */
    d.stop_result[0]=-1;
    assert(pt_paula_voices_trigger(&owner,4,0,0,&r)==PT_PAULA_VOICE_STOP_FAILED && d.starts==before);
    d.stop_result[0]=1;d.start_result=-1;
    assert(pt_paula_voices_trigger(&owner,4,0,0,&r)==PT_PAULA_VOICE_UNCERTAIN);
    assert(d.plan[0].data[0]==127 && d.plan[0].data!=old && d.live==8);
    assert(pt_paula_voices_stop(&owner,7)==1 && d.live==8);
    assert(pt_paula_voices_stop(&owner,10)==1 && d.live==4); /* Last old reader releases retired copy. */
    assert(pt_pattern_undo(&doc.project,&history,-1)==PT_EDIT_OK);
    assert(pt_paula_voices_sync(&owner)==1 && d.live==4 && d.reading[0] && d.plan[0].data[0]==127);
    d.start_result=0;bad=r;bad.offset=2;bad.length=2;
    assert(pt_paula_voices_trigger(&owner,15,1,1,&bad)==PT_PAULA_VOICE_UNCERTAIN);
    assert(d.plan[3].data[0]==127 && d.plan[3].data[1]==0 && d.live==8);
    assert(pt_paula_voices_trigger(&owner,7,0,0,&r)==PT_PAULA_VOICE_REFUSED && d.live==8); /* Both copies pinned. */
    /* Route reassignment cannot reuse a physical slot until stop is confirmed. */
    doc.project.channels.track[4].route=PT_AMIGUS;doc.project.channels.track[2].route=PT_PAULA;
    d.stop_result[0]=0;before=d.starts;
    assert(pt_paula_voices_sync(&owner)==0 && owner.map[4]==0 && owner.map[2]==-1 && d.live==8);
    assert(pt_paula_voices_trigger(&owner,2,0,0,&r)==PT_PAULA_VOICE_STOP_PENDING && d.starts==before);
    d.stop_result[0]=-1;assert(pt_paula_voices_sync(&owner)==-1 && d.reading[0]);
    d.stop_result[0]=1;assert(pt_paula_voices_sync(&owner)==1 && owner.map[4]==-1 && owner.map[2]==0);
    assert(owner.map[7]==1 && owner.map[10]==2 && owner.map[15]==3 && d.live==4);
    d.start_result=1;assert(pt_paula_voices_trigger(&owner,2,0,0,&r)==PT_PAULA_VOICE_ACTIVE && d.live==8);
    before=d.starts;doc.project.channels.track[3].route=PT_PAULA;
    assert(pt_paula_voices_sync(&owner)==-2 && d.starts==before && owner.map[3]==-1);
    doc.project.channels.track[3].route=PT_AMIGUS;
    /* Close attempts every reader once, even if one fails. No new trigger. */
    d.stop_result[0]=0;before=d.stops;
    assert(!pt_paula_voices_close(&owner) && d.stops==before+2 && d.reading[0] && !d.reading[3]);
    assert(pt_paula_voices_trigger(&owner,2,0,0,&r)==PT_PAULA_VOICE_REFUSED);
    d.stop_result[0]=1;d.quiesce_result=0;
    assert(!pt_paula_voices_close(&owner) && !d.reading[0] && owner.bridge && d.barriers==1);
    d.quiesce_result=-1;assert(!pt_paula_voices_close(&owner) && owner.bridge && d.barriers==2);
    d.quiesce_result=1;assert(pt_paula_voices_close(&owner) && !d.live && d.barriers==3);
    assert(pt_paula_voices_close(&owner));
    pt_pattern_history_release(&history);pt_sampler_release(&sampler);pt_document_release(&doc);
}
static void boundaries(void)
{
    struct pt_allocator a={NULL,fast_alloc,fast_free};struct pt_document doc;struct pt_sampler sampler;
    struct pt_sampler_paula cache={0};struct pt_paula_voices owner={0};struct driver d={0};
    struct pt_paula_voice_api api={&d,start,stop,NULL};struct pt_paula_voice_request r={0,131070,1,64};
    int32_t *pcm=malloc(131070*sizeof(*pcm));unsigned i;size_t calls;
    assert(pcm);memset(pcm,0,131070*sizeof(*pcm));d.start_result=2;d.quiesce_result=1;for(i=0;i<4;++i)d.stop_result[i]=1;
    pt_document_init(&doc,&a);assert(pt_document_new(&doc,4,SIZE_MAX)==PT_PROJECT_OK);
    doc.project.samples[0].pcm=(struct pt_pcm){pcm,131070,131070,48000,1,24};
    pt_sampler_init(&sampler,&a,2*1024*1024);
    assert(pt_sampler_paula_bind(&cache,&sampler,&doc.project,&d,chip_alloc,chip_free,131070));
    assert(pt_paula_voices_bind(&owner,&cache,&api));
    assert(pt_paula_voices_trigger(&owner,0,0,0,&r)==PT_PAULA_VOICE_UNCERTAIN && d.plan[0].words==65535);
    assert(pt_paula_voices_control(&owner,0,428,64)==PT_PAULA_VOICE_REFUSED && !d.controls);
    assert(pt_paula_voices_stop(&owner,0)==1);
    assert(!pt_paula_voices_bind_quiesce(&owner,quiesce,&d)); /* Cannot install late barrier. */
    d.start_result=1;r.offset=131068;r.length=2;r.period=65535;
    assert(pt_paula_voices_trigger(&owner,0,0,0,&r)==PT_PAULA_VOICE_ACTIVE && d.plan[0].words==1);
    calls=d.calls;r.offset=UINT32_MAX-1;
    assert(pt_paula_voices_trigger(&owner,0,0,0,&r)==PT_PAULA_VOICE_REFUSED && d.calls==calls && d.reading[0]);
    cache.version=UINT64_MAX;doc.project.channels.track[0].route=PT_AMIGUS;
    assert(pt_paula_voices_sync(&owner)==-2 && d.reading[0] && d.live==131070);
    assert(pt_paula_voices_trigger(&owner,1,0,0,&r)==PT_PAULA_VOICE_REFUSED);
    assert(pt_paula_voices_close(&owner) && !d.live);
#ifndef PT_TEST_NATIVE_CHIP
    d.misaligned=1;r.offset=0;r.length=2;
    assert(pt_sampler_paula_bind(&cache,&sampler,&doc.project,&d,chip_alloc,chip_free,131070));
    assert(pt_paula_voices_bind(&owner,&cache,&api));calls=d.starts;
    assert(pt_paula_voices_trigger(&owner,0,0,0,&r)==PT_PAULA_VOICE_REFUSED && d.starts==calls);
    assert(pt_paula_voices_close(&owner) && !d.live);
#endif
    pt_sampler_release(&sampler);pt_document_release(&doc);free(pcm);
}
int main(void) {fixture(8);fixture(16);fixture(24);boundaries();puts("paula voices tests passed");return 0;}
