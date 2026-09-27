#include "recovery.h"
#include <string.h>
int pt_recovery_configure(struct pt_recovery_schedule *s,const struct pt_recovery_policy *p)
{
    struct pt_recovery_policy value;
    if(!s || !p || s->busy || !p->interval_seconds || p->enabled>1 || p->allow_removable>1)return 0;
    value=*p;memset(s,0,sizeof(*s));s->policy=value;return 1;
}
enum pt_recovery_tick_result pt_recovery_tick(struct pt_recovery_schedule *s,
    uint64_t now,uint64_t revision,uint64_t saved_revision,
    int persistent,int removable,int safe_to_write,int (*snapshot)(void *),void *context)
{
    int result;
    if(!s || !s->policy.interval_seconds || s->busy || !snapshot)return PT_RECOVERY_INVALID;
    if(!s->policy.enabled || revision==saved_revision || !persistent ||
       (removable && !s->policy.allow_removable)) {s->armed=0;return PT_RECOVERY_SKIPPED;}
    if(s->have_snapshot && s->snapshot_revision==revision && s->snapshot_saved_revision==saved_revision)
        return PT_RECOVERY_SKIPPED;
    if(!s->armed || now<s->observed) {s->since=now;s->armed=1;}
    s->observed=now;
    if(!safe_to_write || now-s->since<s->policy.interval_seconds)return PT_RECOVERY_SKIPPED;
    s->since=now;s->busy=1;result=snapshot(context);s->busy=0;
    if(result!=1)return PT_RECOVERY_FAILED;
    s->snapshot_revision=revision;s->snapshot_saved_revision=saved_revision;s->have_snapshot=1;
    return PT_RECOVERY_SAVED;
}
