/* Proposed genuine fixture only. Production owner/queue compile separately.
 * Reuse committed master/cache/holder helpers, but do not run their old main.
 * The unchanged v1 fixtures remain separate acceptance dependencies. */
#define PT_MIXED_READERS_TEST_MAIN ct_committed_resource_fixture_not_called
#include "mixed_scheduled_readers_test.c"
#include "../src/core/mixed_readers_causal.h"
#if defined(__has_feature)
#if __has_feature(address_sanitizer)
#define CT_ASAN 1
#endif
#endif
#if defined(__SANITIZE_ADDRESS__)
#define CT_ASAN 1
#endif
#ifdef CT_ASAN
#include <sanitizer/asan_interface.h>
#define CT_POISON(p,n) __asan_poison_memory_region((p),(n))
#define CT_UNPOISON(p,n) __asan_unpoison_memory_region((p),(n))
#else
#define CT_POISON(p,n) ((void)(p),(void)(n))
#define CT_UNPOISON(p,n) ((void)(p),(void)(n))
#endif
struct ct_command {
    struct pt_mixed_causal_packet packet;
    struct pt_mixed_causal_command_identity identity,predecessor;
    uint64_t serial;
    unsigned live,finished,disabled,first_applied;
};
struct ct_reader {
    struct pt_mixed_readers_key key;struct pt_mixed_readers_action action;
    struct pt_mixed_readers_card card;unsigned live;
};
struct ct_port {
    struct pt_mixed_causal_registration registration;
    struct ct_command command[2];struct ct_reader reader[32];
    struct pt_mixed_readers_key slot[20];unsigned mask;
    uint64_t ticks,unknown_ticket;uint32_t frequency;
    int commit_raw,source_raw;
    unsigned commits,effects,publications,clocks,command_proofs,reader_proofs,shutdowns,probes;
    unsigned late,malformed,bad_adoption,reenter,closed;
};
struct ct_memory {void *block[2];unsigned calls,live,releases;};
struct ct_case {
    struct trial *trial;struct ct_port port;struct ct_memory memory;
    struct pt_mixed_causal_config config;struct pt_mixed_causal_owner *owner;
    void *workspace;uint8_t *before;size_t before_bytes;
};
static unsigned ct_cases;
static unsigned ct_index(unsigned route,unsigned slot)
{assert((route==1&&slot<4)||(route==2&&slot<16));return route==1?slot:4+slot;}
static int ct_registration(const struct pt_mixed_causal_registration *a,const struct pt_mixed_causal_registration *b)
{return a->owner==b->owner&&a->queue==b->queue&&a->session==b->session&&a->generation==b->generation;}
static struct ct_command *ct_command(struct ct_port *p,uint64_t ticket)
{unsigned i;for(i=0;i<2;++i)if(p->command[i].live&&p->command[i].packet.ticket==ticket)return p->command+i;return NULL;}
static struct ct_reader *ct_reader(struct ct_port *p,const struct pt_mixed_readers_key *key)
{unsigned i;for(i=0;i<32;++i)if(p->reader[i].live&&keys_equal(&p->reader[i].key,key))return p->reader+i;return NULL;}
static int ct_expected(struct ct_port *p,const struct pt_mixed_causal_packet *b)
{unsigned i;if(!ct_registration(&p->registration,&b->registration)||p->mask!=b->expected_mask)return 0;
 for(i=0;i<20;++i)if(!keys_equal(p->slot+i,b->expected+i))return 0;
 return 1;}
static void *ct_allocate(void *context,size_t bytes)
{struct ct_memory *m=context;void *p=malloc(bytes);unsigned i;if(!p)return NULL;
 for(i=0;i<2;++i)if(!m->block[i])break;
 assert(i<2);m->block[i]=p;++m->calls;++m->live;return p;}
static void ct_release(void *context,void *pointer)
{struct ct_memory *m=context;unsigned i;for(i=0;i<2;++i)if(m->block[i]==pointer)break;
 assert(i<2&&m->live);m->block[i]=NULL;--m->live;++m->releases;free(pointer);}
static int ct_clock(void *context,uint64_t *ticks,uint32_t *frequency)
{struct ct_port *p=context;++p->clocks;*ticks=p->ticks;*frequency=p->frequency;return 1;}
static int ct_publish(void *context,struct pt_mixed_causal_owner *owner,
    const struct pt_mixed_causal_command_identity *identity,const struct pt_mixed_causal_packet *b)
{struct ct_port *p=context;unsigned i;++p->publications;
 if(p->closed||owner!=p->registration.owner||!ct_expected(p,b)||p->ticks>=b->first||
    !ct_registration(&identity->registration,&p->registration)||identity->ticket!=b->ticket||
    !identity->owner||!identity->event||!identity->binding.context||!identity->binding.context_bytes)return 0;
 for(i=0;i<2;++i)if(!p->command[i].live)break;
 if(i==2)return 0;
 memcpy(&p->command[i].packet,b,sizeof(*b));memcpy(&p->command[i].identity,identity,sizeof(*identity));p->command[i].live=1;return 1;}
static int ct_publish_successor(void *context,struct pt_mixed_causal_owner *owner,const struct pt_mixed_causal_publication *d)
{
    struct ct_port *p=context;struct ct_command *first=ct_command(p,d->first.ticket),*second;
    struct pt_mixed_readers_key predicted[20];unsigned mask,i,index;
    ++p->publications;
    if(p->closed||!first||first->finished||first->disabled||owner!=p->registration.owner||
       !d->serial||d->predecessor.ticket!=first->packet.ticket||
       memcmp(&d->predecessor,&first->identity,sizeof(d->predecessor))||
       !ct_registration(&d->predecessor.registration,&p->registration)||
       !d->predecessor.owner||!d->predecessor.event||!d->predecessor.binding.context||
       !d->predecessor.binding.context_bytes||memcmp(&first->packet,&d->first,sizeof(d->first))||
       !ct_expected(p,&d->first)||d->successor.frame<=d->first.frame||p->ticks>=d->successor.first)return 0;
    /* Independently derive the complete prediction from the actual original
     * model registry plus the exact retained first packet, not d.expected. */
    mask=p->mask;memcpy(predicted,p->slot,sizeof(predicted));
    for(i=0;i<d->first.count;++i){
        if(d->first.action[i].kind!=PT_MIXED_READERS_TRIGGER)return 0;
        index=ct_index(d->first.action[i].route,d->first.action[i].slot);
        predicted[index]=d->first.key[i];mask|=1U<<index;
    }
    if(!ct_registration(&d->successor.registration,&p->registration)||d->successor.expected_mask!=mask||
       !ct_registration(&d->successor_identity.registration,&p->registration)||
       d->successor_identity.ticket!=d->successor.ticket||!d->successor_identity.owner||
       !d->successor_identity.event||!d->successor_identity.binding.context||!d->successor_identity.binding.context_bytes)return 0;
    for(i=0;i<20;++i)if(!keys_equal(d->successor.expected+i,predicted+i))return 0;
    for(i=0;i<2;++i)if(!p->command[i].live)break;
    if(i==2)return 0;
    second=p->command+i;memcpy(&second->packet,&d->successor,sizeof(second->packet));
    memcpy(&second->identity,&d->successor_identity,sizeof(second->identity));memcpy(&second->predecessor,&d->predecessor,sizeof(second->predecessor));
    second->serial=d->serial;second->live=1;return 1;
}
static void ct_geometry_drop(struct ct_command *c)
{unsigned i;for(i=0;i<16;++i){memset(&c->packet.action[i].geometry,0,sizeof(c->packet.action[i].geometry));memset(c->packet.card+i,0,sizeof(c->packet.card[i]));}}
static int ct_commit(void *context,const struct pt_mixed_causal_packet *b,struct pt_mixed_causal_actual *actual)
{
    struct ct_port *p=context;struct ct_command *c=ct_command(p,b->ticket);
    unsigned i,j,selected=0,placement[16]={0};int raw=p->commit_raw;
    ++p->commits;
    if(!c||c->finished||c->disabled||!ct_expected(p,b)||memcmp(&c->packet,b,sizeof(*b))||
       p->ticks<b->first||p->ticks>=b->last||!raw||(c->serial&&!c->first_applied))return 0;
    for(i=0;i<b->count;++i){
        if(b->action[i].kind!=PT_MIXED_READERS_TRIGGER||ct_reader(p,b->key+i))return 0;
        for(j=0;j<32;++j)if(!p->reader[j].live&&!(selected&(1U<<j)))break;
        if(j==32)return 0;
        placement[i]=j;selected|=1U<<j;
    }
    for(i=0;i<b->count;++i){struct ct_reader *r=p->reader+placement[i];unsigned index=ct_index(b->action[i].route,b->action[i].slot);
        r->key=b->key[i];r->action=b->action[i];r->card=b->card[i];r->live=1;
        p->slot[index]=b->key[i];p->mask|=1U<<index;++p->effects;
        if(raw<0)break;
    }
    c->finished=1;ct_geometry_drop(c);
    for(i=0;i<2;++i)if(p->command[i].live&&p->command[i].predecessor.ticket==b->ticket)p->command[i].first_applied=1;
    actual->active_mask=actual->adopted_mask=p->mask;memcpy(actual->slot,p->slot,sizeof(p->slot));
    if(p->malformed){actual->slot[19].serial=777;actual->active_mask|=1U<<19;actual->adopted_mask=actual->active_mask;}
    if(p->bad_adoption)actual->adopted_mask&=~1U;
    if(p->late)p->ticks=b->last;
    if(p->reenter){p->reenter=0;assert(pt_mixed_causal_fire((struct pt_mixed_causal_owner *)(void *)p->registration.owner,b->ticket)==PT_MIXED_CAUSAL_INVALID);}
    return raw;
}
static int ct_command_quiet(void *context,const struct pt_mixed_causal_command_identity *identity,unsigned cancel)
{struct ct_port *p=context;struct ct_command *c=ct_command(p,identity->ticket);++p->command_proofs;
 if(!ct_registration(&p->registration,&identity->registration)||cancel>1||p->unknown_ticket==identity->ticket)return -1;
 if(!c)return 1;
 if(memcmp(identity,&c->identity,sizeof(*identity)))return -1;
 if(!c->finished&&!c->disabled&&!cancel)return 0;
 ct_geometry_drop(c);memset(c,0,sizeof(*c));return 1;}
static int ct_depends(const struct ct_command *c,const struct pt_mixed_readers_key *key)
{unsigned i;if(!c->live)return 0;
 for(i=0;i<20;++i)if(keys_equal(c->packet.expected+i,key))return 1;
 for(i=0;i<16;++i)if(keys_equal(c->packet.key+i,key))return 1;
 return 0;}
static int ct_reader_quiet(void *context,const struct pt_mixed_causal_reader_identity *identity,unsigned cancel)
{
    struct ct_port *p=context;struct ct_reader *r;unsigned i,index;
    ++p->reader_proofs;if(!ct_registration(&p->registration,&identity->registration)||cancel>1)return -1;
    r=ct_reader(p,&identity->key);
    /* Actual command completion never retires its persistent reader. An
     * ordinary noncancel observation preserves this independently held R. */
    if(r&&!cancel)return 0;
    for(i=0;i<2;++i){struct ct_command *c=p->command+i;if(!ct_depends(c,&identity->key))continue;
        /* Exact separate ticket proof before disabling or dropping tails.
         * An unknown second proof cannot be covered by the first success. */
        if(p->unknown_ticket==c->packet.ticket)return -1;
        if(!cancel&&!c->finished&&!c->disabled)return 0;
        c->disabled=1;ct_geometry_drop(c);
    }
    r=ct_reader(p,&identity->key);if(r){memset(r,0,sizeof(*r));}
    index=ct_index(identity->key.route,identity->key.slot);
    if((p->mask&(1U<<index))&&keys_equal(p->slot+index,&identity->key)){
        p->mask&=~(1U<<index);memset(p->slot+index,0,sizeof(p->slot[index]));
    }
    return 1;
}
static int ct_empty(const struct ct_port *p)
{unsigned i;for(i=0;i<2;++i)if(p->command[i].live)return 0;
 for(i=0;i<32;++i)if(p->reader[i].live)return 0;
 return !p->mask;}
static int ct_source_close(void *context,const struct pt_mixed_causal_registration *registration)
{struct ct_port *p=context;++p->shutdowns;assert(p->shutdowns==1);
 if(!ct_registration(&p->registration,registration)||!ct_empty(p))return -1;
 if(p->source_raw==1)p->closed=1;
 return p->source_raw;}
static int ct_source_quiet(void *context,const struct pt_mixed_causal_registration *registration)
{struct ct_port *p=context;++p->probes;
 return ct_registration(&p->registration,registration)&&ct_empty(p)?1:-1;}
static struct ct_case *ct_make(unsigned bits,unsigned cache_bits)
{
    struct ct_case *c=calloc(1,sizeof(*c));assert(c);++ct_cases;
    c->trial=trial_make(bits,cache_bits);c->before=save(c->trial->resources,&c->before_bytes);
    c->port.ticks=100;c->port.frequency=709379;c->port.commit_raw=c->port.source_raw=1;
    c->config.allocator=(struct pt_allocator){&c->memory,ct_allocate,ct_release};
    c->config.allocator_context=(struct pt_mixed_readers_span){&c->memory,sizeof(c->memory)};
    c->config.grid=(struct pt_mixed_readers_grid){100,17,709379,48000};c->config.session=19;
    c->config.control_budget=pt_mixed_causal_control_size();c->config.queue_budget=pt_mixed_readers_control_size();
    c->config.port=(struct pt_mixed_causal_port){&c->port,sizeof(c->port),1,31,ct_clock,ct_publish,ct_commit,
        ct_command_quiet,ct_reader_quiet,ct_source_close,ct_source_quiet,ct_publish_successor};
    c->workspace=calloc(1,pt_mixed_causal_workspace_size());assert(c->workspace);
    assert(pt_mixed_causal_open(&c->config,c->workspace,pt_mixed_causal_workspace_size(),&c->owner)==PT_MIXED_READERS_OK);
    assert(pt_mixed_causal_borrow_queue(c->owner,&c->trial->queue)==PT_MIXED_READERS_OK&&c->memory.live==2);
    /* Original registration binds before any factory/admission/source use. */
    c->port.registration=(struct pt_mixed_causal_registration){c->owner,c->trial->queue,19,17};
    return c;
}
static uint64_t ct_first(struct ct_case *c,unsigned count)
{uint64_t ticket=0;trigger_input(c->trial,count,0,960);
 assert(pt_mixed_causal_enqueue(c->owner,c->trial->input,&ticket)==PT_MIXED_READERS_OK);
 assert(pt_mixed_causal_publish(c->owner,ticket)==PT_MIXED_READERS_OK);return ticket;}
static uint64_t ct_second(struct ct_case *c,uint64_t first,unsigned count)
{uint64_t ticket=0;trigger_input(c->trial,count,1,1920);
 assert(pt_mixed_causal_enqueue_successor(c->owner,first,c->trial->input,&ticket)==PT_MIXED_READERS_OK);
 assert(pt_mixed_causal_publish_successor(c->owner,first,ticket)==PT_MIXED_READERS_OK);return ticket;}
static void ct_retire_readers(struct ct_case *c,uint64_t ticket,unsigned count,unsigned failed)
{unsigned i;for(i=0;i<count;++i)assert(pt_mixed_causal_service_reader(c->owner,ticket,i,1,NULL)==
    (failed?PT_MIXED_READERS_BACKEND:PT_MIXED_READERS_OK));}
static void ct_finish(struct ct_case *c,uint64_t first,uint64_t second,unsigned count,unsigned first_disposed,unsigned order,unsigned failed)
{
    enum pt_mixed_readers_result expected=failed?PT_MIXED_READERS_BACKEND:PT_MIXED_READERS_OK;
    if(!order){if(!first_disposed)assert(pt_mixed_causal_service_command(c->owner,first,1,NULL)==expected);
        if(second)assert(pt_mixed_causal_service_command(c->owner,second,1,NULL)==expected);}
    ct_retire_readers(c,first,count,failed);if(second)ct_retire_readers(c,second,count,failed);
    if(order){if(!first_disposed)assert(pt_mixed_causal_service_command(c->owner,first,1,NULL)==expected);
        if(second)assert(pt_mixed_causal_service_command(c->owner,second,1,NULL)==expected);}
    assert(!pt_mixed_readers_commands_held(c->trial->queue)&&!pt_mixed_readers_readers_held(c->trial->queue));
}
static void ct_drop(struct ct_case *c,unsigned source_unknown)
{
    same_save(c->trial->resources,c->before,c->before_bytes);
    if(source_unknown){c->port.source_raw=-1;assert(!pt_mixed_causal_close(&c->owner)&&c->owner&&c->memory.live==1);
        assert(!c->memory.block[1]);c->trial->queue=NULL;
        assert(pt_mixed_causal_close(&c->owner)&&c->port.shutdowns==1&&c->port.probes==1);}
    else assert(pt_mixed_causal_close(&c->owner)&&c->port.shutdowns==1);
    assert(!c->owner&&!c->memory.live&&pt_mixed_causal_close(&c->owner));c->trial->queue=NULL;
    same_save(c->trial->resources,c->before,c->before_bytes);trial_drop(c->trial);
    free(c->before);free(c->workspace);free(c);
}
static void ct_success(unsigned bits,unsigned cache_bits,unsigned order)
{
    struct ct_case *c=ct_make(bits,cache_bits);uint64_t first=ct_first(c,16),second=ct_second(c,first,16);
    struct pt_mixed_causal_diagnostic d;struct pt_mixed_readers_key key;unsigned i;
    assert(pt_mixed_readers_readers_held(c->trial->queue)==32&&!c->port.effects);
    assert(pt_mixed_causal_diagnostic(c->owner,&d)&&d.admitted&&d.published&&!d.completed&&!d.commit_called);
    c->port.ticks=oracle(960)-1;
    assert(pt_mixed_causal_fire(c->owner,second)==PT_MIXED_CAUSAL_EARLY&&!c->port.commits&&!c->port.effects);
    assert(pt_mixed_causal_diagnostic(c->owner,&d)&&!d.completed&&!d.suppressed&&!d.commit_called);
    assert(pt_mixed_causal_fire(c->owner,first)==PT_MIXED_CAUSAL_EARLY&&!c->port.commits);
    c->port.ticks=oracle(960);assert(pt_mixed_causal_fire(c->owner,first)==PT_MIXED_CAUSAL_COMMITTED&&c->port.effects==16);
    assert(pt_mixed_causal_service_command(c->owner,first,0,NULL)==PT_MIXED_READERS_OK&&c->trial->command[0].releases==1);
    CT_POISON(c->trial->command,sizeof(c->trial->command[0]));
    assert(pt_mixed_causal_diagnostic(c->owner,&d)&&d.completed&&d.commit_called&&d.commit_outcome==1&&
        d.observed==oracle(960)&&d.issued==oracle(960)&&!d.suppressed);
    c->port.ticks=oracle(1920)-1;
    assert(pt_mixed_causal_fire(c->owner,second)==PT_MIXED_CAUSAL_EARLY&&c->port.commits==1&&c->port.effects==16);
    c->port.ticks=oracle(1920);assert(pt_mixed_causal_fire(c->owner,second)==PT_MIXED_CAUSAL_COMMITTED&&c->port.effects==32);
    for(i=0;i<16;++i){assert(pt_mixed_causal_service_reader(c->owner,second,i,0,NULL)==PT_MIXED_READERS_PENDING);
        assert(pt_mixed_causal_reader_key(c->owner,second,i,&key)==PT_MIXED_READERS_OK&&key.trigger==second);}
    ct_finish(c,first,second,16,1,order,0);CT_UNPOISON(c->trial->command,sizeof(c->trial->command[0]));ct_drop(c,order);
}
static void ct_refusal(unsigned mode)
{
    struct ct_case *c=ct_make(24,16);uint64_t first=ct_first(c,6),refused=99;unsigned count=6;
    trigger_input(c->trial,count,1,1920);
    if(mode==1)c->trial->input->batch.action[0].kind=PT_MIXED_READERS_CONTROL;
    if(mode==2)c->trial->input->batch.action[0].kind=PT_MIXED_READERS_STOP;
    assert(pt_mixed_causal_enqueue_successor(c->owner,mode?first:first+999,c->trial->input,&refused)==PT_MIXED_READERS_INVALID&&refused==99);
    assert(pt_mixed_readers_commands_held(c->trial->queue)==1&&pt_mixed_readers_readers_held(c->trial->queue)==6);
    unused_drop(c->trial,count,1);ct_finish(c,first,0,6,0,0,0);ct_drop(c,0);
}
static void ct_third_refusal(void)
{
    struct ct_case *c=ct_make(24,16);uint64_t first=ct_first(c,16),second=ct_second(c,first,16),refused=99;
    trigger_input(c->trial,1,2,2880);
    assert(pt_mixed_causal_enqueue_successor(c->owner,second,c->trial->input,&refused)==PT_MIXED_READERS_INVALID&&refused==99);
    assert(pt_mixed_causal_enqueue(c->owner,c->trial->input,&refused)==PT_MIXED_READERS_INVALID&&refused==99);
    assert(pt_mixed_readers_commands_held(c->trial->queue)==2&&pt_mixed_readers_readers_held(c->trial->queue)==32);
    unused_drop(c->trial,1,2);ct_finish(c,first,second,16,0,1,0);ct_drop(c,0);
}
static void ct_failed_first(unsigned mode)
{
    struct ct_case *c=ct_make(24,16);uint64_t first=ct_first(c,6),second=ct_second(c,first,6);
    struct pt_mixed_causal_diagnostic d;unsigned commits;
    if(mode<3)c->port.commit_raw=mode==0?-1:mode==1?0:2;
    if(mode==3)c->port.late=1;
    if(mode==4)c->port.malformed=1;
    if(mode==5)c->port.reenter=1;
    if(mode==6)c->port.bad_adoption=1;
    c->port.ticks=oracle(960);
    if(mode==7)c->port.ticks=oracle(1920);
    assert(pt_mixed_causal_fire(c->owner,mode==7?second:first)==PT_MIXED_CAUSAL_FAILED);
    commits=c->port.commits;
    assert(pt_mixed_causal_diagnostic(c->owner,&d)&&!d.completed&&d.suppressed);
    if(mode!=7)assert(d.commit_called&&d.commit_outcome==c->port.commit_raw);
    else assert(!d.commit_called&&!c->port.effects);
    c->port.late=c->port.malformed=0;c->port.ticks=oracle(1920);
    assert(pt_mixed_causal_fire(c->owner,second)==PT_MIXED_CAUSAL_FAILED&&c->port.commits==commits);
    ct_finish(c,first,second,6,0,mode&1,1);ct_drop(c,0);
}
static void ct_expected_only(unsigned unknown)
{
    struct ct_case *c=ct_make(24,16);uint64_t first=ct_first(c,6),second=ct_second(c,first,6);unsigned i;
    struct pt_mixed_causal_diagnostic d;
    c->port.ticks=oracle(960);assert(pt_mixed_causal_fire(c->owner,first)==PT_MIXED_CAUSAL_COMMITTED);
    assert(pt_mixed_causal_service_command(c->owner,first,0,NULL)==PT_MIXED_READERS_OK);
    if(unknown){c->port.unknown_ticket=second;
        assert(pt_mixed_causal_service_reader(c->owner,first,0,1,NULL)==PT_MIXED_READERS_BACKEND);
        assert(c->trial->reader[0].live&&!c->trial->reader[0].releases&&pt_mixed_readers_readers_held(c->trial->queue)==12);
        c->port.unknown_ticket=0;}
    assert(pt_mixed_causal_service_reader(c->owner,first,0,1,NULL)==(unknown?PT_MIXED_READERS_BACKEND:PT_MIXED_READERS_OK));
    assert(pt_mixed_causal_diagnostic(c->owner,&d)&&d.suppressed&&d.completed);
    assert(pt_mixed_causal_service_command(c->owner,second,1,NULL)==(unknown?PT_MIXED_READERS_BACKEND:PT_MIXED_READERS_OK));
    for(i=1;i<6;++i)assert(pt_mixed_causal_service_reader(c->owner,first,i,1,NULL)==(unknown?PT_MIXED_READERS_BACKEND:PT_MIXED_READERS_OK));
    ct_retire_readers(c,second,6,unknown);ct_drop(c,0);
}
#ifndef PT_MIXED_CAUSAL_TEST_MAIN
#define PT_MIXED_CAUSAL_TEST_MAIN main
#endif
int PT_MIXED_CAUSAL_TEST_MAIN(void)
{
    unsigned bits,cache_bits,order,mode;
    for(bits=8;bits<=24;bits+=8)for(cache_bits=8;cache_bits<=16;cache_bits+=8)for(order=0;order<2;++order)ct_success(bits,cache_bits,order);
    for(mode=0;mode<3;++mode)ct_refusal(mode);
    ct_third_refusal();for(mode=0;mode<8;++mode)ct_failed_first(mode);
    ct_expected_only(0);ct_expected_only(1);assert(ct_cases==26);
    puts("MIXED CAUSAL TWO TRIGGER PASS:26 genuine heap cases;12 two16 mixed4/12 8/16/24-master cache8/16 lifetimes;internally derived all20 prediction;early successor before/after completion;actual original clocks/adoption/tombstone;poisoned disposed predecessor C;32-reader and third admission bounds;CONTROL/STOP/wrong predecessor refusal;raw first partial/zero/malformed/late/reentry and wrong fire order suppression;expected-only R and unknown second proof;once shutdown then read-only quiet;master beforeimages;SOFTWARE_ONLY");
    return 0;
}
