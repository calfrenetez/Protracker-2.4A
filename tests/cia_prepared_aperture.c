#include "cia_prepared_aperture.h"
#include <string.h>
static int span(const void *p,size_t n)
{return p && n && (uintptr_t)p<=UINTPTR_MAX-(n-1);}
static int apart(const void *a,size_t an,const void *b,size_t bn)
{
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return span(a,an) && span(b,bn) && (x>y?x-y>=bn:y-x>=an);
}
static int key_valid(const struct pt_aperture_key *k)
{return k->queue && k->session && k->generation && k->trigger && k->owner && k->serial && k->action<4 && k->slot<4;}
static int same(const struct pt_aperture_key *a,const struct pt_aperture_key *b)
{return a->queue==b->queue && a->session==b->session && a->generation==b->generation && a->trigger==b->trigger && a->owner==b->owner && a->serial==b->serial && a->action==b->action && a->slot==b->slot;}
enum pt_aperture_result pt_prepared_clock_bind(const struct pt_aperture_state *s,
    const struct pt_aperture_control *c,struct pt_prepared_clock *out)
{
    struct pt_prepared_clock d;
    if(!span(s,sizeof(*s)) || !span(c,sizeof(*c)) || !span(out,sizeof(*out)) ||
       !apart(out,sizeof(*out),s,sizeof(*s)) || !apart(out,sizeof(*out),c,sizeof(*c)) ||
       !apart(s,sizeof(*s),c,sizeof(*c)) || s->result!=PT_APERTURE_READY ||
       s->busy || s->reentry || s->reads || s->commits || s->shadow ||
       !key_valid(&s->expected) || !s->publication || !s->frequency ||
       !s->policy.early_ticks || s->policy.early_ticks>PT_APERTURE_MAX_TICKS ||
       s->policy.maximum_residency_ticks<s->policy.early_ticks || s->policy.maximum_residency_ticks>PT_APERTURE_MAX_TICKS ||
       s->policy.maximum_reads<2 || s->policy.maximum_reads>PT_APERTURE_MAX_READS ||
       s->first>=s->last || s->first<s->policy.early_ticks || s->arm_at!=s->first-s->policy.early_ticks ||
       s->last-s->arm_at>s->policy.maximum_residency_ticks)return PT_APERTURE_INVALID;
    memset(&d,0,sizeof(d));d.arm_hi=(uint32_t)(s->arm_at>>32);d.arm_lo=(uint32_t)s->arm_at;
    d.first_offset=(uint32_t)(s->first-s->arm_at);d.last_offset=(uint32_t)(s->last-s->arm_at);
    d.frequency=s->frequency;d.read_cap=s->policy.maximum_reads;
    d.gate=c->armed==1 && !c->cancelled && c->publication==s->publication && same(&s->expected,&c->key)?PT_APERTURE_READY:PT_APERTURE_STALE;
    *out=d;return (enum pt_aperture_result)d.gate;
}
static int descriptor_valid(const struct pt_prepared_clock *d)
{
    uint64_t arm=((uint64_t)d->arm_hi<<32)|d->arm_lo;
    return d->first_offset && d->first_offset<d->last_offset && d->last_offset<=PT_APERTURE_MAX_TICKS &&
        UINT64_MAX-arm>=d->last_offset && d->frequency && d->read_cap>=2 && d->read_cap<=PT_APERTURE_MAX_READS &&
        (d->gate==PT_APERTURE_READY || d->gate==PT_APERTURE_STALE);
}
int pt_prepared_replay(const struct pt_prepared_clock *d,const struct pt_prepared_read *samples,
    size_t count,unsigned reentry,struct pt_prepared_result *out)
{
    struct pt_prepared_result r;uint64_t arm,now,previous=0;size_t i=0;unsigned post=0;
    if(!span(d,sizeof(*d)) || !span(out,sizeof(*out)) || !apart(d,sizeof(*d),out,sizeof(*out)) ||
       count>PT_APERTURE_MAX_READS || reentry>1 ||
       (count && (!span(samples,count*sizeof(*samples)) || !apart(out,sizeof(*out),samples,count*sizeof(*samples)) ||
                  !apart(d,sizeof(*d),samples,count*sizeof(*samples)))) || !descriptor_valid(d))return 0;
    memset(&r,0,sizeof(r));r.reentry=reentry;
    if(d->gate==PT_APERTURE_STALE){if(count)return 0;r.result=PT_APERTURE_STALE;*out=r;return 1;}
    arm=((uint64_t)d->arm_hi<<32)|d->arm_lo;
    for(;;) {
        uint64_t delta;
        if(i>=count)return 0;
        now=((uint64_t)samples[i].hi<<32)|samples[i].lo;++i;r.reads=(uint32_t)i;
        if(post)r.after_index=r.reads;else r.before_index=r.reads;
        if(!samples[i-1].frequency || samples[i-1].frequency!=d->frequency || (i>1 && now<previous))
            {r.result=PT_APERTURE_CLOCK;break;}
        if(now<arm){r.result=i==1?PT_APERTURE_EARLY:PT_APERTURE_CLOCK;break;}
        delta=now-arm;previous=now;
        if(post) {
            if(reentry)r.result=PT_APERTURE_REENTRY;
            else r.result=delta<d->first_offset || delta>=d->last_offset?PT_APERTURE_POST_WINDOW:PT_APERTURE_COMMITTED;
            break;
        }
        if(delta>=d->last_offset){r.result=PT_APERTURE_EXPIRED;break;}
        if(delta>=d->first_offset) {
            if(reentry){r.result=PT_APERTURE_REENTRY;break;}
            r.shadow=PT_APERTURE_SHADOW;r.commits=1;post=1;
        } else if(i>=d->read_cap-1){r.result=PT_APERTURE_READ_LIMIT;break;}
    }
    /* Extra reads were not executed by this one-shot algorithm. A replay may
     * not silently accept an appended sample or change output on refusal. */
    if(i!=count)return 0;
    *out=r;return 1;
}
