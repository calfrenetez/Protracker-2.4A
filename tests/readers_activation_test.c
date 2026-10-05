/* Public-only genuine owners, with an explicitly injected SOFTWARE model port.
 * Production implementations remain separate TUs. No private owner/core casts,
 * poisoned live immutable descriptors, native ports or activation certificates.
 * The included fixture supplies the genuine sampler/master/workspace builder;
 * its old entry is deliberately not invoked. Substantial storage is heap-owned.
 */
#define PT_COMPACT_NO_MAIN
#include "readers_backend_registration_test.c"
#include "../src/core/readers_activation.h"

struct activation_model_command {
    struct pt_readers_activation_packet packet;
    struct pt_readers_activation *ledger;
    unsigned live,fired,detach,index;
};
struct activation_model_reader {struct pt_readers_key key;const uint8_t *data;unsigned live,retire;};
struct activation_model {
    struct activation_model_command command[2];
    struct activation_model_reader reader[8];
    struct pt_readers_key slot[4];
    unsigned mask,publishes,commits,effects,clock_calls,command_probes,reader_probes;
    unsigned refuse,uncertain,forge_untouched,post_late,partial,callbacks_ended;
    uint64_t now,session,generation;uint32_t frequency;
};
static struct activation_model_command *model_command(struct activation_model *m,uint64_t ticket)
{unsigned i;for(i=0;i<2;++i)if(m->command[i].live&&m->command[i].packet.ticket==ticket)return m->command+i;return NULL;}
static struct activation_model_reader *model_reader(struct activation_model *m,const struct pt_readers_key *k)
{unsigned i;for(i=0;i<8;++i)if(m->reader[i].live&&compact_key_equal(&m->reader[i].key,k))return m->reader+i;return NULL;}
static int model_expected(const struct activation_model *m,const struct pt_readers_activation_packet *p)
{unsigned i;if(p->expected_mask!=m->mask)return 0;for(i=0;i<4;++i)if(!compact_key_equal(p->expected+i,m->slot+i))return 0;return 1;}
static int model_clock(void *c,uint64_t *ticks,uint32_t *frequency)
{struct activation_model *m=c;assert(!m->callbacks_ended);++m->clock_calls;*ticks=m->now;*frequency=m->frequency;return 1;}
static int model_publish(void *c,struct pt_readers_activation *b,const struct pt_readers_activation_packet *p)
{
    struct activation_model *m=c;unsigned i;
    assert(!m->callbacks_ended);++m->publishes;
    if(m->refuse){--m->refuse;return 0;}
    if(p->session!=m->session||p->generation!=m->generation||m->frequency!=COMPACT_FREQUENCY||m->now>=p->first||!model_expected(m,p))return 0;
    for(i=0;i<2;++i)if(!m->command[i].live)break;
    assert(i<2);m->command[i].packet=*p;m->command[i].ledger=b;m->command[i].live=1;
    /* Value-only copy: the packet ADDRESS, registered event/domain/holder/span
     * objects and callback declarations are never retained by this port. */
    assert(!m->effects||p->frame>COMPACT_START);
    return m->uncertain?-1:1;
}
static int model_commit(void *c,const struct pt_readers_activation_packet *p,struct pt_readers_activation_actual *out)
{
    struct activation_model *m=c;struct activation_model_command *command=model_command(m,p->ticket);unsigned i,need=0,available=0;
    assert(!m->callbacks_ended);++m->commits;
    if(!command||command->fired||p->session!=m->session||p->generation!=m->generation||!model_expected(m,p)||m->frequency!=COMPACT_FREQUENCY||m->now<p->first||m->now>=p->last)return 0;
    for(i=0;i<8;++i)available+=!m->reader[i].live;
    for(i=0;i<p->count;++i)need+=p->action[i].kind==PT_SCHEDULED_TRIGGER;
    if(need>available)return 0;
    /* Validate EVERY addressed original and resource before the first effect. */
    for(i=0;i<p->count;++i){const struct pt_scheduled_action *a=p->action+i;
        if(a->kind==PT_SCHEDULED_TRIGGER){unsigned j;
            assert(a->data&&a->words&&a->period);for(j=0;j<8;++j)if(!m->reader[j].live)break;
            if(j==8)return 0;
        }else if(!(m->mask&(1U<<a->slot))||!compact_key_equal(m->slot+a->slot,p->key+i)||!model_reader(m,p->key+i))return 0;
    }
    for(i=0;i<p->count;++i){const struct pt_scheduled_action *a=p->action+i;unsigned slot=a->slot;
        if(a->kind==PT_SCHEDULED_TRIGGER){unsigned j;for(j=0;j<8;++j)if(!m->reader[j].live)break;assert(j<8);
            m->reader[j]=(struct activation_model_reader){p->key[i],a->data,1,0};m->slot[slot]=p->key[i];m->mask|=1U<<slot;
        }else if(a->kind==PT_SCHEDULED_STOP){m->mask&=~(1U<<slot);memset(m->slot+slot,0,sizeof(m->slot[slot]));}
        ++m->effects;if(m->partial)break;
    }
    command->fired=1;
    for(i=0;i<command->packet.count;++i){command->packet.action[i].data=NULL;command->packet.action[i].words=0;}
    out->active_mask=m->mask;out->adopted_mask=m->mask;memcpy(out->slot,m->slot,sizeof(m->slot));
    if(m->forge_untouched){unsigned touched=0;for(i=0;i<p->count;++i)touched|=1U<<p->action[i].slot;
        for(i=0;i<4;++i)if(!(touched&(1U<<i)))break;assert(i<4);out->slot[i].serial^=1;}
    if(m->post_late)m->now=p->last;
    return m->partial?-1:1;
}
static int model_command_quiet(void *c,uint64_t ticket,unsigned cancel)
{
    struct activation_model *m=c;struct activation_model_command *command=model_command(m,ticket);
    assert(!m->callbacks_ended);++m->command_probes;
    if(!command)return 1;
    if(!cancel&&!command->detach)return 0;
    /* Independent command proof removes callback identity and every copied
     * activation packet address. Persistent adopted readers remain separate. */
    memset(command,0,sizeof(*command));return 1;
}
static int model_reader_quiet(void *c,const struct pt_readers_key *key,unsigned cancel)
{
    struct activation_model *m=c;struct activation_model_reader *r=model_reader(m,key);unsigned i,j;
    assert(!m->callbacks_ended);++m->reader_probes;
    if(!cancel&&(!r||!r->retire))return 0;
    for(i=0;i<2;++i)if(m->command[i].live&&!m->command[i].fired)
        for(j=0;j<m->command[i].packet.count;++j)if(compact_key_equal(m->command[i].packet.key+j,key)){
            if(!cancel)return 0;
            /* Exact cancellation quiesces the WHOLE batch, never a successor. */
            m->command[i].fired=1;
            {unsigned k;for(k=0;k<m->command[i].packet.count;++k){m->command[i].packet.action[k].data=NULL;m->command[i].packet.action[k].words=0;}}
        }
    if(r){unsigned slot=r->key.slot;if((m->mask&(1U<<slot))&&compact_key_equal(m->slot+slot,key)){
        m->mask&=~(1U<<slot);memset(m->slot+slot,0,sizeof(m->slot[slot]));}memset(r,0,sizeof(*r));}
    return 1;
}
static struct pt_readers_activation_port model_port(struct activation_model *m)
{return (struct pt_readers_activation_port){m,sizeof(*m),PT_READERS_ACTIVATION_PORT_VERSION,PT_READERS_ACTIVATION_PORT_REQUIRED,
    model_clock,model_publish,model_commit,model_command_quiet,model_reader_quiet};}
static void model_end(struct activation_model *m)
{unsigned i;for(i=0;i<2;++i)assert(!m->command[i].live);for(i=0;i<8;++i)assert(!m->reader[i].live);assert(!m->mask);m->callbacks_ended=1;}
static uint64_t oracle_first(uint64_t frame)
{uint64_t n=frame*COMPACT_FREQUENCY;assert(frame<UINT64_MAX/COMPACT_FREQUENCY);return COMPACT_EPOCH+n/COMPACT_RATE+(n%COMPACT_RATE!=0);}
/* Task-only failure detail; successful runtime markers remain unchanged.
 * This preserves the same PENDING assertion and fixed local/global bounds. */
static void activation_until_publish(struct compact_state *s,unsigned ordinal,const char *stage)
{
    unsigned n=0;
    while(s->status.phase!=PT_PAULA_READERS_SONG_PUBLISH){
        enum pt_paula_readers_song_result result;assert(++n<=COMPACT_STEPS);result=compact_step(s);
        if(result!=PT_PAULA_READERS_SONG_PENDING)
            fprintf(stderr,"ACTIVATION UNEXPECTED mono%u ordinal=%u stage=%s result=%u phase=%u boundary=%lu:%lu terminal=%lu:%lu commands=%u readers=%u\n",
                s->bits,ordinal,stage,(unsigned)result,(unsigned)s->status.phase,
                (unsigned long)(s->status.boundary_frame>>32),(unsigned long)(s->status.boundary_frame&UINT32_MAX),
                (unsigned long)(s->status.terminal_frame>>32),(unsigned long)(s->status.terminal_frame&UINT32_MAX),
                s->status.command_mask,s->status.reader_mask);
        assert(result==PT_PAULA_READERS_SONG_PENDING);
    }
}
static struct activation_model_command *song_publish(struct compact_state *s,struct activation_model *m,unsigned ordinal,unsigned stop)
{
    struct activation_model_command *c=NULL;unsigned i,old=s->status.published_mask,added;
    assert(pt_paula_readers_song_publish_next(s->song,1)==PT_SCHEDULED_OK);
    assert(pt_paula_readers_song_get(s->song,1,&s->status)==PT_PAULA_READERS_SONG_PENDING||s->status.done);
    added=s->status.published_mask&~old;assert(added==1||added==2);
    for(i=0;i<2;++i)if(m->command[i].live&&!m->command[i].fired)c=m->command+i;
    assert(c);c->index=added==1?0:1;
    assert(c->packet.frame==COMPACT_START+(uint64_t)ordinal*COMPACT_TICK_FRAMES&&c->packet.generation==7);
    assert(c->packet.first==oracle_first(c->packet.frame)&&c->packet.last==oracle_first(c->packet.frame+1));
    assert(c->packet.count==1&&c->packet.action[0].slot==0);
    if(!ordinal){const int8_t *data=(const int8_t *)c->packet.action[0].data;
        assert(c->packet.action[0].kind==PT_SCHEDULED_TRIGGER&&c->packet.action[0].words==1024&&c->packet.action[0].period==855&&c->packet.action[0].volume==64);
        assert(data[0]==127&&data[1]==-128&&data[2]==18&&data[3]==-18);
        assert(c->packet.first==208434&&c->packet.last==208455);
    }else if(stop)assert(c->packet.action[0].kind==PT_SCHEDULED_STOP&&!c->packet.action[0].data&&!c->packet.action[0].words&&!c->packet.action[0].period&&!c->packet.action[0].volume);
    else assert(c->packet.action[0].kind==PT_SCHEDULED_CONTROL&&!c->packet.action[0].data&&!c->packet.action[0].words&&c->packet.action[0].period==855&&c->packet.action[0].volume==16+ordinal);
    return c;
}
static void song_fire(struct compact_state *s,struct activation_model *m,struct activation_model_command *c)
{
    unsigned ordinary=s->allocation->ordinary_calls,chip=s->allocation->chip_calls,live=s->allocation->ordinary_live,clive=s->allocation->chip_live;
    unsigned effects=m->effects;uint64_t ticket=c->packet.ticket;
    m->now=c->packet.first-1;assert(pt_readers_activation_fire(c->ledger,ticket)==PT_READERS_ACTIVATION_EARLY&&m->effects==effects);
    m->now=c->packet.first;assert(pt_readers_activation_fire(c->ledger,ticket)==PT_READERS_ACTIVATION_COMMITTED);
    assert(m->effects==effects+1&&s->allocation->ordinary_calls==ordinary&&s->allocation->chip_calls==chip&&s->allocation->ordinary_live==live&&s->allocation->chip_live==clive);
    assert(pt_paula_readers_song_service_command(s->song,c->index,0,NULL)==PT_SCHEDULED_PENDING);
    assert(pt_paula_readers_song_service_reader(s->song,0,0,NULL)==PT_SCHEDULED_PENDING);compact_source_unchanged(s);
}
static void song_detach(struct compact_state *s,struct activation_model_command *c)
{unsigned index=c->index;c->detach=1;assert(pt_paula_readers_song_service_command(s->song,index,0,NULL)==PT_SCHEDULED_OK&&!c->live);}
static void activation_song(const struct pt_compact_song_memory *memory,unsigned bits)
{
    struct compact_state *s=compact_source(memory,bits);struct activation_model *m=compact_allocate(s->allocation,sizeof(*m));
    struct pt_readers_activation *ledger=NULL;struct pt_readers_activation_port port;struct activation_model_command *first,*second,*c;
    const struct pt_readers_output *queue;struct pt_readers_key key,before;unsigned i,controls=24;
    memset(m,0,sizeof(*m));m->now=COMPACT_EPOCH;m->frequency=COMPACT_FREQUENCY;m->session=s->config.session;m->generation=7;port=model_port(m);
    assert(pt_readers_activation_open(&s->allocator,&s->config.grid,s->config.session,&port,&ledger)==PT_SCHEDULED_OK);
    assert(pt_readers_activation_api(ledger,&s->config.backend)==PT_SCHEDULED_OK);
    memset(s->document.project.events,0,s->document.storage.event_capacity*sizeof(*s->document.project.events));
    /* Slow the unchanged2048-frame one-shot so all24 original20ms controls
     * occur while the genuine renderer voice is ACTIVE. Raw856 + E11 ->855;
     * Q32step371187734 ends at23698 outputframes, after control24@23040.
     * Sample rate/words/masters and the original960-frame grid stay exact. */
    s->document.project.events[0]=(struct pt_event){856,0,PT_NOTE_PERIOD,(uint8_t)(s->slot+1),14,0x11,0,0};
    for(i=1;i<=controls;++i){s->document.project.events[i*4].effect=12;s->document.project.events[i*4].parameter=(uint8_t)(16+i);}
    s->document.project.events[(controls+1)*4+3].effect=15;s->config.render.tick_limit=32;
    assert(compact_begin(s)==PT_PAULA_READERS_SONG_PENDING&&s->scratch_discarded==1);activation_until_publish(s,0,"trigger");
    first=song_publish(s,m,0,0);queue=first->packet.key[0].queue;
    memset(&key,0xb3,sizeof(key));before=key;
    assert(pt_readers_reader_key((struct pt_readers_output *)queue,first->packet.ticket,0,&key)==PT_SCHEDULED_STALE&&!memcmp(&key,&before,sizeof(key)));
    for(i=0;i<COMPACT_STEPS;++i)if(compact_step(s)==PT_PAULA_READERS_SONG_WAIT_ACTIVE)break;
    assert(i<COMPACT_STEPS&&!m->effects);song_fire(s,m,first);
    assert(pt_readers_reader_key((struct pt_readers_output *)queue,first->packet.ticket,0,&key)==PT_SCHEDULED_OK&&compact_key_equal(&key,m->slot));
    activation_until_publish(s,1,"control");second=song_publish(s,m,1,0);song_fire(s,m,second);
    for(i=0;i<COMPACT_STEPS;++i)if(compact_step(s)==PT_PAULA_READERS_SONG_WAIT_PRESSURE)break;
    assert(i<COMPACT_STEPS&&s->status.command_mask==3&&s->status.reader_mask==1);
    song_detach(s,first);assert(m->reader[0].live&&s->allocation->chip_live);
    for(i=2;i<=controls;++i){activation_until_publish(s,i,"control");c=song_publish(s,m,i,0);song_fire(s,m,c);song_detach(s,second);second=c;}
    song_detach(s,second);
    for(i=0;i<COMPACT_STEPS;++i)if(compact_step(s)==PT_PAULA_READERS_SONG_DONE)break;
    assert(i<COMPACT_STEPS&&s->status.done&&s->status.phase==PT_PAULA_READERS_SONG_END&&s->status.reader_mask==1&&!s->status.command_mask&&m->reader[0].live);
    assert(s->status.terminal_frame==COMPACT_START+(controls+1U)*COMPACT_TICK_FRAMES&&m->effects==controls+1);
    assert(!pt_paula_readers_song_close(&s->song)&&!pt_readers_activation_close(&ledger));
    assert(pt_paula_readers_song_terminal_stop(s->song,1)==PT_PAULA_READERS_SONG_PENDING);activation_until_publish(s,controls+1,"terminal STOP");
    c=song_publish(s,m,controls+1,1);song_fire(s,m,c);assert(!m->mask&&m->reader[0].live&&s->allocation->chip_live);
    if(bits==16){m->reader[0].retire=1;assert(pt_paula_readers_song_service_reader(s->song,0,0,NULL)==PT_SCHEDULED_OK);
        assert(pt_readers_readers_held(queue)==1&&pt_readers_commands_held(queue)==1);song_detach(s,c);
    }else{song_detach(s,c);assert(m->reader[0].live&&s->allocation->chip_live);m->reader[0].retire=1;
        assert(pt_paula_readers_song_service_reader(s->song,0,0,NULL)==PT_SCHEDULED_OK);}
    assert(!pt_readers_commands_held(queue)&&!pt_readers_readers_held(queue));
    assert(pt_paula_readers_song_close(&s->song)&&!s->song);/* closes genuine queue BEFORE ledger */
    model_end(m);assert(pt_readers_activation_close(&ledger)&&!ledger);compact_release(s->allocation,m);
    printf("ACTIVATION LEDGER mono%u PASS: real workspace/master flow; original frames; 24 controls; independent domains\n",bits);
    compact_source_close(s);
}

/* Genuine independently allocated public queue holders instrument current,
 * terminal and release callbacks. Their declarations are copied by enqueue;
 * only that ended construction scratch is overwritten. No private states. */
struct activation_counts {unsigned current,terminal,releases,live;};
struct activation_holder {struct activation_counts *counts;uint64_t token;uint8_t *data;unsigned live;};
static int holder_current(void *c,uint64_t token,uint64_t generation)
{struct activation_holder *h=c;++h->counts->current;return h->live&&h->token==token&&generation==7;}
static void holder_terminal(void *c,uint64_t token,int valid)
{struct activation_holder *h=c;assert(h->live&&h->token==token);(void)valid;++h->counts->terminal;}
static void holder_release(void *c,uint64_t token)
{struct activation_holder *h=c;struct activation_counts *counts=h->counts;assert(h->live&&h->token==token&&counts->live);h->live=0;--counts->live;++counts->releases;free(h->data);free(h);}
static struct pt_readers_control holder_new(struct activation_counts *counts,uint64_t token,unsigned sample)
{struct activation_holder *h=calloc(1,sizeof(*h));assert(h);h->counts=counts;h->token=token;h->live=1;++counts->live;
    if(sample){h->data=malloc(128);assert(h->data);memset(h->data,17,128);}
    return (struct pt_readers_control){h,sizeof(*h),token,holder_current,holder_release,holder_terminal};}
/* Task-only transparent observer of PUBLIC registered descriptors. This small
 * independent context forwards the real backend unchanged; it is not a port
 * activation input or receipt producer. Observed metadata lives through borrow. */
struct activation_observer {struct pt_readers_backend api;const struct pt_readers_domain *domain;};
static int observer_clock(void *c,uint64_t *t,uint32_t *f)
{struct activation_observer *o=c;return o->api.read_clock(o->api.context,t,f);}
static int observer_submit(void *c,const struct pt_readers_event *e)
{struct activation_observer *o=c;o->domain=e->reader[0];return o->api.submit(o->api.context,e);}
static enum pt_readers_reply observer_poll_command(void *c,uint64_t t,struct pt_readers_command_receipt *r)
{struct activation_observer *o=c;return o->api.poll_command(o->api.context,t,r);}
static enum pt_readers_reply observer_cancel_command(void *c,uint64_t t,struct pt_readers_command_receipt *r)
{struct activation_observer *o=c;return o->api.cancel_command(o->api.context,t,r);}
static enum pt_readers_reply observer_poll_reader(void *c,const struct pt_readers_domain *d,struct pt_readers_reader_receipt *r)
{struct activation_observer *o=c;return o->api.poll_reader(o->api.context,d,r);}
static enum pt_readers_reply observer_cancel_reader(void *c,const struct pt_readers_domain *d,struct pt_readers_reader_receipt *r)
{struct activation_observer *o=c;return o->api.cancel_reader(o->api.context,d,r);}
struct activation_queue_case {
    struct activation_model model;struct activation_counts counts;
    struct pt_readers_activation *ledger;struct pt_readers_output *queue;
    struct pt_readers_backend api;struct pt_allocator allocator;
    struct pt_scheduled_grid grid;struct pt_scheduled_batch batch;
    struct pt_readers_control command;struct pt_readers_owner owners[4];
    struct pt_scheduled_span spans[4];struct pt_readers_key key[4],before_key;
    struct pt_readers_command_receipt command_out,command_before;
    struct pt_readers_reader_receipt reader_out,reader_before;
    uint64_t token,session;struct activation_observer *observer;
};
static void *activation_malloc(void *c,size_t n){(void)c;return malloc(n);}
static void activation_free(void *c,void *p){(void)c;free(p);}
static void activation_chip_free(void *c,void *p,size_t n){(void)n;activation_free(c,p);}
static uint64_t activation_fresh_session(void)
{static uint64_t session=10000;assert(session!=UINT64_MAX);return ++session;}
static struct activation_queue_case *queue_new_observer(unsigned observe)
{
    struct activation_queue_case *q=calloc(1,sizeof(*q));struct pt_readers_activation_port p;assert(q);
    q->allocator=(struct pt_allocator){NULL,activation_malloc,activation_free};q->grid=(struct pt_scheduled_grid){100,7,1000000,48000};
    q->session=activation_fresh_session();q->model.session=q->session;q->model.generation=7;
    q->model.now=100;q->model.frequency=1000000;p=model_port(&q->model);
    assert(pt_readers_activation_open(&q->allocator,&q->grid,q->session,&p,&q->ledger)==PT_SCHEDULED_OK);
    assert(pt_readers_activation_api(q->ledger,&q->api)==PT_SCHEDULED_OK);
    if(observe){q->observer=calloc(1,sizeof(*q->observer));assert(q->observer);q->observer->api=q->api;
        q->api.context=q->observer;q->api.context_bytes=sizeof(*q->observer);
        q->api.read_clock=observer_clock;q->api.submit=observer_submit;
        q->api.poll_command=observer_poll_command;q->api.cancel_command=observer_cancel_command;
        q->api.poll_reader=observer_poll_reader;q->api.cancel_reader=observer_cancel_reader;}
    assert(pt_readers_open(&q->allocator,&q->grid,q->session,&q->api,2,8,&q->queue)==PT_SCHEDULED_OK);return q;
}
static struct activation_queue_case *queue_new(void){return queue_new_observer(0);}
static uint64_t queue_enqueue(struct activation_queue_case *q,unsigned count,unsigned trigger,unsigned slot,uint64_t frame)
{
    unsigned i;uint64_t ticket=0;memset(&q->batch,0,sizeof(q->batch));memset(q->owners,0,sizeof(q->owners));
    q->batch.generation=7;q->batch.frame=frame;q->batch.count=count;q->command=holder_new(&q->counts,++q->token,0);
    for(i=0;i<count;++i){struct pt_scheduled_action *a=q->batch.action+i;a->slot=count==4?i:slot;
        a->kind=trigger?PT_SCHEDULED_TRIGGER:PT_SCHEDULED_CONTROL;a->period=428;a->volume=32;
        if(trigger){struct activation_holder *h;q->owners[i].control=holder_new(&q->counts,++q->token,1);h=q->owners[i].control.context;
            q->spans[i]=(struct pt_scheduled_span){h->data,128};q->owners[i].spans=q->spans+i;q->owners[i].count=1;a->data=h->data;a->words=64;}
    }
    assert(pt_readers_enqueue(q->queue,&q->batch,trigger?NULL:q->key,&q->command,trigger?q->owners:NULL,&ticket)==PT_SCHEDULED_OK);
    memset(&q->command,0x3c,sizeof(q->command));memset(q->owners,0x3c,sizeof(q->owners));memset(q->spans,0x3c,sizeof(q->spans));return ticket;
}
static void queue_fire(struct activation_queue_case *q,uint64_t ticket)
{
    struct activation_model_command *c=model_command(&q->model,ticket);unsigned current=q->counts.current,terminal=q->counts.terminal,releases=q->counts.releases;
    assert(c);q->model.now=c->packet.first;assert(pt_readers_activation_fire(q->ledger,ticket)==PT_READERS_ACTIVATION_COMMITTED);
    assert(q->counts.current==current&&q->counts.terminal==terminal&&q->counts.releases==releases);
    assert(pt_readers_poll_command(q->queue,ticket,&q->command_out)==PT_SCHEDULED_PENDING);
}
static void queue_detach(struct activation_queue_case *q,uint64_t ticket,unsigned failed)
{struct activation_model_command *c=model_command(&q->model,ticket);assert(c);c->detach=1;
    assert(pt_readers_poll_command(q->queue,ticket,&q->command_out)==(failed?PT_SCHEDULED_BACKEND:PT_SCHEDULED_OK));}
static void queue_close(struct activation_queue_case *q)
{assert(!pt_readers_commands_held(q->queue)&&!pt_readers_readers_held(q->queue)&&!q->counts.live&&q->counts.terminal==q->counts.releases);
    assert(pt_readers_close(q->queue));q->queue=NULL;model_end(&q->model);assert(pt_readers_activation_close(&q->ledger)&&!q->ledger);free(q->observer);free(q);}
static void activation_span_output_alias(void)
{
    struct activation_queue_case *q=queue_new_observer(1);uint64_t ticket=queue_enqueue(q,1,1,0,COMPACT_START);
    const struct pt_readers_domain *d;struct pt_scheduled_span before;unsigned probes;
    assert(pt_readers_publish(q->queue,ticket)==PT_SCHEDULED_OK);d=q->observer->domain;
    assert(d&&d->count==1&&d->spans);before=d->spans[0];probes=q->model.reader_probes;
    assert(q->observer->api.poll_reader(q->observer->api.context,d,
        (struct pt_readers_reader_receipt *)d->spans)==PT_READERS_UNCERTAIN);
    assert(pt_readers_poll_reader(q->queue,ticket,0,
        (struct pt_readers_reader_receipt *)d->spans)==PT_SCHEDULED_INVALID);
    assert(q->model.reader_probes==probes&&d->spans[0].data==before.data&&d->spans[0].bytes==before.bytes);
    queue_fire(q,ticket);queue_detach(q,ticket,0);
    assert(pt_readers_cancel_reader(q->queue,ticket,0,&q->reader_out)==PT_SCHEDULED_OK);q->observer->domain=NULL;queue_close(q);
}
static void activation_pending_registry_retry(void)
{
    struct activation_queue_case *q=queue_new();uint64_t first,replacement,control;unsigned i,publishes;
    first=queue_enqueue(q,4,1,0,COMPACT_START);assert(pt_readers_publish(q->queue,first)==PT_SCHEDULED_OK);
    queue_fire(q,first);for(i=0;i<4;++i)assert(pt_readers_reader_key(q->queue,first,i,q->key+i)==PT_SCHEDULED_OK);queue_detach(q,first,0);
    replacement=queue_enqueue(q,1,1,0,COMPACT_START+COMPACT_TICK_FRAMES);
    assert(pt_readers_publish(q->queue,replacement)==PT_SCHEDULED_OK);
    q->key[0]=q->key[1];control=queue_enqueue(q,1,0,1,COMPACT_START+2U*COMPACT_TICK_FRAMES);publishes=q->model.publishes;
    assert(pt_readers_publish(q->queue,control)==PT_SCHEDULED_PENDING&&q->model.publishes==publishes&&!model_command(&q->model,control));
    queue_fire(q,replacement);
    assert(pt_readers_publish(q->queue,control)==PT_SCHEDULED_OK);
    assert(model_command(&q->model,control)->packet.frame==COMPACT_START+2U*COMPACT_TICK_FRAMES&&
        model_command(&q->model,control)->packet.first==oracle_first(COMPACT_START+2U*COMPACT_TICK_FRAMES));
    queue_fire(q,control);queue_detach(q,replacement,0);queue_detach(q,control,0);
    for(i=0;i<4;++i)assert(pt_readers_cancel_reader(q->queue,first,i,&q->reader_out)==PT_SCHEDULED_OK);
    assert(pt_readers_cancel_reader(q->queue,replacement,0,&q->reader_out)==PT_SCHEDULED_OK);queue_close(q);
}
static void activation_one_publishing_queue(void)
{
    struct activation_queue_case *q=queue_new();struct pt_readers_output *original,*other=NULL;uint64_t first,second;unsigned publishes;
    first=queue_enqueue(q,1,1,0,COMPACT_START);assert(pt_readers_publish(q->queue,first)==PT_SCHEDULED_OK);queue_fire(q,first);queue_detach(q,first,0);
    original=q->queue;/* Distinct fresh queue/session: combined identity guard. */
    assert(pt_readers_open(&q->allocator,&q->grid,activation_fresh_session(),&q->api,2,8,&other)==PT_SCHEDULED_OK);q->queue=other;
    second=queue_enqueue(q,1,1,1,COMPACT_START+COMPACT_TICK_FRAMES);publishes=q->model.publishes;
    assert(pt_readers_publish(other,second)==PT_SCHEDULED_PENDING&&q->model.publishes==publishes);
    assert(pt_readers_stop(other)==PT_SCHEDULED_OK&&pt_readers_close(other));q->queue=original;
    assert(pt_readers_cancel_reader(original,first,0,&q->reader_out)==PT_SCHEDULED_OK);queue_close(q);
}
static void activation_four_replacement(void)
{
    struct activation_queue_case *q=queue_new();uint64_t first,second;unsigned i;struct pt_readers_key replacement;
    first=queue_enqueue(q,4,1,0,COMPACT_START);assert(pt_readers_publish(q->queue,first)==PT_SCHEDULED_OK&&!q->model.effects);
    queue_fire(q,first);for(i=0;i<4;++i)assert(pt_readers_reader_key(q->queue,first,i,q->key+i)==PT_SCHEDULED_OK&&compact_key_equal(q->key+i,q->model.slot+i));
    queue_detach(q,first,0);assert(q->counts.live==4&&pt_readers_readers_held(q->queue)==4);
    second=queue_enqueue(q,1,1,0,COMPACT_START+COMPACT_TICK_FRAMES);assert(pt_readers_publish(q->queue,second)==PT_SCHEDULED_OK);
    q->before_key=q->key[0];assert(pt_readers_reader_key(q->queue,first,0,q->key)==PT_SCHEDULED_STALE&&compact_key_equal(q->key,&q->before_key));
    queue_fire(q,second);assert(pt_readers_reader_key(q->queue,second,0,&replacement)==PT_SCHEDULED_OK);
    assert(pt_readers_cancel_reader(q->queue,first,0,&q->reader_out)==PT_SCHEDULED_OK&&compact_key_equal(q->model.slot,&replacement));
    assert(q->counts.live==5&&pt_readers_readers_held(q->queue)==4);queue_detach(q,second,0);
    for(i=1;i<4;++i)assert(pt_readers_cancel_reader(q->queue,first,i,&q->reader_out)==PT_SCHEDULED_OK);
    assert(pt_readers_cancel_reader(q->queue,second,0,&q->reader_out)==PT_SCHEDULED_OK);queue_close(q);
}
static void activation_refusal_uncertainty(unsigned mode)
{
    struct activation_queue_case *q=queue_new();uint64_t ticket;struct activation_model_command *c;unsigned current,effects,i,count=mode==7?4:1;
    ticket=queue_enqueue(q,count,1,0,COMPACT_START);
    if(!mode){q->model.refuse=1;assert(pt_readers_publish(q->queue,ticket)==PT_SCHEDULED_PENDING&&!model_command(&q->model,ticket)&&!q->model.effects);
        assert(pt_readers_publish(q->queue,ticket)==PT_SCHEDULED_OK);queue_fire(q,ticket);queue_detach(q,ticket,0);
        assert(pt_readers_cancel_reader(q->queue,ticket,0,&q->reader_out)==PT_SCHEDULED_OK);queue_close(q);return;}
    q->model.uncertain=mode==1;assert(pt_readers_publish(q->queue,ticket)==(mode==1?PT_SCHEDULED_BACKEND:PT_SCHEDULED_OK));
    c=model_command(&q->model,ticket);assert(c);current=q->counts.current;effects=q->model.effects;
    if(mode==2){q->model.now=c->packet.last;assert(pt_readers_activation_fire(q->ledger,ticket)==PT_READERS_ACTIVATION_FAILED&&q->model.effects==effects);}
    else if(mode==3){q->model.forge_untouched=1;q->model.now=c->packet.first;assert(pt_readers_activation_fire(q->ledger,ticket)==PT_READERS_ACTIVATION_FAILED&&q->model.effects==effects+1);}
    else if(mode==4){/* Port registry changed in a NON-ADDRESSED inactive slot. */
        q->model.slot[3]=c->packet.key[0];q->model.slot[3].slot=3;q->model.mask|=8;q->model.now=c->packet.first;
        assert(pt_readers_activation_fire(q->ledger,ticket)==PT_READERS_ACTIVATION_FAILED&&q->model.effects==effects);
        q->model.mask&=~8U;memset(q->model.slot+3,0,sizeof(q->model.slot[3]));
    }
    else if(mode==5){q->model.frequency+=1;q->model.now=c->packet.first;
        assert(pt_readers_activation_fire(q->ledger,ticket)==PT_READERS_ACTIVATION_FAILED&&q->model.effects==effects&&!q->model.commits);
    }else if(mode==6||mode==7){q->model.post_late=mode==6;q->model.partial=mode==7;q->model.now=c->packet.first;
        assert(pt_readers_activation_fire(q->ledger,ticket)==PT_READERS_ACTIVATION_FAILED&&q->model.effects==effects+1);
    }
    else if(mode==8){/* Honest monotone history first, then a genuine regression. */
        q->model.now=c->packet.first-1;
        assert(q->model.now>=COMPACT_EPOCH&&pt_readers_activation_fire(q->ledger,ticket)==PT_READERS_ACTIVATION_EARLY&&q->model.effects==effects);
        q->model.now=c->packet.first-2;
        assert(q->model.now>=COMPACT_EPOCH&&pt_readers_activation_fire(q->ledger,ticket)==PT_READERS_ACTIVATION_FAILED&&q->model.effects==effects&&!q->model.commits);
    }
    assert(q->counts.current==current);memset(&q->command_out,0xa7,sizeof(q->command_out));q->command_before=q->command_out;
    memset(&q->reader_out,0xb8,sizeof(q->reader_out));q->reader_before=q->reader_out;
    assert(pt_readers_poll_command(q->queue,ticket,&q->command_out)==PT_SCHEDULED_BACKEND&&!memcmp(&q->command_out,&q->command_before,sizeof(q->command_out)));
    assert(pt_readers_commands_held(q->queue)==1&&pt_readers_readers_held(q->queue)==count&&!pt_readers_activation_close(&q->ledger));
    queue_detach(q,ticket,1);assert(!pt_readers_commands_held(q->queue)&&pt_readers_readers_held(q->queue)==count);
    assert(!memcmp(&q->command_out,&q->command_before,sizeof(q->command_out)));
    for(i=0;i<count;++i)assert(pt_readers_cancel_reader(q->queue,ticket,i,&q->reader_out)==PT_SCHEDULED_BACKEND&&!memcmp(&q->reader_out,&q->reader_before,sizeof(q->reader_out)));
    queue_close(q);
}
static void activation_full_key_stale(unsigned field)
{
    struct activation_queue_case *q=queue_new();struct pt_readers_output *other=NULL;
    struct pt_readers_key original;struct activation_model_command *c;uint64_t first,control;unsigned i,effects,current;
    first=queue_enqueue(q,4,1,0,COMPACT_START);assert(pt_readers_publish(q->queue,first)==PT_SCHEDULED_OK);
    queue_fire(q,first);for(i=0;i<4;++i)assert(pt_readers_reader_key(q->queue,first,i,q->key+i)==PT_SCHEDULED_OK);queue_detach(q,first,0);
    control=queue_enqueue(q,1,0,0,COMPACT_START+COMPACT_TICK_FRAMES);assert(pt_readers_publish(q->queue,control)==PT_SCHEDULED_OK);
    c=model_command(&q->model,control);assert(c);original=q->model.slot[3];
    /* Deliberately stale actual PORT registry, with the same active mask. No
     * queue/domain/owner mutation, fabricated receipt or ACTIVE certificate. */
    switch(field){
    case 0:assert(pt_readers_open(&q->allocator,&q->grid,activation_fresh_session(),&q->api,2,8,&other)==PT_SCHEDULED_OK);q->model.slot[3].queue=other;break;
    case 1:++q->model.slot[3].session;break;
    case 2:++q->model.slot[3].generation;break;
    case 3:++q->model.slot[3].trigger;break;
    case 4:++q->model.slot[3].owner;break;
    case 5:++q->model.slot[3].serial;break;
    case 6:q->model.slot[3].action=2;break;
    case 7:q->model.slot[3].slot=2;break;
    default:assert(0);
    }
    effects=q->model.effects;current=q->counts.current;q->model.now=c->packet.first;
    assert(pt_readers_activation_fire(q->ledger,control)==PT_READERS_ACTIVATION_FAILED&&q->model.effects==effects&&q->model.mask==15&&q->counts.current==current);
    q->model.slot[3]=original;if(other)assert(pt_readers_close(other));
    queue_detach(q,control,1);
    for(i=0;i<4;++i)assert(pt_readers_cancel_reader(q->queue,first,i,&q->reader_out)==PT_SCHEDULED_BACKEND);
    queue_close(q);
}
int main(void)
{
    const struct pt_compact_song_memory memory={{NULL,activation_malloc,activation_free},NULL,activation_malloc,activation_chip_free};
    unsigned mode;activation_song(&memory,8);activation_song(&memory,16);activation_song(&memory,24);
    activation_four_replacement();activation_span_output_alias();activation_pending_registry_retry();activation_one_publishing_queue();
    for(mode=0;mode<9;++mode)activation_refusal_uncertainty(mode);
    for(mode=0;mode<8;++mode)activation_full_key_stale(mode);
    puts("READERS ACTIVATION LEDGER PASS: genuine public owners; copied exact windows; positive model ACTIVE; independently proven shutdown; software port only");return 0;
}
