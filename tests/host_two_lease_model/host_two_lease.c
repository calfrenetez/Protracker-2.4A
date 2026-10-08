/* PRIVATE HOST-only transaction executor; no native or device dependencies. */
#include "host_two_lease.h"

static int span(const void *p, size_t n)
{ return p && n && (uintptr_t)p <= UINTPTR_MAX - (n - 1U); }
static int apart(const void *p, size_t n, const void *q, size_t m)
{
    uintptr_t a = (uintptr_t)p, b = (uintptr_t)q;
    return span(p,n) && span(q,m) && (a <= b ? n <= b-a : m <= a-b);
}
static int zero(const void *p, size_t n)
{
    const unsigned char *b = p;
    size_t i;
    for (i=0; i<n; ++i) if (b[i]) return 0;
    return 1;
}
static int registration_same(const struct pt_host_pair_registration *a,
    const struct pt_host_pair_registration *b)
{
    return a->owner==b->owner && a->queue==b->queue &&
        a->session==b->session && a->generation==b->generation;
}
static int image_same(const struct pt_host_pair_image *a,
    const struct pt_host_pair_image *b)
{
    return a->programmed_latch==b->programmed_latch && a->control==b->control &&
        a->mask==b->mask && a->pending==b->pending &&
        a->vector_data==b->vector_data && a->vector_code==b->vector_code;
}
static int binding_same(const struct pt_host_pair_binding *a,
    const struct pt_host_pair_binding *b)
{
    return registration_same(&a->registration,&b->registration) &&
        a->ticket==b->ticket && a->first==b->first && a->frequency==b->frequency &&
        a->resource==b->resource && a->chip==b->chip && a->bit==b->bit &&
        a->server==b->server && a->server_code==b->server_code &&
        a->server_data==b->server_data;
}
static int input_valid(const struct pt_host_pair_input *v)
{
    unsigned i;
    if (!v->source || !v->original_task || !v->timer || !v->port ||
        !v->registration.owner || !v->registration.queue ||
        !v->registration.session || !v->registration.generation) return 0;
    for (i=0; i<2; ++i) {
        const struct pt_host_pair_binding *b = &v->binding[i];
        if (!registration_same(&b->registration,&v->registration) || !b->ticket ||
            (b->frequency!=709379U && b->frequency!=715909U) ||
            !b->resource || !b->server || !b->server_code || !b->server_data ||
            b->chip>1U || b->bit>1U) return 0;
    }
    return v->binding[0].ticket!=v->binding[1].ticket &&
        v->binding[0].first<v->binding[1].first &&
        v->binding[0].frequency==v->binding[1].frequency &&
        (v->binding[0].chip!=v->binding[1].chip ||
         v->binding[0].bit!=v->binding[1].bit) &&
        v->binding[0].server!=v->binding[1].server &&
        v->binding[0].server_code!=v->binding[1].server_code &&
        v->binding[0].server_data!=v->binding[1].server_data;
}
static int input_same(const struct pt_host_pair_input *a,
    const struct pt_host_pair_input *b)
{
    unsigned i;
    if (a->source!=b->source || a->original_task!=b->original_task ||
        a->timer!=b->timer || a->port!=b->port ||
        !registration_same(&a->registration,&b->registration)) return 0;
    for (i=0; i<2; ++i)
        if (!binding_same(&a->binding[i],&b->binding[i]) ||
            !image_same(&a->before[i],&b->before[i])) return 0;
    return 1;
}
static int ops_same(const struct pt_host_pair_ops *a, const struct pt_host_pair_ops *b)
{
    return a->context==b->context && a->context_bytes==b->context_bytes &&
        a->inspect==b->inspect && a->acquire==b->acquire &&
        a->remove_server==b->remove_server && a->return_resource==b->return_resource;
}
static int effect_valid(enum pt_host_pair_recorded_effect e)
{
    switch (e) {
    case PT_HOST_PAIR_NOT_CALLED:
    case PT_HOST_PAIR_RECORDED_NO_EFFECTS:
    case PT_HOST_PAIR_RECORDED_DONE:
    case PT_HOST_PAIR_RECORDED_UNKNOWN: return 1;
    default: return 0;
    }
}
static int current(const struct pt_host_pair_transaction *t)
{
    unsigned i;
    if (!t || t->self!=t || t->initialized!=1U || !input_valid(&t->original) ||
        !input_same(&t->input,&t->original) || !ops_same(&t->ops,&t->original_ops) ||
        !t->ops.inspect || !t->ops.acquire || !t->ops.remove_server || !t->ops.return_resource ||
        !apart(t,sizeof(*t),t->ops.context,t->ops.context_bytes) ||
        t->attempted>1U || t->snapshots_known>1U || t->close_intent>1U ||
        t->fault>1U || t->call_busy>1U) return 0;
    for (i=0; i<2; ++i) {
        const struct pt_host_pair_record *r = &t->record[i];
        if (!binding_same(&r->binding,&t->original.binding[i]) ||
            r->restore_intent>1U || !effect_valid(r->acquire) ||
            !effect_valid(r->remove_server) || !effect_valid(r->return_resource) ||
            (r->acquire!=PT_HOST_PAIR_NOT_CALLED && !t->attempted) ||
            (r->remove_server!=PT_HOST_PAIR_NOT_CALLED &&
             (!r->restore_intent || r->acquire!=PT_HOST_PAIR_RECORDED_DONE)) ||
            (r->return_resource!=PT_HOST_PAIR_NOT_CALLED &&
             r->remove_server!=PT_HOST_PAIR_RECORDED_DONE)) return 0;
    }
    return 1;
}
static int snapshot_same(const struct pt_host_pair_snapshot *a,
    const struct pt_host_pair_snapshot *b)
{
    return image_same(&a->image,&b->image) &&
        a->resource_owner==b->resource_owner && a->server_owner==b->server_owner &&
        a->installed_server==b->installed_server && a->available==b->available &&
        a->resource_held==b->resource_held && a->server_installed==b->server_installed &&
        a->image_complete==b->image_complete && a->whole_exclusion==b->whole_exclusion &&
        a->full_return==b->full_return && a->callbacks_inflight==b->callbacks_inflight &&
        a->delivery_queued==b->delivery_queued;
}
static int facts(const struct pt_host_pair_snapshot *s)
{
    return s->available<=1U && s->resource_held<=1U && s->server_installed<=1U &&
        s->image_complete==1U && s->whole_exclusion==1U && s->full_return==1U &&
        !s->callbacks_inflight && !s->delivery_queued;
}
static int idle(const struct pt_host_pair_transaction *t, unsigned i,
    const struct pt_host_pair_snapshot *s)
{
    return facts(s) && !s->resource_held && !s->server_installed &&
        !s->resource_owner && !s->server_owner && !s->installed_server &&
        image_same(&s->image,&t->original.before[i]);
}
static int owned(const struct pt_host_pair_transaction *t, unsigned i,
    const struct pt_host_pair_snapshot *s)
{
    const struct pt_host_pair_binding *b = &t->original.binding[i];
    return facts(s) && !s->available && s->resource_held==1U && s->server_installed==1U &&
        s->resource_owner==t->original.source && s->server_owner==t->original.source &&
        s->installed_server==b->server && s->image.vector_code==b->server_code &&
        s->image.vector_data==b->server_data;
}
static int removed(const struct pt_host_pair_transaction *t, unsigned i,
    const struct pt_host_pair_snapshot *s)
{
    return facts(s) && !s->available && s->resource_held==1U && !s->server_installed &&
        s->resource_owner==t->original.source && !s->server_owner && !s->installed_server &&
        image_same(&s->image,&t->original.before[i]);
}
static int observe(struct pt_host_pair_transaction *t, struct pt_host_pair_snapshot out[2])
{
    unsigned i;
    for (i=0; i<2; ++i) {
        struct pt_host_pair_snapshot empty = {0};
        out[i]=empty;
        if (t->ops.inspect(t->ops.context,&t->original.binding[i],&out[i])!=1 ||
            !current(t)) return 0;
    }
    return 1;
}
static int unchanged(struct pt_host_pair_transaction *t)
{
    struct pt_host_pair_snapshot now[2];
    unsigned i;
    if (!t->snapshots_known || !observe(t,now)) return 0;
    for (i=0; i<2; ++i)
        if (!facts(&now[i]) || !snapshot_same(&now[i],&t->last[i])) return 0;
    return 1;
}
static int uncertain(struct pt_host_pair_transaction *t)
{ t->fault=1U; return PT_HOST_PAIR_RETAINED; }
static int cached(const struct pt_host_pair_record *r)
{
    if (r->acquire==PT_HOST_PAIR_NOT_CALLED || r->acquire==PT_HOST_PAIR_RECORDED_NO_EFFECTS)
        return PT_HOST_PAIR_REFUSED;
    if (r->acquire==PT_HOST_PAIR_RECORDED_DONE &&
        r->remove_server==PT_HOST_PAIR_RECORDED_DONE &&
        r->return_resource==PT_HOST_PAIR_RECORDED_DONE) return PT_HOST_PAIR_RESTORED;
    return PT_HOST_PAIR_RETAINED;
}
static int cleanup(struct pt_host_pair_transaction *t, unsigned slot)
{
    struct pt_host_pair_record *r = &t->record[slot];
    struct pt_host_pair_snapshot now[2];
    unsigned sibling = 1U-slot;
    int raw;
    if (r->restore_intent) return t->fault ? PT_HOST_PAIR_RETAINED : cached(r);
    r->restore_intent=1U;
    if (t->fault || r->acquire==PT_HOST_PAIR_RECORDED_UNKNOWN) return PT_HOST_PAIR_RETAINED;
    if (r->acquire!=PT_HOST_PAIR_RECORDED_DONE) return PT_HOST_PAIR_REFUSED;
    if (!unchanged(t) || !owned(t,slot,&t->last[slot])) return uncertain(t);
    /* Mark attempt BEFORE the actual mutating primitive. This enum is not
     * completion evidence; no repeat can issue a second removal. */
    r->remove_server=PT_HOST_PAIR_RECORDED_UNKNOWN;
    raw=t->ops.remove_server(t->ops.context,&t->original.binding[slot],
        t->original.source,&t->original.before[slot]);
    if (!current(t) || !observe(t,now) ||
        !snapshot_same(&now[sibling],&t->last[sibling])) return uncertain(t);
    if (raw==0 && snapshot_same(&now[slot],&t->last[slot])) {
        r->remove_server=PT_HOST_PAIR_RECORDED_NO_EFFECTS;
        return PT_HOST_PAIR_RETAINED;
    }
    if (raw!=1 || !removed(t,slot,&now[slot])) return uncertain(t);
    r->remove_server=PT_HOST_PAIR_RECORDED_DONE;
    t->last[slot]=now[slot];
    if (!unchanged(t)) return uncertain(t);
    r->return_resource=PT_HOST_PAIR_RECORDED_UNKNOWN;
    raw=t->ops.return_resource(t->ops.context,&t->original.binding[slot],t->original.source);
    if (!current(t) || !observe(t,now) ||
        !snapshot_same(&now[sibling],&t->last[sibling])) return uncertain(t);
    if (raw==0 && snapshot_same(&now[slot],&t->last[slot])) {
        r->return_resource=PT_HOST_PAIR_RECORDED_NO_EFFECTS;
        return PT_HOST_PAIR_RETAINED;
    }
    if (raw!=1 || !snapshot_same(&now[slot],&t->initial[slot])) return uncertain(t);
    r->return_resource=PT_HOST_PAIR_RECORDED_DONE;
    t->last[slot]=now[slot];
    return PT_HOST_PAIR_RESTORED;
}

int pt_host_pair_init(struct pt_host_pair_transaction *t,
    const struct pt_host_pair_input *input, const struct pt_host_pair_ops *ops)
{
    unsigned i;
    if (!span(t,sizeof(*t)) || !span(input,sizeof(*input)) || !span(ops,sizeof(*ops)) ||
        !apart(t,sizeof(*t),input,sizeof(*input)) || !apart(t,sizeof(*t),ops,sizeof(*ops)) ||
        !zero(t,sizeof(*t)) || !input_valid(input) ||
        !ops->inspect || !ops->acquire || !ops->remove_server || !ops->return_resource ||
        !apart(t,sizeof(*t),ops->context,ops->context_bytes)) return 0;
    t->self=t; t->input=*input; t->original=*input;
    t->ops=*ops; t->original_ops=*ops;
    for (i=0; i<2; ++i) t->record[i].binding=input->binding[i];
    t->initialized=1U;
    return 1;
}
const struct pt_host_pair_record *pt_host_pair_lookup(const struct pt_host_pair_transaction *t,
    const struct pt_host_pair_registration *registration, uint64_t ticket)
{
    unsigned i;
    if (!registration || !ticket || !current(t) ||
        !registration_same(registration,&t->original.registration)) return NULL;
    for (i=0; i<2; ++i) if (ticket==t->original.binding[i].ticket) return &t->record[i];
    return NULL;
}
int pt_host_pair_acquire(struct pt_host_pair_transaction *t)
{
    struct pt_host_pair_snapshot now[2];
    unsigned i;
    int raw, result;
    if (!current(t) || t->call_busy || t->attempted || t->close_intent ||
        t->record[0].restore_intent || t->record[1].restore_intent) return PT_HOST_PAIR_INVALID;
    t->call_busy=1U; t->attempted=1U;
    if (!observe(t,now)) { result=uncertain(t); goto done; }
    if (!idle(t,0,&now[0]) || !idle(t,1,&now[1])) { result=PT_HOST_PAIR_REFUSED; goto done; }
    for (i=0; i<2; ++i) { t->initial[i]=now[i]; t->last[i]=now[i]; }
    t->snapshots_known=1U;
    for (i=0; i<2; ++i) {
        struct pt_host_pair_record *r = &t->record[i];
        unsigned sibling = 1U-i;
        if (!unchanged(t)) { result=uncertain(t); goto done; }
        r->acquire=PT_HOST_PAIR_RECORDED_UNKNOWN;
        raw=t->ops.acquire(t->ops.context,&t->original.binding[i],
            t->original.source,&t->original.before[i]);
        if (!current(t) || !observe(t,now) ||
            !snapshot_same(&now[sibling],&t->last[sibling])) {
            result=uncertain(t); goto done;
        }
        if (raw==0 && snapshot_same(&now[i],&t->last[i])) {
            r->acquire=PT_HOST_PAIR_RECORDED_NO_EFFECTS;
            /* First absence needs no rollback. Second absence cleans the
             * separately acquired first lease, never a logical-slot alias. */
            result=i ? cleanup(t,0) : PT_HOST_PAIR_REFUSED;
            if (result==PT_HOST_PAIR_RESTORED) result=PT_HOST_PAIR_REFUSED;
            goto done;
        }
        if (raw!=1 || !owned(t,i,&now[i])) { result=uncertain(t); goto done; }
        r->acquire=PT_HOST_PAIR_RECORDED_DONE;
        t->last[i]=now[i];
    }
    result=PT_HOST_PAIR_ACQUIRED;
done:
    t->call_busy=0U;
    return result;
}
int pt_host_pair_restore_once(struct pt_host_pair_transaction *t,
    const struct pt_host_pair_registration *registration, uint64_t ticket)
{
    const struct pt_host_pair_record *r = pt_host_pair_lookup(t,registration,ticket);
    unsigned slot;
    int result;
    if (!r || t->call_busy) return PT_HOST_PAIR_INVALID;
    slot=(unsigned)(r-t->record);
    t->call_busy=1U;
    result=cleanup(t,slot);
    t->call_busy=0U;
    return result;
}
int pt_host_pair_close_once(struct pt_host_pair_transaction *t)
{
    int a, b;
    if (!current(t) || t->call_busy) return PT_HOST_PAIR_INVALID;
    if (t->close_intent) {
        if (t->fault) return PT_HOST_PAIR_RETAINED;
        a=cached(&t->record[0]); b=cached(&t->record[1]);
    } else {
        t->close_intent=1U; t->call_busy=1U;
        b=cleanup(t,1); a=cleanup(t,0);
        t->call_busy=0U;
    }
    if (t->fault || a==PT_HOST_PAIR_RETAINED || b==PT_HOST_PAIR_RETAINED)
        return PT_HOST_PAIR_RETAINED;
    return (a==PT_HOST_PAIR_RESTORED || b==PT_HOST_PAIR_RESTORED) ?
        PT_HOST_PAIR_RESTORED : PT_HOST_PAIR_REFUSED;
}
int pt_host_pair_probe(const struct pt_host_pair_transaction *t,
    const struct pt_host_pair_registration *registration, uint64_t ticket,
    struct pt_host_pair_snapshot *out)
{
    const struct pt_host_pair_record *r = pt_host_pair_lookup(t,registration,ticket);
    struct pt_host_pair_snapshot now = {0};
    unsigned slot;
    if (!r || t->call_busy || !apart(t,sizeof(*t),out,sizeof(*out)) ||
        !apart(t->ops.context,t->ops.context_bytes,out,sizeof(*out))) return PT_HOST_PAIR_INVALID;
    slot=(unsigned)(r-t->record);
    if (t->ops.inspect(t->ops.context,&t->original.binding[slot],&now)!=1 ||
        !current(t)) return PT_HOST_PAIR_RETAINED;
    *out=now;
    return facts(&now) ? PT_HOST_PAIR_OBSERVED : PT_HOST_PAIR_RETAINED;
}
