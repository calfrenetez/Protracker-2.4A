/* PRIVATE D2 SOURCE DRAFT. This file has not been compiled or run.
 * Keep the actual legacy factory constructor and typed queue fixture bodies. */
#define PT_MIXED_READERS_TEST_MAIN d2_legacy_main
#include "sampler_mixed_readers_test.c"
#include "../src/core/amigus_trigger_levels.h"

static uint32_t d2_markers[2]={0,2};
static struct smf_trial *d2_make_private(unsigned bits,unsigned cache_bits,unsigned stereo,
    unsigned all_card,unsigned frames,size_t card_budget,unsigned odd)
{
    struct smf_trial *f=smf_make(bits,cache_bits,stereo,frames,card_budget);
    struct resources *r=f->inherited->resources;
    struct pt_project *p=f->config.project;
    unsigned i,j;
    /* Establish genuine masters AFTER final metadata, before factory validation.
     * Retire the preliminary borrowed versions without following freed PCM. */
    for(i=0;i<p->sample_count;++i){pt_sampler_unpin(f->pins[i]);f->pins[i]=NULL;}
    pt_sampler_release(f->config.sampler);
    for(i=0;i<2;++i){p->samples[i].pcm.data=r->original[i];p->samples[i].pcm.capacity=64;}
    if(bits==24)for(i=0;i<2;++i)for(j=0;j<64;++j)r->original[i][j]+=(int32_t)(j%17);
    p->samples[0].slice_count=2;p->samples[0].slices=d2_markers;
    if(all_card){for(i=0;i<p->channels.count;++i)p->channels.track[i].route=PT_AMIGUS;
        p->samples[1].loop=PT_LOOP_FORWARD;p->samples[1].loop_start=cache_bits==8?2:1;
        p->samples[1].loop_end=frames-(cache_bits==8?2:1);p->samples[1].interpolation=1;}
    /* Negative geometry is final before genuine version creation, too. */
    if(odd){assert(all_card&&cache_bits==8&&frames==6);
        if(odd==2)p->samples[1].loop_end=5;else{assert(odd==1);p->samples[1].loop_start=1;}}
    for(i=0;i<p->sample_count;++i)
        assert(pt_sampler_pin(f->config.sampler,p,i,f->config.sampler->generation,
            f->pcm+i,f->pins+i)==PT_EDIT_OK);
    free(f->saved);f->saved=save(r,&f->saved_bytes);
    return f;
}
static struct smf_trial *d2_make(unsigned bits,unsigned cache_bits,unsigned stereo,
    unsigned all_card,unsigned frames,size_t card_budget)
{return d2_make_private(bits,cache_bits,stereo,all_card,frames,card_budget,0);}
static void d2_batch(struct smf_trial *f,struct pt_sampler_mixed_quantized_batch *b,
    unsigned n,unsigned little,unsigned mixed)
{
    unsigned i;
    static const uint16_t pair[][2]={{32639,32895},{0,0},{65535,65535},{0,65535},
        {65535,0},{1,65534},{7,19}};
    memset(b,0,sizeof(*b));b->count=n;f->count=n;
    for(i=0;i<n;++i){struct pt_sampler_mixed_request *x=b->action+i;
        x->kind=PT_MIXED_READERS_TRIGGER;x->track=i;x->sample=i%2;
        x->channel=f->config.project->samples[x->sample].pcm.channels==2?i%2:0;
        x->expected=f->pins[x->sample];
        if(f->config.project->channels.track[i].route==PT_PAULA){
            x->geometry.paula.period=428;x->geometry.paula.volume=64;
        }else{
            x->geometry.amigus.bits=f->inherited->resources->cache_bits;
            x->geometry.amigus.little_endian=little;
            x->geometry.amigus.trigger=(struct pt_amigus_voice_request){8000,1,0,0,0};
            if(mixed&&i%3==0){x->geometry.amigus.trigger.volume=64;
                x->geometry.amigus.trigger.pan=128;
            }else{
                b->levels[i].mode=PT_SAMPLER_MIXED_TRIGGER_QUANTIZED;
                b->levels[i].left=pair[i%7][0];b->levels[i].right=pair[i%7][1];
            }
        }
    }
}
static void d2_prepare(struct smf_trial *f,unsigned ci,uint64_t frame,
    const struct pt_sampler_mixed_quantized_batch *b)
{
    enum pt_sampler_mixed_result result;unsigned steps=0,completed=999,writes;
    struct model before=f->inherited->model;
    assert(pt_sampler_mixed_begin_quantized(f->pool,f->revision,frame,b,f->command+ci)==PT_SAMPLER_MIXED_PENDING);
    do{writes=f->inherited->resources->card.writes;
        result=pt_sampler_mixed_advance(f->pool,f->revision,f->command[ci],&completed);
        assert(f->inherited->resources->card.writes-writes<=128);
        assert(++steps<2000);
    }while(result==PT_SAMPLER_MIXED_PENDING);
    assert(result==PT_SAMPLER_MIXED_OK&&completed==b->count);
    assert(!memcmp(&before,&f->inherited->model,sizeof(before)));
}
static void d2_plan_equal(const struct pt_amigus_voice_plan *a,const struct pt_amigus_voice_plan *b)
{
    assert(a->start==b->start&&a->loop==b->loop&&a->end_exclusive==b->end_exclusive&&
        a->rate==b->rate&&a->control==b->control&&a->left==b->left&&a->right==b->right);
}
static void d2_oracles(struct smf_trial *f,uint64_t ticket,
    const struct pt_sampler_mixed_quantized_batch *b)
{
    unsigned i,j;struct model_command *c=model_command(&f->inherited->model,ticket);
    struct resources *r=f->inherited->resources;
    assert(c&&c->batch.count==b->count&&c->first==oracle(c->batch.frame)&&c->last==oracle(c->batch.frame+1));
    for(i=0;i<b->count;++i){const struct pt_sampler_mixed_request *x=b->action+i;
        const struct pt_mixed_readers_action *a=c->batch.action+i;
        const struct pt_mixed_readers_domain *d=c->reference->reader[i];
        const struct pt_pcm *pcm=&f->config.project->samples[x->sample].pcm;
        assert(a->kind==PT_MIXED_READERS_TRIGGER&&d->key.owner&&d->key.serial);
        if(a->route==PT_MIXED_READERS_PAULA){
            assert(a->geometry.paula.period==428&&a->geometry.paula.volume==64&&
                a->geometry.paula.words==((pcm->frames+1)&~1U)/2);
            for(j=0;j<pcm->frames;++j)
                assert(a->geometry.paula.data[j]==smf_convert8(pcm->data[(size_t)j*pcm->channels+x->channel],r->bits));
            if(pcm->frames&1)assert(!a->geometry.paula.data[pcm->frames]);
        }else{
            struct pt_playback_format format={(uint8_t)x->geometry.amigus.bits,(uint8_t)x->channel,
                (uint8_t)x->geometry.amigus.little_endian,0};
            struct pt_amigus_voice_request request=x->geometry.amigus.trigger;
            struct pt_amigus_voice_plan expected;
            struct pt_cache_lease lease={d->card.cache_slot,d->card.serial};
            void *resource=pt_cache_data(&r->card.cache.cache,lease);
            unsigned arena;
            for(arena=0;arena<PT_CACHE_SLOTS&&resource!=&r->card.cache.arena.block[arena];++arena){}
            assert(arena<PT_CACHE_SLOTS&&d->card.reservation==&r->card.reservation&&
                d->card.cache==&r->card.cache.cache&&d->card.version==1&&
                d->card.full_capacity==r->card.cache.arena.block[arena].reserved&&
                d->card.logical_bytes==pcm->frames*r->cache_bits/8);
            if(b->levels[i].mode==PT_SAMPLER_MIXED_TRIGGER_QUANTIZED){request.volume=64;request.pan=128;}
            assert(pt_amigus_voice_plan_prepare(f->config.project->samples+x->sample,&format,&request,
                d->card.address,d->card.logical_bytes,&expected));
            if(b->levels[i].mode==PT_SAMPLER_MIXED_TRIGGER_QUANTIZED){
                expected.left=b->levels[i].left;expected.right=b->levels[i].right;
            }
            d2_plan_equal(&a->geometry.amigus,&expected);
            for(j=0;j<pcm->frames;++j){int32_t value=pcm->data[(size_t)j*pcm->channels+x->channel];
                uint32_t address=d->card.address+j*r->cache_bits/8;
                if(r->cache_bits==8)assert(r->card.ram[address]==smf_convert8(value,r->bits));
                else{uint16_t v=smf_convert16(value,r->bits);
                    assert(r->card.ram[address]==(uint8_t)(format.little_endian?v:v>>8));
                    assert(r->card.ram[address+1]==(uint8_t)(format.little_endian?v>>8:v));}
            }
            for(j=d->card.logical_bytes;j<d->card.full_capacity;++j)assert(!r->card.ram[d->card.address+j]);
        }
    }
}
static void d2_drain(struct smf_trial *f,unsigned ci,unsigned base,unsigned count,
    uint64_t ticket,unsigned order,unsigned cancel)
{
    unsigned i;
    if(!order)assert(pt_sampler_mixed_service_command(f->pool,ticket,cancel,NULL)==PT_MIXED_READERS_OK);
    for(i=0;i<count;++i)assert(pt_sampler_mixed_service_reader(f->pool,ticket,i,1,NULL)==PT_MIXED_READERS_OK);
    if(order){assert(pt_mixed_readers_readers_held(f->inherited->queue)>=count);
        assert(pt_sampler_mixed_service_command(f->pool,ticket,cancel,NULL)==PT_MIXED_READERS_OK);}
    smf_close_handles(f,ci,base,count);
}
static void d2_lifetime(unsigned bits,unsigned cache_bits,unsigned little,unsigned all_card,unsigned order)
{
    struct smf_trial *f=d2_make(bits,cache_bits,1,all_card,cache_bits==8?6:5,0);
    struct pt_sampler_mixed_quantized_batch b,original;
    uint64_t ticket;unsigned steps=0,completed=999,writes;
    enum pt_sampler_mixed_result result;struct model before;
    smf_open(f);d2_batch(f,&b,16,little,1);original=b;before=f->inherited->model;
    assert(pt_sampler_mixed_begin_quantized(f->pool,f->revision,960,&b,f->command)==PT_SAMPLER_MIXED_PENDING);
    /* The original input expires on begin return. Only private copies may be
     * followed while preparation/queue owners remain genuine and held. */
    memset(&b,0xa5,sizeof(b));
    do{writes=f->inherited->resources->card.writes;
        result=pt_sampler_mixed_advance(f->pool,f->revision,f->command[0],&completed);
        assert(f->inherited->resources->card.writes-writes<=128&&++steps<2000);
    }while(result==PT_SAMPLER_MIXED_PENDING);
    if(result!=PT_SAMPLER_MIXED_OK||completed!=16||
        memcmp(&before,&f->inherited->model,sizeof(before)))
        fprintf(stderr,"D2 lifetime failure: master=%u cache=%u little=%u all_card=%u order=%u result=%u completed=%u modeldiff=%d steps=%u\n",
            bits,cache_bits,little,all_card,order,(unsigned)result,completed,
            memcmp(&before,&f->inherited->model,sizeof(before)),steps);
    assert(result==PT_SAMPLER_MIXED_OK&&completed==16&&
        !memcmp(&before,&f->inherited->model,sizeof(before)));
    smf_handles(f,0,0);
    assert(pt_sampler_mixed_enqueue(f->pool,f->revision,f->command[0],&ticket)==PT_MIXED_READERS_OK&&ticket==1);
    assert(pt_mixed_readers_commands_held(f->inherited->queue)==1&&pt_mixed_readers_readers_held(f->inherited->queue)==16);
    assert(pt_sampler_mixed_publish(f->pool,ticket)==PT_MIXED_READERS_OK);
    d2_oracles(f,ticket,&original);fire(&f->inherited->model,ticket);smf_verify(f);
    assert(!pt_sampler_mixed_command_close(f->pool,f->command)&&!pt_sampler_mixed_reader_close(f->pool,f->reader));
    d2_drain(f,0,0,16,ticket,order,0);smf_drop(f);
}
/* Every invalid semantic field below precedes callbacks and any pool mutation.
 * Compare actual complete owned pool bytes, original fixed input and sentinel. */
static void d2_admission(unsigned mode)
{
    struct smf_trial *f=d2_make(24,16,1,0,5,0);
    struct pt_sampler_mixed_quantized_batch b,copy;
    struct pt_sampler_mixed_command_handle out={(void *)(uintptr_t)1,999};
    const struct pt_sampler_mixed_quantized_batch *input=&b;
    struct pt_sampler_mixed_command_handle *output=&out;
    unsigned calls,chip,writes;void *pool_copy;
    uint64_t frame=960;
    smf_open(f);d2_batch(f,&b,5,0,0);
    switch(mode){
    case 0:b.count=0;break;case 1:b.count=17;break;
    case 2:b.levels[4].mode=(enum pt_sampler_mixed_trigger_level_mode)2;break;
    case 3:b.levels[4].mode=PT_SAMPLER_MIXED_TRIGGER_LEGACY;break;
    case 4:b.action[4].geometry.amigus.trigger.volume=1;break;
    case 5:b.action[4].geometry.amigus.trigger.pan=1;break;
    case 6:b.action[4].geometry.amigus.rate=1;break;
    case 7:b.action[4].geometry.amigus.left=1;break;
    case 8:b.action[4].geometry.amigus.right=1;break;
    case 9:b.levels[0].mode=PT_SAMPLER_MIXED_TRIGGER_QUANTIZED;break;
    case 10:b.levels[0].left=1;break;
    case 11:b.levels[15].mode=PT_SAMPLER_MIXED_TRIGGER_QUANTIZED;break;
    case 12:b.levels[15].left=1;break;case 13:b.levels[15].right=1;break;
    case 14:b.action[15].kind=PT_MIXED_READERS_STOP;break;
    case 15:b.action[15].track=1;break;case 16:b.action[15].sample=1;break;
    case 17:b.action[15].channel=1;break;case 18:b.action[15].expected=f->pins[0];break;
    case 19:b.action[15].key.owner=1;break;
    case 20:b.action[15].geometry.amigus.trigger.offset=1;break;
    case 21:b.action[15].geometry.amigus.rate=1;break;
    case 22:b.action[4].expected=(void *)(uintptr_t)1;break;
    case 23:b.action[4].track=0;break;case 24:b.action[4].sample=255;break;
    case 25:b.action[4].channel=2;break;case 26:b.action[4].key.serial=1;break;
    case 27:b.action[4].geometry.amigus.trigger.rate_numerator=0;break;
    case 28:b.action[4].geometry.amigus.trigger.rate_denominator=0;break;
    case 29:b.action[4].geometry.amigus.bits=24;break;
    case 30:b.action[4].geometry.amigus.little_endian=2;break;
    case 31:frame=UINT64_MAX;break;
    case 32:output=(void *)(b.levels+15);break;
    case 33:output=(void *)&f->ordinary;break;
    case 34:output=(void *)(f->pcm[0].data+f->pcm[0].capacity-1);break;
    case 35:output=(void *)f->config.project->samples[0].slices;break;
    case 36:input=(void *)(f->pcm[0].data+f->pcm[0].capacity-1);break;
    case 37:input=(void *)f->config.project->samples[0].slices;break;
    case 38:input=(void *)((uintptr_t)&b+1);break;
    case 39:input=(void *)(UINTPTR_MAX-15);break;
    case 40:input=NULL;break;
    default:assert(mode==41);output=(void *)f->captured;break;
    }
    copy=b;calls=f->ordinary.calls;chip=f->chip.calls;writes=f->inherited->resources->card.writes;
    pool_copy=malloc(pt_sampler_mixed_pool_size());assert(pool_copy);
    memcpy(pool_copy,f->captured,pt_sampler_mixed_pool_size());
    assert(pt_sampler_mixed_begin_quantized(f->pool,f->revision,frame,input,output)==PT_SAMPLER_MIXED_INVALID);
    assert(out.address==(void *)(uintptr_t)1&&out.token==999&&!memcmp(&b,&copy,sizeof(b))&&
        !memcmp(pool_copy,f->captured,pt_sampler_mixed_pool_size())&&f->ordinary.calls==calls&&
        f->chip.calls==chip&&f->inherited->resources->card.writes==writes&&!f->inherited->model.reads);
    free(pool_copy);smf_drop(f);
}
static void d2_allocation_alias(unsigned mode)
{
    struct smf_trial *f=d2_make(16,16,0,1,6,0);
    struct pt_sampler_mixed_quantized_batch b,copy;
    struct pt_sampler_mixed_command_handle out={(void *)(uintptr_t)1,999};
    unsigned releases,calls;void *bytes_copy,*alias;size_t bytes;
    smf_open(f);d2_batch(f,&b,1,0,0);
    switch(mode){
    case 0:alias=&b;bytes=sizeof(b);break;
    case 1:alias=b.action+15;bytes=sizeof(b.action[15]);break;
    case 2:alias=b.levels+15;bytes=sizeof(b.levels[15]);break;
    case 3:alias=&out;bytes=sizeof(out);break;
    case 4:alias=f->pcm[0].data+f->pcm[0].capacity-1;bytes=sizeof(int32_t);break;
    case 5:alias=f->config.project->samples[0].slices;bytes=2*sizeof(uint32_t);break;
    case 6:alias=&f->ordinary;bytes=sizeof(f->ordinary);break;
    default:assert(mode==7);alias=f->captured;bytes=pt_sampler_mixed_pool_size();break;
    }
    copy=b;bytes_copy=malloc(bytes);assert(bytes_copy);memcpy(bytes_copy,alias,bytes);
    f->ordinary.alias=alias;releases=f->ordinary.releases;calls=f->ordinary.calls;
    assert(pt_sampler_mixed_begin_quantized(f->pool,f->revision,960,&b,&out)==PT_SAMPLER_MIXED_CAPACITY);
    assert(out.address==(void *)(uintptr_t)1&&out.token==999&&!memcmp(&b,&copy,sizeof(b))&&
        f->ordinary.calls==calls+1&&f->ordinary.releases==releases);
    /* The allocator's declared context records its call; other recognized
     * aliases must never be initialized or released as fresh memory. */
    if(mode!=6&&mode!=7)assert(!memcmp(bytes_copy,alias,bytes));
    free(bytes_copy);f->ordinary.alias=NULL;smf_drop(f);
}
static void d2_budget(void)
{
    struct smf_trial *f=d2_make(16,16,0,1,6,0);
    struct pt_sampler_mixed_quantized_batch b;
    struct pt_sampler_mixed_command_handle out={(void *)(uintptr_t)1,999};
    unsigned calls;
    f->config.control_budget=pt_sampler_mixed_pool_size()+pt_sampler_mixed_command_size()+pt_sampler_mixed_reader_size()-1;
    smf_open(f);d2_batch(f,&b,1,0,0);calls=f->ordinary.calls;
    assert(pt_sampler_mixed_begin_quantized(f->pool,f->revision,960,&b,&out)==PT_SAMPLER_MIXED_CAPACITY&&
        out.address==(void *)(uintptr_t)1&&out.token==999&&f->ordinary.calls==calls);
    smf_drop(f);
}
static void d2_reuse(unsigned start_exact)
{
    struct smf_trial *f=d2_make(24,16,0,1,5,0);
    struct pt_sampler_mixed_quantized_batch b;
    uint64_t ticket;unsigned cycle,writes=0;
    smf_open(f);
    for(cycle=0;cycle<4;++cycle){unsigned exact=(start_exact+cycle)%2;
        d2_batch(f,&b,1,cycle%2,0);b.action[0].geometry.amigus.little_endian=0;
        if(exact){b.levels[0].left=32639;b.levels[0].right=32895;
            d2_prepare(f,0,960+cycle*960,&b);
        }else{
            b.levels[0]=(struct pt_sampler_mixed_trigger_levels){PT_SAMPLER_MIXED_TRIGGER_LEGACY,0,0};
            b.action[0].geometry.amigus.trigger.volume=64;b.action[0].geometry.amigus.trigger.pan=128;
            f->request[0]=b.action[0];f->count=1;smf_prepare(f,0,960+cycle*960);
        }
        if(cycle)assert(f->inherited->resources->card.writes==writes);
        writes=f->inherited->resources->card.writes;smf_handles(f,0,0);
        assert(pt_sampler_mixed_enqueue(f->pool,f->revision,f->command[0],&ticket)==PT_MIXED_READERS_OK);
        assert(pt_sampler_mixed_publish(f->pool,ticket)==PT_MIXED_READERS_OK);
        d2_oracles(f,ticket,&b);d2_drain(f,0,0,1,ticket,0,1);
    }
    smf_drop(f);
}
static void d2_pressure(unsigned all_card)
{
    struct smf_trial *f=d2_make(24,16,0,all_card,6,0);
    struct pt_sampler_mixed_quantized_batch b;
    struct pt_sampler_mixed_command_handle out={(void *)(uintptr_t)1,999};
    uint64_t first,second;unsigned i,calls,writes;
    smf_open(f);d2_batch(f,&b,16,0,0);d2_prepare(f,0,960,&b);smf_handles(f,0,0);
    assert(pt_sampler_mixed_enqueue(f->pool,f->revision,f->command[0],&first)==PT_MIXED_READERS_OK);
    assert(pt_sampler_mixed_publish(f->pool,first)==PT_MIXED_READERS_OK);d2_oracles(f,first,&b);
    fire(&f->inherited->model,first);assert(pt_sampler_mixed_service_command(f->pool,first,0,NULL)==PT_MIXED_READERS_OK);
    assert(pt_sampler_mixed_command_close(f->pool,f->command));
    writes=f->inherited->resources->card.writes;
    d2_prepare(f,0,1920,&b);smf_handles(f,0,16);assert(f->inherited->resources->card.writes==writes);
    assert(pt_sampler_mixed_enqueue(f->pool,f->revision,f->command[0],&second)==PT_MIXED_READERS_OK);
    assert(pt_sampler_mixed_publish(f->pool,second)==PT_MIXED_READERS_OK);d2_oracles(f,second,&b);
    fire(&f->inherited->model,second);assert(pt_sampler_mixed_service_command(f->pool,second,0,NULL)==PT_MIXED_READERS_OK);
    assert(pt_sampler_mixed_command_close(f->pool,f->command)&&pt_mixed_readers_readers_held(f->inherited->queue)==32);
    d2_batch(f,&b,1,0,0);calls=f->ordinary.calls;
    assert(pt_sampler_mixed_begin_quantized(f->pool,f->revision,2880,&b,&out)==PT_SAMPLER_MIXED_CAPACITY&&
        out.address==(void *)(uintptr_t)1&&out.token==999&&calls==f->ordinary.calls);
    for(i=0;i<16;++i)assert(pt_sampler_mixed_service_reader(f->pool,first,i,1,NULL)==PT_MIXED_READERS_OK);
    assert(pt_mixed_readers_readers_held(f->inherited->queue)==16);
    for(i=0;i<16;++i)assert(pt_sampler_mixed_service_reader(f->pool,second,i,1,NULL)==PT_MIXED_READERS_OK);
    for(i=0;i<32;++i)assert(pt_sampler_mixed_reader_close(f->pool,f->reader+i));
    smf_drop(f);
}
static void d2_fragmentation(void)
{
    struct smf_trial *f=d2_make(8,8,0,1,6,12);
    struct pt_sampler_mixed_quantized_batch b;
    struct model_command *c;const struct pt_mixed_readers_domain *d;uint64_t ticket;
    smf_open(f);d2_batch(f,&b,2,0,0);d2_prepare(f,0,960,&b);
    assert(pt_sampler_mixed_cancel(f->pool,f->command[0])==PT_SAMPLER_MIXED_OK);
    assert(pt_sampler_mixed_command_close(f->pool,f->command));
    d2_batch(f,&b,1,1,0);d2_prepare(f,0,1920,&b);smf_handles(f,0,0);
    assert(pt_sampler_mixed_enqueue(f->pool,f->revision,f->command[0],&ticket)==PT_MIXED_READERS_OK);
    assert(pt_sampler_mixed_publish(f->pool,ticket)==PT_MIXED_READERS_OK);d2_oracles(f,ticket,&b);
    c=model_command(&f->inherited->model,ticket);d=c->reference->reader[0];
    assert(d->card.cache_slot==2&&d->card.address==0&&d->card.logical_bytes==6&&d->card.full_capacity==8&&
        pt_cache_data(&f->inherited->resources->card.cache.cache,
            (struct pt_cache_lease){d->card.cache_slot,d->card.serial})==&f->inherited->resources->card.cache.arena.block[0]);
    d2_drain(f,0,0,1,ticket,0,1);smf_drop(f);
}
static void d2_cancel(unsigned phase)
{
    struct smf_trial *f=d2_make(24,16,0,0,64,0);
    struct pt_sampler_mixed_quantized_batch b;unsigned i,writes;struct model before;
    smf_open(f);d2_batch(f,&b,5,0,0);
    assert(pt_sampler_mixed_begin_quantized(f->pool,f->revision,960,&b,f->command)==PT_SAMPLER_MIXED_PENDING);
    for(i=0;i<phase;++i){enum pt_sampler_mixed_result result=pt_sampler_mixed_advance(f->pool,f->revision,f->command[0],NULL);
        assert(result==PT_SAMPLER_MIXED_PENDING||result==PT_SAMPLER_MIXED_OK);}
    writes=f->inherited->resources->card.writes;before=f->inherited->model;
    assert(pt_sampler_mixed_cancel(f->pool,f->command[0])==PT_SAMPLER_MIXED_OK&&
        pt_sampler_mixed_cancel(f->pool,f->command[0])==PT_SAMPLER_MIXED_OK);
    assert(writes==f->inherited->resources->card.writes&&!memcmp(&before,&f->inherited->model,sizeof(before)));
    assert(pt_sampler_mixed_command_close(f->pool,f->command));smf_drop(f);
}
static void d2_control_tags(unsigned stop)
{
    struct smf_trial *f=d2_make(16,16,0,1,5,0);
    struct pt_sampler_mixed_quantized_batch b;
    struct pt_mixed_readers_key key;struct pt_sampler_mixed_command_handle out={(void *)(uintptr_t)1,999};
    uint64_t first,second;unsigned calls;
    smf_open(f);d2_batch(f,&b,1,0,0);d2_prepare(f,0,960,&b);smf_handles(f,0,0);
    assert(pt_sampler_mixed_enqueue(f->pool,f->revision,f->command[0],&first)==PT_MIXED_READERS_OK);
    assert(pt_sampler_mixed_publish(f->pool,first)==PT_MIXED_READERS_OK);
    assert(pt_sampler_mixed_reader_key(f->pool,f->reader[0],&key)==PT_MIXED_READERS_STALE);
    fire(&f->inherited->model,first);assert(pt_sampler_mixed_service_command(f->pool,first,0,NULL)==PT_MIXED_READERS_OK);
    assert(pt_sampler_mixed_service_reader(f->pool,first,0,0,NULL)==PT_MIXED_READERS_PENDING&&
        pt_sampler_mixed_reader_key(f->pool,f->reader[0],&key)==PT_MIXED_READERS_OK);
    memset(&b,0,sizeof(b));b.count=1;b.action[0].kind=stop?PT_MIXED_READERS_STOP:PT_MIXED_READERS_CONTROL;
    b.action[0].key=key;
    if(!stop){b.action[0].geometry.amigus.rate=1000;b.action[0].geometry.amigus.left=300;b.action[0].geometry.amigus.right=400;}
    b.levels[0].mode=PT_SAMPLER_MIXED_TRIGGER_QUANTIZED;calls=f->ordinary.calls;
    assert(pt_sampler_mixed_begin_quantized(f->pool,f->revision,1920,&b,&out)==PT_SAMPLER_MIXED_INVALID&&
        out.address==(void *)(uintptr_t)1&&out.token==999&&calls==f->ordinary.calls);
    b.levels[0].mode=PT_SAMPLER_MIXED_TRIGGER_LEGACY;d2_prepare(f,1,1920,&b);
    assert(pt_sampler_mixed_enqueue(f->pool,f->revision,f->command[1],&second)==PT_MIXED_READERS_OK);
    assert(pt_sampler_mixed_publish(f->pool,second)==PT_MIXED_READERS_OK);
    {struct model_command *c=model_command(&f->inherited->model,second);
        assert(c->batch.action[0].kind==b.action[0].kind);
        if(!stop)assert(c->batch.action[0].geometry.amigus.rate==1000&&
            c->batch.action[0].geometry.amigus.left==300&&c->batch.action[0].geometry.amigus.right==400);}
    fire(&f->inherited->model,second);assert(pt_sampler_mixed_service_command(f->pool,second,0,NULL)==PT_MIXED_READERS_OK);
    assert(pt_sampler_mixed_command_close(f->pool,f->command+1));
    assert(pt_sampler_mixed_service_reader(f->pool,first,0,1,NULL)==PT_MIXED_READERS_OK);
    smf_close_handles(f,0,0,1);smf_drop(f);
}
static void d2_expired(void)
{
    struct smf_trial *f=d2_make(24,16,0,1,5,0);
    struct pt_sampler_mixed_quantized_batch b;
    struct pt_project original=*f->config.project;struct pt_sample *table=f->config.sampler->table;
    uint64_t ticket;
    smf_open(f);d2_batch(f,&b,1,0,0);d2_prepare(f,0,960,&b);smf_handles(f,0,0);
    assert(pt_sampler_mixed_enqueue(f->pool,f->revision,f->command[0],&ticket)==PT_MIXED_READERS_OK);
    assert(pt_sampler_mixed_publish(f->pool,ticket)==PT_MIXED_READERS_OK);
    f->config.project->samples=(void *)(uintptr_t)1;f->config.project->events=(void *)(uintptr_t)1;
    f->config.project->extensions=(void *)(uintptr_t)1;f->config.sampler->table=(void *)(uintptr_t)1;
    f->config.sampler->generation++;
    assert(pt_sampler_mixed_service_command(f->pool,ticket,1,NULL)==PT_MIXED_READERS_OK);
    {enum pt_mixed_readers_result result=pt_sampler_mixed_service_reader(f->pool,ticket,0,1,NULL);
        assert(result==PT_MIXED_READERS_OK||result==PT_MIXED_READERS_BACKEND);}
    smf_close_handles(f,0,0,1);assert(pt_sampler_mixed_close(&f->pool));
    *f->config.project=original;f->config.sampler->table=table;f->config.sampler->generation--;
    smf_drop(f);
}
static void d2_persistent_master(unsigned order)
{
    struct smf_trial *f=d2_make(24,16,1,0,5,0);
    struct pt_sampler_mixed_quantized_batch b;
    unsigned i,count=f->config.project->sample_count;uint64_t ticket;
    struct pt_project *p=f->config.project;
    smf_open(f);d2_batch(f,&b,5,1,0);d2_prepare(f,0,960,&b);smf_handles(f,0,0);
    for(i=0;i<count;++i){pt_sampler_unpin(f->pins[i]);f->pins[i]=NULL;}
    assert(pt_sampler_mixed_enqueue(f->pool,f->revision,f->command[0],&ticket)==PT_MIXED_READERS_OK);
    assert(pt_sampler_mixed_publish(f->pool,ticket)==PT_MIXED_READERS_OK);d2_oracles(f,ticket,&b);
    fire(&f->inherited->model,ticket);smf_verify(f);
    pt_sampler_release(f->config.sampler);
    assert(!f->config.sampler->current[0]&&!f->config.sampler->current[1]);
    assert(f->pcm[0].data[1]==f->inherited->resources->original[0][1]&&
        f->pcm[1].data[1]==f->inherited->resources->original[1][1]);
    if(!order){enum pt_mixed_readers_result result=pt_sampler_mixed_service_command(f->pool,ticket,0,NULL);
        assert(result==PT_MIXED_READERS_OK||result==PT_MIXED_READERS_BACKEND);}
    for(i=0;i<5;++i){enum pt_mixed_readers_result result=pt_sampler_mixed_service_reader(f->pool,ticket,i,1,NULL);
        assert(result==PT_MIXED_READERS_OK||result==PT_MIXED_READERS_BACKEND);}
    if(order){assert(f->pcm[0].data[1]==f->inherited->resources->original[0][1]);
        {enum pt_mixed_readers_result result=pt_sampler_mixed_service_command(f->pool,ticket,0,NULL);
            assert(result==PT_MIXED_READERS_OK||result==PT_MIXED_READERS_BACKEND);}}
    smf_close_handles(f,0,0,5);assert(pt_sampler_mixed_close(&f->pool));
    for(i=0;i<2;++i){p->samples[i].pcm.data=f->inherited->resources->original[i];p->samples[i].pcm.capacity=64;}
    p->samples[0].slices=d2_markers;
    smf_drop(f);
}
static void d2_effect_and_close(unsigned mode)
{
    struct smf_trial *f=d2_make(16,16,0,1,5,0);
    struct pt_sampler_mixed_quantized_batch b;
    struct model *m=&f->inherited->model;
    struct pt_mixed_readers_command_receipt out,copy;
    uint64_t ticket;enum pt_mixed_readers_result result;
    smf_open(f);d2_batch(f,&b,1,0,0);d2_prepare(f,0,960,&b);smf_handles(f,0,0);
    assert(pt_sampler_mixed_enqueue(f->pool,f->revision,f->command[0],&ticket)==PT_MIXED_READERS_OK&&ticket==1);
    if(mode==0){f->backend.hook=2;m->submit_result=-1;
        assert(pt_sampler_mixed_publish(f->pool,ticket)==PT_MIXED_READERS_BACKEND&&
            f->backend.consumed==1&&m->submits==1);
    }else assert(pt_sampler_mixed_publish(f->pool,ticket)==PT_MIXED_READERS_OK);
    assert(!pt_sampler_mixed_command_close(f->pool,f->command)&&f->command[0].address&&
        !pt_sampler_mixed_reader_close(f->pool,f->reader)&&f->reader[0].address);
    memset(&out,0xa5,sizeof(out));copy=out;
    if(mode==1){m->malformed=1;
        assert(pt_sampler_mixed_service_command(f->pool,ticket,1,&out)==PT_MIXED_READERS_BACKEND&&
            !memcmp(&out,&copy,sizeof(out))&&pt_mixed_readers_commands_held(f->inherited->queue)==1);
        m->malformed=0;
    }
    result=pt_sampler_mixed_service_command(f->pool,ticket,1,NULL);
    assert(result==PT_MIXED_READERS_OK||result==PT_MIXED_READERS_BACKEND);
    result=pt_sampler_mixed_service_reader(f->pool,ticket,0,1,NULL);
    assert(result==PT_MIXED_READERS_OK||result==PT_MIXED_READERS_BACKEND);
    if(mode==2){f->ordinary.hook=1;
        assert(!pt_sampler_mixed_reader_close(f->pool,f->reader)&&!f->reader[0].address&&!f->reader[0].token);
        assert(pt_sampler_mixed_reader_close(NULL,f->reader)&&f->ordinary.consumed==1);
    }else assert(pt_sampler_mixed_reader_close(f->pool,f->reader)||!f->reader[0].address);
    assert(pt_sampler_mixed_command_close(f->pool,f->command)||!f->command[0].address);
    if(mode==3){f->ordinary.hook=2;assert(!pt_sampler_mixed_close(&f->pool)&&!f->pool&&f->ordinary.consumed==1);}
    smf_drop(f);
}
static void d2_adopted_spare(void)
{
    struct smf_trial *f=d2_make(24,16,0,0,5,0);
    struct pt_pattern_history history;struct pt_pattern_command records[4];
    struct pt_event_change changes[4];struct pt_pcm owned,pcm;
    struct pt_sampler_mixed_quantized_batch b;
    struct pt_sampler_mixed_command_handle out={(void *)(uintptr_t)1,999};
    unsigned added=f->config.project->sample_count,calls,i;uint64_t ticket;
    assert(pt_pattern_history_init(&history,f->config.project,records,4,changes,4)==PT_EDIT_OK);
    owned=(struct pt_pcm){malloc(64*sizeof(int32_t)),64,5,8000,1,24};assert(owned.data);
    for(i=0;i<64;++i)owned.data[i]=i<5?(int32_t)(65536+i):0;
    assert(pt_sampler_append_owned(f->config.sampler,f->config.project,&history,&owned,
        &f->config.sampler->allocator,"D2 full capacity")==PT_EDIT_OK&&!owned.data);
    assert(pt_sampler_pin(f->config.sampler,f->config.project,added,f->config.sampler->generation,
        &pcm,f->pins+added)==PT_EDIT_OK&&pcm.capacity==64);
    f->pcm[added]=pcm;free(f->saved);f->saved=save(f->inherited->resources,&f->saved_bytes);
    smf_open(f);d2_batch(f,&b,1,0,0);
    b.action[0].track=4;b.action[0].sample=added;b.action[0].expected=f->pins[added];
    memset(&b.action[0].geometry,0,sizeof(b.action[0].geometry));
    b.action[0].geometry.amigus.bits=16;
    b.action[0].geometry.amigus.trigger=(struct pt_amigus_voice_request){8000,1,0,0,0};
    b.levels[0]=(struct pt_sampler_mixed_trigger_levels){PT_SAMPLER_MIXED_TRIGGER_QUANTIZED,32639,32895};
    calls=f->ordinary.calls;
    assert(pt_sampler_mixed_begin_quantized(f->pool,f->revision,960,
        (const void *)(pcm.data+pcm.capacity-1),&out)==PT_SAMPLER_MIXED_INVALID);
    assert(pt_sampler_mixed_begin_quantized(f->pool,f->revision,960,&b,
        (void *)(pcm.data+pcm.capacity-1))==PT_SAMPLER_MIXED_INVALID&&
        f->ordinary.calls==calls&&pcm.data[63]==0&&out.address==(void *)(uintptr_t)1&&out.token==999);
    d2_prepare(f,0,960,&b);smf_handles(f,0,0);
    assert(pt_sampler_mixed_enqueue(f->pool,f->revision,f->command[0],&ticket)==PT_MIXED_READERS_OK);
    assert(pt_sampler_mixed_publish(f->pool,ticket)==PT_MIXED_READERS_OK);d2_oracles(f,ticket,&b);
    d2_drain(f,0,0,1,ticket,0,1);assert(pcm.data[63]==0);
    assert(pt_sampler_mixed_close(&f->pool));pt_pattern_history_release(&history);smf_drop(f);
}
/* Actual untransferred factory owners must retain strict odd-byte refusal.
 * No queue or model effect may follow a rejected loop/end; caches may have
 * prepared before lowering, and are released through the genuine pool close. */
static void d2_odd_card_geometry(unsigned end)
{
    struct smf_trial *f=d2_make_private(8,8,1,1,6,0,end?2:1);
    struct pt_sampler_mixed_quantized_batch b;
    struct model before;
    enum pt_sampler_mixed_result result;
    unsigned steps=0;
    smf_open(f);d2_batch(f,&b,1,0,0);
    b.action[0].sample=1;b.action[0].expected=f->pins[1];
    before=f->inherited->model;
    assert(pt_sampler_mixed_begin_quantized(f->pool,f->revision,960,&b,f->command)==PT_SAMPLER_MIXED_PENDING);
    do{result=pt_sampler_mixed_advance(f->pool,f->revision,f->command[0],NULL);
        assert(++steps<2000);
    }while(result==PT_SAMPLER_MIXED_PENDING);
    assert(result==PT_SAMPLER_MIXED_STALE&&
        !memcmp(&before,&f->inherited->model,sizeof(before))&&
        !pt_mixed_readers_commands_held(f->inherited->queue)&&
        !pt_mixed_readers_readers_held(f->inherited->queue));
    /* The failed untransferred command is consumed first. Its0 result reports
     * the fault while the cleared handle positively reports owned storage freed. */
    assert(!pt_sampler_mixed_command_close(f->pool,f->command)&&
        !f->command[0].address&&!f->command[0].token);
    assert(pt_sampler_mixed_close(&f->pool)&&!f->pool);smf_drop(f);
}
int main(void)
{
    unsigned bits,cache_bits,little,all_card,order,mode;
    assert(d2_legacy_main()==0);
    for(mode=0;mode<42;++mode)d2_admission(mode);
    for(mode=0;mode<8;++mode)d2_allocation_alias(mode);
    d2_budget();d2_control_tags(0);d2_control_tags(1);d2_adopted_spare();d2_odd_card_geometry(0);d2_odd_card_geometry(1);
    puts("SAMPLER QUANTIZED ADMISSION PASS:42 complete fixed16 semantic/span refusals;8 allocating original/tail/output/master/slice/context aliases; genuine adopted64-value spare-capacity guards/reader; honest queried one-byte budget;2 odd8bit loop/end refusals with genuine owner close; genuine CONTROL/STOP tags; SOFTWARE_ONLY");
    for(bits=8;bits<=24;bits+=8)for(cache_bits=8;cache_bits<=16;cache_bits+=8)
        for(little=0;little<2;++little)for(all_card=0;all_card<2;++all_card)
            for(order=0;order<2;++order)d2_lifetime(bits,cache_bits,little,all_card,order);
    puts("SAMPLER QUANTIZED OWNERSHIP PASS:48 genuine16-reader mixed4Paula/12card and0Paula/16card lifetimes; copied expired inputs; seven-field levels/endian/channel/cache padding and master-save oracles; actual typed enqueue and independent proof orders; SOFTWARE_ONLY");
    d2_reuse(0);d2_reuse(1);d2_pressure(0);d2_pressure(1);d2_fragmentation();
    for(mode=0;mode<24;++mode)d2_cancel(mode);
    d2_expired();
    d2_persistent_master(0);d2_persistent_master(1);
    for(mode=0;mode<4;++mode)d2_effect_and_close(mode);
    puts("SAMPLER QUANTIZED LIFETIME PASS:8 exact/legacy scratch reuse admissions and cache HITs;2 genuine32-reader pressure/replacement groups; matched cache-slot2 arena-block0;24 phase cancels without output calls; expired-table NULL drains;2 persistent master proof orders;4 uncertain/malformed/consumed-close groups; SOFTWARE_ONLY");
    puts("SAMPLER QUANTIZED PASS:D2 genuine software factory only; unchanged legacy suite and request/queue/proof contracts; no controller/normalizer facade, native/device capacity/stop/timing/audio authority");
    return 0;
}
