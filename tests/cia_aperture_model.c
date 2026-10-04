#include "cia_aperture_model.h"
#include <string.h>
static int span(const void *p,size_t n)
{return p && n && (uintptr_t)p<=UINTPTR_MAX-(n-1);}
static int apart(const void *a,size_t an,const void *b,size_t bn)
{
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    if(!span(a,an) || !span(b,bn))return 0;
    return x>y?x-y>=bn:y-x>=an;
}
static int key_valid(const struct pt_aperture_key *k)
{return k->queue && k->session && k->generation && k->trigger && k->owner && k->serial && k->action<4 && k->slot<4;}
static int key_equal(const struct pt_aperture_key *a,const struct pt_aperture_key *b)
{return a->queue==b->queue && a->session==b->session && a->generation==b->generation && a->trigger==b->trigger && a->owner==b->owner && a->serial==b->serial && a->action==b->action && a->slot==b->slot;}
static int policy_valid(const struct pt_aperture_policy *p)
{return p->early_ticks && p->early_ticks<=PT_APERTURE_MAX_TICKS && p->maximum_residency_ticks>=p->early_ticks && p->maximum_residency_ticks<=PT_APERTURE_MAX_TICKS && p->maximum_reads>=2 && p->maximum_reads<=PT_APERTURE_MAX_READS;}
static int current(const struct pt_aperture_state *s,const struct pt_aperture_control *c)
{return c->armed==1 && !c->cancelled && c->publication==s->publication && key_equal(&c->key,&s->expected);}
enum pt_aperture_result pt_aperture_prepare(const struct pt_elapsed_clock *epoch,
    uint64_t frame,const struct pt_aperture_policy *policy,const struct pt_aperture_control *c,
    struct pt_aperture_state *out)
{
    struct pt_aperture_state value;uint64_t first,last;
    if(!span(epoch,sizeof(*epoch)) || !span(policy,sizeof(*policy)) || !span(c,sizeof(*c)) || !span(out,sizeof(*out)) ||
       !apart(out,sizeof(*out),epoch,sizeof(*epoch)) || !apart(out,sizeof(*out),policy,sizeof(*policy)) || !apart(out,sizeof(*out),c,sizeof(*c)) ||
       !policy_valid(policy) || !key_valid(&c->key) || !c->publication || c->armed!=1 || c->cancelled || frame==UINT64_MAX ||
       pt_elapsed_clock_deadline(epoch,frame,&first)!=PT_ELAPSED_OK ||
       pt_elapsed_clock_deadline(epoch,frame+1,&last)!=PT_ELAPSED_OK || first>=last || first<policy->early_ticks)return PT_APERTURE_INVALID;
    memset(&value,0,sizeof(value));value.expected=c->key;value.publication=c->publication;
    value.frame=frame;value.first=first;value.last=last;value.arm_at=first-policy->early_ticks;
    value.frequency=epoch->frequency;value.policy=*policy;value.result=PT_APERTURE_READY;
    *out=value;return PT_APERTURE_READY;
}
static enum pt_aperture_result finish(struct pt_aperture_state *s,enum pt_aperture_result r)
{s->result=r;s->busy=0;return r;}
static int actual(struct pt_aperture_state *s,int (*read)(void *,uint64_t *,uint32_t *),void *context,
    uint64_t *now,uint32_t *frequency)
{
    int ok;
    *now=0;*frequency=0;++s->reads;
    ok=read(context,now,frequency)==1;
    s->last_read=*now;s->last_read_frequency=*frequency;s->last_read_valid=(unsigned)ok;
    return ok;
}
enum pt_aperture_result pt_aperture_run(struct pt_aperture_state *s,
    const struct pt_aperture_control *c,int (*read)(void *,uint64_t *,uint32_t *),void *context,size_t context_bytes)
{
    uint64_t now,previous;uint32_t frequency;int ok;
    if(!span(s,sizeof(*s)) || !span(c,sizeof(*c)) || !span(context,context_bytes) || !read ||
       !apart(s,sizeof(*s),c,sizeof(*c)) || !apart(s,sizeof(*s),context,context_bytes) || !apart(c,sizeof(*c),context,context_bytes))return PT_APERTURE_INVALID;
    if(s->busy){s->reentry=1;return PT_APERTURE_REENTRY;}
    if(s->result!=PT_APERTURE_READY || !policy_valid(&s->policy) || !key_valid(&s->expected) || !s->publication || !s->frequency ||
       s->first>=s->last || s->first<s->policy.early_ticks || s->arm_at!=s->first-s->policy.early_ticks || s->reads || s->commits || s->shadow)return PT_APERTURE_INVALID;
    s->busy=1;ok=actual(s,read,context,&now,&frequency);
    s->entry=now;s->entry_frequency=frequency;s->entry_valid=(unsigned)ok;s->last_observed=now;
    if(s->reentry)return finish(s,PT_APERTURE_REENTRY);
    if(!ok || frequency!=s->frequency)return finish(s,PT_APERTURE_CLOCK);
    if(now<s->arm_at)return finish(s,PT_APERTURE_EARLY);
    previous=now;
    for(;;) {
        s->before=now;s->before_valid=1;s->last_observed=now;
        if(!current(s,c))return finish(s,PT_APERTURE_STALE);
        if(now<s->entry || now-s->entry>s->policy.maximum_residency_ticks)return finish(s,PT_APERTURE_RESIDENCY);
        if(now>=s->last)return finish(s,PT_APERTURE_EXPIRED);
        if(now>=s->first)break;
        if(s->reads>=s->policy.maximum_reads-1)return finish(s,PT_APERTURE_READ_LIMIT);
        ok=actual(s,read,context,&now,&frequency);
        if(s->reentry)return finish(s,PT_APERTURE_REENTRY);
        if(!ok || frequency!=s->frequency || now<previous)return finish(s,PT_APERTURE_CLOCK);
        previous=now;
    }
    /* Under native publication exclusion, no task may change this key between
     * this check and the store. A host callback can inject precommit changes;
     * the loop checks those after each actual read, never after acceptance only. */
    if(!current(s,c))return finish(s,PT_APERTURE_STALE);
    s->shadow=PT_APERTURE_SHADOW;s->commits=1;
    ok=actual(s,read,context,&now,&frequency);s->after=now;s->after_frequency=frequency;s->after_valid=(unsigned)ok;
    s->last_observed=now;
    if(s->reentry)return finish(s,PT_APERTURE_REENTRY);
    if(!ok || frequency!=s->frequency || now<previous)return finish(s,PT_APERTURE_CLOCK);
    if(!current(s,c))return finish(s,PT_APERTURE_CHANGED_AFTER);
    if(now<s->first || now>=s->last)return finish(s,PT_APERTURE_POST_WINDOW);
    if(now-s->entry>s->policy.maximum_residency_ticks)return finish(s,PT_APERTURE_RESIDENCY);
    return finish(s,PT_APERTURE_COMMITTED);
}
