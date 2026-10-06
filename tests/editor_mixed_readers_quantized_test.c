/* Private SOURCE draft: genuine existing controller fixture bodies execute once.
 * No generic main macro, fabricated owner, READY token or hardware backend. */
#define PT_EDITOR_MIXED_READERS_TEST_MAIN emq_legacy_main
#include "editor_mixed_readers_prepare_test.c"
#undef PT_EDITOR_MIXED_READERS_TEST_MAIN
#include "../src/core/amigus_trigger_levels.h"

static void *emq_allocate(void *context,size_t n)
{
    struct emp_trial *f=(void *)((char *)context- offsetof(struct emp_trial,ordinary));
    unsigned mode=f->ordinary.hook;void *alias=f->ordinary.alias,*p;
    if(mode<100)return emp_allocate(context,n);
    f->ordinary.hook=0;f->ordinary.alias=NULL;
    if(mode==100||mode==101){
        /* Separately labelled numeric guard-tail oracle. No queue/lease proof
         * or owner is manufactured: this adds only a caller-owned numeric span. */
        unsigned k=f->control.guard_count;
        assert(f->control.busy&&k>=2&&k<PT_EDITOR_MIXED_READERS_GUARDS);
        f->control.guards[k]=(struct pt_sampler_storage_span){alias,mode==100?sizeof(uint64_t):sizeof(struct pt_editor_mixed_command_ref)};
        f->control.guard_count=k+1;
    }else{
        struct pt_editor_mixed_readers_quantized_batch *other=f->chip.alias;
        unsigned reentries=f->control.reentries;enum pt_editor_mixed_readers_result error=f->control.first_error;
        assert((mode==102||mode==103)&&other);
        if(mode==102){assert(pt_editor_mixed_readers_prepare_batch_begin_quantized(&f->control,1920,other,f->command+1)==PT_EDITOR_MIXED_READERS_INVALID);
            assert(f->control.reentries==reentries&&f->control.first_error==error);}
        else{assert(pt_editor_mixed_readers_prepare_batch_begin_quantized(&f->control,1920,other,f->command+1)==PT_EDITOR_MIXED_READERS_FAULT);
            assert(f->control.reentries==reentries+1);}
    }
    p=emp_allocate(context,n);return p;
}
static int emq_owned(void *context)
{
    struct emp_trial *f=(void *)((char *)context- offsetof(struct emp_trial,bus));
    unsigned mode=f->owned_hook;int raw;
    if(mode<100)return emp_owned(context);
    f->owned_hook=0;raw=emp_owned(context);
    if(mode==100){struct pt_editor_mixed_readers_quantized_batch *b=f->chip.alias;
        assert(b&&b->count==2);b->action[1].reader.serial=UINT64_MAX;}
    else{assert(mode==101||mode==102);++f->editor->history.revision;
        f->editor->project->samples=(void *)(UINTPTR_MAX-7);
        f->editor->project->events=(void *)(UINTPTR_MAX-7);
        f->editor->project->extensions=(void *)(UINTPTR_MAX-7);
        return mode==102?raw:0;}
    return raw;
}
static struct emp_trial *emq_make(unsigned bits,unsigned cache,unsigned little,unsigned all_card)
{
    struct emp_trial *f=emp_make(bits,cache,little);unsigned i;
    if(all_card){for(i=0;i<16;++i)f->editor->project->channels.track[i].route=PT_AMIGUS;
        ++f->editor->history.revision;free(f->saved);f->saved=emp_save(f,&f->saved_bytes);}
    f->input.activation.allocator.allocate=emq_allocate;
    assert(pt_amigus_wavetable_cache_detach(&f->card->cache));
    assert(pt_amigus_wavetable_cache_attach(&f->card->cache,&f->card->reservation,0,4096,4096,&f->bus,emq_owned,emp_write));
    return f;
}
static void emq_batch(struct emp_trial *f,struct pt_editor_mixed_readers_quantized_batch *b,unsigned n,unsigned exact)
{
    unsigned i;memset(b,0,sizeof(*b));emp_requests(f,n);b->count=n;
    for(i=0;i<n;++i){b->action[i]=f->request[i];
        if(f->editor->project->channels.track[i].route==PT_AMIGUS){
            b->action[i].channel=1;memset(&b->action[i].geometry,0,sizeof(b->action[i].geometry));
            b->action[i].geometry.amigus.bits=f->cache_bits;b->action[i].geometry.amigus.little_endian=f->little;
            b->action[i].geometry.amigus.trigger=(struct pt_amigus_voice_request){8000,1,0,exact?0:64,exact?0:128};
            if(exact){b->levels[i].mode=PT_SAMPLER_MIXED_TRIGGER_QUANTIZED;
                switch(i%4){case 0:b->levels[i].left=32639;b->levels[i].right=32895;break;
                    case 1:break;case 2:b->levels[i].left=b->levels[i].right=UINT16_MAX;break;
                    default:b->levels[i].left=111;b->levels[i].right=222;break;}}
        }
    }
}
static void emq_advance(struct emp_trial *f,unsigned ci)
{
    unsigned n=0,writes;enum pt_editor_mixed_readers_result r;
    do{writes=f->card->writes;r=pt_editor_mixed_readers_prepare_batch_advance(&f->control,f->command[ci]);
        assert(f->card->writes-writes<=128&&++n<2000);}while(r==PT_EDITOR_MIXED_READERS_PENDING);
    assert(r==PT_EDITOR_MIXED_READERS_OPEN);
}
static void emq_plan_equal(const struct pt_amigus_voice_plan *a,const struct pt_amigus_voice_plan *b)
{assert(a->start==b->start&&a->loop==b->loop&&a->end_exclusive==b->end_exclusive&&a->rate==b->rate&&
    a->control==b->control&&a->left==b->left&&a->right==b->right);}
static void emq_geometry(struct emp_trial *f,uint64_t ticket,const struct pt_editor_mixed_readers_quantized_batch *b)
{
    struct sma_command *c=sma_command(&f->port,ticket);unsigned i,j;
    assert(c&&c->packet.count==b->count&&c->packet.first==oracle(960)&&c->packet.last==oracle(961));
    for(i=0;i<b->count;++i){const struct pt_editor_mixed_readers_request *x=b->action+i;
        const struct pt_mixed_readers_action *a=c->packet.action+i;unsigned sample=x->sample;
        assert(a->kind==PT_MIXED_READERS_TRIGGER);
        if(f->editor->project->channels.track[x->track].route==PT_PAULA){
            assert(a->geometry.paula.words==3&&a->geometry.paula.period==428&&a->geometry.paula.volume==64);
            for(j=0;j<6;++j)assert(a->geometry.paula.data[j]==smf_convert8(f->pcm[sample].data[j*2],f->bits));
        }else{const struct pt_mixed_readers_card *card=c->packet.card+i;
            struct pt_playback_format format={f->cache_bits,x->channel,f->little,0};
            struct pt_amigus_trigger_levels_request request={x->geometry.amigus.trigger,b->levels[i].left,b->levels[i].right};
            struct pt_amigus_voice_plan expected;unsigned arena;void *resource;
            struct pt_cache_lease lease={card->cache_slot,card->serial};
            resource=pt_cache_data(&f->card->cache.cache,lease);
            for(arena=0;arena<PT_CACHE_SLOTS&&resource!=&f->card->cache.arena.block[arena];++arena){}
            assert(arena<PT_CACHE_SLOTS&&card->full_capacity==f->card->cache.arena.block[arena].reserved&&
                card->bits==f->cache_bits&&card->little_endian==f->little&&card->source_channel==x->channel&&
                card->logical_bytes==6*f->cache_bits/8&&card->full_capacity>=card->logical_bytes);
            if(b->levels[i].mode==PT_SAMPLER_MIXED_TRIGGER_QUANTIZED){
                assert(pt_amigus_trigger_levels_prepare(f->editor->project->samples+sample,&format,&request,
                    card->address,card->logical_bytes,&expected));
            }else assert(pt_amigus_voice_plan_prepare(f->editor->project->samples+sample,&format,&request.geometry,
                    card->address,card->logical_bytes,&expected));
            emq_plan_equal(&a->geometry.amigus,&expected);
            if(b->levels[i].mode==PT_SAMPLER_MIXED_TRIGGER_QUANTIZED){assert(a->geometry.amigus.left==b->levels[i].left&&a->geometry.amigus.right==b->levels[i].right);}
            else assert(a->geometry.amigus.left==32768&&a->geometry.amigus.right==32768);
            for(j=0;j<6;++j){int32_t v=f->pcm[sample].data[j*2+x->channel];
                if(f->cache_bits==8)assert(f->card->ram[card->address+j]==smf_convert8(v,f->bits));
                else{uint16_t w=smf_convert16(v,f->bits);assert(f->card->ram[card->address+2*j]==(uint8_t)(f->little?w:w>>8));
                    assert(f->card->ram[card->address+2*j+1]==(uint8_t)(f->little?w>>8:w));}}
            for(j=card->logical_bytes;j<card->full_capacity;++j)assert(!f->card->ram[card->address+j]);
        }
    }
}
static void emq_lifetime(unsigned bits,unsigned cache,unsigned little,unsigned all_card,unsigned order)
{
    struct emp_trial *f=emq_make(bits,cache,little,all_card);unsigned guard_count;
    struct pt_editor_mixed_readers_quantized_batch *b=malloc(sizeof(*b)),copy;uint64_t ticket;assert(b);
    emp_open(f);emq_batch(f,b,16,1);copy=*b;guard_count=f->control.guard_count;
    assert(pt_editor_mixed_readers_prepare_batch_begin_quantized(&f->control,960,b,f->command)==PT_EDITOR_MIXED_READERS_PENDING);
    assert(f->control.guard_count==guard_count);memset(b,0xa5,sizeof(*b));free(b);
    emq_advance(f,0);emp_refs(f,0,0);ticket=emp_issue(f,0,960);emq_geometry(f,ticket,&copy);
    /* Positive proof orders do not request the destructive edit barrier. */
    emp_drain(f,0,0,16,order);emp_same(f);emp_drop(f,0);
}
static void emq_barrier(unsigned mode,unsigned order)
{
    struct emp_trial *f=emq_make(16,16,0,0);struct pt_editor_mixed_readers_quantized_batch b;
    unsigned revision,generation,i;struct pt_event event;
    emp_open(f);emq_batch(f,&b,16,1);
    assert(pt_editor_mixed_readers_prepare_batch_begin_quantized(&f->control,960,&b,f->command)==PT_EDITOR_MIXED_READERS_PENDING);
    emq_advance(f,0);emp_refs(f,0,0);emp_issue(f,0,960);
    revision=f->editor->history.revision;generation=f->editor->sampler.generation;event=f->editor->project->events[0];
    f->port.close_result=0;f->port.quiet_result=0;
    /* A real edit request cancels preparation. Preserve this separate failure
     * domain and drain genuine proofs without expecting positive OPEN/OK. */
    if(mode==0){f->editor->editing=1;f->editor->panel=0;f->editor->row=0;f->editor->project->channels.selected=0;
        pt_editor_key(f->editor,0x46,0);}
    else if(mode==1)pt_editor_key(f->editor,0x31,8);
    else if(mode==2){f->editor->panel=4;f->editor->channel_details=0;f->editor->project->channels.selected=0;
        pt_editor_key(f->editor,0x20,0);assert(f->editor->project->channels.track[0].route==PT_PAULA);}
    else {assert(mode==3);assert(!pt_editor_dispose(f->editor));}
    assert(revision==f->editor->history.revision&&generation==f->editor->sampler.generation&&
        !memcmp(&event,f->editor->project->events,sizeof(event))&&f->port.mask==UINT16_MAX&&
        f->control.first_error==PT_EDITOR_MIXED_READERS_CANCELLED&&f->binding->preparation_context==&f->control);
    if(!order)assert(pt_editor_mixed_readers_prepare_service_command(&f->control,f->command[0],0,NULL)==PT_MIXED_READERS_BACKEND);
    for(i=0;i<16;++i){assert(pt_editor_mixed_readers_prepare_service_reader(&f->control,f->reader[i],1,NULL)==PT_MIXED_READERS_BACKEND);
        assert(!pt_editor_prepare_change(f->editor));}
    if(order)assert(pt_editor_mixed_readers_prepare_service_command(&f->control,f->command[0],0,NULL)==PT_MIXED_READERS_BACKEND);
    assert(!pt_editor_prepare_change(f->editor)&&!f->control.pool&&f->control.activation&&f->port.close_calls==1);
    assert(!pt_editor_mixed_readers_prepare_close(&f->control)&&f->port.close_calls==1&&f->port.quiet_calls);
    f->port.callback_owner=NULL;f->port.quiet_result=1;
    assert(pt_editor_prepare_change(f->editor));assert(!f->binding->preparation_context&&!f->control.activation);
    assert(pt_editor_mixed_readers_prepare_get(&f->control)==PT_EDITOR_MIXED_READERS_CANCELLED);
    emp_same(f);emp_drop(f,0);
}
static void emq_admission(unsigned mode)
{
    struct emp_trial *f=emq_make(24,16,0,0);struct pt_editor_mixed_readers_quantized_batch b,copy;
    struct pt_editor_mixed_command_ref out={1,999},*output=&out;const struct pt_editor_mixed_readers_quantized_batch *input=&b;
    uint64_t frame=960;unsigned calls,chip,writes;void *control=malloc(sizeof(f->control));assert(control);
    emp_open(f);emq_batch(f,&b,5,1);
    switch(mode){case 0:b.count=0;break;case 1:b.count=17;break;
    case 2:b.levels[4].mode=(enum pt_sampler_mixed_trigger_level_mode)2;break;
    case 3:b.levels[4].mode=PT_SAMPLER_MIXED_TRIGGER_LEGACY;break;
    case 4:b.action[4].geometry.amigus.trigger.volume=1;break;case 5:b.action[4].geometry.amigus.trigger.pan=1;break;
    case 6:b.action[4].geometry.amigus.rate=1;break;case 7:b.action[4].geometry.amigus.left=1;break;case 8:b.action[4].geometry.amigus.right=1;break;
    case 9:b.levels[0].mode=PT_SAMPLER_MIXED_TRIGGER_QUANTIZED;break;case 10:b.levels[0].left=1;break;
    case 11:b.levels[15].mode=PT_SAMPLER_MIXED_TRIGGER_QUANTIZED;break;case 12:b.levels[15].left=1;break;case 13:b.levels[15].right=1;break;
    case 14:b.action[15].kind=PT_MIXED_READERS_STOP;break;case 15:b.action[15].track=1;break;case 16:b.action[15].sample=1;break;
    case 17:b.action[15].channel=1;break;case 18:b.action[15].expected=f->pin[0];break;
    case 19:b.action[15].reader.serial=1;break;case 20:b.action[15].reader.slot=1;break;
    case 21:b.action[15].geometry.amigus.trigger.offset=1;break;case 22:b.action[15].geometry.amigus.rate=1;break;
    case 23:b.action[4].expected=(void *)(uintptr_t)1;break;case 24:b.action[4].track=0;break;case 25:b.action[4].sample=255;break;
    case 26:b.action[4].channel=2;break;case 27:b.action[4].reader.serial=1;break;
    case 28:b.action[4].geometry.amigus.trigger.rate_numerator=0;break;case 29:b.action[4].geometry.amigus.trigger.rate_denominator=0;break;
    case 30:b.action[4].geometry.amigus.bits=24;break;case 31:b.action[4].geometry.amigus.little_endian=2;break;
    case 32:frame=UINT64_MAX;break;case 33:output=(void *)(b.levels+15);break;
    case 34:output=(void *)&f->ordinary;break;case 35:output=(void *)(f->pcm[31].data+63);break;
    case 36:input=(void *)(f->pcm[31].data+63);break;case 37:input=(void *)((uintptr_t)&b+1);break;
    case 38:input=(void *)(UINTPTR_MAX-15);break;case 39:input=NULL;break;
    case 40:output=(void *)f->control.pool;break;case 41:output=(void *)((uintptr_t)&out+1);break;
    case 42:b.action[4].kind=(enum pt_mixed_readers_kind)9;break;
    default:assert(mode==43);b.action[15].geometry.amigus.bits=8;break;
    }
    copy=b;calls=f->ordinary.calls;chip=f->chip.calls;writes=f->card->writes;memcpy(control,&f->control,sizeof(f->control));
    assert(pt_editor_mixed_readers_prepare_batch_begin_quantized(&f->control,frame,input,output)==PT_EDITOR_MIXED_READERS_INVALID);
    assert(out.slot==1&&out.serial==999&&!memcmp(&b,&copy,sizeof(b))&&!memcmp(control,&f->control,sizeof(f->control))&&
        calls==f->ordinary.calls&&chip==f->chip.calls&&writes==f->card->writes&&!f->port.reads);
    free(control);emp_same(f);emp_drop(f,0);
}
static void emq_alias(unsigned quantized,unsigned mode)
{
    struct emp_trial *f=emq_make(16,16,0,0);struct pt_editor_mixed_readers_quantized_batch b,copy;
    struct pt_editor_mixed_command_ref out={1,999};unsigned guards,calls,releases;
    void *alias,*saved;size_t bytes;emp_open(f);emq_batch(f,&b,5,1);
    memcpy(f->request,b.action,5*sizeof(*f->request));
    if(!quantized){f->request[4].geometry.amigus.trigger.volume=64;f->request[4].geometry.amigus.trigger.pan=128;}
    switch(mode){case 0:alias=quantized?(void *)&b:(void *)f->request;bytes=quantized?sizeof(b):5*sizeof(*f->request);break;
    case 1:alias=quantized?(void *)(b.action+15):(void *)(f->request+4);bytes=sizeof(b.action[0]);break;
    case 2:alias=quantized?(void *)(b.levels+15):(void *)&out;bytes=quantized?sizeof(b.levels[0]):sizeof(out);break;
    case 3:alias=(char *)&out+sizeof(out)-1;bytes=1;break;
    case 4:alias=f->pcm[31].data+63;bytes=sizeof(int32_t);break;
    default:assert(mode==5);alias=quantized?(void *)((char *)&b+sizeof(b)-1):(void *)((char *)f->request+5*sizeof(*f->request)-1);bytes=1;break;
    }
    saved=malloc(bytes);assert(saved);memcpy(saved,alias,bytes);copy=b;guards=f->control.guard_count;
    calls=f->ordinary.calls;releases=f->ordinary.releases;f->ordinary.alias=alias;
    if(quantized)assert(pt_editor_mixed_readers_prepare_batch_begin_quantized(&f->control,960,&b,&out)==PT_EDITOR_MIXED_READERS_FAULT);
    else assert(pt_editor_mixed_readers_prepare_batch_begin(&f->control,960,f->request,5,&out)==PT_EDITOR_MIXED_READERS_FAULT);
    assert(f->ordinary.calls==calls+1&&f->ordinary.releases==releases&&!f->ordinary.alias_releases&&
        !memcmp(saved,alias,bytes)&&!memcmp(&b,&copy,sizeof(b))&&out.slot==1&&out.serial==999&&
        f->control.guard_count==guards&&!f->control.busy&&!f->port.reads&&!f->port.publishes&&!f->card->writes);
    f->ordinary.alias=NULL;free(saved);emp_same(f);emp_drop(f,0);
}
static void emq_alias_waiting(unsigned quantized)
{
    struct emp_trial *f=emq_make(24,16,0,0);struct pt_editor_mixed_readers_quantized_batch b;
    struct pt_editor_mixed_command_ref out={1,999};unsigned i,guards;uint64_t ticket=999;
    emp_open(f);emp_requests(f,5);emp_prepare(f,0,960);emp_refs(f,0,0);
    assert(pt_editor_mixed_readers_prepare_enqueue(&f->control,f->command[0],&ticket)==PT_MIXED_READERS_OK&&ticket!=999);
    assert(pt_editor_mixed_readers_prepare_publish(&f->control,f->command[0])==PT_MIXED_READERS_OK&&!f->port.commits);
    memset(&b,0,sizeof(b));b.count=1;b.action[0].kind=PT_MIXED_READERS_TRIGGER;b.action[0].track=5;
    b.action[0].expected=f->pin[0];b.action[0].channel=1;b.action[0].geometry.amigus.bits=16;
    b.action[0].geometry.amigus.trigger=(struct pt_amigus_voice_request){8000,1,0,quantized?0:64,quantized?0:128};
    if(quantized){b.levels[0]=(struct pt_sampler_mixed_trigger_levels){PT_SAMPLER_MIXED_TRIGGER_QUANTIZED,32639,32895};
        f->ordinary.alias=b.levels+15;}else{f->request[0]=b.action[0];f->ordinary.alias=&out;}
    guards=f->control.guard_count;
    if(quantized)assert(pt_editor_mixed_readers_prepare_batch_begin_quantized(&f->control,1440,&b,&out)==PT_EDITOR_MIXED_READERS_FAULT);
    else assert(pt_editor_mixed_readers_prepare_batch_begin(&f->control,1440,f->request,1,&out)==PT_EDITOR_MIXED_READERS_FAULT);
    assert(f->control.guard_count==guards&&out.serial==999&&!f->ordinary.alias_releases);
    f->ordinary.alias=NULL;f->port.ticks=oracle(960);
    assert(pt_mixed_activation_fire(f->control.activation,ticket)==PT_MIXED_ACTIVATION_FAILED&&!f->port.commits&&!f->port.mask);
    assert(!pt_editor_prepare_change(f->editor));
    assert(pt_editor_mixed_readers_prepare_service_command(&f->control,f->command[0],1,NULL)==PT_MIXED_READERS_BACKEND);
    for(i=0;i<5;++i)assert(pt_editor_mixed_readers_prepare_service_reader(&f->control,f->reader[i],1,NULL)==PT_MIXED_READERS_BACKEND);
    emp_same(f);emp_drop(f,0);
}
static void emq_capacity(unsigned quantized,unsigned extra)
{
    struct emp_trial *f=emq_make(24,16,0,0);struct pt_editor_mixed_readers_quantized_batch b;
    struct pt_editor_mixed_command_ref out={1,999};unsigned initial,i,calls;void *copy=malloc(sizeof(f->control));assert(copy);
    emp_open(f);emq_batch(f,&b,5,1);initial=f->control.guard_count;
    for(i=initial;i<PT_EDITOR_MIXED_READERS_GUARDS-extra;++i)f->control.guards[i]=f->control.guards[0];
    f->control.guard_count=PT_EDITOR_MIXED_READERS_GUARDS-extra;
    memcpy(copy,&f->control,sizeof(f->control));calls=f->ordinary.calls;
    if(quantized)assert(pt_editor_mixed_readers_prepare_batch_begin_quantized(&f->control,960,&b,&out)==(extra==2?PT_EDITOR_MIXED_READERS_PENDING:PT_EDITOR_MIXED_READERS_CAPACITY));
    else assert(pt_editor_mixed_readers_prepare_batch_begin(&f->control,960,f->request,5,&out)==(extra==2?PT_EDITOR_MIXED_READERS_PENDING:PT_EDITOR_MIXED_READERS_CAPACITY));
    if(extra<2)assert(!memcmp(copy,&f->control,sizeof(f->control))&&out.serial==999&&calls==f->ordinary.calls);
    else assert(out.serial!=999&&f->control.guard_count==PT_EDITOR_MIXED_READERS_GUARDS-2&&!f->control.busy);
    memset(f->control.guards+initial,0,(f->control.guard_count-initial)*sizeof(f->control.guards[0]));f->control.guard_count=initial;
    free(copy);emp_same(f);emp_drop(f,0);
}
static void emq_suffix(unsigned quantized,unsigned alias_output)
{
    struct emp_trial *f=emq_make(24,16,0,0);struct pt_editor_mixed_readers_quantized_batch b;
    struct pt_editor_mixed_command_ref out={1,999};unsigned initial,i,registrations=0;uint64_t *neutral=calloc(1,sizeof(*neutral));assert(neutral);
    emp_open(f);emq_batch(f,&b,5,1);initial=f->control.guard_count;
    f->ordinary.hook=100+alias_output;f->ordinary.alias=alias_output?(void *)&out:(void *)neutral;
    if(quantized)assert(pt_editor_mixed_readers_prepare_batch_begin_quantized(&f->control,960,&b,&out)==
        (alias_output?PT_EDITOR_MIXED_READERS_FAULT:PT_EDITOR_MIXED_READERS_PENDING));
    else assert(pt_editor_mixed_readers_prepare_batch_begin(&f->control,960,f->request,5,&out)==
        (alias_output?PT_EDITOR_MIXED_READERS_FAULT:PT_EDITOR_MIXED_READERS_PENDING));
    assert(f->control.guard_count==initial+1&&f->control.guards[initial].data==(alias_output?(void *)&out:(void *)neutral)&&
        f->control.guards[initial].bytes==(alias_output?sizeof(out):sizeof(*neutral))&&!f->control.busy);
    for(i=0;i<PT_EDITOR_MIXED_READERS_ORDINARY;++i)if(f->control.ordinary[i].data)++registrations;
    assert(registrations==9&&f->control.command[0].handle.address);
    if(alias_output)assert(out.slot==1&&out.serial==999&&f->control.first_error==PT_EDITOR_MIXED_READERS_FAULT);
    else assert(out.serial!=999&&out.slot==0);
    f->ordinary.alias=NULL;emp_same(f);emp_drop(f,0);free(neutral);
}
static void emq_reentry(unsigned valid)
{
    struct emp_trial *f=emq_make(24,16,0,0);struct pt_editor_mixed_readers_quantized_batch b,other;
    unsigned initial;emp_open(f);emq_batch(f,&b,5,1);emq_batch(f,&other,5,1);
    if(!valid)other.levels[15].mode=(enum pt_sampler_mixed_trigger_level_mode)2;
    initial=f->control.guard_count;f->chip.alias=&other;f->ordinary.hook=102+valid;
    assert(pt_editor_mixed_readers_prepare_batch_begin_quantized(&f->control,960,&b,f->command)==
        (valid?PT_EDITOR_MIXED_READERS_FAULT:PT_EDITOR_MIXED_READERS_PENDING));
    assert(f->control.guard_count==initial&&f->control.reentries==valid&&!f->control.busy);
    f->chip.alias=NULL;emp_same(f);emp_drop(f,0);
}
static void emq_authority(unsigned mode)
{
    struct emp_trial *f=emq_make(24,16,0,0);struct pt_editor_mixed_readers_quantized_batch b;
    struct pt_editor_mixed_command_ref out={1,999};struct pt_project header;unsigned i,guards,calls;
    emp_open(f);emq_batch(f,&b,16,1);
    assert(pt_editor_mixed_readers_prepare_batch_begin_quantized(&f->control,960,&b,f->command)==PT_EDITOR_MIXED_READERS_PENDING);
    emq_advance(f,0);emp_refs(f,0,0);emp_issue(f,0,960);
    assert(pt_editor_mixed_readers_prepare_service_command(&f->control,f->command[0],0,NULL)==PT_MIXED_READERS_OK);
    memset(&b,0,sizeof(b));b.count=2;
    for(i=0;i<2;++i){b.action[i].kind=PT_MIXED_READERS_CONTROL;b.action[i].track=4+i;b.action[i].sample=(4+i)%2;
        b.action[i].channel=1;b.action[i].reader=f->reader[4+i];b.action[i].geometry.amigus.rate=0x18000;
        b.action[i].geometry.amigus.left=111;b.action[i].geometry.amigus.right=222;}
    guards=f->control.guard_count;calls=f->ordinary.calls;
    if(mode<2){b.levels[1].mode=PT_SAMPLER_MIXED_TRIGGER_QUANTIZED;
        if(mode){b.action[1].kind=PT_MIXED_READERS_STOP;memset(&b.action[1].geometry,0,sizeof(b.action[1].geometry));}
        assert(pt_editor_mixed_readers_prepare_batch_begin_quantized(&f->control,1440,&b,&out)==PT_EDITOR_MIXED_READERS_INVALID);
        assert(calls==f->ordinary.calls&&!f->control.first_error&&out.serial==999);
    }else if(mode==2){f->chip.alias=&b;f->owned_hook=100;
        assert(pt_editor_mixed_readers_prepare_batch_begin_quantized(&f->control,1440,&b,f->command+1)==PT_EDITOR_MIXED_READERS_PENDING);
        assert(b.action[1].reader.serial==UINT64_MAX);f->chip.alias=NULL;emq_advance(f,1);emp_issue(f,1,1440);
        assert(pt_editor_mixed_readers_prepare_service_command(&f->control,f->command[1],0,NULL)==PT_MIXED_READERS_OK);
        memset(&b,0,sizeof(b));b.count=2;
        for(i=0;i<2;++i){b.action[i].kind=PT_MIXED_READERS_STOP;b.action[i].track=4+i;
            b.action[i].sample=(4+i)%2;b.action[i].channel=1;b.action[i].reader=f->reader[4+i];}
        assert(pt_editor_mixed_readers_prepare_batch_begin_quantized(&f->control,1920,&b,f->command+1)==PT_EDITOR_MIXED_READERS_PENDING);
        emq_advance(f,1);emp_issue(f,1,1920);
        assert(pt_editor_mixed_readers_prepare_service_command(&f->control,f->command[1],0,NULL)==PT_MIXED_READERS_OK);
    }else{assert(mode==3||mode==4);header=*f->editor->project;f->owned_hook=98+mode;
        assert(pt_editor_mixed_readers_prepare_batch_begin_quantized(&f->control,1440,&b,&out)==PT_EDITOR_MIXED_READERS_STALE);
        assert(f->control.first_error==PT_EDITOR_MIXED_READERS_STALE&&out.serial==999&&calls==f->ordinary.calls);
        for(i=0;i<16;++i)assert(pt_editor_mixed_readers_prepare_service_reader(&f->control,f->reader[i],1,NULL)==PT_MIXED_READERS_BACKEND);
        *f->editor->project=header;--f->editor->history.revision;
    }
    if(mode<3)for(i=0;i<16;++i)assert(pt_editor_mixed_readers_prepare_service_reader(&f->control,f->reader[i],1,NULL)==PT_MIXED_READERS_OK);
    assert(f->control.guard_count==guards);emp_same(f);emp_drop(f,0);
}
static void emq_cancel(unsigned steps)
{
    struct emp_trial *f=emq_make(24,16,0,0);struct pt_editor_mixed_readers_quantized_batch b;
    unsigned i,calls,initial;enum pt_editor_mixed_readers_result r;emp_open(f);emq_batch(f,&b,5,1);initial=f->control.guard_count;
    assert(pt_editor_mixed_readers_prepare_batch_begin_quantized(&f->control,960,&b,f->command)==PT_EDITOR_MIXED_READERS_PENDING);
    emp_refs(f,0,0);
    for(i=0;i<steps;++i){r=pt_editor_mixed_readers_prepare_batch_advance(&f->control,f->command[0]);
        assert(r==PT_EDITOR_MIXED_READERS_PENDING||r==PT_EDITOR_MIXED_READERS_OPEN);}
    calls=f->ordinary.calls;assert(pt_editor_mixed_readers_prepare_cancel(&f->control)==PT_EDITOR_MIXED_READERS_CANCELLED);
    assert(pt_editor_mixed_readers_prepare_close(&f->control)&&f->ordinary.calls==calls&&f->control.guard_count==initial&&
        !f->port.reads&&!f->port.publishes&&!f->port.command_calls&&!f->port.reader_calls);
    emp_same(f);emp_drop(f,0);
}
static void emq_odd_geometry(unsigned odd_end)
{
    struct emp_trial *f=emq_make(24,8,0,0);struct pt_editor_mixed_readers_quantized_batch b,full;
    unsigned steps=0;enum pt_editor_mixed_readers_result r;
    assert(pt_sampler_loop(&f->editor->sampler,f->editor->project,&f->editor->history,0,PT_LOOP_FORWARD,
        odd_end?2:1,odd_end?5:6,0)==PT_EDIT_OK);
    pt_sampler_unpin(f->pin[0]);f->pin[0]=NULL;
    assert(pt_sampler_pin(&f->editor->sampler,f->editor->project,0,f->editor->sampler.generation,&f->pcm[0],f->pin)==PT_EDIT_OK);
    free(f->saved);f->saved=emp_save(f,&f->saved_bytes);
    emp_open(f);emq_batch(f,&full,5,1);memset(&b,0,sizeof(b));b.count=1;b.action[0]=full.action[4];b.levels[0]=full.levels[4];
    assert(pt_editor_mixed_readers_prepare_batch_begin_quantized(&f->control,960,&b,f->command)==PT_EDITOR_MIXED_READERS_PENDING);
    do{r=pt_editor_mixed_readers_prepare_batch_advance(&f->control,f->command[0]);assert(++steps<100);}while(r==PT_EDITOR_MIXED_READERS_PENDING);
    assert(r==PT_EDITOR_MIXED_READERS_STALE&&!f->port.reads&&!f->port.publishes&&!f->port.commits);
    assert(f->editor->project->samples[0].loop_start==(odd_end?2U:1U)&&f->editor->project->samples[0].loop_end==(odd_end?5U:6U));
    emp_same(f);emp_drop(f,0);
}
static void emq_budget(unsigned workspace)
{
    struct emp_trial *f=emq_make(24,16,0,0);void *copy=malloc(sizeof(f->control));assert(copy);
    if(workspace)f->input.factory_capacity=pt_sampler_mixed_workspace_size()-1;
    else f->input.factory_budget=pt_sampler_mixed_pool_size()-1;
    memcpy(copy,&f->control,sizeof(f->control));
    assert(pt_editor_mixed_readers_prepare_begin(&f->control,&f->input)==PT_EDITOR_MIXED_READERS_INVALID);
    assert(!memcmp(copy,&f->control,sizeof(f->control))&&!f->ordinary.calls&&!f->chip.calls&&!f->binding->preparation_context);
    free(copy);emp_same(f);emp_drop(f,0);
}
static void emq_enqueue_fault(void)
{
    struct emp_trial *f=emq_make(24,16,0,0);struct pt_editor_mixed_readers_quantized_batch b;uint64_t ticket=999;
    emp_open(f);emq_batch(f,&b,5,1);
    assert(pt_editor_mixed_readers_prepare_batch_begin_quantized(&f->control,960,&b,f->command)==PT_EDITOR_MIXED_READERS_PENDING);
    emq_advance(f,0);emp_refs(f,0,0);f->owned_hook=1;
    assert(pt_editor_mixed_readers_prepare_enqueue(&f->control,f->command[0],&ticket)==PT_MIXED_READERS_OK);
    assert(ticket!=999&&f->control.command[f->command[0].slot].ticket==ticket&&
        f->control.command[f->command[0].slot].transferred&&f->control.first_error==PT_EDITOR_MIXED_READERS_FAULT&&!f->port.publishes);
    emp_same(f);emp_drop(f,0);
}
int main(void)
{
    unsigned i,bits,cache,little,all_card,order,quantized;
    assert(emq_legacy_main()==0); /* Complete pre-existing positive/ownership suite exactly once. */
    for(i=0;i<44;++i)emq_admission(i);
    for(quantized=0;quantized<2;++quantized){for(i=0;i<6;++i)emq_alias(quantized,i);
        for(i=0;i<3;++i)emq_capacity(quantized,i);
        for(i=0;i<2;++i)emq_suffix(quantized,i);
        emq_alias_waiting(quantized);}
    puts("EDITOR QUANTIZED ADMISSION PASS:44 fixed16 semantic/span refusals;14 genuine legacy/quantized ordinary batch-allocation original/tail/output/spare aliases including earlier copied-fire veto;4 reserve-two refusals,2 exact-two-slot admissions and4 separately labelled numeric suffix/publication oracles; SOFTWARE_ONLY");
    for(bits=8;bits<=24;bits+=8)for(cache=8;cache<=16;cache+=8)for(little=0;little<2;++little)
        for(all_card=0;all_card<2;++all_card)for(order=0;order<2;++order)emq_lifetime(bits,cache,little,all_card,order);
    puts("EDITOR QUANTIZED OWNERSHIP PASS:48 genuine16-reader 8/16/24 masters with mixed4Paula/12card or16card; exact copied gain pairs, seven-field geometry/endian/channel/padding and matched arena/full-capacity oracles; expired inputs, exact grid/master saves and both proof orders; SOFTWARE_ONLY");
    for(i=0;i<4;++i)for(order=0;order<2;++order)emq_barrier(i,order);
    for(i=0;i<2;++i)emq_reentry(i);
    for(i=0;i<5;++i)emq_authority(i);
    for(i=0;i<24;++i)emq_cancel(i);
    for(i=0;i<2;++i){emq_odd_geometry(i);emq_budget(i);}
    emq_enqueue_fault();
    puts("EDITOR QUANTIZED LIFETIME PASS:2 invalid/valid callback reentries;5 genuine CONTROL/STOP tag/copy/raw-key-failure and poisoned-source groups;24 incremental unpublished phase cancels;2 genuine odd8 geometry and2 queried one-byte budget/workspace refusals; actual enqueue transfer under callback fault; unchanged original editor hook/transfer/proof/source-quiet suite invoked once; SOFTWARE_ONLY");
    puts("EDITOR QUANTIZED PASS:genuine controller request/ref route plus allocator-span repair in BOTH entries; no new owner/layout/normalizer certificate, full-song producer, native/device/IRQ/timing/audio/listening authority");
    return 0;
}
