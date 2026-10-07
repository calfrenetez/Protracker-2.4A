/* Full committed genuine producer/queue fixture remains an independent oracle. */
#include "mixed_scheduled_readers_test.c"
#include "../src/core/mixed_readers_activation.h"
#if defined(__has_feature)
#if __has_feature(address_sanitizer)
#define ACTIVATION_ASAN 1
#endif
#endif
#if defined(__SANITIZE_ADDRESS__)
#define ACTIVATION_ASAN 1
#endif
#ifdef ACTIVATION_ASAN
#include <sanitizer/asan_interface.h>
#define A_POISON(p,n) __asan_poison_memory_region((p),(n))
#define A_UNPOISON(p,n) __asan_unpoison_memory_region((p),(n))
#else
#define A_POISON(p,n) ((void)(p),(void)(n))
#define A_UNPOISON(p,n) ((void)(p),(void)(n))
#endif
struct paired_ram_reader {
    struct pt_mixed_readers_key key;struct pt_mixed_readers_action action;
    struct pt_mixed_readers_card card;unsigned live,active;
};
struct paired_ram_command {struct pt_mixed_activation_packet packet;unsigned live,finished;};
struct paired_ram {
    struct pt_mixed_readers_activation *owner;struct pt_mixed_activation_registration registration;
    struct paired_ram_command command[2];struct paired_ram_reader reader[32];
    struct pt_mixed_readers_key slot[20];unsigned mask;
    uint64_t ticks;uint32_t frequency;
    int publish_result,commit_result,source_result,quiet_override;
    unsigned publications,commits,effects,command_probes,reader_probes,source_probes,quiet_probes,clock_reads;
    unsigned hook,hooks,late,publish_late,malformed,command_pending,reader_pending,closed,identity_mutation;
    /* Explicit adversarial observations; expire before every quiet proof. */
    unsigned packet_observation,packet_phase,packet_mutation,packet_reentry,packet_mutations;
    const struct pt_mixed_activation_packet *observed_packet;
    struct pt_mixed_activation_packet original_packet;
};
struct paired_allocator {
    void *block[4],*base[4];size_t bytes[4];unsigned calls,releases,live,fail_at,alias_at,misaligned_at,hook,hooks;
    void *alias;struct pt_mixed_readers_activation *owner;struct pt_mixed_readers_output *queue;
    const struct pt_mixed_activation_config *config;void *workspace;size_t capacity;
    struct pt_mixed_readers_activation **output;
};
struct activation_case {
    struct trial *trial;struct paired_ram ram;struct paired_allocator memory;
    struct pt_mixed_activation_config config;void *workspace;struct pt_mixed_readers_activation *owner;
};
static int a_index(unsigned route,unsigned slot)
{assert((route==1&&slot<4)||(route==2&&slot<16));return (int)(route==1?slot:slot+4);}
static int a_registration_same(const struct pt_mixed_activation_registration *a,const struct pt_mixed_activation_registration *b)
{return a->owner==b->owner&&a->queue==b->queue&&a->session==b->session&&a->generation==b->generation;}
static struct paired_ram_command *a_command(struct paired_ram *p,uint64_t ticket)
{unsigned i;for(i=0;i<2;++i)if(p->command[i].live&&p->command[i].packet.ticket==ticket)return p->command+i;return NULL;}
static struct paired_ram_reader *a_reader(struct paired_ram *p,const struct pt_mixed_readers_key *key)
{unsigned i;for(i=0;i<32;++i)if(p->reader[i].live&&keys_equal(&p->reader[i].key,key))return p->reader+i;return NULL;}
static void a_hook(struct paired_ram *p,unsigned which)
{if(p->hook==which){p->hook=0;++p->hooks;assert(pt_mixed_activation_fire(p->owner,1)==PT_MIXED_ACTIVATION_INVALID);}}
static int a_clock(void *context,uint64_t *ticks,uint32_t *frequency)
{struct paired_ram *p=context;++p->clock_reads;a_hook(p,1);*ticks=p->ticks;*frequency=p->frequency;return 1;}
static int a_expected(struct paired_ram *p,const struct pt_mixed_activation_packet *b)
{unsigned i;if(b->expected_mask!=p->mask)return 0;
 if(p->registration.owner&&!a_registration_same(&p->registration,&b->registration))return 0;
 for(i=0;i<20;++i)if(!keys_equal(b->expected+i,p->slot+i))return 0;
 return 1;}
static int a_publish_model(void *context,struct pt_mixed_readers_activation *owner,const struct pt_mixed_activation_packet *b)
{
    struct paired_ram *p=context;unsigned i,before=p->hooks;++p->publications;a_hook(p,2);
    if(!p->publish_result){if(p->hooks!=before)p->registration=b->registration;return 0;}
    if(p->closed||!a_expected(p,b)||p->ticks>=b->first||owner!=b->registration.owner)return 0;
    for(i=0;i<2;++i)if(!p->command[i].live)break;
    assert(i<2);memcpy(&p->command[i].packet,b,sizeof(*b));p->command[i].live=1;p->registration=b->registration;
    if(p->publish_late)p->ticks=b->first;
    return p->publish_result;
}
static void a_drop_packet(struct paired_ram_command *c)
{unsigned i;for(i=0;i<c->packet.count;++i){memset(&c->packet.action[i].geometry,0,sizeof(c->packet.action[i].geometry));memset(c->packet.card+i,0,sizeof(c->packet.card[i]));}}
static int a_commit_model(void *context,const struct pt_mixed_activation_packet *b,struct pt_mixed_activation_actual *actual)
{
    struct paired_ram *p=context;struct paired_ram_command *c=a_command(p,b->ticket);
    unsigned i,j,used=0,selected=0,placement[16]={0};++p->commits;a_hook(p,3);
    if(!c||c->finished||!a_expected(p,b)||memcmp(b,&c->packet,sizeof(*b))||p->ticks<b->first||p->ticks>=b->last||!p->commit_result)return 0;
    /* Complete prevalidation/reservation before the first software effect. */
    for(i=0;i<b->count;++i){const struct pt_mixed_readers_action *a=b->action+i;unsigned index=(unsigned)a_index(a->route,a->slot);
        assert(!(used&(1U<<index)));used|=1U<<index;
        assert(b->key[i].queue==b->registration.queue&&b->key[i].session==b->registration.session&&b->key[i].generation==b->registration.generation);
        if(a->kind==PT_MIXED_READERS_TRIGGER){
            if(a_reader(p,b->key+i))return 0;
            for(j=0;j<32;++j)if(!p->reader[j].live&&!(selected&(1U<<j)))break;
            if(j==32)return 0;
            if(a->route==PT_MIXED_READERS_PAULA){assert(a->geometry.paula.data&&a->geometry.paula.words&&a->geometry.paula.period&&a->geometry.paula.volume<=64);}
            else{assert(b->card[i].reservation&&b->card[i].cache&&b->card[i].logical_bytes<=b->card[i].full_capacity);
                 assert(a->geometry.amigus.start>=b->card[i].address&&a->geometry.amigus.end_exclusive<=b->card[i].address+b->card[i].logical_bytes);}
            placement[i]=j;selected|=1U<<j;
        }else{struct paired_ram_reader *r=a_reader(p,b->key+i);
            if(!r||!r->active||!(p->mask&(1U<<index))||!keys_equal(p->slot+index,b->key+i))return 0;
        }
    }
    for(i=0;i<b->count;++i){const struct pt_mixed_readers_action *a=b->action+i;unsigned index=(unsigned)a_index(a->route,a->slot);
        struct paired_ram_reader *old=a_reader(p,p->slot+index);
        if(a->kind==PT_MIXED_READERS_TRIGGER){struct paired_ram_reader *r=p->reader+placement[i];
            if(old)old->active=0;
            *r=(struct paired_ram_reader){b->key[i],*a,b->card[i],1,1};p->slot[index]=b->key[i];p->mask|=1U<<index;
        }else if(a->kind==PT_MIXED_READERS_STOP){old->active=0;p->mask&=~(1U<<index);memset(p->slot+index,0,sizeof(p->slot[index]));}
        else if(a->route==PT_MIXED_READERS_PAULA){old->action.geometry.paula.period=a->geometry.paula.period;old->action.geometry.paula.volume=a->geometry.paula.volume;}
        else{old->action.geometry.amigus.rate=a->geometry.amigus.rate;old->action.geometry.amigus.left=a->geometry.amigus.left;old->action.geometry.amigus.right=a->geometry.amigus.right;}
        ++p->effects;if(p->commit_result<0)break;
    }
    c->finished=1;a_drop_packet(c);
    actual->active_mask=p->mask;actual->adopted_mask=p->mask;memcpy(actual->slot,p->slot,sizeof(p->slot));
    if(p->malformed){actual->slot[19].serial=777;actual->active_mask|=1U<<19;actual->adopted_mask=actual->active_mask;}
    if(p->late)p->ticks=b->last;
    return p->commit_result;
}
/* An intentionally faulty port casts away const after its genuine MODEL
 * operation. Production must restore its original packet before any walk.
 * The pointer and copied oracle exist only in explicit observation cases,
 * and both expire before the first subsequent successful quiet proof. */
static void a_packet_fault(struct paired_ram *p,const struct pt_mixed_activation_packet *b,unsigned phase)
{
    struct pt_mixed_activation_packet *w=(struct pt_mixed_activation_packet *)(void *)b;
    if(p->packet_phase!=phase)return;
    assert(p->packet_observation&&p->observed_packet==b);
    ++p->packet_mutations;
    if(phase==1)p->registration=p->original_packet.registration;
    switch(p->packet_mutation){
    case 0:w->expected[19].serial^=1;break; /* Untouched twentieth original key. */
    case 1:w->card[4].serial^=1;break;
    case 2:w->action[0].geometry.paula.words^=1;break;
    case 3:w->ticket^=1;break;
    case 4:w->count=PT_MIXED_READERS_ACTIONS+1;break;
    case 5:w->action[5].geometry.amigus.end_exclusive^=2;break;
    case 6:w->action[PT_MIXED_READERS_ACTIONS-1].geometry.amigus.rate^=1;break;
    case 7:w->registration.session^=1;break;
    }
    if(p->packet_reentry){++p->hooks;
        assert(pt_mixed_activation_fire(p->owner,p->original_packet.ticket)==PT_MIXED_ACTIVATION_INVALID);}
}
static int a_publish(void *context,struct pt_mixed_readers_activation *owner,const struct pt_mixed_activation_packet *b)
{
    struct paired_ram *p=context;int raw;
    if(p->packet_observation){memcpy(&p->original_packet,b,sizeof(*b));p->observed_packet=b;}
    raw=a_publish_model(context,owner,b);a_packet_fault(p,b,1);return raw;
}
static int a_commit(void *context,const struct pt_mixed_activation_packet *b,struct pt_mixed_activation_actual *actual)
{
    struct paired_ram *p=context;int raw;
    if(p->packet_observation){memcpy(&p->original_packet,b,sizeof(*b));p->observed_packet=b;}
    raw=a_commit_model(context,b,actual);a_packet_fault(p,b,2);return raw;
}
/* No observer reference may survive a genuine command/reader/source proof. */
static void a_packet_observation_expire(struct paired_ram *p)
{
    p->packet_observation=0;p->packet_phase=0;p->packet_reentry=0;p->observed_packet=NULL;
    memset(&p->original_packet,0,sizeof(p->original_packet));
}
static int a_packet_observation_empty(const struct paired_ram *p)
{
    const uint8_t *bytes=(const uint8_t *)(const void *)&p->original_packet;size_t i;
    if(p->packet_observation||p->observed_packet)return 0;
    for(i=0;i<sizeof(p->original_packet);++i)if(bytes[i])return 0;
    return 1;
}
static int a_command_quiet(void *context,const struct pt_mixed_activation_command_identity *identity,unsigned cancel)
{
    struct paired_ram *p=context;struct paired_ram_command *c=a_command(p,identity->ticket);++p->command_probes;a_hook(p,4);
    if(p->command_pending)return 0;
    assert(a_packet_observation_empty(p));
    assert(a_registration_same(&identity->registration,&p->registration));
    if(!c)return 1;
    if(!c->finished&&!cancel)return 0;
    memset(c,0,sizeof(*c));
    if(p->identity_mutation==4){p->identity_mutation=0;((struct pt_mixed_activation_command_identity *)(void *)identity)->owner++;}
    return 1;
}
static int a_reader_quiet(void *context,const struct pt_mixed_activation_reader_identity *identity,unsigned cancel)
{
    struct paired_ram *p=context;struct paired_ram_reader *r=a_reader(p,&identity->key);unsigned i,j,index;
    ++p->reader_probes;a_hook(p,5);if(p->reader_pending)return 0;
    assert(a_packet_observation_empty(p));
    assert(a_registration_same(&identity->registration,&p->registration));
    if(r&&r->active&&!cancel)return 0;
    for(i=0;i<2;++i){struct paired_ram_command *c=p->command+i;unsigned names=0;
        if(!c->live)continue;
        for(j=0;j<c->packet.count;++j)names|=keys_equal(c->packet.key+j,&identity->key);
        if(names){if(!c->finished&&!cancel)return 0;c->finished=1;a_drop_packet(c);}
    }
    index=(unsigned)a_index(identity->key.route,identity->key.slot);
    if((p->mask&(1U<<index))&&keys_equal(p->slot+index,&identity->key)){p->mask&=~(1U<<index);memset(p->slot+index,0,sizeof(p->slot[index]));}
    if(r)memset(r,0,sizeof(*r));
    if(p->identity_mutation==5){p->identity_mutation=0;((struct pt_mixed_activation_reader_identity *)(void *)identity)->key.serial++;}
    return 1;
}
static int a_source_close(void *context,const struct pt_mixed_activation_registration *registration)
{
    struct paired_ram *p=context;unsigned i;++p->source_probes;a_hook(p,6);
    assert(a_packet_observation_empty(p));
    assert(registration->owner==p->owner&&registration->queue&&registration->session==19&&registration->generation==17);
    if(p->registration.owner)assert(a_registration_same(registration,&p->registration));
    for(i=0;i<2;++i)assert(!p->command[i].live);
    for(i=0;i<32;++i)assert(!p->reader[i].live);
    if(p->source_result==1){p->closed=1;p->owner=NULL;memset(&p->registration,0,sizeof(p->registration));}
    if(p->identity_mutation==6){p->identity_mutation=0;((struct pt_mixed_activation_registration *)(void *)registration)->session++;}
    return p->source_result;
}
static int a_source_quiet(void *context,const struct pt_mixed_activation_registration *registration)
{
    struct paired_ram *p=context;unsigned i;++p->quiet_probes;
    assert(a_packet_observation_empty(p));
    assert(registration->owner&&registration->queue&&registration->session==19&&registration->generation==17);
    if(p->registration.owner)assert(a_registration_same(registration,&p->registration));
    if(p->hook==7){p->hook=0;++p->hooks;assert(pt_mixed_activation_fire((struct pt_mixed_readers_activation *)registration->owner,1)==PT_MIXED_ACTIVATION_INVALID);}
    if(p->quiet_override)return p->quiet_override;
    if(!p->closed||p->owner)return 0;
    for(i=0;i<2;++i)if(p->command[i].live)return 0;
    for(i=0;i<32;++i)if(p->reader[i].live)return 0;
    return 1;
}
static void *a_allocate(void *context,size_t bytes)
{
    struct paired_allocator *a=context;void *p;unsigned i;++a->calls;
    if(a->hook==1){a->hook=0;++a->hooks;assert(pt_mixed_activation_open(a->config,a->workspace,a->capacity,a->output)==PT_MIXED_READERS_BACKEND);}
    if(a->fail_at==a->calls)return NULL;
    if(a->alias_at==a->calls)return a->alias;
    p=malloc(bytes+1);assert(p);for(i=0;i<4;++i)if(!a->block[i])break;assert(i<4);
    a->base[i]=p;if(a->misaligned_at==a->calls)p=(uint8_t *)p+1;
    a->block[i]=p;a->bytes[i]=bytes;++a->live;return p;
}
static void a_release(void *context,void *p)
{
    struct paired_allocator *a=context;unsigned i;++a->releases;
    for(i=0;i<4;++i)if(a->block[i]==p)break;assert(i<4&&a->live);
    a->block[i]=NULL;a->bytes[i]=0;--a->live;
    if(a->hook==2){a->hook=0;++a->hooks;assert(pt_mixed_readers_stop(a->queue)==PT_MIXED_READERS_BACKEND);}
    if(a->hook==3&&p==a->owner){a->hook=0;++a->hooks;assert(pt_mixed_activation_stop(a->owner)==PT_MIXED_READERS_BACKEND);}
    free(a->base[i]);a->base[i]=NULL;
}
static struct activation_case *a_make(unsigned bits,unsigned cache_bits)
{
    struct activation_case *c=calloc(1,sizeof(*c));assert(c);c->trial=trial_make(bits,cache_bits);
    c->ram.ticks=100;c->ram.frequency=709379;c->ram.publish_result=c->ram.commit_result=c->ram.source_result=1;
    c->config.allocator=(struct pt_allocator){&c->memory,a_allocate,a_release};c->config.allocator_context=(struct pt_mixed_readers_span){&c->memory,sizeof(c->memory)};
    c->config.grid=(struct pt_mixed_readers_grid){100,17,709379,48000};c->config.session=19;
    c->config.control_budget=pt_mixed_activation_control_size();c->config.queue_budget=pt_mixed_readers_control_size();
    c->config.port=(struct pt_mixed_activation_port){&c->ram,sizeof(c->ram),1,15,a_clock,a_publish,a_commit,a_command_quiet,a_reader_quiet,a_source_close,a_source_quiet};
    c->workspace=calloc(1,pt_mixed_activation_workspace_size());assert(c->workspace);
    c->memory.config=&c->config;c->memory.workspace=c->workspace;c->memory.capacity=pt_mixed_activation_workspace_size();c->memory.output=&c->owner;
    return c;
}
static void a_open(struct activation_case *c)
{
    assert(pt_mixed_activation_open(&c->config,c->workspace,pt_mixed_activation_workspace_size(),&c->owner)==PT_MIXED_READERS_OK);
    assert(pt_mixed_activation_borrow_queue(c->owner,&c->trial->queue)==PT_MIXED_READERS_OK);
    assert(c->memory.live==2);c->ram.owner=c->memory.owner=c->owner;c->memory.queue=c->trial->queue;
}
static void a_destroy(struct activation_case *c)
{
    assert(!c->owner&&!c->memory.live);c->trial->queue=NULL;trial_drop(c->trial);free(c->workspace);free(c);
}
static uint64_t a_enqueue(struct activation_case *c,unsigned count,unsigned generation,uint64_t frame)
{
    uint64_t ticket;trigger_input(c->trial,count,generation,frame);
    assert(pt_mixed_activation_enqueue(c->owner,c->trial->input,&ticket)==PT_MIXED_READERS_OK);return ticket;
}
static void a_drain(struct activation_case *c,uint64_t ticket,unsigned count,unsigned order,unsigned failed)
{
    unsigned i;enum pt_mixed_readers_result expected=failed?PT_MIXED_READERS_BACKEND:PT_MIXED_READERS_OK;
    if(!order)assert(pt_mixed_activation_service_command(c->owner,ticket,1,NULL)==expected);
    for(i=0;i<count;++i)assert(pt_mixed_activation_service_reader(c->owner,ticket,i,1,NULL)==expected);
    if(order)assert(pt_mixed_activation_service_command(c->owner,ticket,1,NULL)==expected);
    assert(!pt_mixed_readers_commands_held(c->trial->queue)&&!pt_mixed_readers_readers_held(c->trial->queue));
}
static void a_success(unsigned bits,unsigned cache_bits,unsigned order)
{
    struct activation_case *c=a_make(bits,cache_bits);uint64_t ticket;size_t n;uint8_t *saved=save(c->trial->resources,&n);unsigned i;
    struct pt_mixed_readers_key key;struct pt_mixed_readers_reader_receipt receipt;
    a_open(c);ticket=a_enqueue(c,16,0,960);assert(!pt_mixed_activation_close(&c->owner)&&!c->ram.source_probes);
    assert(pt_mixed_activation_publish(c->owner,ticket)==PT_MIXED_READERS_OK&&c->ram.publications==1&&!c->ram.effects);
    c->ram.ticks=oracle(960)-1;assert(pt_mixed_activation_fire(c->owner,ticket)==PT_MIXED_ACTIVATION_EARLY&&!c->ram.commits);
    c->ram.ticks=oracle(960);assert(pt_mixed_activation_fire(c->owner,ticket)==PT_MIXED_ACTIVATION_COMMITTED&&c->ram.effects==16);
    for(i=0;i<16;++i){assert(pt_mixed_activation_service_reader(c->owner,ticket,i,0,&receipt)==PT_MIXED_READERS_PENDING);
        assert(receipt.adoption==PT_MIXED_ADOPTED&&receipt.state==PT_MIXED_READER_ACTIVE);
        assert(pt_mixed_activation_reader_key(c->owner,ticket,i,&key)==PT_MIXED_READERS_OK&&key.route==(i<4?1U:2U));}
    same_save(c->trial->resources,saved,n);a_drain(c,ticket,16,order,0);
    assert(pt_mixed_activation_close(&c->owner)&&!c->owner&&c->ram.closed&&c->ram.source_probes==1);
    assert(pt_mixed_activation_close(&c->owner));same_save(c->trial->resources,saved,n);free(saved);a_destroy(c);
}
static void a_constructor(unsigned mode)
{
    struct activation_case *c=a_make(8,8);struct pt_mixed_readers_activation *sentinel=(void *)(uintptr_t)1;
    void *workspace=c->workspace;size_t capacity=pt_mixed_activation_workspace_size();unsigned calls=0,releases=0;
    c->owner=sentinel;
    switch(mode){
    case 0:--capacity;break;case 1:workspace=(uint8_t *)workspace+1;break;
    case 2:c->config.port.flags=7;break;case 3:c->config.port.source_close=NULL;break;
    case 4:--c->config.control_budget;break;case 5:--c->config.queue_budget;break;
    case 6:c->config.allocator_context.bytes=0;break;case 7:c->config.port.context_bytes=0;break;
    case 8:c->config.session=0;break;case 9:c->config.grid.frequency=100;break;
    case 10:c->config.port.context=c->workspace;c->config.port.context_bytes=capacity;*(unsigned *)c->workspace=1;break;
    case 11:c->config.allocator_context=(struct pt_mixed_readers_span){c->workspace,capacity};break;
    case 12:c->memory.alias=&c->config;c->memory.alias_at=1;calls=1;break;
    case 13:c->memory.alias=c->workspace;c->memory.alias_at=1;calls=1;break;
    case 14:c->memory.alias=&c->owner;c->memory.alias_at=1;calls=1;break;
    case 15:c->memory.alias=&c->memory;c->memory.alias_at=1;calls=1;break;
    case 16:c->memory.alias=&c->ram;c->memory.alias_at=1;calls=1;break;
    case 17:c->memory.fail_at=1;calls=1;break;
    case 18:c->memory.fail_at=2;calls=2;releases=1;break;
    case 19:c->memory.alias=&c->config;c->memory.alias_at=2;calls=2;releases=1;break;
    case 20:c->memory.alias=&c->ram;c->memory.alias_at=2;calls=2;releases=1;break;
    case 21:c->memory.hook=1;calls=1;releases=1;break;
    case 22:c->memory.misaligned_at=1;calls=1;releases=1;break;
    }
    assert(pt_mixed_activation_open(&c->config,workspace,capacity,&c->owner)!=PT_MIXED_READERS_OK&&c->owner==sentinel);
    assert(c->memory.calls==calls&&c->memory.releases==releases&&!c->memory.live&&!c->ram.source_probes);
    if(mode==10)assert(*(unsigned *)c->workspace==1);
    c->owner=NULL;a_destroy(c);
}
static void a_failures(unsigned mode,unsigned order)
{
    struct activation_case *c=a_make(24,16);uint64_t ticket;unsigned failed=1;struct pt_mixed_readers_key before,key;
    a_open(c);ticket=a_enqueue(c,6,0,960);memset(&before,0xa5,sizeof(before));memcpy(&key,&before,sizeof(key));
    if(mode==0){c->ram.publish_result=0;assert(pt_mixed_activation_publish(c->owner,ticket)==PT_MIXED_READERS_PENDING);
        assert(!c->ram.effects&&!c->ram.registration.owner);c->ram.publish_result=1;failed=0;}
    if(mode==1)c->ram.publish_result=-1;
    if(mode==2)c->ram.hook=2;
    if(mode==10)c->ram.publish_late=1;
    assert(pt_mixed_activation_publish(c->owner,ticket)==(mode==1||mode==2||mode==10?PT_MIXED_READERS_BACKEND:PT_MIXED_READERS_OK));
    if((mode>2&&mode!=10)||mode==0){
        c->ram.ticks=oracle(960);
        if(mode==3)c->ram.ticks=oracle(961);
        if(mode==4)c->ram.commit_result=-1;
        if(mode==5)c->ram.late=1;
        if(mode==6)c->ram.malformed=1;
        if(mode==7)c->ram.frequency=715909;
        if(mode==8)c->ram.hook=3;
        if(mode==9){c->ram.slot[19].serial=123;c->ram.mask|=1U<<19;}
        if(mode==11)c->ram.ticks=99;
        assert(pt_mixed_activation_fire(c->owner,ticket)==(mode==0?PT_MIXED_ACTIVATION_COMMITTED:PT_MIXED_ACTIVATION_FAILED));
        if(mode==4)assert(c->ram.effects==1&&a_reader(&c->ram,c->ram.slot)!=NULL);
        if(mode==5||mode==6||mode==8)assert(c->ram.effects==6);
        if(mode==3||mode==7||mode==9||mode==11)assert(!c->ram.effects);
    }
    assert(pt_mixed_activation_reader_key(c->owner,ticket,0,&key)!=PT_MIXED_READERS_OK&&!memcmp(&key,&before,sizeof(key)));
    a_drain(c,ticket,6,order,failed);assert(pt_mixed_activation_close(&c->owner));a_destroy(c);
}
static void a_borrow_and_close(unsigned mode)
{
    struct activation_case *c=a_make(16,8);uint64_t ticket;
    a_open(c);
    if(mode==0){ticket=a_enqueue(c,5,0,960);assert(!pt_mixed_activation_close(&c->owner)&&c->memory.live==2&&!c->ram.source_probes);
        assert(pt_mixed_activation_stop(c->owner)==PT_MIXED_READERS_OK);(void)ticket;}
    if(mode==1){c->ram.source_result=0;assert(!pt_mixed_activation_close(&c->owner)&&c->memory.live==1&&c->ram.source_probes==1);
        assert(pt_mixed_activation_borrow_queue(c->owner,&c->trial->queue)==PT_MIXED_READERS_BACKEND);
        c->ram.closed=1;c->ram.owner=NULL;}
    if(mode==2){c->memory.hook=2;assert(!pt_mixed_activation_close(&c->owner)&&c->memory.live==1&&!c->ram.source_probes);
        assert(c->memory.hooks==1);}
    if(mode==3){c->memory.hook=3;assert(!pt_mixed_activation_close(&c->owner)&&!c->owner&&!c->memory.live&&c->memory.hooks==1);a_destroy(c);return;}
    assert(pt_mixed_activation_close(&c->owner));a_destroy(c);
}
static void a_uncertain_source(void)
{
    struct activation_case *c=a_make(8,16);a_open(c);c->ram.source_result=-1;
    assert(!pt_mixed_activation_close(&c->owner)&&c->owner&&c->memory.live==1&&c->ram.source_probes==1);
    assert(!pt_mixed_activation_close(&c->owner)&&c->ram.source_probes==1&&c->ram.quiet_probes==1&&c->owner);
    /* A separate actual MODEL transition disables future owner callback
     * identity; the later pure quiet probe observes that state, not a supplied
     * ready/quiet flag passed into production. Never repeat source_close. */
    c->ram.closed=1;c->ram.owner=NULL;
    c->ram.quiet_override=2;
    assert(!pt_mixed_activation_close(&c->owner)&&c->owner&&c->ram.quiet_probes==2&&c->memory.live==1);
    c->ram.quiet_override=0;
    assert(pt_mixed_activation_close(&c->owner)&&c->ram.source_probes==1&&c->ram.quiet_probes==3);a_destroy(c);
}
static void a_quiet_probe_fault(unsigned mode)
{
    struct activation_case *c=a_make(24,16);uint64_t ticket;unsigned i;size_t n;
    uint8_t *saved=save(c->trial->resources,&n);struct holder before;
    struct pt_mixed_readers_command_receipt command,command_before;
    struct pt_mixed_readers_reader_receipt reader,reader_before;
    a_open(c);ticket=a_enqueue(c,6,0,960);assert(pt_mixed_activation_publish(c->owner,ticket)==PT_MIXED_READERS_OK);
    c->ram.ticks=oracle(960);assert(pt_mixed_activation_fire(c->owner,ticket)==PT_MIXED_ACTIVATION_COMMITTED);
    memset(&command_before,0xa5,sizeof(command_before));memcpy(&command,&command_before,sizeof(command));
    memset(&reader_before,0xa5,sizeof(reader_before));memcpy(&reader,&reader_before,sizeof(reader));
    memcpy(&before,c->trial->reader,sizeof(before));
    if(mode<2){if(mode)c->ram.identity_mutation=4;else c->ram.hook=4;
        assert(pt_mixed_activation_service_command(c->owner,ticket,1,&command)==PT_MIXED_READERS_BACKEND);
        assert(!memcmp(&command,&command_before,sizeof(command))&&pt_mixed_readers_commands_held(c->trial->queue)==1);
    }else{if(mode==3)c->ram.identity_mutation=5;else c->ram.hook=5;
        assert(pt_mixed_activation_service_reader(c->owner,ticket,0,1,&reader)==PT_MIXED_READERS_BACKEND);
        assert(!memcmp(&reader,&reader_before,sizeof(reader))&&pt_mixed_readers_readers_held(c->trial->queue)==6);
    }
    assert(!memcmp(&before,c->trial->reader,sizeof(before))&&pt_mixed_readers_commands_held(c->trial->queue)==1&&
           pt_mixed_readers_readers_held(c->trial->queue)==6&&!pt_mixed_activation_close(&c->owner)&&c->memory.live==2);
    same_save(c->trial->resources,saved,n);
    /* The faulting probe returned actual1 and dropped MODEL references, but
     * production retained its records and the core's genuine leases. Only
     * separate explicit unchanged exact probes now supply drainage proofs. */
    assert(pt_mixed_activation_service_command(c->owner,ticket,1,NULL)==PT_MIXED_READERS_BACKEND);
    for(i=0;i<6;++i)assert(pt_mixed_activation_service_reader(c->owner,ticket,i,1,NULL)==PT_MIXED_READERS_BACKEND);
    assert(!pt_mixed_readers_commands_held(c->trial->queue)&&!pt_mixed_readers_readers_held(c->trial->queue));
    assert(pt_mixed_activation_close(&c->owner));same_save(c->trial->resources,saved,n);free(saved);a_destroy(c);
}
static void a_source_probe_fault(unsigned mode)
{
    struct activation_case *c=a_make(8,8);a_open(c);
    if(mode==0)c->ram.hook=6;
    else if(mode==1)c->ram.identity_mutation=6;
    else{c->ram.source_result=0;assert(!pt_mixed_activation_close(&c->owner)&&c->memory.live==1);
        c->ram.closed=1;c->ram.owner=NULL;c->ram.hook=7;}
    assert(!pt_mixed_activation_close(&c->owner)&&c->owner&&c->memory.live==1&&c->ram.source_probes==1);
    assert(pt_mixed_activation_close(&c->owner)&&c->ram.source_probes==1&&c->ram.quiet_probes==(mode==2?2U:1U));
    a_destroy(c);
}
static void a_faulting_refusal(unsigned order)
{
    struct activation_case *c=a_make(24,16);uint64_t ticket;unsigned i;size_t n;
    uint8_t *saved=save(c->trial->resources,&n);struct holder before;
    struct pt_mixed_readers_key key,key_before;
    a_open(c);ticket=a_enqueue(c,6,0,960);memcpy(&before,c->trial->reader,sizeof(before));
    c->ram.publish_result=0;c->ram.hook=2;
    assert(pt_mixed_activation_publish(c->owner,ticket)==PT_MIXED_READERS_BACKEND);
    /* Publish legitimately revalidates every genuine current holder. That
     * call counter changes; all resource/lease/terminal/release bytes do not. */
    assert(c->trial->reader[0].currents>before.currents);before.currents=c->trial->reader[0].currents;
    assert(c->ram.publications==1&&c->ram.hooks==1&&!c->ram.commits&&!c->ram.effects);
    for(i=0;i<2;++i)assert(!c->ram.command[i].live);
    for(i=0;i<32;++i)assert(!c->ram.reader[i].live);
    assert(pt_mixed_readers_commands_held(c->trial->queue)==1&&pt_mixed_readers_readers_held(c->trial->queue)==6&&
           !memcmp(&before,c->trial->reader,sizeof(before))&&!pt_mixed_activation_close(&c->owner)&&c->memory.live==2);
    memset(&key_before,0xa5,sizeof(key_before));memcpy(&key,&key_before,sizeof(key));
    assert(pt_mixed_activation_reader_key(c->owner,ticket,0,&key)!=PT_MIXED_READERS_OK&&!memcmp(&key,&key_before,sizeof(key)));
    c->ram.command_pending=c->ram.reader_pending=1;
    assert(pt_mixed_activation_service_command(c->owner,ticket,1,NULL)==PT_MIXED_READERS_BACKEND);
    assert(pt_mixed_activation_service_reader(c->owner,ticket,0,1,NULL)==PT_MIXED_READERS_BACKEND);
    assert(pt_mixed_readers_commands_held(c->trial->queue)==1&&pt_mixed_readers_readers_held(c->trial->queue)==6&&
           !memcmp(&before,c->trial->reader,sizeof(before)));
    same_save(c->trial->resources,saved,n);
    /* Actual raw0 had no MODEL packet/reader refs. Its reentry nevertheless
     * invalidated absence at that call. Later exact quiet callbacks observe
     * genuine model absence independently, in either domain proof order. */
    c->ram.command_pending=c->ram.reader_pending=0;a_drain(c,ticket,6,order,1);
    assert(pt_mixed_activation_close(&c->owner));same_save(c->trial->resources,saved,n);free(saved);a_destroy(c);
}
static void a_poison_and_alias(void)
{
    struct activation_case *c=a_make(24,16);uint64_t ticket;struct pt_mixed_readers_key key;
    struct pt_mixed_readers_output *before=c->trial->queue;unsigned currents;
    a_open(c);before=c->trial->queue;
    assert(pt_mixed_activation_borrow_queue(c->owner,(struct pt_mixed_readers_output **)&c->ram.mask)==PT_MIXED_READERS_INVALID);
    assert(c->trial->queue==before);trigger_input(c->trial,6,0,960);currents=c->trial->reader[0].currents;
    assert(pt_mixed_activation_enqueue(c->owner,c->trial->input,(uint64_t *)&c->ram.mask)==PT_MIXED_READERS_INVALID);
    assert(c->trial->reader[0].currents==currents);
    assert(pt_mixed_activation_enqueue(c->owner,c->trial->input,&ticket)==PT_MIXED_READERS_OK);
    assert(pt_mixed_activation_publish(c->owner,ticket)==PT_MIXED_READERS_OK);
    free(c->trial->input);c->trial->input=NULL;free(c->workspace);c->workspace=NULL;memset(&c->config,0xa5,sizeof(c->config));
    A_POISON(c->trial->queue,pt_mixed_readers_control_size());A_POISON(c->trial->command,sizeof(c->trial->command));
    A_POISON(c->trial->reader,sizeof(c->trial->reader));A_POISON(&c->trial->resources->sampler,sizeof(c->trial->resources->sampler));
    A_POISON(&c->trial->resources->document.project,sizeof(c->trial->resources->document.project));
    A_POISON(&c->trial->resources->paula,sizeof(c->trial->resources->paula));A_POISON(&c->trial->resources->card.cache,sizeof(c->trial->resources->card.cache));
    c->ram.ticks=oracle(960);assert(pt_mixed_activation_fire(c->owner,ticket)==PT_MIXED_ACTIVATION_COMMITTED&&c->ram.effects==6);
    A_UNPOISON(c->trial->queue,pt_mixed_readers_control_size());A_UNPOISON(c->trial->command,sizeof(c->trial->command));
    A_UNPOISON(c->trial->reader,sizeof(c->trial->reader));A_UNPOISON(&c->trial->resources->sampler,sizeof(c->trial->resources->sampler));
    A_UNPOISON(&c->trial->resources->document.project,sizeof(c->trial->resources->document.project));
    A_UNPOISON(&c->trial->resources->paula,sizeof(c->trial->resources->paula));A_UNPOISON(&c->trial->resources->card.cache,sizeof(c->trial->resources->card.cache));
    assert(pt_mixed_activation_service_command(c->owner,ticket,0,NULL)==PT_MIXED_READERS_OK);
    assert(pt_mixed_activation_reader_key(c->owner,ticket,0,&key)==PT_MIXED_READERS_OK);
    assert(pt_mixed_activation_reader_key(c->owner,ticket,0,(struct pt_mixed_readers_key *)&c->ram)==PT_MIXED_READERS_INVALID);
    {unsigned i;for(i=0;i<6;++i)assert(pt_mixed_activation_service_reader(c->owner,ticket,i,1,NULL)==PT_MIXED_READERS_OK);}
    assert(pt_mixed_activation_close(&c->owner));a_destroy(c);
}
static struct activation_case *a_guard_case;
static int a_guard_current(void *context,uint64_t token,uint64_t generation)
{
    struct activation_case *c=a_guard_case;struct holder *h=context;
    if(c){struct holder before;void *alias=c->trial->reader[0].pcm.data;
        memcpy(&before,c->trial->reader,sizeof(before));
        a_guard_case=NULL;
        assert(!pt_mixed_readers_output_disjoint(c->trial->queue,alias,sizeof(struct pt_mixed_readers_key)));
        assert(!pt_mixed_readers_admission_valid(c->trial->queue,c->trial->input,alias));
        assert(pt_mixed_activation_borrow_queue(c->owner,alias)==PT_MIXED_READERS_INVALID);
        assert(pt_mixed_activation_reader_key(c->owner,1,0,alias)==PT_MIXED_READERS_INVALID);
        assert(pt_mixed_activation_service_command(c->owner,1,0,alias)==PT_MIXED_READERS_INVALID);
        assert(pt_mixed_activation_service_reader(c->owner,1,0,0,alias)==PT_MIXED_READERS_INVALID);
        assert(pt_mixed_activation_enqueue(c->owner,c->trial->input,alias)==PT_MIXED_READERS_INVALID);
        assert(!memcmp(&before,c->trial->reader,sizeof(before)));
    }
    return held_current(h,token,generation);
}
static void a_pure_guard_queries(void)
{
    struct activation_case *c=a_make(24,16);uint64_t ticket=999,second;size_t bytes=pt_mixed_readers_control_size();
    uint8_t *snapshot=malloc(bytes);struct pt_mixed_readers_inputs input;struct holder holder_before;
    assert(snapshot);a_open(c);trigger_input(c->trial,6,0,960);memcpy(&input,c->trial->input,sizeof(input));memcpy(&holder_before,c->trial->reader,sizeof(holder_before));
    memcpy(snapshot,c->trial->queue,bytes);
    assert(pt_mixed_readers_admission_valid(c->trial->queue,c->trial->input,&ticket)&&ticket==999);
    assert(!pt_mixed_readers_admission_valid(c->trial->queue,c->trial->input,(uint64_t *)c->trial->reader[0].pcm.data));
    assert(!memcmp(snapshot,c->trial->queue,bytes)&&!memcmp(&input,c->trial->input,sizeof(input))&&
           !memcmp(&holder_before,c->trial->reader,sizeof(holder_before)));
    /* Genuine RAW factory-style admission: the owner ledger has no command or
     * reader record until publish, but core queries protect every held byte. */
    assert(pt_mixed_readers_enqueue(c->trial->queue,c->trial->input,&ticket)==PT_MIXED_READERS_OK);
    assert(!pt_mixed_activation_close(&c->owner)&&!c->ram.source_probes);
    trigger_input(c->trial,6,1,1920);c->trial->input->command.current=a_guard_current;a_guard_case=c;
    assert(pt_mixed_activation_enqueue(c->owner,c->trial->input,&second)==PT_MIXED_READERS_OK&&second==2&&!a_guard_case);
    assert(!c->ram.publications&&!c->ram.commits&&!c->ram.clock_reads);
    assert(pt_mixed_activation_stop(c->owner)==PT_MIXED_READERS_OK);
    assert(pt_mixed_activation_close(&c->owner));free(snapshot);a_destroy(c);
}
static void a_controls_and_replacements(unsigned order)
{
    struct activation_case *c=a_make(16,16);uint64_t first,second,third;unsigned i;
    struct pt_mixed_readers_key keys[6],actual;
    a_open(c);first=a_enqueue(c,6,0,960);assert(pt_mixed_activation_publish(c->owner,first)==PT_MIXED_READERS_OK);
    c->ram.ticks=oracle(960);assert(pt_mixed_activation_fire(c->owner,first)==PT_MIXED_ACTIVATION_COMMITTED);
    assert(pt_mixed_activation_service_command(c->owner,first,0,NULL)==PT_MIXED_READERS_OK);
    for(i=0;i<6;++i)assert(pt_mixed_activation_reader_key(c->owner,first,i,keys+i)==PT_MIXED_READERS_OK);
    memset(c->trial->input,0,sizeof(*c->trial->input));holder_init(c->trial->command+1,101);
    c->trial->input->command=control_of(c->trial->command+1);c->trial->input->batch=(struct pt_mixed_readers_batch){17,1920,6,{{0}}};
    for(i=0;i<6;++i){struct pt_mixed_readers_action *a=c->trial->input->batch.action+i;
        a->route=keys[i].route;a->slot=keys[i].slot;a->kind=(i==0||i==5)?PT_MIXED_READERS_STOP:PT_MIXED_READERS_CONTROL;
        c->trial->input->target[i]=keys[i];
        if(a->kind==PT_MIXED_READERS_CONTROL){if(a->route==1){a->geometry.paula.period=400;a->geometry.paula.volume=32;}
            else{a->geometry.amigus.rate=0x04000000;a->geometry.amigus.left=123;a->geometry.amigus.right=456;}}
    }
    assert(pt_mixed_activation_enqueue(c->owner,c->trial->input,&second)==PT_MIXED_READERS_OK);
    assert(pt_mixed_activation_publish(c->owner,second)==PT_MIXED_READERS_OK);
    c->ram.ticks=oracle(1920);assert(pt_mixed_activation_fire(c->owner,second)==PT_MIXED_ACTIVATION_COMMITTED&&c->ram.effects==12);
    assert(pt_mixed_activation_service_command(c->owner,second,0,NULL)==PT_MIXED_READERS_OK);
    assert(a_reader(&c->ram,keys+1)->action.geometry.paula.period==400&&a_reader(&c->ram,keys+4)->action.geometry.amigus.left==123);
    third=a_enqueue(c,6,2,2880);assert(pt_mixed_activation_publish(c->owner,third)==PT_MIXED_READERS_OK);
    c->ram.ticks=oracle(2880);assert(pt_mixed_activation_fire(c->owner,third)==PT_MIXED_ACTIVATION_COMMITTED&&c->ram.effects==18);
    assert(pt_mixed_activation_service_command(c->owner,third,0,NULL)==PT_MIXED_READERS_OK);
    for(i=0;i<6;++i){assert(pt_mixed_activation_reader_key(c->owner,first,i,&actual)==PT_MIXED_READERS_STALE);
        assert(pt_mixed_activation_reader_key(c->owner,third,i,&actual)==PT_MIXED_READERS_OK);
        assert(pt_mixed_activation_service_reader(c->owner,first,i,1,NULL)==PT_MIXED_READERS_OK);
        assert(pt_mixed_activation_reader_key(c->owner,third,i,&actual)==PT_MIXED_READERS_OK);}
    (void)order;
    for(i=0;i<6;++i)assert(pt_mixed_activation_service_reader(c->owner,third,i,1,NULL)==PT_MIXED_READERS_OK);
    assert(pt_mixed_activation_close(&c->owner));a_destroy(c);
}
/* Observe our genuine allocator's still-live ordinary owner block, without
 * a private struct mirror, guessed offset, added product API or mutation.
 * A retained full backup would create a second byte-identical packet. The
 * SOURCE packet-only memset establishes that the entire scratch packet is0. */
static unsigned a_owner_packet_copies(const struct activation_case *c,const struct pt_mixed_activation_packet *p)
{
    const uint8_t *bytes=(const uint8_t *)(const void *)c->owner;
    size_t i,capacity=pt_mixed_activation_control_size();unsigned copies=0;
    assert(c->owner&&c->memory.live==2&&capacity>=sizeof(*p));
    for(i=0;i<=capacity-sizeof(*p);++i)copies+=!memcmp(bytes+i,p,sizeof(*p));
    return copies;
}
static void a_packet_mutation_case(unsigned phase,unsigned mode,int raw,unsigned reentry,unsigned order)
{
    struct activation_case *c=a_make(24,16);uint64_t ticket;size_t n;
    uint8_t *saved=save(c->trial->resources,&n);
    struct pt_mixed_readers_key sentinel,key;
    a_open(c);ticket=a_enqueue(c,6,0,960);
    c->ram.packet_observation=1;c->ram.packet_phase=phase;c->ram.packet_mutation=mode;c->ram.packet_reentry=reentry;
    if(phase==1){c->ram.publish_result=raw;
        assert(pt_mixed_activation_publish(c->owner,ticket)==PT_MIXED_READERS_BACKEND);
        assert(!c->ram.commits&&!c->ram.effects&&c->ram.publications==1);
    }else{
        assert(pt_mixed_activation_publish(c->owner,ticket)==PT_MIXED_READERS_OK);
        c->ram.commit_result=raw;c->ram.ticks=oracle(960);
        assert(pt_mixed_activation_fire(c->owner,ticket)==PT_MIXED_ACTIVATION_FAILED);
        assert(c->ram.commits==1&&c->ram.effects==(raw==0?0U:raw<0?1U:6U));
    }
    assert(c->ram.packet_mutations==1&&c->ram.hooks==reentry);
    assert(!memcmp(c->ram.observed_packet,&c->ram.original_packet,sizeof(c->ram.original_packet)));
    assert(a_owner_packet_copies(c,&c->ram.original_packet)==1);
    assert(pt_mixed_readers_commands_held(c->trial->queue)==1&&pt_mixed_readers_readers_held(c->trial->queue)==6);
    assert(!pt_mixed_activation_close(&c->owner)&&c->memory.live==2&&!c->ram.source_probes);
    memset(&sentinel,0xa5,sizeof(sentinel));memcpy(&key,&sentinel,sizeof(key));
    assert(pt_mixed_activation_reader_key(c->owner,ticket,0,&key)!=PT_MIXED_READERS_OK&&!memcmp(&key,&sentinel,sizeof(key)));
    c->ram.command_pending=c->ram.reader_pending=1;
    assert(pt_mixed_activation_service_command(c->owner,ticket,1,NULL)==PT_MIXED_READERS_BACKEND);
    assert(pt_mixed_activation_service_reader(c->owner,ticket,0,1,NULL)==PT_MIXED_READERS_BACKEND);
    assert(pt_mixed_readers_commands_held(c->trial->queue)==1&&pt_mixed_readers_readers_held(c->trial->queue)==6);
    assert(!memcmp(c->ram.observed_packet,&c->ram.original_packet,sizeof(c->ram.original_packet)));
    assert(a_owner_packet_copies(c,&c->ram.original_packet)==1);same_save(c->trial->resources,saved,n);
    /* Mutated raw0 is not absence. Only subsequent independent unchanged exact
     * command AND reader proofs drain either real or possible MODEL references. */
    a_packet_observation_expire(&c->ram);assert(a_packet_observation_empty(&c->ram));
    c->ram.command_pending=c->ram.reader_pending=0;a_drain(c,ticket,6,order,1);
    assert(pt_mixed_activation_close(&c->owner));
    same_save(c->trial->resources,saved,n);free(saved);a_destroy(c);
}
static void a_packet_consecutive_reuse(void)
{
    struct activation_case *c=a_make(16,16);uint64_t first,second;unsigned i;size_t n;
    uint8_t *saved=save(c->trial->resources,&n);
    a_open(c);first=a_enqueue(c,6,0,960);c->ram.publish_result=0;c->ram.packet_observation=1;
    assert(pt_mixed_activation_publish(c->owner,first)==PT_MIXED_READERS_PENDING);
    assert(a_owner_packet_copies(c,&c->ram.original_packet)==0&&!c->ram.effects);
    c->ram.publish_result=1;
    assert(pt_mixed_activation_publish(c->owner,first)==PT_MIXED_READERS_OK);
    assert(a_owner_packet_copies(c,&c->ram.original_packet)==1);
    c->ram.ticks=oracle(960)-1;
    assert(pt_mixed_activation_fire(c->owner,first)==PT_MIXED_ACTIVATION_EARLY);
    assert(a_owner_packet_copies(c,&c->ram.original_packet)==1&&!c->ram.effects);
    c->ram.ticks=oracle(960);
    assert(pt_mixed_activation_fire(c->owner,first)==PT_MIXED_ACTIVATION_COMMITTED);
    assert(a_owner_packet_copies(c,&c->ram.original_packet)==0&&c->ram.effects==6);
    a_packet_observation_expire(&c->ram);assert(a_packet_observation_empty(&c->ram));
    assert(pt_mixed_activation_service_command(c->owner,first,0,NULL)==PT_MIXED_READERS_OK);
    second=a_enqueue(c,6,1,1920);c->ram.packet_observation=1;
    assert(pt_mixed_activation_publish(c->owner,second)==PT_MIXED_READERS_OK);
    assert(a_owner_packet_copies(c,&c->ram.original_packet)==1);
    c->ram.ticks=oracle(1920);
    assert(pt_mixed_activation_fire(c->owner,second)==PT_MIXED_ACTIVATION_COMMITTED);
    assert(a_owner_packet_copies(c,&c->ram.original_packet)==0&&c->ram.effects==12);
    a_packet_observation_expire(&c->ram);assert(a_packet_observation_empty(&c->ram));
    assert(pt_mixed_activation_service_command(c->owner,second,0,NULL)==PT_MIXED_READERS_OK);
    for(i=0;i<6;++i)assert(pt_mixed_activation_service_reader(c->owner,first,i,1,NULL)==PT_MIXED_READERS_OK);
    for(i=0;i<6;++i)assert(pt_mixed_activation_service_reader(c->owner,second,i,1,NULL)==PT_MIXED_READERS_OK);
    assert(pt_mixed_activation_close(&c->owner));
    same_save(c->trial->resources,saved,n);free(saved);a_destroy(c);
}
static void __attribute__((constructor)) mixed_activation_fixture(void)
{
    unsigned bits,cache_bits,order,mode,phase,reentry;int raw;
    for(phase=1;phase<=2;++phase)for(mode=0;mode<8;++mode)
        for(raw=-1;raw<=1;++raw)for(reentry=0;reentry<2;++reentry)for(order=0;order<2;++order)
            a_packet_mutation_case(phase,mode,raw,reentry,order);
    a_packet_consecutive_reuse();
    for(mode=0;mode<23;++mode)a_constructor(mode);
    for(bits=8;bits<=24;bits+=8)for(cache_bits=8;cache_bits<=16;cache_bits+=8)for(order=0;order<2;++order)a_success(bits,cache_bits,order);
    for(mode=0;mode<12;++mode)for(order=0;order<2;++order)a_failures(mode,order);
    for(mode=0;mode<4;++mode)a_borrow_and_close(mode);
    for(mode=0;mode<4;++mode)a_quiet_probe_fault(mode);
    for(mode=0;mode<3;++mode)a_source_probe_fault(mode);
    for(order=0;order<2;++order)a_faulting_refusal(order);
    a_poison_and_alias();a_pure_guard_queries();a_controls_and_replacements(0);a_uncertain_source();
    puts("MIXED ACTIVATION PASS:12 genuine8/16/24 paired16-action activations;23 guarded constructors;24 exact refusal/clock/partial/late/key/reentry proof orders;2 raw publish0+NEW reentry faults retain pins until independent absence proofs;4 new quiet identity/reentry faults retain exact pins/outputs then independent drains;3 source close/quiet faults retain;raw/unpublished pure guards;paired CONTROL/STOP/replacement;owned nonempty queue registration and consumed-close0;source pending/unknown/malformed retained then independent quiet;poisoned descriptors/copy-only activation;SOFTWARE_ONLY");
}
