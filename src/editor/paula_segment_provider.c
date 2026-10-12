#include "paula_segment_provider.h"
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#define ALIGNMENT(T) offsetof(struct {char before;T object;},object)
struct provider_row {
    struct pt_paula_provider_record record;
    struct pt_paula_future_packet copy;
    const struct pt_paula_future_packet *original;
    struct pt_paula_provider_row_status state;
    unsigned clean_refusal,ack_attempted;
    int ack_result,cancel_result;
};
struct pt_paula_segment_provider {
    struct pt_paula_segment_provider *self;
    struct pt_paula_provider_resources ops;
    struct pt_paula_provider_config config;
    struct provider_row row[PT_PAULA_PROVIDER_RECORDS];
    uint64_t token,now;
    enum pt_paula_future_entry entry;
    unsigned live,busy,entered,restore_hold,clock_seen,count,failed,closing,
        reader_quiet,source_attempted,source_quiet;
    int enter_raw,restore_raw,source_close_raw,source_close_result;
};
/* A serialized HOST witness, checked before any borrowed callback context read.
 * It is not an atomic/native interrupt exclusion primitive. */
static struct pt_paula_segment_provider *active;
static unsigned reentered;
static int object(const void *p,size_t n,size_t a)
{uintptr_t x=(uintptr_t)p;return p&&n&&a&&!(x%a)&&n<=UINTPTR_MAX-x;}
static int apart(const void *a,size_t n,const void *b,size_t m)
{uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return object(a,n,1)&&object(b,m,1)&&(x+n<=y||y+m<=x);}
static int refused_reentry(void)
{if(!active)return 0;reentered=1;return 1;}
static int valid(const struct pt_paula_segment_provider *p)
{return object(p,sizeof(*p),pt_paula_segment_provider_alignment())&&p->self==p&&p->live;}
static int current(const struct pt_paula_segment_provider *p)
{
    unsigned i;
    for(i=0;i<PT_PAULA_PROVIDER_RECORDS;++i)
        if(p->row[i].original&&memcmp(p->row[i].original,&p->row[i].copy,sizeof(p->row[i].copy)))return 0;
    return 1;
}
static void begin(struct pt_paula_segment_provider *p,struct pt_paula_segment_provider *saved)
{p->busy=1;memcpy(saved,p,sizeof(*saved));active=p;reentered=0;}
static int finish(struct pt_paula_segment_provider *p,const struct pt_paula_segment_provider *saved)
{
    int changed=memcmp(p,saved,sizeof(*p))!=0;unsigned nested=reentered;
    if(changed)memcpy(p,saved,sizeof(*p));
    /* The witness stays installed through holder repair and original checks. */
    if(!current(p))changed=1;
    p->busy=0;active=NULL;reentered=0;
    if(changed||nested){p->failed=1;return 0;}return 1;
}
static int gate(struct pt_paula_segment_provider *p)
{return !refused_reentry()&&valid(p)&&!p->busy&&!p->restore_hold;}
static int in_entry(struct pt_paula_segment_provider *p,enum pt_paula_future_entry e)
{return gate(p)&&p->entered&&p->entry==e;}
static int packet_apart(const struct pt_paula_segment_provider *p,const struct pt_paula_future_packet *q)
{
    unsigned i,j;const struct pt_paula_segment_plan *g;
    if(!object(q,sizeof(*q),ALIGNMENT(struct pt_paula_future_packet))||!apart(p,sizeof(*p),q,sizeof(*q)))return 0;
    g=&q->geometry;if(!g->source_count||g->source_count>2)return 0;
    for(i=0;i<g->source_count;++i){const struct pt_paula_segment_source *s=g->source+i;size_t bytes;
        if(s->master.capacity>SIZE_MAX/sizeof(*s->master.data))return 0;
        bytes=s->master.capacity*sizeof(*s->master.data);
        if(!bytes||!apart(p,sizeof(*p),s->master.data,bytes)||
           !apart(p,sizeof(*p),s->descriptor,sizeof(*s->descriptor))||
           !apart(p,sizeof(*p),s->cache_data,s->cache_bytes)||s->guard_count>PT_PAULA_SEGMENT_SOURCE_GUARDS)return 0;
        for(j=0;j<s->guard_count;++j)if(!apart(p,sizeof(*p),s->guard[j].data,s->guard[j].bytes))return 0;
    }return 1;
}
static int mapping(const struct pt_paula_segment_provider *p,const struct pt_paula_future_packet *q,
                   struct pt_paula_packet32_mapping *m)
{
    unsigned i,j;memset(m,0,sizeof(*m));m->source_count=q->geometry.source_count;
    for(i=0;i<m->source_count;++i){const struct pt_paula_segment_source *s=q->geometry.source+i;
        struct pt_paula_packet32_source_map *r=m->source+i;const struct pt_paula_provider_label *label=NULL;
        for(j=0;j<PT_PAULA_PROVIDER_LABELS;++j)if(p->config.label[j].sample==s->sample)label=p->config.label+j;
        if(!label)return 0;
        r->descriptor=s->descriptor;r->master_data=s->master.data;r->version=s->version;
        r->sampler_owner=s->sampler_owner;r->project_identity=s->project_identity;
        r->cache_owner=s->cache_owner;r->cache_data=s->cache_data;
        r->descriptor_tag=label->descriptor;r->master_tag=label->master;r->version_tag=label->version;
        r->sampler_tag=label->sampler;r->project_tag=label->project;r->cache_owner_tag=label->cache_owner;
        r->cache_base32=label->cache_base32;
    }return 1;
}
static struct provider_row *find(struct pt_paula_segment_provider *p,uint64_t ticket)
{unsigned i;for(i=0;i<PT_PAULA_PROVIDER_RECORDS;++i)if(p->row[i].state.present&&p->row[i].record.ticket==ticket)return p->row+i;return NULL;}
static int output_apart(const struct pt_paula_segment_provider *,const void *,size_t);
static int enter_adapter(void *v,enum pt_paula_future_entry e,uint64_t *token)
{
    struct pt_paula_segment_provider *p=v,saved;uint64_t t=0;int raw,clean;
    if(!gate(p)||p->entered||(e!=PT_PAULA_FUTURE_TASK&&e!=PT_PAULA_FUTURE_DELIVERY)||
       !object(token,sizeof(*token),ALIGNMENT(uint64_t))||!output_apart(p,token,sizeof(*token)))return -1;
    begin(p,&saved);raw=p->ops.enter(p->ops.context,e,&t);clean=finish(p,&saved);p->enter_raw=raw;
    if(raw==1){p->entered=1;p->entry=e;p->token=t;p->clock_seen=0;if(clean){*token=t;return 1;}}
    if(raw!=0||!clean){p->entry=e;p->token=t;p->restore_hold=1;p->failed=1;return -1;}return 0;
}
static int leave_adapter(void *v,enum pt_paula_future_entry e,uint64_t token)
{
    struct pt_paula_segment_provider *p=v,saved;unsigned i;int raw,clean;
    if(!gate(p)||!p->entered||p->entry!=e||p->token!=token)return -1;
    begin(p,&saved);raw=p->ops.restore(p->ops.context,e,token);clean=finish(p,&saved);p->restore_raw=raw;
    if(raw!=1||!clean){p->restore_hold=1;p->failed=1;return -1;}
    p->entered=0;p->entry=0;p->clock_seen=0;
    for(i=0;i<PT_PAULA_PROVIDER_RECORDS;++i){struct provider_row *r=p->row+i;
        if(r->clean_refusal){memset(r,0,sizeof(*r));}
        else if(r->state.quiet){r->original=NULL;r->state.original_released=1;}}
    return 1;
}
static int clock_adapter(void *v,uint64_t *tick,uint32_t *frequency)
{
    struct pt_paula_segment_provider *p=v,saved;uint64_t t=0;uint32_t f=0;int raw,clean;
    if(!gate(p)||!p->entered||!object(tick,sizeof(*tick),ALIGNMENT(uint64_t))||
       !object(frequency,sizeof(*frequency),ALIGNMENT(uint32_t))||!apart(tick,sizeof(*tick),frequency,sizeof(*frequency))||
       !output_apart(p,tick,sizeof(*tick))||!output_apart(p,frequency,sizeof(*frequency)))return -1;
    begin(p,&saved);raw=p->ops.clock(p->ops.context,&t,&f);clean=finish(p,&saved);
    if(raw!=1||!clean||f!=p->config.frequency){p->failed=1;return -1;}
    p->now=t;p->clock_seen=1;*tick=t;*frequency=f;return 1;
}
static int arm_adapter(void *v,const struct pt_paula_future_packet *q)
{
    struct pt_paula_segment_provider *p=v,saved;struct provider_row *r;
    struct pt_paula_packet32_mapping m;union pt_paula_packet32_record encoded;int raw,clean;
    if(!in_entry(p,PT_PAULA_FUTURE_TASK)||p->failed||p->closing||!p->clock_seen||p->count>=PT_PAULA_PROVIDER_RECORDS||
       p->row[p->count].state.present||
       !packet_apart(p,q)||!current(p)||q->geometry.frame!=p->config.frame[p->count]||find(p,q->ticket)||
       !mapping(p,q,&m)||pt_paula_segment_packet32_build(q,&m,&encoded)!=PT_PAULA_PACKET32_OK)return 0;
    if(p->now>=q->first)return 0;
    r=p->row+p->count;memset(r,0,sizeof(*r));memcpy(&r->copy,q,sizeof(r->copy));r->original=q;
    memcpy(&r->record.packet,&encoded,sizeof(encoded));r->record.ticket=q->ticket;r->record.first=q->first;
    r->record.last=q->last;r->record.frame=q->geometry.frame;r->record.publication=p->count;
    r->state.present=1;r->state.attempted=1;r->state.ticket=q->ticket;r->state.first=q->first;
    r->state.last=q->last;r->state.frame=q->geometry.frame;
    begin(p,&saved);raw=p->ops.packet(p->ops.context,PT_PAULA_PROVIDER_ARM,&r->record);clean=finish(p,&saved);
    r=p->row+p->count;r->state.arm_raw=raw;
    if(raw==0&&clean){r->clean_refusal=1;return 0;}
    ++p->count;if(raw==1&&clean)return 1;p->failed=1;return -1;
}
static int packet_adapter(void *v,const struct pt_paula_future_packet *q,enum pt_paula_provider_packet_action a)
{
    struct pt_paula_segment_provider *p=v,saved;struct provider_row *r;unsigned index;int raw,clean;
    if(!in_entry(p,PT_PAULA_FUTURE_DELIVERY)||p->failed||!p->clock_seen||
       !object(q,sizeof(*q),ALIGNMENT(struct pt_paula_future_packet))||!apart(p,sizeof(*p),q,sizeof(*q))||!(r=find(p,q->ticket))||
       r->original!=q||r->state.quiet||r->state.applied||r->clean_refusal||!current(p)||
       (a==PT_PAULA_PROVIDER_REARM?p->now>=r->record.first:
        p->now<r->record.first||p->now>=r->record.last))return -1;
    index=(unsigned)(r-p->row);
    begin(p,&saved);raw=p->ops.packet(p->ops.context,a,&r->record);clean=finish(p,&saved);
    r=p->row+index;if(a==PT_PAULA_PROVIDER_REARM)r->state.rearm_raw=raw;
    else {r->state.apply_raw=raw;if(raw==1&&clean)r->state.applied=1;}
    if(raw==1&&clean)return 1;p->failed=1;return -1;
}
static int rearm_adapter(void *v,const struct pt_paula_future_packet *q)
{return packet_adapter(v,q,PT_PAULA_PROVIDER_REARM);}
static int apply_adapter(void *v,const struct pt_paula_future_packet *q)
{return packet_adapter(v,q,PT_PAULA_PROVIDER_APPLY);}
static int ticket_adapter(void *v,uint64_t ticket,enum pt_paula_provider_ticket_action a)
{
    struct pt_paula_segment_provider *p=v,saved;struct provider_row *r;int raw,clean;
    enum pt_paula_future_entry e=a==PT_PAULA_PROVIDER_ACK?PT_PAULA_FUTURE_DELIVERY:PT_PAULA_FUTURE_TASK;
    if(!in_entry(p,e)||!(r=find(p,ticket))||r->clean_refusal)return -1;
    if(a==PT_PAULA_PROVIDER_TICKET_QUIET&&r->state.quiet)return 1;
    if(a==PT_PAULA_PROVIDER_CANCEL&&r->state.cancel_attempted)return r->cancel_result;
    if(a==PT_PAULA_PROVIDER_ACK&&r->ack_attempted)return r->ack_result;
    if(a==PT_PAULA_PROVIDER_CANCEL){r->state.cancel_attempted=1;p->closing=1;}
    if(a==PT_PAULA_PROVIDER_ACK)r->ack_attempted=1;
    begin(p,&saved);raw=p->ops.ticket(p->ops.context,a,ticket);clean=finish(p,&saved);r=find(p,ticket);
    if(a==PT_PAULA_PROVIDER_ACK){r->state.ack_raw=raw;r->ack_result=clean&&(raw==0||raw==1)?raw:-1;}
    else if(a==PT_PAULA_PROVIDER_CANCEL){r->state.cancel_raw=raw;r->cancel_result=clean&&(raw==0||raw==1)?raw:-1;}
    else {r->state.quiet_raw=raw;if(raw==1&&clean)r->state.quiet=1;}
    if(!clean||(raw!=0&&raw!=1)){p->failed=1;return -1;}return raw;
}
static int ack_adapter(void *v,uint64_t t){return ticket_adapter(v,t,PT_PAULA_PROVIDER_ACK);}
static int cancel_adapter(void *v,uint64_t t){return ticket_adapter(v,t,PT_PAULA_PROVIDER_CANCEL);}
static int quiet_adapter(void *v,uint64_t t){return ticket_adapter(v,t,PT_PAULA_PROVIDER_TICKET_QUIET);}
static int all_quiet(const struct pt_paula_segment_provider *p)
{unsigned i;for(i=0;i<PT_PAULA_PROVIDER_RECORDS;++i)if(p->row[i].state.present&&!p->row[i].state.quiet)return 0;return 1;}
static int output_apart(const struct pt_paula_segment_provider *p,const void *out,size_t bytes)
{
    unsigned i,j,k;
    if(!apart(p,sizeof(*p),out,bytes)||!apart(p->ops.context,p->ops.context_bytes,out,bytes))return 0;
    for(i=0;i<PT_PAULA_PROVIDER_RECORDS;++i)if(p->row[i].state.present){
        const struct provider_row *r=p->row+i;
        if(r->original&&!apart(out,bytes,r->original,sizeof(*r->original)))return 0;
        for(j=0;j<r->copy.geometry.source_count;++j){const struct pt_paula_segment_source *s=r->copy.geometry.source+j;
            if(!apart(out,bytes,s->descriptor,sizeof(*s->descriptor))||
               !apart(out,bytes,s->master.data,s->master.capacity*sizeof(*s->master.data))||
               !apart(out,bytes,s->cache_data,s->cache_bytes))return 0;
            for(k=0;k<s->guard_count;++k)if(!apart(out,bytes,s->guard[k].data,s->guard[k].bytes))return 0;
        }
    }
    return 1;
}
static int reader_adapter(void *v)
{
    struct pt_paula_segment_provider *p=v,saved;int raw,clean;
    if(!in_entry(p,PT_PAULA_FUTURE_TASK))return 0;p->closing=1;
    if(!all_quiet(p))return 0;if(p->reader_quiet)return 1;
    begin(p,&saved);raw=p->ops.reader_quiet(p->ops.context);clean=finish(p,&saved);
    if(raw==1&&clean){p->reader_quiet=1;return 1;}if(raw!=0||!clean)p->failed=1;return raw==0&&clean?0:-1;
}
static int source_close_adapter(void *v)
{
    struct pt_paula_segment_provider *p=v,saved;int raw,clean;
    if(!in_entry(p,PT_PAULA_FUTURE_TASK)||!all_quiet(p)||!p->reader_quiet)return 0;
    if(p->source_attempted)return p->source_close_result;p->source_attempted=1;
    begin(p,&saved);raw=p->ops.source_close(p->ops.context);clean=finish(p,&saved);p->source_close_raw=raw;
    p->source_close_result=clean&&(raw==0||raw==1)?raw:-1;
    if(!clean||(raw!=0&&raw!=1))p->failed=1;return p->source_close_result;
}
static int source_quiet_adapter(void *v)
{
    struct pt_paula_segment_provider *p=v,saved;int raw,clean;
    if(!in_entry(p,PT_PAULA_FUTURE_TASK)||!p->source_attempted||!p->reader_quiet||!all_quiet(p))return 0;
    if(p->source_quiet)return 1;begin(p,&saved);raw=p->ops.source_quiet(p->ops.context);clean=finish(p,&saved);
    if(raw==1&&clean){p->source_quiet=1;return 1;}if(raw!=0||!clean)p->failed=1;return raw==0&&clean?0:-1;
}
size_t pt_paula_segment_provider_size(void){return sizeof(struct pt_paula_segment_provider);}
size_t pt_paula_segment_provider_alignment(void){return ALIGNMENT(struct pt_paula_segment_provider);}
int pt_paula_segment_provider_init(void *storage,size_t bytes,const struct pt_paula_provider_resources *ops,
    const struct pt_paula_provider_config *config,struct pt_paula_future_ops *out)
{
    struct pt_paula_segment_provider *p=storage;struct pt_paula_future_ops result;unsigned i,j;
    if(refused_reentry()||bytes<sizeof(*p)||!object(p,sizeof(*p),pt_paula_segment_provider_alignment())||
       !object(ops,sizeof(*ops),ALIGNMENT(struct pt_paula_provider_resources))||
       !object(config,sizeof(*config),ALIGNMENT(struct pt_paula_provider_config))||
       !object(out,sizeof(*out),ALIGNMENT(struct pt_paula_future_ops))||
       !apart(p,sizeof(*p),ops,sizeof(*ops))||!apart(p,sizeof(*p),config,sizeof(*config))||
       !apart(p,sizeof(*p),out,sizeof(*out))||!apart(out,sizeof(*out),ops,sizeof(*ops))||
       !apart(out,sizeof(*out),config,sizeof(*config)))return 0;
    if(!object(ops->context,ops->context_bytes,1)||!apart(p,sizeof(*p),ops->context,ops->context_bytes)||
       !apart(out,sizeof(*out),ops->context,ops->context_bytes)||!ops->enter||!ops->restore||!ops->clock||
       !ops->packet||!ops->ticket||!ops->reader_quiet||!ops->source_close||!ops->source_quiet||!config->frequency)return 0;
    for(i=0;i<sizeof(*p);++i)if(((const unsigned char *)p)[i])return 0;
    for(i=0;i<PT_PAULA_PROVIDER_RECORDS;++i)if(i&&config->frame[i]<=config->frame[i-1])return 0;
    for(i=0;i<PT_PAULA_PROVIDER_LABELS;++i){const struct pt_paula_provider_label *l=config->label+i;
        if(!l->descriptor||!l->master||!l->version||!l->sampler||!l->project||!l->cache_owner||
           !l->cache_base32||(l->cache_base32&1U))return 0;
        for(j=0;j<i;++j)if(l->sample==config->label[j].sample)return 0;
    }
    memset(&result,0,sizeof(result));result.context=p;result.context_bytes=sizeof(*p);
    result.enter=enter_adapter;result.leave=leave_adapter;result.clock=clock_adapter;
    result.arm=arm_adapter;result.rearm=rearm_adapter;result.apply=apply_adapter;
    result.ack=ack_adapter;result.cancel=cancel_adapter;result.ticket_quiet=quiet_adapter;
    result.reader_quiet=reader_adapter;result.source_close=source_close_adapter;result.source_quiet=source_quiet_adapter;
    p->self=p;p->live=1;memcpy(&p->ops,ops,sizeof(*ops));memcpy(&p->config,config,sizeof(*config));
    memcpy(out,&result,sizeof(result));return 1;
}
int pt_paula_segment_provider_get(const struct pt_paula_segment_provider *p,struct pt_paula_provider_status *out)
{
    struct pt_paula_provider_status s;unsigned i;
    if(refused_reentry()||!valid(p)||p->busy||!object(out,sizeof(*out),ALIGNMENT(struct pt_paula_provider_status))||
       !output_apart(p,out,sizeof(*out)))return 0;
    memset(&s,0,sizeof(s));s.published=p->count;s.entered=p->entered;s.restore_hold=p->restore_hold;s.failed=p->failed;
    s.reader_quiet=p->reader_quiet;s.source_attempted=p->source_attempted;s.source_quiet=p->source_quiet;
    s.enter_raw=p->enter_raw;s.restore_raw=p->restore_raw;s.source_close_raw=p->source_close_raw;
    s.entry_kind=(unsigned)p->entry;s.entry_token=p->token;
    for(i=0;i<PT_PAULA_PROVIDER_RECORDS;++i)if(p->row[i].state.present){++s.retained;if(p->row[i].state.quiet)++s.quiet;}
    memcpy(out,&s,sizeof(s));return 1;
}
int pt_paula_segment_provider_row(const struct pt_paula_segment_provider *p,unsigned at,struct pt_paula_provider_row_status *out)
{
    if(refused_reentry()||!valid(p)||p->busy||at>=PT_PAULA_PROVIDER_RECORDS||
       !object(out,sizeof(*out),ALIGNMENT(struct pt_paula_provider_row_status))||!output_apart(p,out,sizeof(*out)))return 0;
    memcpy(out,&p->row[at].state,sizeof(*out));return 1;
}
int pt_paula_segment_provider_dispose(struct pt_paula_segment_provider *p)
{
    if(!gate(p)||p->entered||!all_quiet(p)||!p->reader_quiet||!p->source_attempted||!p->source_quiet)return 0;
    p->live=0;p->self=NULL;return 1;
}
