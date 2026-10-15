#define main pt_sampler_paula_original_main
#include "sampler_paula_test.c"
#undef main
#include "../src/core/playback_internal.h"
static unsigned source_checks,format_checks;
static void source_check(struct pt_sampler_paula *owner,unsigned track,unsigned sample,unsigned channel,
    struct pt_cache_lease lease,int wanted)
{
    struct pt_sampler_paula before;struct pt_sampler sampler;
    const uint8_t *data=(const uint8_t *)(uintptr_t)1;size_t bytes=17;
    size_t fast=fast_calls,chip=chip_calls;int result;
    memcpy(&before,owner,sizeof(before));memcpy(&sampler,owner->sampler,sizeof(sampler));
    result=pt_sampler_paula_prepared_source_location_validated(owner,track,sample,channel,lease,&data,&bytes);
    assert(result==wanted);
    if(wanted)assert(data&&data!=(const uint8_t *)(uintptr_t)1&&bytes==4);
    else assert(data==(const uint8_t *)(uintptr_t)1&&bytes==17);
    assert(!memcmp(owner,&before,sizeof(before))&&!memcmp(owner->sampler,&sampler,sizeof(sampler)));
    assert(fast_calls==fast&&chip_calls==chip);++source_checks;
}
static void source_binding(unsigned bits)
{
    struct pt_allocator a={NULL,fast_alloc,fast_free};struct pt_document d;struct pt_sampler sampler;
    struct pt_sampler_paula owner={0},before;struct pt_cache_lease lease[4],bad,unpublished={99,99};
    struct pt_sampler_paula_job job={0};struct pt_sample_version *pin;struct pt_pcm pcm;
    struct pt_playback_format f={8,0,0,1},changed;unsigned i;
    int32_t stereo[6]={-1,1,-2,2,-3,3},mono[3]={-1,-2,-3};
    const uint8_t *data=NULL;size_t bytes=17;
    pt_document_init(&d,&a);assert(pt_document_new(&d,4,SIZE_MAX)==PT_PROJECT_OK);
    d.project.channels.track[3].route=PT_AMIGUS;
    d.project.samples[0].pcm=d.project.samples[1].pcm=(struct pt_pcm){stereo,6,3,8000,2,(uint8_t)bits};
    d.project.samples[2].pcm=d.project.samples[3].pcm=(struct pt_pcm){mono,3,3,8000,1,(uint8_t)bits};
    pt_sampler_init(&sampler,&a,1024*1024);
    assert(pt_sampler_paula_bind(&owner,&sampler,&d.project,NULL,chip_alloc,chip_free,32));
    assert(pt_sampler_paula_acquire(&owner,0,0,0,lease)==PT_CACHE_LOAD);
    assert(pt_sampler_paula_acquire(&owner,0,0,1,lease+1)==PT_CACHE_LOAD);
    assert(pt_sampler_paula_acquire(&owner,1,1,0,lease+2)==PT_CACHE_LOAD);
    assert(pt_sampler_paula_acquire(&owner,2,2,0,lease+3)==PT_CACHE_LOAD);
    assert(pt_sampler_paula_prepared_current(&owner));
    source_check(&owner,0,0,0,lease[0],1);source_check(&owner,1,0,1,lease[1],1);
    source_check(&owner,2,1,0,lease[2],1);source_check(&owner,0,2,0,lease[3],1);
    source_check(&owner,0,1,0,lease[0],0); /* identical original PCM, different sample identity */
    source_check(&owner,0,0,1,lease[0],0);source_check(&owner,0,0,0,lease[1],0);
    source_check(&owner,0,2,1,lease[3],0);source_check(&owner,0,PT_PROJECT_SAMPLES,0,lease[0],0);
    source_check(&owner,3,0,0,lease[0],0);
    bad=lease[0];bad.slot=PT_CACHE_SLOTS;source_check(&owner,0,0,0,bad,0);
    bad=lease[0];++bad.serial;source_check(&owner,0,0,0,bad,0);
    memcpy(&before,&owner,sizeof(before));
    assert(!pt_sampler_paula_prepared_source_location_validated(&owner,0,0,0,lease[0],NULL,&bytes)&&bytes==17);
    assert(!pt_sampler_paula_prepared_source_location_validated(&owner,0,0,0,lease[0],&data,NULL)&&!data);
    assert(!memcmp(&owner,&before,sizeof(before)));source_checks+=2;
    for(i=0;i<12;++i){
        int wanted=i==0;changed=f;
        if(i==1)changed.bits=16;else if(i==2)changed.channel=1;else if(i==3)changed.little_endian=1;
        else if(i==4)changed.word_pad=0;else if(i==5)changed.bits=24;
        else if(i==6)changed.channel=~0U;else if(i==7)changed.little_endian=2;else if(i==8)changed.word_pad=2;
        assert(pt_playback_pcm_lease_matches(&owner.cache,i==10?2:1,owner.version+(i==11),i==9?NULL:&changed,lease[0])==wanted);
        assert(!memcmp(&owner,&before,sizeof(before)));++format_checks;
    }
    assert(pt_sampler_pin(&sampler,&d.project,3,sampler.generation,&pcm,&pin)==PT_EDIT_OK);
    assert(pt_sampler_paula_job_begin(&job,&owner,0,3,0,pin,&unpublished)==PT_CACHE_PENDING);
    source_check(&owner,0,3,0,job.upload.lease,0);
    pt_sampler_paula_job_cancel(&job);pt_sampler_unpin(pin);
    assert(!pt_sampler_paula_close(&owner));
    for(i=0;i<4;++i)source_check(&owner,0,i==2?1:i==3?2:0,i==1,lease[i],0);
    for(i=0;i<4;++i)assert(pt_sampler_paula_unpin(&owner,lease[i]));
    assert(pt_sampler_paula_close(&owner));pt_sampler_release(&sampler);pt_document_release(&d);
    assert(!fast_live&&!chip_live&&!sampler.bytes);
}
int main(void)
{
    assert(pt_sampler_paula_original_main()==0);
    source_binding(8);source_binding(16);source_binding(24);
    assert(source_checks==57&&format_checks==36);
    puts("PAULA SOURCE BINDING PASS: 57 source/lease checks, 36 representation checks; 8/16/24 masters, explicit stereo side, no lookup mutation, zero owned resources");
    return 0;
}
