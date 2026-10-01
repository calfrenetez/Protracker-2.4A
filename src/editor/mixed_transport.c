#include <string.h>
#include "mixed_transport.h"
#include "mixed_owner_internal.h"
static enum pt_mixed_owner_result fault(struct pt_mixed_transport *t,enum pt_mixed_owner_result r)
{
    if(t->captured && t->owner && *t->owner==t->captured &&
       (r==PT_MIXED_OWNER_DEVICE || r==PT_MIXED_OWNER_CLOCK || r==PT_MIXED_OWNER_DEADLINE))
        r=pt_mixed_owner_transport_fault(t->captured,r);
    t->failure=r;t->closing=1;return r;
}
enum pt_mixed_owner_result pt_mixed_transport_begin(struct pt_mixed_transport *t,
    struct pt_mixed_owner **owner,uint64_t delay,uint32_t quantum,const struct pt_mixed_timer_api *api)
{
    enum pt_mixed_owner_result r;
    if(!t || t->active || !owner || !*owner || !quantum || quantum>256 || !api ||
       !api->read || !api->poll || !api->arm || !api->alarm_close || !api->counter_close || !api->signal)
        return PT_MIXED_OWNER_INVALID;
    memset(t,0,sizeof(*t));t->owner=owner;t->captured=*owner;t->timer=*api;t->quantum=quantum;t->active=1;
    r=pt_mixed_owner_clocked_begin(*owner,delay,api->read,api->context);
    if(r!=PT_MIXED_OWNER_OK){t->failure=r;t->closing=1;}
    return r;
}
enum pt_mixed_owner_result pt_mixed_transport_service(struct pt_mixed_transport *t)
{
    enum pt_mixed_owner_result r;enum pt_mixed_timer_result timer;uint64_t frame,tick;
    if(!t || !t->active)return PT_MIXED_OWNER_INVALID;
    if(t->failure)return t->failure;
    if(t->closing)return PT_MIXED_OWNER_INVALID;
    if(!t->owner || *t->owner!=t->captured)return fault(t,PT_MIXED_OWNER_INVALID);
    if(t->done)return PT_MIXED_OWNER_DONE;
    if(t->pending) {
        timer=t->timer.poll(t->timer.context);
        if(timer==PT_MIXED_TIMER_WAITING)return PT_MIXED_OWNER_WAITING;
        if(timer!=PT_MIXED_TIMER_READY)return fault(t,PT_MIXED_OWNER_DEVICE);
        t->pending=0;
    }
    r=pt_mixed_owner_clocked_service(t->captured,&frame);
    if(r==PT_MIXED_OWNER_DONE){t->done=1;return r;}
    if(r!=PT_MIXED_OWNER_OK && r!=PT_MIXED_OWNER_WAITING)return fault(t,r);
    r=pt_mixed_owner_transport_wake(t->captured,t->quantum,&tick);
    if(r==PT_MIXED_OWNER_PREPARING)return r;
    if(r!=PT_MIXED_OWNER_OK)return fault(t,r);
    timer=t->timer.arm(t->timer.context,tick);
    if(timer!=PT_MIXED_TIMER_WAITING)return fault(t,timer==PT_MIXED_TIMER_LATE?PT_MIXED_OWNER_DEADLINE:PT_MIXED_OWNER_DEVICE);
    t->pending=1;t->armed=tick;return PT_MIXED_OWNER_WAITING;
}
uint32_t pt_mixed_transport_signal(const struct pt_mixed_transport *t)
{return t && t->active && !t->alarm_closed?t->timer.signal(t->timer.context):0;}
int pt_mixed_transport_close(struct pt_mixed_transport *t)
{
    if(!t)return 0;
    if(!t->active)return 1;
    t->closing=1;
    if(!t->alarm_closed)t->alarm_closed=t->timer.alarm_close(t->timer.context)==1;
    if(!t->owner || *t->owner!=t->captured)return 0;
    if(t->captured) {
        if(!pt_mixed_owner_close(t->owner))return 0;
        t->captured=NULL;
    }
    if(!t->alarm_closed)return 0;
    if(!t->counter_closed)t->counter_closed=t->timer.counter_close(t->timer.context)==1;
    if(!t->counter_closed)return 0;
    t->active=t->pending=0;t->owner=NULL;return 1;
}
