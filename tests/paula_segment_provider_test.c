/* Genuine producer/packet32 donor preparation is used, not the unqualified
 * future backend. This provider is a resource model; its APPLY is not a native
 * activation or a replacement for the genuine scheduled core submission. */
#include "paula_segment_packet32_provider_basis.c"
#include "../src/editor/paula_segment_provider.h"
union provider_storage {uint64_t alignment;unsigned char bytes[32768];};
struct provider_model;
struct provider_context {struct provider_model *owner;};
struct provider_model {
    union provider_storage storage;struct pt_paula_future_ops adapter;struct provider_context context;
    struct pt_paula_future_packet original;
    struct pt_paula_provider_record observed[4];
    const struct pt_paula_provider_record *borrowed[4];
    uint64_t now,token;uint32_t frequency;
    unsigned locked,arms,rearms,applies,acks,cancels,ticket_probes,clocks,
        reader_probes,source_closes,source_probes,ticket_release,reader_release,source_release;
    int mode,fired,arm_return,restore_return,enter_return,close_return;
};
static struct provider_model provider_negatives[28],provider_positive,provider_alias_holder;
static struct sequence_fixture provider_retained_owner;
static struct sequence_fixture *provider_owner;
static unsigned provider_cases,provider_packets,ran_negatives,provider_alias_cases;
static void provider_callback_fault(struct provider_model *m,unsigned phase,const struct pt_paula_provider_record *r)
{
    uint64_t token=0;struct pt_paula_provider_status status;
    if(m->fired)return;
    if((m->mode==2||m->mode==24)&&phase==1){memset(m->storage.bytes,0,pt_paula_segment_provider_size());m->fired=1;}
    else if(m->mode==3&&phase==1){((struct pt_paula_provider_record *)(void *)r)->packet.bytes[40]^=1;m->fired=1;}
    else if(m->mode==4&&phase==1){assert(!pt_paula_segment_provider_get(m->adapter.context,&status));m->fired=1;}
    else if(m->mode==5&&phase==1){++m->original.ticket;m->fired=1;}
    else if(m->mode==8&&phase==2){assert(m->adapter.enter(m->adapter.context,PT_PAULA_FUTURE_DELIVERY,&token)==-1);m->fired=1;}
    else if(m->mode==11&&phase==3){assert(m->adapter.enter(m->adapter.context,PT_PAULA_FUTURE_TASK,&token)==-1);m->fired=1;}
    else if(m->mode==22&&phase==4&&r){((struct pt_paula_provider_record *)(void *)r)->packet.bytes[40]^=1;m->fired=1;}
}
static int provider_model_enter(void *v,enum pt_paula_future_entry e,uint64_t *token)
{
    struct provider_model *m=v;assert(e==PT_PAULA_FUTURE_TASK||e==PT_PAULA_FUTURE_DELIVERY);assert(!m->locked);
    if(m->enter_return!=0){m->locked=1;m->token+=17;*token=m->token;}
    provider_callback_fault(m,3,NULL);return m->enter_return;
}
static int provider_model_restore(void *v,enum pt_paula_future_entry e,uint64_t token)
{
    struct provider_model *m=v;assert(e==PT_PAULA_FUTURE_TASK||e==PT_PAULA_FUTURE_DELIVERY);
    assert(m->locked&&token==m->token);provider_callback_fault(m,2,NULL);
    if(m->restore_return==1)m->locked=0;return m->restore_return;
}
static int provider_model_clock(void *v,uint64_t *tick,uint32_t *frequency)
{struct provider_model *m=v;assert(m->locked);++m->clocks;*tick=m->now;*frequency=m->frequency;return 1;}
static int provider_model_packet(void *v,enum pt_paula_provider_packet_action action,const struct pt_paula_provider_record *r)
{
    struct provider_model *m=v;unsigned at=r->publication;assert(m->locked&&at<4);
    assert(read_wide(&r->packet,10)==r->ticket&&read_wide(&r->packet,12)==r->first&&
        read_wide(&r->packet,14)==r->last&&read_wide(&r->packet,16)==r->frame);
    if(action==PT_PAULA_PROVIDER_ARM){++m->arms;if(m->arm_return!=0){m->borrowed[at]=r;memcpy(m->observed+at,r,sizeof(*r));}
        provider_callback_fault(m,1,r);return m->arm_return;}
    assert(m->borrowed[at]==r&&!memcmp(m->observed+at,r,sizeof(*r)));
    if(action==PT_PAULA_PROVIDER_REARM){assert(m->now<r->first);++m->rearms;return 1;}
    assert(action==PT_PAULA_PROVIDER_APPLY&&m->now>=r->first&&m->now<r->last);++m->applies;return 1;
}
static int provider_model_ticket(void *v,enum pt_paula_provider_ticket_action action,uint64_t ticket)
{
    struct provider_model *m=v;unsigned i;const struct pt_paula_provider_record *r=NULL;assert(m->locked);
    for(i=0;i<4;++i)if(m->borrowed[i]&&m->borrowed[i]->ticket==ticket){r=m->borrowed[i];break;}
    if(action==PT_PAULA_PROVIDER_ACK){++m->acks;return 1;}
    if(action==PT_PAULA_PROVIDER_CANCEL){++m->cancels;return 1;}
    assert(action==PT_PAULA_PROVIDER_TICKET_QUIET);++m->ticket_probes;
    if(!m->ticket_release)return 0;
    provider_callback_fault(m,4,r);
    /* Independent removal of the model's actual retained pointer. This does
     * not end any genuine sampler/cache/reader obligation. */
    if(r)m->borrowed[i]=NULL;return 1;
}
static int provider_model_reader(void *v)
{struct provider_model *m=v;assert(m->locked);++m->reader_probes;return m->reader_release?1:0;}
static int provider_model_close(void *v)
{struct provider_model *m=v;assert(m->locked);++m->source_closes;return m->close_return;}
static int provider_model_source(void *v)
{struct provider_model *m=v;assert(m->locked);++m->source_probes;return m->source_release?1:0;}
static struct provider_model *provider_model_context(void *v)
{return ((struct provider_context *)v)->owner;}
/* Explicit typed wrappers preserve the context's declared disjoint subextent. */
static int provider_context_enter(void *v,enum pt_paula_future_entry e,uint64_t *t){return provider_model_enter(provider_model_context(v),e,t);}
static int provider_context_restore(void *v,enum pt_paula_future_entry e,uint64_t t){return provider_model_restore(provider_model_context(v),e,t);}
static int provider_context_clock(void *v,uint64_t *t,uint32_t *f){return provider_model_clock(provider_model_context(v),t,f);}
static int provider_context_packet(void *v,enum pt_paula_provider_packet_action a,const struct pt_paula_provider_record *r){return provider_model_packet(provider_model_context(v),a,r);}
static int provider_context_ticket(void *v,enum pt_paula_provider_ticket_action a,uint64_t t){return provider_model_ticket(provider_model_context(v),a,t);}
static int provider_context_reader(void *v){return provider_model_reader(provider_model_context(v));}
static int provider_context_close(void *v){return provider_model_close(provider_model_context(v));}
static int provider_context_source(void *v){return provider_model_source(provider_model_context(v));}
static void provider_model_init(struct provider_model *m,int mode)
{
    struct pt_paula_provider_resources ops;struct pt_paula_provider_config c;unsigned i;
    memset(m,0,sizeof(*m));m->mode=mode;m->arm_return=m->enter_return=m->restore_return=m->close_return=1;
    m->frequency=709379;m->now=100;m->token=31;assert(pt_paula_segment_provider_size()<=sizeof(m->storage));
    memset(&ops,0,sizeof(ops));memset(&c,0,sizeof(c));c.frequency=709379;
    for(i=0;i<4;++i)c.frame[i]=960U*(i+1U);
    for(i=0;i<3;++i)c.label[i]=(struct pt_paula_provider_label){(uint16_t)i,0x100U+i,0x200U+i,
        0x300U+i,0x400U,0x500U,0x600U,0x100000U+0x1000U*i};
    m->context.owner=m;ops.context=&m->context;ops.context_bytes=sizeof(m->context);
    ops.enter=provider_context_enter;ops.restore=provider_context_restore;ops.clock=provider_context_clock;ops.packet=provider_context_packet;
    ops.ticket=provider_context_ticket;ops.reader_quiet=provider_context_reader;ops.source_close=provider_context_close;ops.source_quiet=provider_context_source;
    /* The complete typed context points to a live caller-owned holder; the
     * injected model never claims it is a native address or ownership proof. */
    assert(pt_paula_segment_provider_init(m->storage.bytes,sizeof(m->storage),&ops,&c,&m->adapter));
}
static uint64_t provider_entry(struct provider_model *m,enum pt_paula_future_entry e)
{uint64_t token=0,tick=0;uint32_t frequency=0;assert(m->adapter.enter(m->adapter.context,e,&token)==1);
 assert(m->adapter.clock(m->adapter.context,&tick,&frequency)==1&&tick==m->now&&frequency==709379);return token;}
static void provider_exit(struct provider_model *m,enum pt_paula_future_entry e,uint64_t token)
{assert(m->adapter.leave(m->adapter.context,e,token)==1&&!m->locked);}
static void provider_drain(struct provider_model *m,uint64_t ticket)
{
    uint64_t token;unsigned calls;struct pt_paula_provider_status s;
    token=provider_entry(m,PT_PAULA_FUTURE_TASK);m->ticket_release=1;
    assert(m->adapter.ticket_quiet(m->adapter.context,ticket)==1);
    m->reader_release=1;assert(m->adapter.reader_quiet(m->adapter.context)==1);
    calls=m->source_closes;assert(m->adapter.source_close(m->adapter.context)==m->close_return);
    assert(m->adapter.source_close(m->adapter.context)==m->close_return&&m->source_closes==calls+1);
    m->source_release=1;assert(m->adapter.source_quiet(m->adapter.context)==1);
    assert(!pt_paula_segment_provider_dispose(m->adapter.context));provider_exit(m,PT_PAULA_FUTURE_TASK,token);
    assert(pt_paula_segment_provider_get(m->adapter.context,&s)&&s.quiet==s.retained&&s.source_quiet);
    assert(pt_paula_segment_provider_dispose(m->adapter.context));
}

enum provider_alias_output {PROVIDER_ALIAS_TOKEN,PROVIDER_ALIAS_TICK,
    PROVIDER_ALIAS_FREQUENCY,PROVIDER_ALIAS_STATUS,PROVIDER_ALIAS_ROW};
/* All targets are authenticated existing HOST objects, not cache_base32 labels.
 * Snapshot comparisons never restore source, core, reader or resource state. */
static void provider_alias_refusal(struct provider_model *m,unsigned which,void *alias,size_t available)
{
    struct provider_model before;struct pt_paula_provider_status a,b;
    unsigned char bytes[sizeof(struct pt_paula_provider_status)+sizeof(struct pt_paula_provider_row_status)];
    uint64_t token=0,tick=11;uint32_t frequency=13;size_t n;unsigned pins,fast_live,chip_live;size_t chip_bytes;
    assert(which<=PROVIDER_ALIAS_ROW);
    if(which==PROVIDER_ALIAS_TICK||which==PROVIDER_ALIAS_FREQUENCY)
        token=provider_entry(m,PT_PAULA_FUTURE_TASK);
    n=which==PROVIDER_ALIAS_TOKEN||which==PROVIDER_ALIAS_TICK?sizeof(uint64_t):
        which==PROVIDER_ALIAS_FREQUENCY?sizeof(uint32_t):
        which==PROVIDER_ALIAS_STATUS?sizeof(struct pt_paula_provider_status):sizeof(struct pt_paula_provider_row_status);
    assert(alias&&n<=available&&n<=sizeof(bytes)&&!((uintptr_t)alias%pt_paula_segment_provider_alignment()));
    assert(pt_paula_segment_provider_get(m->adapter.context,&a));
    pins=pinned(&provider_owner->f);fast_live=provider_owner->f.fast.live;
    chip_live=provider_owner->f.chip.live;chip_bytes=provider_owner->f.chip.bytes;
    storage(&provider_owner->f);memcpy(bytes,alias,n);memcpy(&before,m,sizeof(before));
    if(which==PROVIDER_ALIAS_TOKEN)
        assert(m->adapter.enter(m->adapter.context,PT_PAULA_FUTURE_TASK,alias)==-1);
    else if(which==PROVIDER_ALIAS_TICK)
        assert(m->adapter.clock(m->adapter.context,alias,&frequency)==-1&&frequency==13);
    else if(which==PROVIDER_ALIAS_FREQUENCY)
        assert(m->adapter.clock(m->adapter.context,&tick,alias)==-1&&tick==11);
    else if(which==PROVIDER_ALIAS_STATUS)
        assert(!pt_paula_segment_provider_get(m->adapter.context,alias));
    else assert(!pt_paula_segment_provider_row(m->adapter.context,0,alias));
    assert(!memcmp(bytes,alias,n)&&!memcmp(&before,m,sizeof(before)));
    assert(pt_paula_segment_provider_get(m->adapter.context,&b)&&!memcmp(&a,&b,sizeof(a)));
    assert(pinned(&provider_owner->f)==pins&&provider_owner->f.fast.live==fast_live&&
        provider_owner->f.chip.live==chip_live&&provider_owner->f.chip.bytes==chip_bytes);
    storage(&provider_owner->f);
    if(which==PROVIDER_ALIAS_TICK||which==PROVIDER_ALIAS_FREQUENCY)
        provider_exit(m,PT_PAULA_FUTURE_TASK,token);
    ++provider_cases;++provider_alias_cases;
}
static void provider_alias_sources(struct provider_model *m,const struct pt_paula_future_packet *p)
{
    const struct pt_paula_segment_source *s=p->geometry.source;
    const struct pt_project *project=&provider_owner->f.document.project;
    void *target[4];size_t available[4],table_bytes,offset,count;unsigned i,j;
    assert(p->geometry.source_count&&s->sample<project->sample_count&&
        s->descriptor==&project->samples[s->sample].pcm);
    count=project->sample_count;assert(count&&sizeof(*project->samples)<=SIZE_MAX/count);
    table_bytes=count*sizeof(*project->samples);
    offset=(size_t)((uintptr_t)s->descriptor-(uintptr_t)project->samples);
    assert(offset<table_bytes&&s->master.capacity<=SIZE_MAX/sizeof(*s->master.data));
    target[0]=(void *)s->descriptor;available[0]=table_bytes-offset;
    target[1]=(void *)s->master.data;available[1]=s->master.capacity*sizeof(*s->master.data);
    target[2]=(void *)s->cache_data;available[2]=s->cache_bytes;target[3]=NULL;available[3]=0;
    for(i=0;i<s->guard_count;++i)if(s->guard[i].data==s->sampler_owner){
        target[3]=(void *)s->guard[i].data;available[3]=s->guard[i].bytes;
    }
    assert(target[3]);
    for(i=0;i<4;++i)for(j=0;j<=PROVIDER_ALIAS_ROW;++j)
        provider_alias_refusal(m,j,target[i],available[i]);
}
static void provider_genuine_aliases(const struct pt_paula_future_packet *p)
{
    struct provider_model *m=&provider_alias_holder;struct pt_paula_provider_row_status row;
    uint64_t token;unsigned i;
    provider_model_init(m,-1);memcpy(&m->original,p,sizeof(*p));m->now=p->first-1;
    /* Context has a declared disjoint subextent in this live complete holder.
     * Even before publication every public output must exclude that subextent. */
    for(i=0;i<=PROVIDER_ALIAS_ROW;++i)
        provider_alias_refusal(m,i,&m->context,sizeof(*m)-offsetof(struct provider_model,context));
    token=provider_entry(m,PT_PAULA_FUTURE_TASK);
    assert(m->adapter.arm(m->adapter.context,&m->original)==1);
    provider_exit(m,PT_PAULA_FUTURE_TASK,token);
    provider_alias_sources(m,&m->original);
    token=provider_entry(m,PT_PAULA_FUTURE_TASK);m->ticket_release=1;
    assert(m->adapter.ticket_quiet(m->adapter.context,p->ticket)==1);
    provider_exit(m,PT_PAULA_FUTURE_TASK,token);
    assert(pt_paula_segment_provider_row(m->adapter.context,0,&row)&&row.quiet&&row.original_released);
    /* Genuine source owners are still pinned. Their numeric extents remain
     * protected after the adapter's original packet anchor has been released. */
    provider_alias_sources(m,&m->original);
    provider_drain(m,p->ticket);assert(provider_alias_cases==45&&pinned(&provider_owner->f)==3);
}

static void provider_negative(const struct pt_paula_future_packet *p,unsigned mode)
{
    struct provider_model *m=provider_negatives+mode;struct pt_paula_provider_status s;
    struct pt_paula_provider_row_status row;uint64_t token=0;unsigned before;int raw;
    provider_model_init(m,(int)mode);memcpy(&m->original,p,sizeof(*p));m->now=p->first-1;
    if(mode==9||mode==10||mode==11){m->enter_return=mode==9?0:(mode==10?-1:1);token=UINT64_MAX;
        raw=m->adapter.enter(m->adapter.context,PT_PAULA_FUTURE_TASK,&token);
        assert(raw==(mode==9?0:-1)&&token==UINT64_MAX);
        assert(pt_paula_segment_provider_get(m->adapter.context,&s)&&!s.published);
        if(mode==9)assert(!s.entry_kind&&!s.entry_token&&!s.restore_hold);
        else assert(s.entry_kind==PT_PAULA_FUTURE_TASK&&s.entry_token==m->token&&s.restore_hold);
        assert(!pt_paula_segment_provider_dispose(m->adapter.context));++provider_cases;return;}
    if(mode==27){struct pt_paula_future_packet copied;memcpy(&copied,&m->original,sizeof(copied));
        copied.geometry.frame++;token=provider_entry(m,PT_PAULA_FUTURE_TASK);before=m->arms;
        assert(m->adapter.arm(m->adapter.context,&copied)==0&&m->arms==before);provider_exit(m,PT_PAULA_FUTURE_TASK,token);
        ++provider_cases;return;}
    token=provider_entry(m,PT_PAULA_FUTURE_TASK);
    if(mode==15){uint64_t tick=11;uint32_t frequency=13;m->frequency++;
        assert(m->adapter.clock(m->adapter.context,&tick,&frequency)==-1&&tick==11&&frequency==13);
        assert(m->adapter.arm(m->adapter.context,&m->original)==0&&!m->arms);m->frequency=709379;
        provider_exit(m,PT_PAULA_FUTURE_TASK,token);++provider_cases;return;}
    if(mode==23){m->original.geometry.source[0].master.data=(int32_t *)(void *)m->storage.bytes;
        assert(m->adapter.arm(m->adapter.context,&m->original)==0&&!m->arms);provider_exit(m,PT_PAULA_FUTURE_TASK,token);++provider_cases;return;}
    if(mode==0||mode==24)m->arm_return=0;else if(mode==1)m->arm_return=-1;else if(mode==25)m->arm_return=2;
    raw=m->adapter.arm(m->adapter.context,&m->original);
    assert(raw==(mode==0?0:((mode==1||mode==2||mode==3||mode==4||mode==5||mode==24||mode==25)?-1:1)));
    assert(pt_paula_segment_provider_row(m->adapter.context,0,&row)&&row.present&&row.arm_raw==m->arm_return);
    if(mode==6||mode==7||mode==8||mode==5){m->restore_return=mode==6?0:(mode==7?-1:1);
        assert(m->adapter.leave(m->adapter.context,PT_PAULA_FUTURE_TASK,token)==-1);
        assert(pt_paula_segment_provider_get(m->adapter.context,&s)&&s.restore_hold&&s.retained==1);
        assert(!pt_paula_segment_provider_dispose(m->adapter.context));++provider_cases;return;}
    provider_exit(m,PT_PAULA_FUTURE_TASK,token);
    if(mode==0){assert(pt_paula_segment_provider_get(m->adapter.context,&s)&&!s.published&&!s.retained);
        m->arm_return=1;token=provider_entry(m,PT_PAULA_FUTURE_TASK);assert(m->adapter.arm(m->adapter.context,&m->original)==1);
        provider_exit(m,PT_PAULA_FUTURE_TASK,token);}
    if(mode==12||mode==13||mode==14||mode==17){struct pt_paula_future_packet copy;
        memcpy(&copy,&m->original,sizeof(copy));m->now=mode==13?p->first-1:(mode==14?p->last:p->first);
        token=provider_entry(m,PT_PAULA_FUTURE_DELIVERY);before=m->applies;
        if(mode==12){++copy.ticket;assert(m->adapter.apply(m->adapter.context,&copy)==-1&&m->applies==before);}
        else if(mode==14)assert(m->adapter.apply(m->adapter.context,&m->original)==-1&&m->applies==before);
        else if(mode==13){assert(m->adapter.rearm(m->adapter.context,&m->original)==1&&m->rearms==1);
            assert(m->adapter.apply(m->adapter.context,&m->original)==-1&&m->applies==before);}
        else {assert(m->adapter.apply(m->adapter.context,&m->original)==1);before=m->acks;
            assert(m->adapter.ack(m->adapter.context,p->ticket)==1&&m->adapter.ack(m->adapter.context,p->ticket)==1&&m->acks==before+1);}
        provider_exit(m,PT_PAULA_FUTURE_DELIVERY,token);}
    token=provider_entry(m,PT_PAULA_FUTURE_TASK);
    if(mode==16){before=m->cancels;assert(m->adapter.cancel(m->adapter.context,p->ticket)==1);
        assert(m->adapter.cancel(m->adapter.context,p->ticket)==1&&m->cancels==before+1);}
    if(mode==18){assert(!m->adapter.ticket_quiet(m->adapter.context,p->ticket));
        assert(pt_paula_segment_provider_row(m->adapter.context,0,&row)&&!row.quiet&&!row.original_released);}
    if(mode==19){before=m->reader_probes;assert(!m->adapter.reader_quiet(m->adapter.context)&&m->reader_probes==before);}
    if(mode==20){before=m->source_probes;assert(!m->adapter.source_quiet(m->adapter.context)&&m->source_probes==before);}
    if(mode==26){unsigned char old[sizeof(s)];void *alias=(void *)p->geometry.source[0].master.data;
        memcpy(old,alias,sizeof(old));assert(!pt_paula_segment_provider_get(m->adapter.context,alias));assert(!memcmp(old,alias,sizeof(old)));}
    m->ticket_release=1;raw=m->adapter.ticket_quiet(m->adapter.context,p->ticket);
    if(mode==22){assert(raw==-1);assert(pt_paula_segment_provider_row(m->adapter.context,0,&row)&&row.quiet_raw==1&&!row.quiet);
        assert(m->adapter.ticket_quiet(m->adapter.context,p->ticket)==1);}else assert(raw==1);
    provider_exit(m,PT_PAULA_FUTURE_TASK,token);
    if(mode==21)m->close_return=-1;
    provider_drain(m,p->ticket);assert(pinned(&provider_owner->f)==3);storage(&provider_owner->f);++provider_cases;
}
static int provider_submit(void *v,const struct pt_readers_event *event,const struct pt_paula_segment_plan *plan)
{
    struct pt_paula_future_packet *p=&provider_positive.original;struct pt_paula_provider_row_status row;uint64_t token;unsigned i;
    assert(v==&provider_owner->provider&&provider_packets<4&&pinned(&provider_owner->f)==3);
    if(!provider_packets)provider_model_init(&provider_positive,-1);
    p->ticket=event->scheduled.ticket;p->first=event->scheduled.first;p->last=event->scheduled.last;
    memcpy(&p->geometry,plan,sizeof(p->geometry));provider_positive.now=p->first-1;
    token=provider_entry(&provider_positive,PT_PAULA_FUTURE_TASK);assert(provider_positive.adapter.arm(provider_positive.adapter.context,p)==1);
    provider_exit(&provider_positive,PT_PAULA_FUTURE_TASK,token);
    provider_positive.now=p->first;token=provider_entry(&provider_positive,PT_PAULA_FUTURE_DELIVERY);
    assert(provider_positive.adapter.apply(provider_positive.adapter.context,p)==1&&provider_positive.adapter.ack(provider_positive.adapter.context,p->ticket)==1);
    assert(provider_positive.adapter.apply(provider_positive.adapter.context,p)==-1);provider_exit(&provider_positive,PT_PAULA_FUTURE_DELIVERY,token);
    token=provider_entry(&provider_positive,PT_PAULA_FUTURE_TASK);provider_positive.ticket_release=1;
    assert(provider_positive.adapter.ticket_quiet(provider_positive.adapter.context,p->ticket)==1);
    assert(pt_paula_segment_provider_row(provider_positive.adapter.context,provider_packets,&row)&&row.quiet&&!row.original_released);
    provider_exit(&provider_positive,PT_PAULA_FUTURE_TASK,token);
    assert(pt_paula_segment_provider_row(provider_positive.adapter.context,provider_packets,&row)&&row.original_released);
    if(!ran_negatives){for(i=0;i<28;++i)provider_negative(p,i);ran_negatives=1;}
    ++provider_packets;return sequence_submit(v,event,plan);
}
static void provider_measured_provider(unsigned reader_first)
{
    struct sequence_fixture x;uint64_t token;unsigned i;
    sequence_init(&x,0);provider_owner=&x;provider_packets=0;x.f.backend.submit_plan=provider_submit;sequence_open(&x);
    for(i=0;i<4;++i){assert(sequence_until(&x,PT_PAULA_SEQUENCE_READY)==PT_PAULA_SEQUENCE_READY);sequence_activate(&x,i,i<3);}
    token=provider_entry(&provider_positive,PT_PAULA_FUTURE_TASK);provider_positive.reader_release=provider_positive.source_release=1;
    assert(provider_positive.adapter.reader_quiet(provider_positive.adapter.context)==1);
    assert(provider_positive.adapter.source_close(provider_positive.adapter.context)==1&&provider_positive.adapter.source_quiet(provider_positive.adapter.context)==1);
    assert(!pt_paula_segment_provider_dispose(provider_positive.adapter.context));provider_exit(&provider_positive,PT_PAULA_FUTURE_TASK,token);
    assert(pt_paula_segment_provider_dispose(provider_positive.adapter.context));sequence_terminal(&x,reader_first);++provider_cases;
}
static int provider_retain_submit(void *v,const struct pt_readers_event *event,const struct pt_paula_segment_plan *plan)
{
    struct pt_paula_future_packet p;unsigned i;p.ticket=event->scheduled.ticket;p.first=event->scheduled.first;p.last=event->scheduled.last;
    memcpy(&p.geometry,plan,sizeof(p.geometry));for(i=0;i<28;++i)provider_negative(&p,i);provider_genuine_aliases(&p);ran_negatives=1;
    return sequence_submit(v,event,plan);
}
int main(void)
{
    /* Unknown entry/restore obligations keep their real source owner and copied
     * original packet storage live for the whole qualifier. No fake retirement,
     * disposal, budget-zero or OS-process exit is an absence certificate. */
    sequence_init(&provider_retained_owner,0);provider_owner=&provider_retained_owner;
    provider_retained_owner.f.backend.submit_plan=provider_retain_submit;sequence_open(&provider_retained_owner);
    assert(sequence_until(&provider_retained_owner,PT_PAULA_SEQUENCE_READY)==PT_PAULA_SEQUENCE_READY);
    sequence_activate(&provider_retained_owner,0,1);assert(pinned(&provider_retained_owner.f)==3&&provider_retained_owner.f.chip.bytes==2304);
    provider_measured_provider(0);provider_measured_provider(1);assert(provider_cases==75&&provider_alias_cases==45);
    puts("typed Paula geometry provider: 75 cases passed");
    puts("genuine measured four boundaries; both genuine retirement orders");
    puts("45 caller-output alias refusals preserve context, retained/retired sources and ownership");
    puts("owned BE records; exact entry token/window; independent quiet and final restore");
    puts("unknown restoration retains genuine source owner; no forced retirement");
    puts("HOST injectable resource model; future/native/IRQ/MMIO/timing NOT QUALIFIED");
    return 0;
}
