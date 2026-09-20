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
    unsigned position,before,after;
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
    size_t common;unsigned i,value=direction<0?c->before:c->after;
    if(!matches(p,from) || (s->current && s->current!=from->storage) || pt_project_validate(p,NULL)!=PT_PROJECT_OK)return 0;
    if(c->position<from->orders && p->orders[c->position]!=(direction<0?c->after:c->before))return 0;
    if(to->patterns<from->patterns) {
        struct pt_event empty;memset(&empty,0,sizeof(empty));
        for(i=to->patterns*64*p->channels.count;i<from->patterns*64*p->channels.count;++i)
            if(memcmp(&p->events[i],&empty,sizeof(empty)))return 0;
    }
    /* Validate surviving references before removing a pattern. No failed
     * callback may change an array, active owner, history binding or revision. */
    probe=*p;probe.pattern_count=(uint16_t)to->patterns;probe.order_count=(uint16_t)(to->orders<from->orders?to->orders:from->orders);
    if(to->patterns<from->patterns && pt_project_validate(&probe,NULL)!=PT_PROJECT_OK)return 0;
    common=(size_t)(to->patterns<from->patterns?to->patterns:from->patterns)*64*p->channels.count;
    if(to->storage!=from->storage) {
        memcpy(to->storage->event,from->storage->event,common*sizeof(struct pt_event));
        memcpy(to->storage->order,from->storage->order,(to->orders<from->orders?to->orders:from->orders)*sizeof(uint16_t));
    }
    if(to->patterns>from->patterns)memset(to->storage->event+common,0,(size_t)64*p->channels.count*sizeof(struct pt_event));
    if(c->position<to->orders)to->storage->order[c->position]=(uint16_t)value;
    retain(to->storage);drop(s,s->current);s->current=to->storage;
    p->orders=to->storage->order;p->events=to->storage->event;p->order_count=(uint16_t)to->orders;p->pattern_count=(uint16_t)to->patterns;
    c->history->bound_events=p->events;c->history->bound_patterns=p->pattern_count;++s->generation;return 1;
}
static void discard(void *context)
{
    struct change *c=context;struct pt_song *s=c->owner;
    drop(s,c->state[0].storage);drop(s,c->state[1].storage);release(s,c,sizeof(*c));
}
static enum pt_edit_result edit(struct pt_song *s,struct pt_project *p,struct pt_pattern_history *h,unsigned pos,unsigned value,int append,int pattern)
{
    struct pt_song_storage *before,*after;struct change *c;struct pt_edit_resource r;enum pt_edit_result result;
    if(!s || !s->allocator.allocate || !s->allocator.release || !h || pt_project_validate(p,NULL)!=PT_PROJECT_OK ||
       (append?pos!=p->order_count:pos>=p->order_count) || (pattern?value!=p->pattern_count:value>=p->pattern_count))return PT_EDIT_INVALID;
    if((append && p->order_count==PT_PROJECT_ORDERS) || (pattern && p->pattern_count==PT_PROJECT_PATTERNS))return PT_EDIT_CAPACITY;
    if(s->current && (s->current->event!=p->events || s->current->order!=p->orders || s->current->channels!=p->channels.count))return PT_EDIT_CONFLICT;
    if(!append && p->orders[pos]==value)return PT_EDIT_OK;
    c=allocate(s,sizeof(*c));if(!c)return PT_EDIT_CAPACITY;
    before=s->current;if(before)retain(before);else before=storage(s,p,p->pattern_count,1);
    if(!before) {release(s,c,sizeof(*c));return PT_EDIT_CAPACITY;}
    after=before;
    if(before->patterns<p->pattern_count+(unsigned)pattern || before->orders<p->order_count+(unsigned)append)
        after=storage(s,p,p->pattern_count+(unsigned)pattern,0);
    else retain(after);
    if(!after) {drop(s,before);release(s,c,sizeof(*c));return PT_EDIT_CAPACITY;}
    c->owner=s;c->history=h;c->state[0]=(struct state){before,p->pattern_count,p->order_count};
    c->state[1]=(struct state){after,p->pattern_count+(unsigned)pattern,p->order_count+(unsigned)append};
    c->position=pos;c->before=append?0:p->orders[pos];c->after=value;
    r=(struct pt_edit_resource){c,apply,discard};result=pt_pattern_resource_apply(p,h,&r);
    if(result!=PT_EDIT_OK)discard(c);
    return result;
}
enum pt_edit_result pt_song_append(struct pt_song *s,struct pt_project *p,struct pt_pattern_history *h,unsigned pattern,int empty)
{
    if(!p || (empty!=0 && empty!=1))return PT_EDIT_INVALID;
    return edit(s,p,h,p->order_count,empty?p->pattern_count:pattern,1,empty);
}
enum pt_edit_result pt_song_assign(struct pt_song *s,struct pt_project *p,struct pt_pattern_history *h,unsigned position,unsigned pattern)
{return edit(s,p,h,position,pattern,0,0);}
