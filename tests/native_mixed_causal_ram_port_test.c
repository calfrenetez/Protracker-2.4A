/* SOURCE-only ordinary-RAM pair/source qualification. Root alone executes.
 * Genuine donor suite entries are renamed and UNCALLED; no mirrored core.
 * The fixture carrier enlarges only cp_make's first caller-owned allocation.
 * All other calloc/free and all production allocator calls remain genuine. */
#include <stdlib.h>
#include <stddef.h>
#include "../src/native/readers_ram/native_mixed_causal_ram_port.h"
static void *cr_calloc(size_t,size_t);
#define calloc cr_calloc
#define PT_EDITOR_MIXED_CAUSAL_PREPARE_TEST_MAIN inherited_causal_preparation_suite_not_called
#include "editor_mixed_causal_prepare_test.c"
#undef PT_EDITOR_MIXED_CAUSAL_PREPARE_TEST_MAIN
#undef calloc

struct cr_arm {
    struct pt_mixed_causal_registration registration;
    uint64_t ticket,tick;
    uint32_t frequency;
    unsigned known,live,calls,proofs;
    int arm_raw,quiet_raw;
};
struct cr_state {
    struct cp_trial *trial;
    struct pt_private_mixed_causal_ram_port *port;
    struct pt_mixed_causal_registration registration;
    struct cr_arm arm[2];
    uint64_t now,script[8];
    uint32_t frequency,script_frequency[8];
    int clock_raw,script_raw[8],source_raw,quiet_raw;
    unsigned clocks,script_count,script_index,arms,proofs,shutdowns,probes;
    unsigned arm_hook,source_hook,clock_hook,packet_mode,poisoned;
    void *poison_command;
};
struct cr_carrier {
    struct cp_trial trial; /* Proven offset0, never a causal/queue type cast. */
    struct pt_private_mixed_causal_ram_port port;
    struct cr_state state;
};
struct cr_carrier_alignment {char prefix;struct cr_carrier value;};
struct cr_trial_alignment {char prefix;struct cp_trial value;};
struct cr_master {struct pt_pcm pcm;struct pt_sample_version *pin;int32_t *bytes;};
struct cr_before {
    unsigned sample_count;
    uint32_t revision,generation;
    struct cr_master master[PT_PROJECT_SAMPLES];
};
static struct cr_carrier *cr_current;
static unsigned cr_expand,cr_cases;
static void *cr_calloc(size_t count,size_t bytes)
{
    if(cr_expand){
        assert(count==1&&bytes==sizeof(struct cp_trial));cr_expand=0;
        cr_current=calloc(1,sizeof(*cr_current));assert(cr_current);
        assert(offsetof(struct cr_carrier,trial)==0);
        assert((uintptr_t)cr_current%offsetof(struct cr_carrier_alignment,value)==0);
        assert((uintptr_t)&cr_current->trial%offsetof(struct cr_trial_alignment,value)==0);
        return &cr_current->trial;
    }
    return calloc(count,bytes);
}
static int cr_registration(const struct pt_mixed_causal_registration *a,
 const struct pt_mixed_causal_registration *b)
{return a->owner==b->owner&&a->queue==b->queue&&a->session==b->session&&a->generation==b->generation;}
static unsigned cr_zero(const void *p,size_t n)
{const unsigned char *q=p;size_t i;for(i=0;i<n;++i)if(q[i])return 0;return 1;}
static struct cr_arm *cr_find(struct cr_state *s,uint64_t ticket)
{unsigned i;for(i=0;i<2;++i)if(s->arm[i].known&&s->arm[i].ticket==ticket)return s->arm+i;return NULL;}
static int cr_clock(void *context,uint64_t *ticks,uint32_t *frequency)
{
    struct cr_state *s=context;int raw=s->clock_raw;uint64_t t=s->now;uint32_t f=s->frequency;
    ++s->clocks;
    if(s->script_count){assert(s->script_index<s->script_count);
        t=s->script[s->script_index];f=s->script_frequency[s->script_index];raw=s->script_raw[s->script_index++];}
    if(s->clock_hook){s->clock_hook=0;
        assert(pt_private_mixed_causal_ram_dispatch(s->port,s->arm[0].ticket)==PT_MIXED_CAUSAL_INVALID);}
    *ticks=t;*frequency=f;return raw;
}
static int cr_arm_at(void *context,const struct pt_mixed_causal_registration *r,
 uint64_t ticket,uint64_t tick,uint32_t frequency)
{
    struct cr_state *s=context;struct cr_arm *a;unsigned i;
    assert(cr_registration(r,&s->registration)&&s->arms<2&&!cr_find(s,ticket));
    i=s->arms++;a=s->arm+i;
    memcpy(&a->registration,r,sizeof(*r));a->ticket=ticket;a->tick=tick;a->frequency=frequency;
    a->known=1;++a->calls;
    /* Only an unchanged raw0 claims no newly acquired callback. Hook raw0
     * deliberately records a possible callback and is not positive absence. */
    a->live=a->arm_raw!=0||s->arm_hook;
    assert(tick==oracle(i?1920:960)&&frequency==709379&&!s->port->effects);
    if(s->arm_hook){s->arm_hook=0;
        assert(pt_private_mixed_causal_ram_dispatch(s->port,ticket)==PT_MIXED_CAUSAL_INVALID);}
    return a->arm_raw;
}
static int cr_ticket_quiet(void *context,const struct pt_mixed_causal_registration *r,
 uint64_t ticket,unsigned cancel)
{
    struct cr_state *s=context;struct cr_arm *a=cr_find(s,ticket);
    assert(cr_registration(r,&s->registration)&&cancel<=1&&a&&cr_registration(r,&a->registration));
    ++s->proofs;++a->proofs;
    if(a->quiet_raw==1)a->live=0;
    return a->quiet_raw; /* Independent per ticket; never touches its sibling. */
}
static int cr_source_close(void *context,const struct pt_mixed_causal_registration *r)
{
    struct cr_state *s=context;unsigned i;
    assert(cr_registration(r,&s->registration)&&++s->shutdowns==1);
    for(i=0;i<2;++i)assert(!s->arm[i].live);
    if(s->source_hook){s->source_hook=0;
        assert(pt_private_mixed_causal_ram_dispatch(s->port,UINT64_MAX)==PT_MIXED_CAUSAL_INVALID);}
    return s->source_raw;
}
static int cr_source_quiet(void *context,const struct pt_mixed_causal_registration *r)
{
    struct cr_state *s=context;unsigned i;
    assert(cr_registration(r,&s->registration)&&s->shutdowns==1);++s->probes;
    for(i=0;i<2;++i)assert(!s->arm[i].live);
    return s->quiet_raw;
}
static int cr_bind(void *context,const struct pt_mixed_causal_registration *r)
{
    struct pt_private_mixed_causal_ram_port *p=context;struct cr_state *s=p->adapter.context;
    struct cp_trial *f=s->trial;
    assert(++f->port.bind_calls==1&&f->ordinary.calls==2&&cp_live(&f->ordinary)==2);
    assert(!f->control.pool&&!f->chip.calls&&!f->card->writes&&!s->clocks&&!s->arms);
    assert(r->owner==f->control.causal&&r->queue==f->control.queue&&r->session==31&&r->generation==17);
    assert(!pt_mixed_readers_commands_held(f->control.queue)&&!pt_mixed_readers_readers_held(f->control.queue));
    memcpy(&s->registration,r,sizeof(*r));
    return pt_private_mixed_causal_ram_bind(p,f->control.causal,f->control.queue,r->session,r->generation);
}
static void cr_release(void *context,void *p)
{
    struct cp_memory *m=context;struct cp_trial *f=m->owner;struct cr_state *s=&cr_current->state;unsigned i;
    assert(f==&cr_current->trial);
    if(p==s->poison_command){
        for(i=0;i<80&&m->live[i].p!=p;++i){}assert(i<80&&m->live[i].n);
        /* Actual release callback owns this complete consumed command block.
         * Poison immediately before unchanged genuine release/free. */
        memset(p,0xa5,m->live[i].n);s->poison_command=NULL;++s->poisoned;
    }
    cp_release(context,p);
}
static int cr_bad_packet(void *context,struct pt_mixed_causal_owner *owner,
 const struct pt_mixed_causal_command_identity *identity,const struct pt_mixed_causal_packet *packet)
{
    struct pt_private_mixed_causal_ram_port *p=context;struct cr_state *s=p->adapter.context;
    struct pt_mixed_causal_packet copy;struct pt_mixed_causal_port genuine=pt_private_mixed_causal_ram_api(p);
    assert(s->packet_mode&&packet->count==6);memcpy(&copy,packet,sizeof(copy));
    if(s->packet_mode==1)copy.expected[19].serial=1;
    else{assert(s->packet_mode==2);copy.card[15].full_capacity=4;}
    return genuine.publish(context,owner,identity,&copy);
}
static struct cp_trial *cr_make_bound(unsigned bits,unsigned cache_bits,unsigned little,unsigned residency)
{
    struct cp_trial *f;struct cr_state *s;struct pt_private_mixed_causal_ram_adapter a;
    assert(!cr_expand);cr_expand=1;f=cp_make(bits,cache_bits,little);
    assert(!cr_expand&&f==&cr_current->trial);++cr_cases;s=&cr_current->state;
    s->trial=f;s->port=&cr_current->port;s->now=100;s->frequency=709379;
    s->clock_raw=s->source_raw=s->quiet_raw=1;
    s->arm[0].arm_raw=s->arm[1].arm_raw=s->arm[0].quiet_raw=s->arm[1].quiet_raw=1;
    a=(struct pt_private_mixed_causal_ram_adapter){s,sizeof(*s),cr_clock,cr_arm_at,cr_ticket_quiet,cr_source_close,cr_source_quiet};
    assert(pt_private_mixed_causal_ram_init(&cr_current->port,31,17,709379,&a,residency,8));
    /* The genuine facade captures complete port+adapter extent BEFORE any
     * callback. Other context pointers still point into the unchanged base. */
    f->input.contexts.bytes=sizeof(*cr_current);
    f->input.causal.port=pt_private_mixed_causal_ram_api(&cr_current->port);
    f->input.causal.allocator.release=cr_release;f->input.bind_original=cr_bind;
    assert(f->input.contexts.data==f&&f->input.contexts.bytes==sizeof(*cr_current));return f;
}
static struct cp_trial *cr_make(unsigned bits,unsigned cache_bits,unsigned little)
{return cr_make_bound(bits,cache_bits,little,64);}
static struct cr_before *cr_capture(struct cp_trial *f)
{
    struct cr_before *b=calloc(1,sizeof(*b));unsigned i;assert(b);
    b->sample_count=f->editor->project->sample_count;b->revision=f->editor->history.revision;
    b->generation=f->editor->sampler.generation;
    for(i=0;i<b->sample_count;++i){struct cr_master *m=b->master+i;
        m->pcm=f->editor->project->samples[i].pcm;m->pin=f->pin[i];assert(m->pin);
        if(m->pcm.capacity){assert(m->pcm.data&&m->pcm.capacity<=SIZE_MAX/sizeof(int32_t));
            m->bytes=malloc(m->pcm.capacity*sizeof(int32_t));assert(m->bytes);
            memcpy(m->bytes,m->pcm.data,m->pcm.capacity*sizeof(int32_t));}}
    assert(b->master[31].pcm.capacity==64&&b->master[31].pcm.bits==24);return b;
}
static void cr_same(struct cp_trial *f,const struct cr_before *b)
{
    unsigned i;cp_same(f);assert(b->sample_count==f->editor->project->sample_count&&
        b->revision==f->editor->history.revision&&b->generation==f->editor->sampler.generation);
    for(i=0;i<b->sample_count;++i){const struct cr_master *m=b->master+i;const struct pt_pcm *p=&f->editor->project->samples[i].pcm;
        assert(p->data==m->pcm.data&&p->capacity==m->pcm.capacity&&p->frames==m->pcm.frames&&p->rate==m->pcm.rate&&
            p->channels==m->pcm.channels&&p->bits==m->pcm.bits&&f->pin[i]==m->pin&&f->editor->sampler.current[i]==m->pin);
        if(p->capacity)assert(!memcmp(p->data,m->bytes,p->capacity*sizeof(int32_t)));}
}
static void cr_before_drop(struct cr_before *b)
{unsigned i;for(i=0;i<b->sample_count;++i)free(b->master[i].bytes);free(b);}
static void cr_geometry_empty(const struct pt_private_mixed_causal_ram_command *c)
{unsigned i;for(i=0;i<16;++i){assert(cr_zero(&c->packet.action[i].geometry,sizeof(c->packet.action[i].geometry)));
 assert(cr_zero(c->packet.card+i,sizeof(c->packet.card[i])));}}
static unsigned cr_readers(const struct pt_private_mixed_causal_ram_port *p)
{unsigned i,n=0;for(i=0;i<32;++i)n+=p->reader[i].live!=0;return n;}
static void cr_closed(struct cp_trial *f,struct cr_before *b)
{
    struct pt_private_mixed_causal_ram_port *p=&cr_current->port;struct cr_state *s=&cr_current->state;
    unsigned i,releases=f->ordinary.releases;int closed;
    cr_same(f,b);closed=pt_editor_mixed_causal_prepare_close(&f->control);
    if(!closed){assert(!f->control.pool&&!f->control.queue&&!f->control.causal);
        releases=f->ordinary.releases;assert(pt_editor_mixed_causal_prepare_close(&f->control));
        assert(f->ordinary.releases==releases);}
    assert(!f->control.pool&&!f->control.queue&&!f->control.causal&&!f->binding->preparation_context);
    assert(!cp_live(&f->ordinary)&&!cp_live(&f->chip)&&s->shutdowns==1&&p->source_closed);
    assert(!p->mask&&cr_zero(p->slot,sizeof(p->slot))&&!cr_readers(p));
    for(i=0;i<2;++i){assert(!p->command[i].live&&!p->command[i].armed&&!p->command[i].owner&&!s->arm[i].live);
        cr_geometry_empty(p->command+i);assert(!f->control.command[i].handle.address);}
    for(i=0;i<32;++i)assert(!f->control.reader[i].handle.address);
    cr_same(f,b);cr_before_drop(b);cp_drop(f);cr_current=NULL;
}
static void cr_pair(struct cp_trial *f,unsigned count,uint64_t *first,uint64_t *second)
{
    cp_open(f);cp_requests(f,count);cp_prepare(f,0,960);cp_refs(f,0,0);*first=cp_admit(f,0);
    cp_prepare(f,1,1920);cp_refs(f,1,16);*second=cp_admit(f,1);
    assert(*first&&*second&&*first!=*second&&cr_current->state.arms==2);
    assert(cr_current->port.pair_used==2&&!cr_current->port.effects&&!cr_current->port.mask);
    assert(cr_zero(cr_current->port.slot,sizeof(cr_current->port.slot)));
    assert(pt_mixed_readers_commands_held(f->control.queue)==2&&pt_mixed_readers_readers_held(f->control.queue)==2*count);
}
static void cr_observe(struct cp_trial *f,unsigned base,unsigned count,uint64_t tick)
{
    unsigned i,releases=f->ordinary.releases,chips=f->chip.releases,proofs=cr_current->state.proofs;
    for(i=0;i<count;++i){struct pt_editor_mixed_reader_record *record=f->control.reader+f->reader[base+i].slot;
        struct pt_mixed_readers_reader_receipt receipt;struct pt_mixed_readers_key key;
        void *handle=record->handle.address;uint64_t token=record->handle.token;
        assert(handle&&token&&record->ticket);
        assert(pt_editor_mixed_causal_prepare_service_reader(&f->control,f->reader[base+i],0,NULL)==PT_MIXED_READERS_PENDING);
        /* The facade deliberately publishes outputs only on OK. Use the
         * genuine core task-only noncancel observation for its PENDING receipt,
         * after the facade has already refreshed the real queue. This separate
         * stack output is disjoint from the complete retained carrier. */
        assert(pt_mixed_causal_service_reader(f->control.causal,record->ticket,record->action,0,&receipt)==PT_MIXED_READERS_PENDING);
        assert(receipt.state==PT_MIXED_READER_ACTIVE&&receipt.adoption==PT_MIXED_ADOPTED&&receipt.observed==tick&&receipt.issued==tick);
        assert(keys_equal(&receipt.key,cr_current->port.command[base?1:0].packet.key+i));
        /* The first actual ACTIVE reader is still barred from future control
         * admission by its queued replacing successor. Query only the second
         * set after its actual observation, when no future batch follows. */
        if(base){assert(pt_mixed_causal_reader_key(f->control.causal,record->ticket,record->action,&key)==PT_MIXED_READERS_OK);
            assert(keys_equal(&key,&receipt.key));}
        assert(record->handle.address==handle&&record->handle.token==token&&record->ticket);}
    assert(releases==f->ordinary.releases&&chips==f->chip.releases&&proofs==cr_current->state.proofs);
}
static void cr_packet(struct cp_trial *f,const struct pt_mixed_causal_packet *packet,unsigned ci,unsigned count)
{
    static const uint8_t mono8[2][6]={{0x7f,1,3,0x40,0,0xfe},{0x80,0xff,0xfd,0xc0,2,0x7e}};
    unsigned i,j;assert(packet->count==count&&packet->frame==(ci?1920U:960U));
    assert(packet->first==oracle(packet->frame)&&packet->last==oracle(packet->frame+1));
    assert(cr_registration(&packet->registration,&cr_current->state.registration));
    for(i=0;i<count;++i){const struct pt_mixed_readers_action *a=packet->action+i;
        assert(a->kind==PT_MIXED_READERS_TRIGGER&&a->route==f->editor->project->channels.track[i].route);
        assert(packet->key[i].trigger==packet->ticket&&packet->key[i].action==i&&packet->key[i].route==a->route&&packet->key[i].slot==a->slot);
        if(a->route==PT_MIXED_READERS_PAULA){assert(a->geometry.paula.words==3&&a->geometry.paula.period==428&&a->geometry.paula.volume==64);
            assert(!memcmp(a->geometry.paula.data,mono8[0],6));}
        else{const struct pt_mixed_readers_card *c=packet->card+i;const struct pt_amigus_voice_plan *v=&a->geometry.amigus;
            struct pt_cache_lease lease={c->cache_slot,c->serial};struct pt_cache_entry *e;
            void *block;struct pt_amigus_ram_block *r;
            assert(c->cache_slot<PT_CACHE_SLOTS);e=f->card->cache.cache.entry+c->cache_slot;
            block=pt_cache_data(&f->card->cache.cache,lease);assert(block);
            for(j=0;j<PT_CACHE_SLOTS&&block!=f->card->cache.arena.block+j;++j){}assert(j<PT_CACHE_SLOTS);r=f->card->cache.arena.block+j;
            assert(c->cache==&f->card->cache.cache&&c->reservation==&f->card->reservation&&c->version==1&&e->valid&&e->pins>=2);
            assert(c->serial==e->serial&&c->source_channel==1&&c->bits==f->cache_bits&&c->little_endian==f->little);
            assert(c->address==r->address&&c->logical_bytes==(f->cache_bits==8?6U:12U)&&c->full_capacity==r->reserved);
            assert(c->logical_bytes==e->bytes&&r->bytes==e->bytes&&r->written==r->bytes&&!r->failed);
            for(j=0;j<6;++j){uint16_t w=(uint16_t)((uint16_t)mono8[1][j]<<8);const uint8_t *d=f->card->ram+r->address;
                if(f->cache_bits==8)assert(d[j]==mono8[1][j]);
                else{assert(d[2*j]==(uint8_t)(f->little?w:w>>8));assert(d[2*j+1]==(uint8_t)(f->little?w>>8:w));}}
            for(j=r->bytes;j<r->reserved;++j)assert(!f->card->ram[r->address+j]);
            assert(v->start==r->address&&v->loop==r->address&&v->end_exclusive==r->address+r->bytes&&v->rate==44739242);
            assert(v->control==(f->cache_bits==8?0x8000U:(f->little?0x8009U:0x8001U))&&v->left==32768&&v->right==32768);}
    }
}
static void cr_success(unsigned bits,unsigned cache,unsigned order,unsigned all_card)
{
    struct cp_trial *f=cr_make(bits,cache,order);struct cr_before *b;struct cr_state *s=&cr_current->state;
    struct pt_private_mixed_causal_ram_port *p=&cr_current->port;
    struct pt_mixed_causal_packet *copies=calloc(2,sizeof(*copies));uint8_t *ram;uint64_t first,second;unsigned i,j,writes,chips,live;
    struct pt_mixed_readers_key key,sentinel;
    assert(copies);if(all_card)cp_all_card(f);b=cr_capture(f);cp_open(f);cp_requests(f,16);
    cp_prepare(f,0,960);cp_refs(f,0,0);first=cp_admit(f,0);memcpy(copies,&p->command[0].packet,sizeof(*copies));
    ram=malloc(sizeof(f->card->ram));assert(ram);memcpy(ram,f->card->ram,sizeof(f->card->ram));writes=f->card->writes;chips=f->chip.calls;
    cp_prepare(f,1,1920);cp_refs(f,1,16);second=cp_admit(f,1);memcpy(copies+1,&p->command[1].packet,sizeof(*copies));
    assert(first!=second&&s->arms==2&&p->publishes==2&&p->pair_used==2&&!p->effects&&!p->mask&&!cr_readers(p));
    assert(writes==f->card->writes&&chips==f->chip.calls&&!memcmp(ram,f->card->ram,sizeof(f->card->ram)));free(ram);
    cr_packet(f,copies,0,16);cr_packet(f,copies+1,1,16);
    for(i=0;i<16;++i){assert(!keys_equal(copies->key+i,copies[1].key+i));
        if(copies->action[i].route==PT_MIXED_READERS_PAULA)assert(copies->action[i].geometry.paula.data==copies[1].action[i].geometry.paula.data);
        else assert(!memcmp(copies->card+i,copies[1].card+i,sizeof(copies->card[i])));}
    assert(cr_zero(p->slot,sizeof(p->slot))&&!cr_readers(p));
    assert(pt_mixed_readers_commands_held(f->control.queue)==2&&pt_mixed_readers_readers_held(f->control.queue)==32);
    live=cp_live(&f->ordinary);assert(live==37);cr_same(f,b);cp_barrier(f);
    memset(&sentinel,0xa5,sizeof(sentinel));memcpy(&key,&sentinel,sizeof(key));
    assert(pt_mixed_causal_reader_key(f->control.causal,first,0,&key)==PT_MIXED_READERS_STALE&&!memcmp(&key,&sentinel,sizeof(key)));
    assert(pt_mixed_causal_reader_key(f->control.causal,second,0,&key)==PT_MIXED_READERS_STALE&&!memcmp(&key,&sentinel,sizeof(key)));
    assert(!p->effects&&!p->mask&&!cr_readers(p)&&!s->proofs);
    s->now=oracle(960)-1;
    assert(pt_private_mixed_causal_ram_dispatch(p,second)==PT_MIXED_CAUSAL_EARLY);
    assert(pt_private_mixed_causal_ram_dispatch(p,first)==PT_MIXED_CAUSAL_EARLY);
    assert(!p->effects&&!p->commits&&!cr_readers(p)&&p->fires==2&&!p->command[1].predecessor_completed);
    assert(!memcmp(copies,&p->command[0].packet,sizeof(*copies))&&!memcmp(copies+1,&p->command[1].packet,sizeof(*copies)));
    assert(s->arm[0].live&&s->arm[1].live&&!s->proofs);
    s->now=oracle(960);assert(pt_private_mixed_causal_ram_dispatch(p,first)==PT_MIXED_CAUSAL_COMMITTED);
    assert(p->effects==16&&cr_readers(p)==16&&p->command[1].predecessor_completed);
    assert(p->command[1].predecessor_observed==oracle(960)&&p->command[1].predecessor_issued==oracle(960));
    cr_geometry_empty(p->command);cr_observe(f,0,16,oracle(960));assert(cp_live(&f->ordinary)==live);
    if(!order){s->poison_command=f->control.command[f->command[0].slot].handle.address;assert(s->poison_command);
        cp_drain_command(f,0,0);assert(s->poisoned==1&&!s->poison_command&&!p->command[0].owner&&!p->command[0].live);
        assert(s->arm[1].live&&p->command[1].armed&&p->command[1].predecessor_completed);
        assert(pt_mixed_readers_commands_held(f->control.queue)==1&&pt_mixed_readers_readers_held(f->control.queue)==32);}
    s->now=oracle(1920)-1;assert(pt_private_mixed_causal_ram_dispatch(p,second)==PT_MIXED_CAUSAL_EARLY);
    assert(p->effects==16&&s->arm[1].live&&!memcmp(copies+1,&p->command[1].packet,sizeof(*copies)));
    s->now=oracle(1920);assert(pt_private_mixed_causal_ram_dispatch(p,second)==PT_MIXED_CAUSAL_COMMITTED);
    assert(p->effects==32&&cr_readers(p)==32&&p->fires==5&&p->commits==2);
    assert(p->trace_count==3&&p->trace_count<=p->maximum_reads&&p->ledger_result==PT_MIXED_CAUSAL_COMMITTED);
    cr_observe(f,16,16,oracle(1920));cr_geometry_empty(p->command+1);cr_same(f,b);
    if(!order)cp_drain_command(f,1,0);
    cp_drain_readers(f,0,16,0);
    assert(cr_readers(p)==16&&p->mask==(all_card?0xffff0U:0xffffU));
    for(i=0;i<16;++i){unsigned index=copies[1].action[i].route==PT_MIXED_READERS_PAULA?copies[1].action[i].slot:copies[1].action[i].slot+4;
        assert(keys_equal(p->slot+index,copies[1].key+i));}
    cp_drain_readers(f,16,16,0);
    if(order){assert(pt_mixed_readers_commands_held(f->control.queue)==2);cp_drain_command(f,0,0);cp_drain_command(f,1,0);}
    assert(!pt_mixed_readers_commands_held(f->control.queue)&&!pt_mixed_readers_readers_held(f->control.queue));
    assert(!p->mask&&cr_zero(p->slot,sizeof(p->slot))&&!cr_readers(p));
    for(i=0;i<32;++i)assert(cr_zero(p->reader+i,sizeof(p->reader[i])));
    for(i=0;i<2;++i)for(j=0;j<16;++j)assert(!p->command[i].packet.card[j].cache);
    assert(cp_live(&f->ordinary)==(order?35U:3U));free(copies);cr_closed(f,b);
}
static void cr_arm_failure(unsigned mode)
{
    struct cp_trial *f=cr_make(24,16,0);struct cr_before *b=cr_capture(f);struct cr_state *s=&cr_current->state;
    struct pt_private_mixed_causal_ram_port *p=&cr_current->port;uint64_t ticket=999;
    unsigned successor=mode>=3,ci=successor?1:0,i,proofs;
    cp_open(f);cp_requests(f,6);cp_prepare(f,0,960);cp_refs(f,0,0);
    if(successor){assert(cp_admit(f,0));cp_prepare(f,1,1920);cp_refs(f,1,16);}
    s->arm[ci].arm_raw=mode==0?0:(mode==1||mode==3?-1:(mode==5?0:2));
    if(mode==5)s->arm_hook=1;
    assert(pt_editor_mixed_causal_prepare_enqueue(&f->control,f->command[ci],&ticket)==PT_MIXED_READERS_OK&&ticket!=999);
    assert(pt_editor_mixed_causal_prepare_publish(&f->control,f->command[ci])==
        (mode==0?PT_MIXED_READERS_PENDING:PT_MIXED_READERS_BACKEND));
    assert(p->command[ci].arm_outcome==s->arm[ci].arm_raw&&s->arms==ci+1&&!p->effects&&!cr_readers(p));
    if(mode==0){assert(!p->command[0].live&&!p->command[0].armed&&!p->command[0].owner&&p->command[0].quiet_confirmed);
        assert(!s->arm[0].live&&!p->failed&&!s->proofs);cr_closed(f,b);return;}
    assert(p->command[ci].live&&p->command[ci].owner&&p->command[ci].uncertain&&p->failed&&s->arm[ci].live);
    assert(pt_mixed_readers_commands_held(f->control.queue)==ci+1&&pt_mixed_readers_readers_held(f->control.queue)==6*(ci+1));
    proofs=s->proofs;s->arm[ci].quiet_raw=0;
    assert(pt_editor_mixed_causal_prepare_service_command(&f->control,f->command[ci],1,NULL)==PT_MIXED_READERS_BACKEND);
    assert(p->command[ci].live&&s->arm[ci].live&&s->proofs==proofs+1&&p->command[ci].ticket_outcome==0);
    assert(f->control.command[f->command[ci].slot].handle.address&&f->binding->preparation_context==&f->control);
    s->arm[ci].quiet_raw=1;
    for(i=0;i<=ci;++i)cp_drain_command(f,i,1);
    cp_drain_readers(f,0,6,1);if(successor)cp_drain_readers(f,16,6,1);
    assert(!p->effects&&!p->mask&&!cr_readers(p));cr_same(f,b);cr_closed(f,b);
}
static void cr_expected_only(int second_raw)
{
    struct cp_trial *f=cr_make(24,16,0);struct cr_before *b=cr_capture(f);struct cr_state *s=&cr_current->state;
    struct pt_private_mixed_causal_ram_port *p=&cr_current->port;uint64_t first,second;unsigned i,proofs;
    cr_pair(f,16,&first,&second);
    assert(keys_equal(p->command[1].packet.expected,p->command[0].packet.key));
    for(i=0;i<16;++i)assert(!keys_equal(p->command[1].packet.key+i,p->command[0].packet.key));
    s->arm[1].quiet_raw=second_raw;proofs=s->proofs;
    assert(pt_editor_mixed_causal_prepare_service_reader(&f->control,f->reader[0],1,NULL)==
        (second_raw==0?PT_MIXED_READERS_PENDING:PT_MIXED_READERS_BACKEND));
    assert(s->proofs==proofs+2&&!s->arm[0].live&&s->arm[1].live);
    assert(p->command[0].disabled&&!p->command[0].armed&&p->command[0].live&&p->command[0].owner);
    assert(!p->command[1].disabled&&p->command[1].armed&&p->command[1].live&&p->command[1].ticket_outcome==second_raw);
    cr_geometry_empty(p->command);assert(!cr_zero(&p->command[1].packet.action[0].geometry,sizeof(p->command[1].packet.action[0].geometry)));
    assert(pt_mixed_readers_commands_held(f->control.queue)==2&&pt_mixed_readers_readers_held(f->control.queue)==32);
    assert(f->control.reader[f->reader[0].slot].handle.address&&f->binding->preparation_context==&f->control);
    assert(!p->effects&&!p->mask&&!p->command[1].predecessor_completed);
    i=p->fires;assert(pt_private_mixed_causal_ram_dispatch(p,second)==
        (second_raw==0?PT_MIXED_CAUSAL_FAILED:PT_MIXED_CAUSAL_INVALID)&&p->fires==i);
    assert(p->command[1].live&&p->command[1].owner&&s->arm[1].live);cr_same(f,b);
    s->arm[1].quiet_raw=1;cp_drain_readers(f,0,16,second_raw!=0);cp_drain_readers(f,16,16,second_raw!=0);
    cr_geometry_empty(p->command+1);assert(pt_mixed_readers_commands_held(f->control.queue)==2);
    cp_drain_command(f,0,second_raw!=0);cp_drain_command(f,1,second_raw!=0);cr_closed(f,b);
}
static void cr_timing_failure(unsigned mode)
{
    struct cp_trial *f=cr_make_bound(24,16,0,mode==2?1U:64U);struct cr_before *b=cr_capture(f);struct cr_state *s=&cr_current->state;
    struct pt_private_mixed_causal_ram_port *p=&cr_current->port;uint64_t first,second;unsigned i;
    cr_pair(f,16,&first,&second);
    if(mode==0)s->now=oracle(961);
    else{s->script_count=3;s->script_index=0;
        for(i=0;i<3;++i){s->script[i]=oracle(960)+(mode==2&&i?2U:0U);s->script_frequency[i]=709379;s->script_raw[i]=1;}
        if(mode==1)s->script_frequency[2]=715909;}
    assert(pt_private_mixed_causal_ram_dispatch(p,first)==PT_MIXED_CAUSAL_FAILED);
    assert(p->failed&&p->suppressed&&!p->command[1].predecessor_completed&&p->command[0].uncertain);
    assert(p->effects==(mode?16U:0U)&&cr_readers(p)==(mode?16U:0U));
    assert(p->ledger_result==(mode==2?PT_MIXED_CAUSAL_COMMITTED:PT_MIXED_CAUSAL_FAILED));
    assert(p->command[0].fire_result==p->ledger_result&&p->dispatch_result==PT_MIXED_CAUSAL_FAILED);
    cr_geometry_empty(p->command);s->script_count=0;s->now=oracle(1920);
    i=p->fires;assert(pt_private_mixed_causal_ram_dispatch(p,second)==PT_MIXED_CAUSAL_FAILED&&p->fires==i);
    cr_same(f,b);cp_drain_command(f,0,mode!=2);cp_drain_command(f,1,mode!=2);
    cp_drain_readers(f,0,16,mode!=2);cp_drain_readers(f,16,16,mode!=2);cr_closed(f,b);
}
static void cr_source_failure(int raw,unsigned hook)
{
    struct cp_trial *f=cr_make(24,16,0);struct cr_before *b=cr_capture(f);struct cr_state *s=&cr_current->state;
    struct pt_private_mixed_causal_ram_port *p=&cr_current->port;unsigned releases;
    cp_open(f);s->source_raw=raw;s->source_hook=hook;
    assert(!pt_editor_mixed_causal_prepare_close(&f->control)&&!f->control.pool&&!f->control.queue&&f->control.causal);
    assert(cp_live(&f->ordinary)==1&&s->shutdowns==1&&!s->probes&&p->source_attempted&&!p->source_closed);
    assert(p->source_outcome==raw&&f->binding->preparation_context==&f->control);cr_same(f,b);
    releases=f->ordinary.releases;
    assert(pt_editor_mixed_causal_prepare_close(&f->control)&&!f->control.causal);
    assert(s->shutdowns==1&&s->probes==1&&p->source_closed&&f->ordinary.releases==releases+1);
    cr_closed(f,b);
}
static void cr_packet_refusal(unsigned mode)
{
    struct cp_trial *f=cr_make(24,16,0);struct cr_before *b=cr_capture(f);struct cr_state *s=&cr_current->state;
    struct pt_private_mixed_causal_ram_port *p=&cr_current->port;uint64_t ticket=999;
    s->packet_mode=mode;f->input.causal.port.publish=cr_bad_packet;
    cp_open(f);cp_requests(f,6);cp_prepare(f,0,960);cp_refs(f,0,0);
    assert(pt_editor_mixed_causal_prepare_enqueue(&f->control,f->command[0],&ticket)==PT_MIXED_READERS_OK&&ticket!=999);
    assert(pt_editor_mixed_causal_prepare_publish(&f->control,f->command[0])==PT_MIXED_READERS_PENDING);
    assert(p->publishes==1&&!p->pair_used&&!s->arms&&!p->failed&&!p->effects&&!cr_readers(p));cr_same(f,b);cr_closed(f,b);
}
static void cr_empty(void)
{
    struct cp_trial *f=cr_make(24,16,0);struct cr_before *b=cr_capture(f);
    cp_open(f);assert(cr_current->port.bound&&!cr_current->state.arms&&!cr_current->state.clocks);cr_closed(f,b);
}
static void cr_init_lifetimes(void)
{
    struct cp_trial *f=cr_make(24,16,0);struct cr_before *b=cr_capture(f);
    struct pt_private_mixed_causal_ram_port *p=&cr_current->port,*before=malloc(sizeof(*before));
    struct pt_private_mixed_causal_ram_adapter a=p->adapter;
    struct pt_private_mixed_causal_ram_port *fresh=calloc(1,sizeof(*fresh)),*dirty=malloc(sizeof(*dirty));
    assert(fresh&&dirty);fresh->source_probes=1;memcpy(dirty,fresh,sizeof(*fresh));
    assert(!pt_private_mixed_causal_ram_init(fresh,31,17,709379,&a,64,8));
    assert(!memcmp(fresh,dirty,sizeof(*fresh)));memset(fresh,0,sizeof(*fresh));
    assert(!pt_private_mixed_causal_ram_init(fresh,31,17,709379,&fresh->adapter,64,8)&&cr_zero(fresh,sizeof(*fresh)));
    free(fresh);free(dirty);
    assert(before);memcpy(before,p,sizeof(*p));assert(!pt_private_mixed_causal_ram_init(p,31,17,709379,&a,64,8));
    assert(!memcmp(p,before,sizeof(*p)));cp_open(f);
    assert(!pt_private_mixed_causal_ram_bind(p,f->control.causal,f->control.queue,31,17));
    assert(!pt_private_mixed_causal_ram_bind(p,f->control.causal,f->control.queue,32,17));
    assert(!pt_private_mixed_causal_ram_init(p,32,17,709379,&a,64,8));free(before);cr_closed(f,b);
}
static void cr_authority(void)
{
    struct cp_trial *f=cr_make(24,16,0);struct cr_before *b=cr_capture(f);
    struct cr_state *s=&cr_current->state;struct pt_private_mixed_causal_ram_port *p=&cr_current->port;
    struct pt_editor_mixed_command_ref out={99,999};uint64_t first,second;unsigned calls,writes,clocks;
    struct pt_mixed_causal_command_identity wrong;struct pt_mixed_causal_port api;
    struct pt_private_mixed_causal_ram_port *before=malloc(sizeof(*before));assert(before);
    cp_open(f);cp_requests(f,16);calls=f->ordinary.calls;writes=f->card->writes;
    f->request[0].kind=PT_MIXED_READERS_CONTROL;
    assert(pt_editor_mixed_causal_prepare_batch_begin(&f->control,960,f->request,16,&out)==PT_EDITOR_MIXED_READERS_INVALID);
    f->request[0].kind=PT_MIXED_READERS_STOP;
    assert(pt_editor_mixed_causal_prepare_batch_begin(&f->control,960,f->request,16,&out)==PT_EDITOR_MIXED_READERS_INVALID);
    assert(out.slot==99&&out.serial==999&&calls==f->ordinary.calls&&writes==f->card->writes&&!f->control.first_error);
    cp_requests(f,16);cp_prepare(f,0,960);cp_refs(f,0,0);first=cp_admit(f,0);
    cp_prepare(f,1,1920);cp_refs(f,1,16);second=cp_admit(f,1);assert(first!=second);
    clocks=s->clocks;assert(pt_private_mixed_causal_ram_dispatch(p,UINT64_MAX)==PT_MIXED_CAUSAL_INVALID);
    assert(s->clocks==clocks&&!p->fires&&!p->effects&&!cr_readers(p));
    memcpy(&wrong,&p->command[0].identity,sizeof(wrong));++wrong.registration.session;
    api=pt_private_mixed_causal_ram_api(p);memcpy(before,p,sizeof(*p));
    assert(api.command_quiet(p,&wrong,1)==-1&&!memcmp(before,p,sizeof(*p))&&!s->proofs);
    free(before);calls=f->ordinary.calls;
    assert(pt_editor_mixed_causal_prepare_batch_begin(&f->control,2880,f->request,16,&out)==PT_EDITOR_MIXED_READERS_CAPACITY);
    assert(out.slot==99&&out.serial==999&&calls==f->ordinary.calls&&p->pair_used==2);
    cp_drain_command(f,0,0);cp_drain_command(f,1,0);cp_drain_readers(f,0,16,0);cp_drain_readers(f,16,16,0);
    calls=f->ordinary.calls;assert(pt_editor_mixed_causal_prepare_batch_begin(&f->control,2880,f->request,16,&out)==PT_EDITOR_MIXED_READERS_CAPACITY);
    assert(calls==f->ordinary.calls&&f->control.prepared_batches==2&&!p->effects);cr_closed(f,b);
}
#ifndef PT_NATIVE_MIXED_CAUSAL_RAM_PORT_TEST_MAIN
#define PT_NATIVE_MIXED_CAUSAL_RAM_PORT_TEST_MAIN main
#endif
int PT_NATIVE_MIXED_CAUSAL_RAM_PORT_TEST_MAIN(void)
{
    unsigned bits,cache,order,mode;
    for(bits=8;bits<=24;bits+=8)for(cache=8;cache<=16;cache+=8)for(order=0;order<2;++order)cr_success(bits,cache,order,0);
    cr_success(24,16,0,1);cr_success(24,16,1,1);
    for(mode=0;mode<6;++mode)cr_arm_failure(mode);
    cr_expected_only(0);cr_expected_only(-1);
    for(mode=0;mode<3;++mode)cr_timing_failure(mode);
    cr_source_failure(0,0);cr_source_failure(-1,0);cr_source_failure(2,0);cr_source_failure(1,1);
    cr_packet_refusal(1);cr_packet_refusal(2);cr_empty();cr_init_lifetimes();cr_authority();assert(cr_cases==34);
    puts("NATIVE MIXED CAUSAL RAM PORT PASS:34 genuine software cases;14 two-ticket 4Paula+12card/16card 8/16/24-master cache8+16 pairs;original960/1920 arms and EARLY preservation;actual post-clock tombstone and poisoned disposed C;32 persistent readers,independent C/R/expected-only second proofs;clean0 versus unknown/reentrant0 arm retention,late/post-clock/residency failures,once source shutdown then read-only quiet;all20/tail refusals,full master capacities/exact saves/zero owned ledgers; SOFTWARE_ONLY");
    return 0;
}
