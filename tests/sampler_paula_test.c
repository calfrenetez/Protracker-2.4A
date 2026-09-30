#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/editor/sampler_paula.h"
static size_t fast_live,fast_calls,fast_fail,chip_live,chip_calls;
static unsigned chip_fail;
static void *fast_alloc(void *c,size_t n)
{void *p;(void)c;if(++fast_calls==fast_fail)return NULL;p=malloc(n);if(p)++fast_live;return p;}
static void fast_free(void *c,void *p) {(void)c;if(p){assert(fast_live);--fast_live;free(p);}}
static void *chip_alloc(void *c,size_t n)
{
    void *p;(void)c;++chip_calls;if(chip_fail)return NULL;
#ifdef PT_TEST_NATIVE_CHIP
    p=pt_paula_chip_allocate(NULL,n);if(p)assert((TypeOfMem(p)&(MEMF_CHIP|MEMF_FAST))==MEMF_CHIP);
#else
    p=malloc(n);
#endif
    if(p)chip_live+=n;
    return p;
}
static void chip_free(void *c,void *p,size_t n)
{
    (void)c;assert(p && chip_live>=n);chip_live-=n;
#ifdef PT_TEST_NATIVE_CHIP
    assert((TypeOfMem(p)&(MEMF_CHIP|MEMF_FAST))==MEMF_CHIP);pt_paula_chip_release(NULL,p,n);
#else
    free(p);
#endif
}
static void fixture(unsigned bits)
{
    struct pt_allocator a={NULL,fast_alloc,fast_free};struct pt_document d;struct pt_sampler sampler;
    struct pt_sampler_paula owner={0};struct pt_pattern_history history;
    struct pt_pattern_command commands[4];struct pt_event_change changes[4];
    struct pt_cache_lease old,newer,hit,other,sentinel={99,99};const uint8_t *data,*old_data;size_t bytes,before;
    int32_t mono[3]={-(1L<<(bits-1)),1,(1L<<(bits-1))-1},saved[3];
    int32_t stereo[6]={1,-(1L<<(bits-1)),1,0,1,(1L<<(bits-1))-1};unsigned i;
    memcpy(saved,mono,sizeof(saved));pt_document_init(&d,&a);assert(pt_document_new(&d,16,SIZE_MAX)==PT_PROJECT_OK);
    for(i=0;i<16;++i)d.project.channels.track[i].route=PT_AMIGUS;
    d.project.channels.track[0].route=PT_MIDI;
    for(i=4;i<11;i+=3)d.project.channels.track[i].route=PT_PAULA;
    d.project.channels.track[15].route=PT_PAULA;
    d.project.samples[0].pcm=(struct pt_pcm){mono,3,3,48000,1,(uint8_t)bits};
    d.project.samples[1].pcm=(struct pt_pcm){stereo,6,3,48000,2,(uint8_t)bits};
    d.project.samples[2].pcm=d.project.samples[0].pcm;
    pt_sampler_init(&sampler,&a,1024*1024);
    assert(pt_pattern_history_init(&history,&d.project,commands,4,changes,4)==PT_EDIT_OK);
    before=fast_calls;assert(pt_sampler_paula_bind(&owner,&sampler,&d.project,NULL,chip_alloc,chip_free,8));
    assert(fast_calls==before && !chip_live);
    assert(!pt_sampler_paula_bind(&owner,&sampler,&d.project,NULL,chip_alloc,chip_free,8));
    old=sentinel;
    assert(pt_sampler_paula_acquire(&owner,0,0,0,&old)==PT_CACHE_INVALID); /* MIDI */
    assert(pt_sampler_paula_acquire(&owner,1,0,0,&old)==PT_CACHE_INVALID); /* AmiGUS */
    assert(pt_sampler_paula_acquire(&owner,4,0,1,&old)==PT_CACHE_INVALID);
    assert(old.slot==99 && old.serial==99 && fast_calls==before && !chip_live);
    d.project.channels.track[14].route=PT_PAULA;
    assert(pt_sampler_paula_acquire(&owner,4,0,0,&old)==PT_CACHE_INVALID && old.slot==99 && !chip_live);
    d.project.channels.track[14].route=PT_AMIGUS;
    fast_fail=fast_calls+1;
    assert(pt_sampler_paula_acquire(&owner,4,0,0,&old)==PT_CACHE_CAPACITY);fast_fail=0;
    assert(!sampler.bytes && !chip_live && old.slot==99);
    chip_fail=1;assert(pt_sampler_paula_acquire(&owner,4,0,0,&old)==PT_CACHE_CAPACITY);chip_fail=0;
    assert(!chip_live && old.slot==99); /* Unchanged promoted master may remain. */
    assert(pt_sampler_paula_acquire(&owner,4,0,0,&old)==PT_CACHE_LOAD);
    assert(pt_sampler_paula_location(&owner,4,old,&old_data,&bytes) && bytes==4);
    assert(old_data[0]==128 && old_data[1]==(bits==8?1:0) && old_data[2]==127 && old_data[3]==0);
    before=chip_calls;assert(pt_sampler_paula_acquire(&owner,15,0,0,&hit)==PT_CACHE_HIT);
    assert(chip_calls==before && hit.slot==old.slot && hit.serial==old.serial);
    d.project.channels.track[4].muted=1;d.project.channels.track[7].solo=1;d.project.channels.selected=15;
    assert(pt_sampler_paula_location(&owner,4,old,&data,&bytes));
    assert(pt_sampler_paula_unpin(&owner,hit));
    assert(pt_sampler_edit(&sampler,&d.project,&history,0,PT_PCM_REVERSE,0,3,0)==PT_EDIT_OK);
    data=NULL;bytes=123;assert(!pt_sampler_paula_location(&owner,4,old,&data,&bytes) && !data && bytes==123);
    assert(chip_live==4 && old_data[0]==128 && old_data[2]==127);
    assert(pt_sampler_paula_acquire(&owner,7,0,0,&newer)==PT_CACHE_LOAD && chip_live==8);
    assert(pt_sampler_paula_location(&owner,7,newer,&data,&bytes) && data[0]==127 && data[2]==128);
    other=sentinel;assert(pt_sampler_paula_acquire(&owner,10,1,1,&other)==PT_CACHE_CAPACITY && other.slot==99);
    assert(pt_sampler_paula_unpin(&owner,old) && chip_live==4);
    assert(!pt_sampler_paula_unpin(&owner,old));
    assert(pt_sampler_paula_acquire(&owner,10,1,1,&other)==PT_CACHE_LOAD && chip_live==8);
    assert(pt_sampler_paula_location(&owner,10,other,&data,&bytes) && data[0]==128 && data[1]==0 && data[2]==127 && data[3]==0);
    assert(pt_sampler_paula_unpin(&owner,other));
    /* Pressure evicts only the unpinned stereo representation. */
    assert(pt_sampler_paula_acquire(&owner,15,2,0,&other)==PT_CACHE_LOAD && chip_live==8);
    assert(!memcmp(mono,saved,sizeof(saved)) && d.project.samples[0].pcm.bits==bits);
    assert(pt_pattern_undo(&d.project,&history,-1)==PT_EDIT_OK);
    assert(!pt_sampler_paula_location(&owner,7,newer,&data,&bytes));
    assert(!pt_sampler_paula_close(&owner));
    assert(pt_sampler_paula_acquire(&owner,7,0,0,&hit)==PT_CACHE_INVALID);
    assert(chip_live==8 && pt_sampler_paula_unpin(&owner,newer) && pt_sampler_paula_unpin(&owner,other));
    assert(!chip_live && pt_sampler_paula_close(&owner) && pt_sampler_paula_close(&owner));
    assert(pt_sampler_paula_bind(&owner,&sampler,&d.project,NULL,chip_alloc,chip_free,4));
    assert(pt_sampler_paula_acquire(&owner,4,0,0,&old)==PT_CACHE_LOAD);
    /* Route removal invalidates even without a sampler PCM generation change. */
    d.project.channels.track[4].route=PT_AMIGUS;
    assert(!pt_sampler_paula_location(&owner,4,old,&data,&bytes) && chip_live==4);
    assert(pt_sampler_paula_unpin(&owner,old) && !chip_live);
    assert(pt_sampler_paula_acquire(&owner,7,0,0,&old)==PT_CACHE_LOAD);
    owner.version=UINT64_MAX;d.project.channels.track[4].route=PT_PAULA;
    assert(!pt_sampler_paula_sync(&owner) && owner.closing && chip_live==4);
    assert(!pt_sampler_paula_close(&owner));assert(pt_sampler_paula_unpin(&owner,old));
    assert(pt_sampler_paula_close(&owner));
    pt_pattern_history_release(&history);pt_sampler_release(&sampler);pt_document_release(&d);
    assert(!fast_live && !chip_live && !sampler.bytes);
}
int main(void)
{
    fixture(8);fixture(16);fixture(24);
    puts("SAMPLER PAULA PASS: selected routed8-bit copies, explicit stereo side, odd padding, master preservation, revisions/undo/routes, pressure eviction and retained active leases; no DMA or mixed-backend playback");return 0;
}
