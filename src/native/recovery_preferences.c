#include "recovery_preferences.h"
#include <string.h>

static int span(const void *p,size_t bytes,uintptr_t *begin,uintptr_t *end)
{
    uintptr_t address=(uintptr_t)p;
    if(!p || !bytes || bytes>UINTPTR_MAX-address)return 0;
    *begin=address;*end=address+bytes;return 1;
}
static int disjoint(const void *a,size_t a_bytes,const void *b,size_t b_bytes)
{
    uintptr_t a0,a1,b0,b1;
    return span(a,a_bytes,&a0,&a1) && span(b,b_bytes,&b0,&b1) &&
        (a1<=b0 || b1<=a0);
}
static int text_length(const char *text,size_t capacity,size_t *length)
{
    size_t i;
    for(i=0;i<capacity;++i) {
        unsigned char c=(unsigned char)text[i];
        if(!c) {*length=i;return 1;}
        if(c<32 || c==127)return 0;
    }
    return 0;
}
static int source_valid(const struct pt_native_recovery_source *s,size_t *length)
{
    uintptr_t first,last;
    if(!span(s,sizeof(*s),&first,&last) ||
        !text_length(s->name,sizeof(s->name),length))return 0;
    return (s->kind==PT_NATIVE_RECOVERY_SOURCE_UNTITLED && !*length) ||
        (s->kind==PT_NATIVE_RECOVERY_SOURCE_NAMED && *length);
}
int pt_native_recovery_source_commit(struct pt_native_recovery_source *s,
    enum pt_native_recovery_source_transition transition,int succeeded,
    const char *name,size_t capacity)
{
    struct pt_native_recovery_source candidate;size_t length,limit;
    uintptr_t first,last;
    if(!span(s,sizeof(*s),&first,&last) || succeeded!=1)return 0;
    if(transition==PT_NATIVE_RECOVERY_SOURCE_RESTORE)return source_valid(s,&length);
    memset(&candidate,0,sizeof(candidate));
    if(transition==PT_NATIVE_RECOVERY_SOURCE_NEW) {
        if(name || capacity)return 0;
        candidate.kind=PT_NATIVE_RECOVERY_SOURCE_UNTITLED;
    } else {
        if(transition!=PT_NATIVE_RECOVERY_SOURCE_LOAD &&
            transition!=PT_NATIVE_RECOVERY_SOURCE_SAVE &&
            transition!=PT_NATIVE_RECOVERY_SOURCE_SAVE_AS)return 0;
        if(!disjoint(s,sizeof(*s),name,capacity))return 0;
        limit=capacity<sizeof(candidate.name)?capacity:sizeof(candidate.name);
        if(!text_length(name,limit,&length) || !length)return 0;
        memcpy(candidate.name,name,length+1);
        candidate.kind=PT_NATIVE_RECOVERY_SOURCE_NAMED;
    }
    *s=candidate;return 1;
}
int pt_native_recovery_source_get(const struct pt_native_recovery_source *s,
    char *out,size_t capacity)
{
    size_t length;
    if(!disjoint(s,sizeof(*s),out,capacity) || !source_valid(s,&length) ||
        capacity<=length)return 0;
    memcpy(out,s->name,length+1);return 1;
}
static int controller_idle(const struct pt_native_recovery *r)
{
    return r->configured<=1 && r->bound<=1 && !r->schedule.busy &&
        !r->store.busy && !r->store.opened && !r->store.owned;
}
static int configuration_same(const struct pt_native_recovery_configuration *a,
    const struct pt_native_recovery_configuration *b)
{
    size_t an,bn;
    return text_length(a->directory,sizeof(a->directory),&an) &&
        text_length(b->directory,sizeof(b->directory),&bn) && an==bn &&
        !memcmp(a->directory,b->directory,an+1) &&
        a->interval_seconds==b->interval_seconds && a->enabled==b->enabled &&
        a->media==b->media && a->allow_removable==b->allow_removable;
}
int pt_native_recovery_preferences_open(struct pt_native_recovery_preferences *p,
    const struct pt_native_recovery *r,pt_native_recovery_preferences_idle idle,void *context)
{
    struct pt_native_recovery_preferences candidate;
    if(!disjoint(p,sizeof(*p),r,sizeof(*r)) || p->open || !idle ||
        !controller_idle(r) || idle(context)!=1)return 0;
    memset(&candidate,0,sizeof(candidate));
    if(!pt_native_recovery_get_configuration(r,&candidate.draft) ||
        idle(context)!=1 || !controller_idle(r))return 0;
    candidate.open=1;*p=candidate;return 1;
}
int pt_native_recovery_preferences_get(const struct pt_native_recovery_preferences *p,
    struct pt_native_recovery_configuration *out)
{
    if(!disjoint(p,sizeof(*p),out,sizeof(*out)) || p->open!=1)return 0;
    *out=p->draft;return 1;
}
int pt_native_recovery_preferences_edit(struct pt_native_recovery_preferences *p,
    const struct pt_native_recovery_configuration *draft)
{
    if(!disjoint(p,sizeof(*p),draft,sizeof(*draft)) || p->open!=1)return 0;
    p->draft=*draft;return 1;
}
int pt_native_recovery_preferences_cancel(struct pt_native_recovery_preferences *p)
{
    uintptr_t first,last;
    if(!span(p,sizeof(*p),&first,&last) || p->open!=1)return 0;
    p->open=0;return 1;
}
enum pt_native_recovery_preferences_result pt_native_recovery_preferences_apply(
    struct pt_native_recovery_preferences *p,struct pt_native_recovery *r,
    const struct pt_native_recovery_source *source,
    pt_native_recovery_preferences_idle idle,void *context)
{
    struct pt_native_recovery candidate;
    struct pt_native_recovery_configuration before,after;
    size_t length;int changed;
    if(!disjoint(p,sizeof(*p),r,sizeof(*r)) || p->open!=1 || !idle ||
        (source && (!disjoint(r,sizeof(*r),source,sizeof(*source)) ||
                    !disjoint(p,sizeof(*p),source,sizeof(*source)))) ||
        !controller_idle(r) || idle(context)!=1 ||
        !pt_native_recovery_get_configuration(r,&before))
        return PT_NATIVE_RECOVERY_PREFERENCES_REFUSED;
    candidate=*r;
    if(!pt_native_recovery_apply_configuration(&candidate,&p->draft) ||
        !pt_native_recovery_get_configuration(&candidate,&after))
        return PT_NATIVE_RECOVERY_PREFERENCES_REFUSED;
    changed=!configuration_same(&before,&after);
    if(changed && !r->bound) {
        if(!controller_idle(&candidate) || !source_valid(source,&length) ||
            !pt_native_recovery_bind(&candidate,
                source->kind==PT_NATIVE_RECOVERY_SOURCE_NAMED?source->name:NULL))
            return PT_NATIVE_RECOVERY_PREFERENCES_REFUSED;
    }
    if(idle(context)!=1 || !controller_idle(r))
        return PT_NATIVE_RECOVERY_PREFERENCES_REFUSED;
    if(changed)*r=candidate;
    p->open=0;
    return changed?PT_NATIVE_RECOVERY_PREFERENCES_APPLIED:
        PT_NATIVE_RECOVERY_PREFERENCES_NOOP;
}
