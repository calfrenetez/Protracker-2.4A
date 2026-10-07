/* SOURCE-only proposal. Root alone compiles/runs/adopts. Reuse genuine
 * resource/controller helpers; all inherited suite entries remain uncalled.
 * The adapter models ordinary RAM and injected original clocks, not hardware. */
#define PT_EDITOR_MIXED_CAUSAL_PREPARE_TEST_MAIN inherited_causal_preparation_suite_not_called
#include "editor_mixed_causal_prepare_test.c"
#undef PT_EDITOR_MIXED_CAUSAL_PREPARE_TEST_MAIN

struct cq_master {
    struct pt_pcm pcm;
    struct pt_sample_version *pin;
    int32_t *bytes;
};
struct cq_before {
    unsigned sample_count;
    uint32_t revision,generation;
    struct cq_master master[PT_PROJECT_SAMPLES];
};
static unsigned cq_successes,cq_refusals;
static const uint16_t cq_levels[4][2]={{32639,32895},{0,0},{65535,65535},{111,222}};
static const uint8_t cq_pcm8[2][6]={{0x7f,0x01,0x03,0x40,0x00,0xfe},
                                 {0x80,0xff,0xfd,0xc0,0x02,0x7e}};
static const uint16_t cq_pcm16[2][6]={{0x7f00,0x0100,0x0300,0x4000,0x0000,0xfe00},
                                    {0x8000,0xff00,0xfd00,0xc000,0x0200,0x7e00}};
static struct cq_before *cq_capture(struct cp_trial *f)
{
    struct cq_before *b=calloc(1,sizeof(*b));unsigned i;assert(b);
    b->sample_count=f->editor->project->sample_count;
    b->revision=f->editor->history.revision;b->generation=f->editor->sampler.generation;
    for(i=0;i<b->sample_count;++i){struct cq_master *m=b->master+i;
        m->pcm=f->editor->project->samples[i].pcm;m->pin=f->pin[i];assert(m->pin);
        if(m->pcm.capacity){assert(m->pcm.data&&m->pcm.capacity<=SIZE_MAX/sizeof(int32_t));
            m->bytes=malloc(m->pcm.capacity*sizeof(int32_t));assert(m->bytes);
            memcpy(m->bytes,m->pcm.data,m->pcm.capacity*sizeof(int32_t));}
    }
    assert(b->master[31].pcm.capacity==64&&b->master[31].pcm.bits==24);
    return b;
}
static void cq_same(struct cp_trial *f,const struct cq_before *b)
{
    unsigned i;cp_same(f);
    assert(b->sample_count==f->editor->project->sample_count&&b->revision==f->editor->history.revision&&
        b->generation==f->editor->sampler.generation);
    for(i=0;i<b->sample_count;++i){const struct cq_master *m=b->master+i;
        const struct pt_pcm *p=&f->editor->project->samples[i].pcm;
        assert(p->data==m->pcm.data&&p->capacity==m->pcm.capacity&&p->frames==m->pcm.frames&&
            p->rate==m->pcm.rate&&p->channels==m->pcm.channels&&p->bits==m->pcm.bits&&f->pin[i]==m->pin);
        assert(f->editor->sampler.current[i]==m->pin);
        if(p->capacity)assert(!memcmp(p->data,m->bytes,p->capacity*sizeof(int32_t)));
    }
}
static void cq_before_drop(struct cq_before *b)
{unsigned i;for(i=0;i<b->sample_count;++i)free(b->master[i].bytes);free(b);}
static struct pt_editor_mixed_readers_quantized_batch *cq_batch(struct cp_trial *f,unsigned count,unsigned successor)
{
    struct pt_editor_mixed_readers_quantized_batch *b=calloc(1,sizeof(*b));unsigned i;assert(b);
    cp_requests(f,count);b->count=count;
    for(i=0;i<count;++i){b->action[i]=f->request[i];
        if(f->editor->project->channels.track[i].route==PT_AMIGUS){
            b->action[i].geometry.amigus.trigger.volume=0;b->action[i].geometry.amigus.trigger.pan=0;
            b->levels[i].mode=PT_SAMPLER_MIXED_TRIGGER_QUANTIZED;
            b->levels[i].left=cq_levels[(i+successor)%4][0];b->levels[i].right=cq_levels[(i+successor)%4][1];
            if(count>1&&i==count-1){b->levels[i].mode=PT_SAMPLER_MIXED_TRIGGER_LEGACY;
                b->levels[i].left=b->levels[i].right=0;
                b->action[i].geometry.amigus.trigger.volume=64;b->action[i].geometry.amigus.trigger.pan=128;}
        }
    }
    return b;
}
static void cq_prepare(struct cp_trial *f,unsigned ci,uint64_t frame,
    struct pt_editor_mixed_readers_quantized_batch *b)
{
    enum pt_editor_mixed_readers_result r;unsigned steps=0,writes,guards=f->control.guard_count;
    struct pt_editor_mixed_readers_quantized_batch before;
    memcpy(&before,b,sizeof(before));f->count=b->count;
    assert(pt_editor_mixed_causal_prepare_batch_begin_quantized(&f->control,frame,b,f->command+ci)==PT_EDITOR_MIXED_READERS_PENDING);
    assert(!memcmp(b,&before,sizeof(before))&&f->control.guard_count==guards);
    /* The returned genuine command owns the copied values. Poison and release
     * the complete former input only after the original numeric guards compact. */
    memset(b,0xa5,sizeof(*b));free(b);
    do{writes=f->card->writes;r=pt_editor_mixed_causal_prepare_batch_advance(&f->control,f->command[ci]);
        assert(f->card->writes-writes<=128&&++steps<2000);}while(r==PT_EDITOR_MIXED_READERS_PENDING);
    assert(r==PT_EDITOR_MIXED_READERS_OPEN);
}
static void cq_bytes(const uint8_t *data,unsigned channel,unsigned bits,unsigned little)
{
    unsigned i;
    if(bits==8){assert(!memcmp(data,cq_pcm8[channel],6));return;}
    assert(bits==16&&little<=1);
    for(i=0;i<6;++i){uint16_t word=cq_pcm16[channel][i];
        assert(data[2*i]==(uint8_t)(little?word:word>>8));
        assert(data[2*i+1]==(uint8_t)(little?word>>8:word));}
}
static void cq_packet(struct cp_trial *f,const struct pt_mixed_causal_packet *p,unsigned count,unsigned successor)
{
    unsigned i,j;assert(p->count==count&&p->frame==(successor?1920U:960U));
    assert(p->first==oracle(p->frame)&&p->last==oracle(p->frame+1));
    assert(ct_registration(&p->registration,&f->port.model.registration));
    for(i=0;i<count;++i){const struct pt_mixed_readers_action *a=p->action+i;
        assert(a->kind==PT_MIXED_READERS_TRIGGER&&a->route==f->editor->project->channels.track[i].route);
        assert(p->key[i].trigger==p->ticket&&p->key[i].action==i&&p->key[i].route==a->route&&p->key[i].slot==a->slot);
        if(a->route==PT_PAULA){assert(a->geometry.paula.data&&a->geometry.paula.words==3&&
            a->geometry.paula.period==428&&a->geometry.paula.volume==64);
            cq_bytes(a->geometry.paula.data,0,8,0);
        }else{const struct pt_mixed_readers_card *c=p->card+i;
            const struct pt_amigus_voice_plan *v=&a->geometry.amigus;
            struct pt_cache_lease lease={c->cache_slot,c->serial};struct pt_cache_entry *entry;
            void *block;struct pt_amigus_ram_block *r;unsigned level=(i+successor)%4;
            assert(a->route==PT_AMIGUS&&c->cache_slot<PT_CACHE_SLOTS);
            entry=f->card->cache.cache.entry+c->cache_slot;block=pt_cache_data(&f->card->cache.cache,lease);assert(block);
            for(j=0;j<PT_CACHE_SLOTS&&block!=f->card->cache.arena.block+j;++j){}
            assert(j<PT_CACHE_SLOTS);r=f->card->cache.arena.block+j;
            assert(c->reservation==&f->card->reservation&&c->cache==&f->card->cache.cache&&
                c->version==entry->version&&c->version==1&&c->serial==entry->serial&&entry->valid&&entry->pins>=2);
            assert(c->bits==f->cache_bits&&c->little_endian==f->little&&c->source_channel==1&&
                c->address==r->address&&c->logical_bytes==(f->cache_bits==8?6U:12U)&&
                c->logical_bytes==r->bytes&&c->logical_bytes==entry->bytes&&c->full_capacity==r->reserved);
            assert(!r->failed&&r->written==r->bytes&&r->reserved<=sizeof(f->card->ram)&&
                r->address<=sizeof(f->card->ram)-r->reserved);
            cq_bytes(f->card->ram+r->address,1,f->cache_bits,f->little);
            for(j=r->bytes;j<r->reserved;++j)assert(!f->card->ram[r->address+j]);
            assert(v->start==r->address&&v->loop==r->address&&v->end_exclusive==r->address+r->bytes&&v->rate==44739242);
            assert(v->control==(f->cache_bits==8?0x8000U:(f->little?0x8009U:0x8001U)));
            if(count>1&&i==count-1)assert(v->left==32768&&v->right==32768);
            else assert(v->left==cq_levels[level][0]&&v->right==cq_levels[level][1]);
        }
    }
}
static void cq_identity(struct cp_trial *f,unsigned count,uint64_t first,uint64_t second)
{
    unsigned i,j;assert(first&&second&&first!=second&&f->command[0].serial!=f->command[1].serial);
    for(i=0;i<2*count;++i){unsigned n=i<count?i:16+i-count;
        struct pt_editor_mixed_reader_record *r=f->control.reader+f->reader[n].slot;
        assert(r->handle.address&&r->handle.token&&r->ticket==(i<count?first:second));
        for(j=0;j<i;++j){unsigned k=j<count?j:16+j-count;
            struct pt_editor_mixed_reader_record *prior=f->control.reader+f->reader[k].slot;
            assert(r->handle.address!=prior->handle.address&&r->handle.token!=prior->handle.token&&r->serial!=prior->serial);}
    }
}
static void cq_closed(struct cp_trial *f,const struct cq_before *before,unsigned pending_source)
{
    unsigned i,release,shutdowns,probes;int closed;
    if(pending_source){f->port.model.source_raw=0;
        assert(!pt_editor_mixed_causal_prepare_close(&f->control)&&!f->control.pool&&!f->control.queue&&f->control.causal);
        assert(cp_live(&f->ordinary)==1&&f->port.model.shutdowns==1&&!f->port.model.probes);
        cp_barrier(f);cq_same(f,before);release=f->ordinary.releases;
        assert(pt_editor_mixed_causal_prepare_close(&f->control)&&!f->control.causal);
        assert(f->port.model.shutdowns==1&&f->port.model.probes==1&&f->ordinary.releases==release+1);
    }else{closed=pt_editor_mixed_causal_prepare_close(&f->control);
        if(!closed){assert(!f->control.pool&&!f->control.queue&&!f->control.causal);
            release=f->ordinary.releases;shutdowns=f->port.model.shutdowns;probes=f->port.model.probes;
            assert(pt_editor_mixed_causal_prepare_close(&f->control));
            assert(release==f->ordinary.releases&&shutdowns==f->port.model.shutdowns&&probes==f->port.model.probes);}}
    assert(!f->control.pool&&!f->control.queue&&!f->control.causal&&!f->binding->preparation_context);
    for(i=0;i<2;++i)assert(!f->control.command[i].handle.address);
    for(i=0;i<32;++i)assert(!f->control.reader[i].handle.address);
    assert(!cp_live(&f->ordinary)&&!cp_live(&f->chip)&&f->port.model.shutdowns==1);
    cq_same(f,before);
}
static void cq_success(unsigned bits,unsigned cache,unsigned little,unsigned order,unsigned count,unsigned all_card,unsigned pending_source)
{
    struct cp_trial *f=cp_make(bits,cache,little);struct cq_before *before;
    struct pt_mixed_causal_packet *packet=calloc(2,sizeof(*packet));uint8_t *ram;
    uint64_t first,second;unsigned writes,chips,i,live;assert(packet);
    ++cq_successes;if(all_card)cp_all_card(f);before=cq_capture(f);cp_open(f);
    cq_prepare(f,0,960,cq_batch(f,count,0));cp_refs(f,0,0);first=cp_admit(f,0);cq_same(f,before);
    assert(ct_command(&f->port.model,first));memcpy(packet,&ct_command(&f->port.model,first)->packet,sizeof(*packet));
    writes=f->card->writes;chips=f->chip.calls;ram=malloc(sizeof(f->card->ram));assert(ram);memcpy(ram,f->card->ram,sizeof(f->card->ram));
    cq_prepare(f,1,1920,cq_batch(f,count,1));cp_refs(f,1,16);second=cp_admit(f,1);
    memcpy(packet+1,&ct_command(&f->port.model,second)->packet,sizeof(*packet));
    assert(writes==f->card->writes&&chips==f->chip.calls&&!memcmp(ram,f->card->ram,sizeof(f->card->ram)));free(ram);
    cq_packet(f,packet,count,0);cq_packet(f,packet+1,count,1);cq_identity(f,count,first,second);
    for(i=0;i<count;++i){assert(!keys_equal(packet->key+i,packet[1].key+i));
        if(packet->action[i].route==PT_PAULA)assert(packet->action[i].geometry.paula.data==packet[1].action[i].geometry.paula.data);
        else assert(!memcmp(packet->card+i,packet[1].card+i,sizeof(packet->card[i])));}
    assert(f->port.model.publications==2&&!f->port.model.commits&&!f->port.model.effects);
    assert(pt_mixed_readers_commands_held(f->control.queue)==2&&pt_mixed_readers_readers_held(f->control.queue)==2*count);
    live=cp_live(&f->ordinary);assert(live==5+2*count);cp_barrier(f);cq_same(f,before);
    f->port.model.ticks=oracle(960)-1;
    assert(pt_mixed_causal_fire(f->control.causal,second)==PT_MIXED_CAUSAL_EARLY);
    assert(pt_mixed_causal_fire(f->control.causal,first)==PT_MIXED_CAUSAL_EARLY&&!f->port.model.commits&&!f->port.model.effects);
    f->port.model.ticks=oracle(960);
    assert(pt_mixed_causal_fire(f->control.causal,first)==PT_MIXED_CAUSAL_COMMITTED&&f->port.model.commits==1&&f->port.model.effects==count);
    cp_observe(f,0,count);cq_same(f,before);assert(cp_live(&f->ordinary)==live);
    for(i=0;i<count;++i){struct ct_reader *r=ct_reader(&f->port.model,packet->key+i);assert(r);
        assert(!memcmp(&r->action,packet->action+i,sizeof(r->action))&&!memcmp(&r->card,packet->card+i,sizeof(r->card)));}
    if(!order){cp_drain_command(f,0,0);assert(!f->control.command[f->command[0].slot].handle.address);
        assert(pt_mixed_readers_commands_held(f->control.queue)==1&&pt_mixed_readers_readers_held(f->control.queue)==2*count);}
    f->port.model.ticks=oracle(1920)-1;
    assert(pt_mixed_causal_fire(f->control.causal,second)==PT_MIXED_CAUSAL_EARLY&&f->port.model.effects==count);
    f->port.model.ticks=oracle(1920);
    assert(pt_mixed_causal_fire(f->control.causal,second)==PT_MIXED_CAUSAL_COMMITTED&&f->port.model.commits==2&&f->port.model.effects==2*count);
    cp_observe(f,16,count);cq_same(f,before);cp_barrier(f);
    for(i=0;i<count;++i){struct ct_reader *r=ct_reader(&f->port.model,packet[1].key+i);assert(r);
        assert(!memcmp(&r->action,packet[1].action+i,sizeof(r->action))&&!memcmp(&r->card,packet[1].card+i,sizeof(r->card)));}
    if(!order)cp_drain_command(f,1,0);
    cp_drain_readers(f,0,count,0);cp_drain_readers(f,16,count,0);
    if(order){for(i=0;i<count;++i)assert(f->control.reader[f->reader[i].slot].handle.address&&
            f->control.reader[f->reader[16+i].slot].handle.address);
        cp_drain_command(f,0,0);cp_drain_command(f,1,0);}
    assert(!pt_mixed_readers_commands_held(f->control.queue)&&!pt_mixed_readers_readers_held(f->control.queue)&&ct_empty(&f->port.model));
    for(i=0;i<20;++i)assert(!f->port.model.slot[i].serial);
    assert(cp_live(&f->ordinary)==(order?3+2*count:3));cq_same(f,before);
    free(packet);cq_closed(f,before,pending_source);cq_before_drop(before);cp_drop(f);
}

/* All immediate refusals start from a genuine OPEN owner with a valid full
 * fixed batch. Full caller/controller/master before-images remain independent. */
static void cq_refusal(unsigned mode)
{
    struct cp_trial *f=cp_make(24,16,0);struct cq_before *masters;
    struct pt_editor_mixed_readers_quantized_batch *b,*copy;
    struct pt_editor_mixed_command_ref *out=malloc(sizeof(*out)),out_before;
    struct pt_editor_mixed_causal_prepare *control=malloc(sizeof(*control));
    uint64_t frame=960;unsigned calls,writes,chips;assert(out&&control);++cq_refusals;
    masters=cq_capture(f);cp_open(f);b=cq_batch(f,(mode>=8&&mode<=13)?1:16,0);copy=malloc(sizeof(*copy));assert(copy);
    *out=(struct pt_editor_mixed_command_ref){99,999};
    switch(mode){
    case 0:b->levels[0].mode=PT_SAMPLER_MIXED_TRIGGER_QUANTIZED;break;
    case 1:b->levels[4].mode=2;break;
    case 2:b->levels[15].left=1;break;
    case 3:b->action[4].geometry.amigus.trigger.volume=1;break;
    case 4:b->action[4].geometry.amigus.trigger.pan=1;break;
    case 5:b->action[4].geometry.amigus.rate=1;break;
    case 6:b->action[4].geometry.amigus.left=1;break;
    case 7:b->action[4].geometry.amigus.right=1;break;
    case 8:b->count=0;break;
    case 9:b->count=17;break;
    case 10:frame=UINT64_MAX;break;
    case 11:b->action[15].track=1;break;
    case 12:b->levels[15].mode=PT_SAMPLER_MIXED_TRIGGER_QUANTIZED;break;
    case 13:b->levels[15].left=1;break;
    case 14:b->action[0].kind=PT_MIXED_READERS_CONTROL;break;
    case 15:b->action[0].kind=PT_MIXED_READERS_STOP;break;
    case 16:b->action[5].track=4;break;
    case 17:b->action[4].sample=f->editor->project->sample_count;break;
    case 18:b->action[4].channel=2;break;
    case 19:b->action[4].expected=f->pin[1];break;
    case 20:b->action[4].reader.serial=1;break;
    case 21:b->action[4].geometry.amigus.bits=24;break;
    case 22:b->action[4].geometry.amigus.little_endian=2;break;
    case 23:b->action[4].geometry.amigus.trigger.rate_numerator=0;break;
    case 24:b->action[4].geometry.amigus.trigger.rate_denominator=0;break;
    default:assert(0);
    }
    memcpy(copy,b,sizeof(*copy));memcpy(control,&f->control,sizeof(*control));memcpy(&out_before,out,sizeof(out_before));
    calls=f->ordinary.calls;writes=f->card->writes;chips=f->chip.calls;
    assert(pt_editor_mixed_causal_prepare_batch_begin_quantized(&f->control,frame,b,out)==PT_EDITOR_MIXED_READERS_INVALID);
    assert(!memcmp(b,copy,sizeof(*copy))&&!memcmp(out,&out_before,sizeof(out_before))&&!memcmp(control,&f->control,sizeof(*control)));
    assert(calls==f->ordinary.calls&&writes==f->card->writes&&chips==f->chip.calls&&!f->port.model.clocks&&
        !f->port.model.publications&&!f->port.model.effects&&!f->control.first_error);
    cq_same(f,masters);free(b);free(copy);free(out);free(control);cq_closed(f,masters,0);cq_before_drop(masters);cp_drop(f);
}
static void cq_third(void)
{
    struct cp_trial *f=cp_make(24,16,0);struct cq_before *masters;unsigned calls,writes,pass;
    struct pt_editor_mixed_readers_quantized_batch *b,*copy;
    struct pt_editor_mixed_command_ref out={99,999};struct pt_editor_mixed_causal_prepare *control=malloc(sizeof(*control));
    assert(control);++cq_refusals;cp_all_card(f);masters=cq_capture(f);cp_open(f);
    cq_prepare(f,0,960,cq_batch(f,1,0));cp_refs(f,0,0);assert(cp_admit(f,0));
    cq_prepare(f,1,1920,cq_batch(f,1,1));cp_refs(f,1,16);assert(cp_admit(f,1));
    b=cq_batch(f,1,0);copy=malloc(sizeof(*copy));assert(copy);memcpy(copy,b,sizeof(*copy));
    for(pass=0;pass<2;++pass){memcpy(control,&f->control,sizeof(*control));calls=f->ordinary.calls;writes=f->card->writes;
        assert(pt_editor_mixed_causal_prepare_batch_begin_quantized(&f->control,2880,b,&out)==PT_EDITOR_MIXED_READERS_CAPACITY);
        assert(!memcmp(b,copy,sizeof(*copy))&&out.slot==99&&out.serial==999&&!memcmp(control,&f->control,sizeof(*control)));
        assert(calls==f->ordinary.calls&&writes==f->card->writes&&f->control.prepared_batches==2&&!f->port.model.effects);
        cq_same(f,masters);
        if(!pass){cp_drain_command(f,0,0);cp_drain_command(f,1,0);cp_drain_readers(f,0,1,0);cp_drain_readers(f,16,1,0);
            assert(!f->control.command[f->command[0].slot].handle.address&&!f->control.command[f->command[1].slot].handle.address);}
    }
    free(b);free(copy);free(control);cq_closed(f,masters,0);cq_before_drop(masters);cp_drop(f);
}
static void cq_spans(unsigned mode)
{
    struct cp_trial *f=cp_make(24,16,0);struct cq_before *masters;unsigned calls,writes;
    struct pt_editor_mixed_readers_quantized_batch *b,*copy,*incoming;
    struct pt_editor_mixed_command_ref out={99,999},*output=&out;
    struct pt_editor_mixed_causal_prepare *control=malloc(sizeof(*control));uint8_t *causal,*factory;
    assert(control);++cq_refusals;
    if(mode==0){
        /* Supply a genuinely readable whole fixed batch inside caller-owned
         * causal storage BEFORE begin captures its original full extent.
         * The donor's queried workspace plus128 need not hold this batch. */
        assert(!f->ordinary.calls&&!f->port.bind_calls&&!f->binding->preparation_context&&
            !f->control.inputs&&!f->control.causal&&!f->control.queue&&!f->control.pool);
        if(f->causal_capacity<sizeof(*b)){
            void *workspace=calloc(1,sizeof(*b));assert(workspace);
            free(f->causal_workspace);f->causal_workspace=workspace;f->causal_capacity=sizeof(*b);
            f->input.causal_workspace=workspace;f->input.causal_capacity=f->causal_capacity;
        }
        assert(f->input.causal_workspace==f->causal_workspace&&
            f->input.causal_capacity==f->causal_capacity&&f->causal_capacity>=sizeof(*b)&&
            !((uintptr_t)f->causal_workspace%pt_mixed_causal_workspace_alignment()));
    }
    masters=cq_capture(f);cp_open(f);b=cq_batch(f,16,0);incoming=b;
    copy=malloc(sizeof(*copy));causal=malloc(f->causal_capacity);factory=malloc(f->factory_capacity);assert(copy&&causal&&factory);
    if(mode==0){unsigned guard;
        assert(f->causal_capacity>=sizeof(*b));
        assert(f->control.saved.causal_workspace==f->causal_workspace&&
            f->control.saved.causal_capacity==f->causal_capacity);
        for(guard=0;guard<f->control.guard_count;++guard)
            if(f->control.guards[guard].data==f->causal_workspace&&
               f->control.guards[guard].bytes==f->causal_capacity)break;
        assert(guard<f->control.guard_count);incoming=f->causal_workspace;
    }
    else if(mode==1){assert(f->factory_capacity>=sizeof(*output));
        output=(void *)((uint8_t *)f->factory_workspace+f->factory_capacity-sizeof(*output));}
    else if(mode==2){assert(f->pcm[31].capacity==64&&sizeof(*output)<=4*sizeof(int32_t));output=(void *)(f->pcm[31].data+60);}
    else{assert(mode==3&&sizeof(*output)<=2*sizeof(b->levels[0]));output=(void *)(b->levels+14);}
    memcpy(copy,b,sizeof(*copy));memcpy(control,&f->control,sizeof(*control));
    memcpy(causal,f->causal_workspace,f->causal_capacity);memcpy(factory,f->factory_workspace,f->factory_capacity);
    calls=f->ordinary.calls;writes=f->card->writes;
    assert(pt_editor_mixed_causal_prepare_batch_begin_quantized(&f->control,960,incoming,output)==PT_EDITOR_MIXED_READERS_INVALID);
    assert(!memcmp(copy,b,sizeof(*copy))&&!memcmp(control,&f->control,sizeof(*control))&&out.slot==99&&out.serial==999);
    assert(!memcmp(causal,f->causal_workspace,f->causal_capacity)&&!memcmp(factory,f->factory_workspace,f->factory_capacity));
    assert(calls==f->ordinary.calls&&writes==f->card->writes&&!f->chip.calls&&!f->port.model.clocks&&!f->port.model.publications);
    cq_same(f,masters);free(b);free(copy);free(control);free(causal);free(factory);
    cq_closed(f,masters,0);cq_before_drop(masters);cp_drop(f);
}
static void cq_allocator_alias(void)
{
    struct cp_trial *f=cp_make(24,16,0);struct cq_before *masters;unsigned calls,writes,releases,guards;
    struct pt_editor_mixed_readers_quantized_batch *b,*copy;struct pt_editor_mixed_command_ref out={99,999};
    ++cq_refusals;masters=cq_capture(f);cp_open(f);b=cq_batch(f,1,0);copy=malloc(sizeof(*copy));assert(copy);memcpy(copy,b,sizeof(*copy));
    f->ordinary.alias=b->levels+15;assert(!((uintptr_t)f->ordinary.alias%pt_sampler_mixed_workspace_alignment()));
    calls=f->ordinary.calls;releases=f->ordinary.releases;writes=f->card->writes;guards=f->control.guard_count;
    assert(pt_editor_mixed_causal_prepare_batch_begin_quantized(&f->control,960,b,&out)==PT_EDITOR_MIXED_READERS_FAULT);
    assert(f->control.first_error==PT_EDITOR_MIXED_READERS_FAULT&&calls+1==f->ordinary.calls&&releases==f->ordinary.releases&&
        !f->ordinary.alias_releases&&cp_live(&f->ordinary)==3&&!f->control.command[0].handle.address);
    assert(!memcmp(b,copy,sizeof(*copy))&&out.slot==99&&out.serial==999&&guards==f->control.guard_count);
    assert(writes==f->card->writes&&!f->chip.calls&&!f->port.model.publications&&!f->port.model.effects);
    cq_same(f,masters);f->ordinary.alias=NULL;
    /* Keep the actual refused callback in the call census. It is not an owned
     * allocation and receives no release. This separate teardown checks that
     * difference directly; it never rewrites the donor allocator counters. */
    assert(f->ordinary.calls==f->ordinary.releases+4);
    free(b);free(copy);cq_closed(f,masters,0);cq_before_drop(masters);
    assert(pt_editor_mixed_causal_prepare_close(&f->control)&&pt_editor_mixed_detach(f->binding));
    {unsigned i;for(i=0;i<PT_PROJECT_SAMPLES;++i){pt_sampler_unpin(f->pin[i]);f->pin[i]=NULL;}}
    assert(pt_editor_dispose(f->editor));assert(pt_amigus_wavetable_cache_detach(&f->card->cache));
    assert(pt_amigus_reservation_close(&f->card->reservation));pt_document_release(f->document);
    assert(!cp_live(&f->ordinary)&&!cp_live(&f->chip)&&!cp_live(&f->masters));
    assert(f->ordinary.calls==f->ordinary.releases+1&&f->chip.calls==f->chip.releases&&
        !f->ordinary.alias_releases&&!f->chip.alias_releases);
    free(f->saved);free(f->causal_workspace);free(f->factory_workspace);free(f->card);free(f->document);
    free(f->request);free(f->command);free(f->reader);free(f->editor);free(f->binding);free(f);
}
#ifndef PT_EDITOR_MIXED_CAUSAL_QUANTIZED_PAIR_TEST_MAIN
#define PT_EDITOR_MIXED_CAUSAL_QUANTIZED_PAIR_TEST_MAIN main
#endif
int PT_EDITOR_MIXED_CAUSAL_QUANTIZED_PAIR_TEST_MAIN(void)
{
    static const unsigned bits[]={8,16,24},cache[]={8,16};unsigned i,j;
    for(i=0;i<3;++i)for(j=0;j<2;++j)cq_success(bits[i],cache[j],j?(i%2):0,(i+j)%2,16,0,i==0&&j==0);
    cq_success(24,16,1,1,16,1,0);cq_success(8,8,0,0,1,1,0);
    for(i=0;i<25;++i)cq_refusal(i);
    cq_third();for(i=0;i<4;++i)cq_spans(i);cq_allocator_alias();
    assert(cq_successes==8&&cq_refusals==31&&cp_cases==39&&ct_cases==0);
    puts("EDITOR MIXED CAUSAL QUANTIZED PAIR PASS:39 genuine cases;8 typed pure-TRIGGER quantized pairs and31 bounded refusals;4Paula+12card/16card/count1,8/16/24 masters/cache8+16 BE+LE;literal direct levels/legacy levels/channel bytes/padding and cache HIT identity;original two windows,32 persistent pins,independent C/R/source quiet,full master capacities/exact saves/fixed-span alias/sticky pair scope; SOFTWARE_ONLY");
    return 0;
}
