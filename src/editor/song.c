#include <string.h>
#include "song.h"
struct pt_song_storage {
    unsigned refs,channels,patterns,orders;
    size_t bytes;
    uint16_t *order;
    struct pt_event *event;
};
struct state {struct pt_song_storage *storage;unsigned patterns,orders;};
struct change {
    struct pt_song *owner;
    struct pt_pattern_history *history;
    struct state state[2];
    uint16_t orders[2][PT_PROJECT_ORDERS];
};
static void *allocate(struct pt_song *s,size_t n)
{
    void *p;
    if(s->bytes>s->budget || n>s->budget-s->bytes)return NULL;
    p=s->allocator.allocate(s->allocator.context,n);if(p)s->bytes+=n;return p;
}
static void release(struct pt_song *s,void *p,size_t n)
{s->allocator.release(s->allocator.context,p);s->bytes-=n;}
static void retain(struct pt_song_storage *p) {++p->refs;}
static void drop(struct pt_song *s,struct pt_song_storage *p)
{if(p && !--p->refs)release(s,p,p->bytes);}
void pt_song_init(struct pt_song *s,const struct pt_allocator *a,size_t budget)
{memset(s,0,sizeof(*s));s->allocator=*a;s->budget=budget;}
void pt_song_release(struct pt_song *s)
{if(s) {drop(s,s->current);s->current=NULL;}}
static struct pt_song_storage *storage(struct pt_song *s,struct pt_project *p,unsigned needed,int borrow)
{
    struct pt_song_storage *v;size_t bytes=sizeof(*v);unsigned capacity=1;
    while(capacity<needed)capacity*=2;
    if(!borrow)bytes+=PT_PROJECT_ORDERS*sizeof(uint16_t)+(size_t)capacity*64*p->channels.count*sizeof(struct pt_event);
    v=allocate(s,bytes);if(!v)return NULL;
    memset(v,0,sizeof(*v));v->refs=1;v->channels=p->channels.count;v->bytes=bytes;
    if(borrow) {v->patterns=p->pattern_count;v->orders=p->order_count;v->order=p->orders;v->event=p->events;}
    else {v->patterns=capacity;v->orders=PT_PROJECT_ORDERS;v->order=(uint16_t *)(v+1);v->event=(struct pt_event *)(v->order+PT_PROJECT_ORDERS);}
    return v;
}
static int matches(const struct pt_project *p,const struct state *a)
{return p->events==a->storage->event && p->orders==a->storage->order && p->channels.count==a->storage->channels && p->pattern_count==a->patterns && p->order_count==a->orders;}
static int apply(void *context,struct pt_project *p,int direction)
{
    struct change *c=context;struct pt_song *s=c->owner;struct pt_project probe;
    const struct state *from=&c->state[direction<0?1:0],*to=&c->state[direction<0?0:1];
    size_t common;unsigned i;const uint16_t *orders=c->orders[direction<0?0:1];
    if(!matches(p,from) || (s->current && s->current!=from->storage) || pt_project_validate(p,NULL)!=PT_PROJECT_OK)return 0;
    if(memcmp(p->orders,c->orders[direction<0?1:0],from->orders*sizeof(uint16_t)))return 0;
    for(i=0;i<to->orders;++i)if(orders[i]>=to->patterns)return 0;
    if(to->patterns<from->patterns) {
        struct pt_event empty;memset(&empty,0,sizeof(empty));
        for(i=to->patterns*64*p->channels.count;i<from->patterns*64*p->channels.count;++i)
            if(memcmp(&p->events[i],&empty,sizeof(empty)))return 0;
    }
    /* Validate surviving references before removing a pattern. No failed
     * callback may change an array, active owner, history binding or revision. */
    probe=*p;probe.pattern_count=(uint16_t)to->patterns;probe.order_count=(uint16_t)to->orders;probe.orders=(uint16_t *)orders;
    if(to->patterns<from->patterns && pt_project_validate(&probe,NULL)!=PT_PROJECT_OK)return 0;
    common=(size_t)(to->patterns<from->patterns?to->patterns:from->patterns)*64*p->channels.count;
    if(to->storage!=from->storage) {
        memcpy(to->storage->event,from->storage->event,common*sizeof(struct pt_event));
    }
    if(to->patterns>from->patterns)memset(to->storage->event+common,0,(size_t)64*p->channels.count*sizeof(struct pt_event));
    memcpy(to->storage->order,orders,to->orders*sizeof(uint16_t));
    retain(to->storage);drop(s,s->current);s->current=to->storage;
    p->orders=to->storage->order;p->events=to->storage->event;p->order_count=(uint16_t)to->orders;p->pattern_count=(uint16_t)to->patterns;
    c->history->bound_events=p->events;c->history->bound_patterns=p->pattern_count;++s->generation;return 1;
}
static void discard(void *context)
{
    struct change *c=context;struct pt_song *s=c->owner;
    drop(s,c->state[0].storage);drop(s,c->state[1].storage);release(s,c,sizeof(*c));
}
static enum pt_edit_result edit(struct pt_song *s,struct pt_project *p,struct pt_pattern_history *h,const uint16_t *orders,unsigned count,unsigned patterns)
{
    struct pt_song_storage *before,*after;struct change *c;struct pt_edit_resource r;enum pt_edit_result result;unsigned i;
    if(!s || !s->allocator.allocate || !s->allocator.release || !h || !orders ||
       !count || patterns<p->pattern_count || patterns>p->pattern_count+1U)return PT_EDIT_INVALID;
    if(count>PT_PROJECT_ORDERS || patterns>PT_PROJECT_PATTERNS)return PT_EDIT_CAPACITY;
    for(i=0;i<count;++i)if(orders[i]>=patterns)return PT_EDIT_INVALID;
    if(s->current && (s->current->event!=p->events || s->current->order!=p->orders || s->current->channels!=p->channels.count))return PT_EDIT_CONFLICT;
    if(count==p->order_count && patterns==p->pattern_count && !memcmp(orders,p->orders,count*sizeof(uint16_t)))return PT_EDIT_OK;
    c=allocate(s,sizeof(*c));if(!c)return PT_EDIT_CAPACITY;
    before=s->current;if(before)retain(before);else before=storage(s,p,p->pattern_count,1);
    if(!before) {release(s,c,sizeof(*c));return PT_EDIT_CAPACITY;}
    after=before;
    if(before->patterns<patterns || before->orders<count)after=storage(s,p,patterns,0);
    else retain(after);
    if(!after) {drop(s,before);release(s,c,sizeof(*c));return PT_EDIT_CAPACITY;}
    c->owner=s;c->history=h;c->state[0]=(struct state){before,p->pattern_count,p->order_count};
    c->state[1]=(struct state){after,patterns,count};
    memcpy(c->orders[0],p->orders,p->order_count*sizeof(uint16_t));memcpy(c->orders[1],orders,count*sizeof(uint16_t));
    r=(struct pt_edit_resource){c,apply,discard};result=pt_pattern_resource_apply(p,h,&r);
    if(result!=PT_EDIT_OK)discard(c);
    return result;
}
static int valid(const struct pt_project *p)
{return pt_project_validate(p,NULL)==PT_PROJECT_OK;}
enum pt_edit_result pt_song_append(struct pt_song *s,struct pt_project *p,struct pt_pattern_history *h,unsigned pattern,int empty)
{
    uint16_t orders[PT_PROJECT_ORDERS];
    if(!valid(p) || (empty!=0 && empty!=1) || (!empty && pattern>=p->pattern_count))return PT_EDIT_INVALID;
    if(p->order_count==PT_PROJECT_ORDERS || (empty && p->pattern_count==PT_PROJECT_PATTERNS))return PT_EDIT_CAPACITY;
    memcpy(orders,p->orders,p->order_count*sizeof(uint16_t));orders[p->order_count]=(uint16_t)(empty?p->pattern_count:pattern);
    return edit(s,p,h,orders,p->order_count+1,p->pattern_count+(unsigned)empty);
}
enum pt_edit_result pt_song_assign(struct pt_song *s,struct pt_project *p,struct pt_pattern_history *h,unsigned position,unsigned pattern)
{
    uint16_t orders[PT_PROJECT_ORDERS];
    if(!valid(p) || position>=p->order_count || pattern>=p->pattern_count)return PT_EDIT_INVALID;
    memcpy(orders,p->orders,p->order_count*sizeof(uint16_t));orders[position]=(uint16_t)pattern;
    return edit(s,p,h,orders,p->order_count,p->pattern_count);
}
enum pt_edit_result pt_song_insert(struct pt_song *s,struct pt_project *p,struct pt_pattern_history *h,unsigned position,unsigned pattern)
{
    uint16_t orders[PT_PROJECT_ORDERS];unsigned i;
    if(!valid(p) || position>p->order_count || pattern>=p->pattern_count)return PT_EDIT_INVALID;
    if(p->order_count==PT_PROJECT_ORDERS)return PT_EDIT_CAPACITY;
    for(i=0;i<position;++i)orders[i]=p->orders[i];
    orders[position]=(uint16_t)pattern;
    for(i=position;i<p->order_count;++i)orders[i+1]=p->orders[i];
    return edit(s,p,h,orders,p->order_count+1,p->pattern_count);
}
enum pt_edit_result pt_song_remove(struct pt_song *s,struct pt_project *p,struct pt_pattern_history *h,unsigned position)
{
    uint16_t orders[PT_PROJECT_ORDERS];unsigned i;
    if(!valid(p) || position>=p->order_count)return PT_EDIT_INVALID;
    if(p->order_count==1)return PT_EDIT_UNSUPPORTED;
    for(i=0;i<position;++i)orders[i]=p->orders[i];
    for(i=position+1;i<p->order_count;++i)orders[i-1]=p->orders[i];
    return edit(s,p,h,orders,p->order_count-1,p->pattern_count);
}
enum pt_edit_result pt_song_move(struct pt_song *s,struct pt_project *p,struct pt_pattern_history *h,unsigned position,unsigned destination)
{
    uint16_t orders[PT_PROJECT_ORDERS],value;
    if(!valid(p) || position>=p->order_count || destination>=p->order_count)return PT_EDIT_INVALID;
    memcpy(orders,p->orders,p->order_count*sizeof(uint16_t));value=orders[position];
    if(position<destination)memmove(orders+position,orders+position+1,(destination-position)*sizeof(uint16_t));
    else if(position>destination)memmove(orders+destination+1,orders+destination,(position-destination)*sizeof(uint16_t));
    orders[destination]=value;return edit(s,p,h,orders,p->order_count,p->pattern_count);
}
