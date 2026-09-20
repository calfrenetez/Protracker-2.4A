#include <string.h>
#include "pattern.h"
static const uint16_t periods[36]={856,808,762,720,678,640,604,570,538,508,480,453,
    428,404,381,360,339,320,302,285,269,254,240,226,214,202,190,180,170,160,151,143,135,127,120,113};
static int overlap(const void *a,size_t an,const void *b,size_t bn)
{
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    if(!an || !bn)return 0;
    if(an>UINTPTR_MAX-x || bn>UINTPTR_MAX-y)return 1;
    return x<y+bn && y<x+an;
}
static int shape(const struct pt_project *p)
{return p && p->events && p->pattern_count && p->pattern_count<=256 && pt_channels_validate(&p->channels)==PT_CHANNEL_OK;}
static size_t event_count(const struct pt_project *p) {return (size_t)p->pattern_count*64*p->channels.count;}
static int equal(const struct pt_event *a,const struct pt_event *b)
{return a->pitch==b->pitch && a->slice==b->slice && a->kind==b->kind && a->instrument==b->instrument &&
    a->effect==b->effect && a->parameter==b->parameter && a->velocity==b->velocity && a->flags==b->flags;}
static int valid(const struct pt_project *p,const struct pt_pattern_history *h)
{
    size_t i,used=0;
    if(!shape(p) || !h || h->bound_events!=p->events || h->bound_channels!=p->channels.count || h->bound_patterns!=p->pattern_count || !h->commands || !h->changes ||
       !h->command_capacity || !h->change_capacity || h->count>h->command_capacity ||
       h->cursor>h->count || h->used>h->change_capacity || !h->next_revision ||
       h->command_capacity>SIZE_MAX/sizeof(*h->commands) || h->change_capacity>SIZE_MAX/sizeof(*h->changes))return 0;
    for(i=0;i<h->count;++i) {
        const struct pt_pattern_command *c=&h->commands[i];
        if(c->offset!=used || c->count>h->used-used)return 0;
        if(c->kind==PT_COMMAND_EVENTS) {if(!c->count)return 0;}
        else if(c->kind==PT_COMMAND_CHANNEL) {if(c->count || c->channel>=p->channels.count)return 0;}
        else if(c->kind==PT_COMMAND_TITLE) {if(c->count || !memchr(c->data.titles[0],0,PT_PROJECT_NAME) || !memchr(c->data.titles[1],0,PT_PROJECT_NAME))return 0;}
        else if(c->kind==PT_COMMAND_RESOURCE) {if(c->count || !c->data.resource.context || !c->data.resource.apply || !c->data.resource.discard)return 0;}
        else return 0;
        used+=c->count;
    }
    return used==h->used;
}
static int scratch_alias(const struct pt_project *p,const struct pt_pattern_history *h,const void *data,size_t bytes)
{
    return overlap(data,bytes,p,sizeof(*p)) || overlap(data,bytes,p->events,event_count(p)*sizeof(*p->events)) ||
        overlap(data,bytes,h,sizeof(*h)) || overlap(data,bytes,h->commands,h->command_capacity*sizeof(*h->commands)) ||
        overlap(data,bytes,h->changes,h->change_capacity*sizeof(*h->changes));
}
enum pt_edit_result pt_pattern_history_init(struct pt_pattern_history *h,const struct pt_project *p,
    struct pt_pattern_command *commands,size_t command_capacity,struct pt_event_change *changes,size_t change_capacity)
{
    size_t cb,eb,pb;
    if(!shape(p) || !h || !commands || !changes || !command_capacity || !change_capacity ||
       command_capacity>SIZE_MAX/sizeof(*commands) || change_capacity>SIZE_MAX/sizeof(*changes))return PT_EDIT_INVALID;
    cb=command_capacity*sizeof(*commands);eb=change_capacity*sizeof(*changes);pb=event_count(p)*sizeof(*p->events);
    if(overlap(commands,cb,changes,eb) || overlap(h,sizeof(*h),commands,cb) || overlap(h,sizeof(*h),changes,eb) ||
       overlap(commands,cb,p->events,pb) || overlap(changes,eb,p->events,pb) || overlap(h,sizeof(*h),p->events,pb) ||
       overlap(commands,cb,p,sizeof(*p)) || overlap(changes,eb,p,sizeof(*p)) || overlap(h,sizeof(*h),p,sizeof(*p)))return PT_EDIT_ALIAS;
    memset(h,0,sizeof(*h));h->bound_events=p->events;h->bound_channels=p->channels.count;h->bound_patterns=p->pattern_count;h->commands=commands;h->changes=changes;
    h->command_capacity=command_capacity;h->change_capacity=change_capacity;h->next_revision=1;return PT_EDIT_OK;
}
static void discard(struct pt_pattern_command *c)
{if(c->kind==PT_COMMAND_RESOURCE)c->data.resource.discard(c->data.resource.context);}
void pt_pattern_history_release(struct pt_pattern_history *h)
{
    size_t i;if(!h)return;
    for(i=0;i<h->count;++i)discard(&h->commands[i]);
    memset(h,0,sizeof(*h));
}
static struct pt_pattern_command *reserve(struct pt_pattern_history *h,size_t needed)
{
    size_t i;
    /* All validation precedes this mutation of the journal. */
    if(h->cursor<h->count) {
        for(i=h->cursor;i<h->count;++i)discard(&h->commands[i]);
        h->used=h->cursor?h->commands[h->cursor-1].offset+h->commands[h->cursor-1].count:0;
        h->count=h->cursor;
    }
    while(h->count==h->command_capacity || needed>h->change_capacity-h->used) {
        size_t drop=h->commands[0].count;discard(&h->commands[0]);
        memmove(h->changes,h->changes+drop,(h->used-drop)*sizeof(*h->changes));h->used-=drop;
        --h->count;--h->cursor;
        for(i=0;i<h->count;++i) {h->commands[i]=h->commands[i+1];h->commands[i].offset-=drop;}
    }
    memset(&h->commands[h->count],0,sizeof(*h->commands));
    h->commands[h->count].offset=h->used;h->commands[h->count].count=needed;
    h->commands[h->count].before_revision=h->revision;h->commands[h->count].after_revision=h->next_revision++;
    return &h->commands[h->count];
}
enum pt_edit_result pt_pattern_apply(struct pt_project *p,struct pt_pattern_history *h,
    const struct pt_event_update *updates,size_t count)
{
    size_t i,needed=0,n;
    if(!valid(p,h) || (count && !updates) || count>SIZE_MAX/sizeof(*updates))return PT_EDIT_INVALID;
    if(scratch_alias(p,h,updates,count*sizeof(*updates)))return PT_EDIT_ALIAS;
    n=event_count(p);
    for(i=0;i<count;++i) {
        if(updates[i].index>=n || (i && updates[i-1].index>=updates[i].index) ||
           !pt_project_event_valid(p,&updates[i].event))return PT_EDIT_INVALID;
        if(!equal(&updates[i].event,&p->events[updates[i].index]))++needed;
    }
    if(!needed)return PT_EDIT_OK;
    if(needed>h->change_capacity || h->next_revision==UINT32_MAX)return PT_EDIT_CAPACITY;
    reserve(h,needed);
    for(i=0;i<count;++i)if(!equal(&updates[i].event,&p->events[updates[i].index])) {
        struct pt_event_change *c=&h->changes[h->used++];c->index=updates[i].index;
        c->before=p->events[c->index];c->after=updates[i].event;p->events[c->index]=c->after;
    }
    h->revision=h->commands[h->count++].after_revision;h->cursor=h->count;return PT_EDIT_OK;
}
enum pt_edit_result pt_pattern_channel_apply(struct pt_project *p,struct pt_pattern_history *h,
    unsigned index,const struct pt_channel *candidate)
{
    struct pt_channels next;struct pt_channel value;struct pt_pattern_command *command;
    enum pt_channel_result result;
    if(!valid(p,h) || !candidate || index>=p->channels.count)return PT_EDIT_INVALID;
    value=*candidate;next=p->channels;next.track[index]=value;
    result=pt_channels_validate(&next);
    if(result!=PT_CHANNEL_OK)return result==PT_CHANNEL_PAULA_LIMIT?PT_EDIT_PAULA_LIMIT:PT_EDIT_INVALID;
    if(!memcmp(&value,&p->channels.track[index],sizeof(value)))return PT_EDIT_OK;
    if(h->next_revision==UINT32_MAX)return PT_EDIT_CAPACITY;
    command=reserve(h,0);command->kind=PT_COMMAND_CHANNEL;command->channel=(uint8_t)index;
    command->data.channels[0]=p->channels.track[index];command->data.channels[1]=value;
    p->channels.track[index]=value;h->revision=command->after_revision;++h->count;h->cursor=h->count;
    return PT_EDIT_OK;
}
enum pt_edit_result pt_pattern_title_apply(struct pt_project *p,struct pt_pattern_history *h,const char *name)
{
    char value[PT_PROJECT_NAME];size_t length=0;struct pt_pattern_command *command;
    if(!valid(p,h) || !name || !memchr(p->title,0,sizeof(p->title)))return PT_EDIT_INVALID;
    while(length<sizeof(value) && name[length])++length;
    if(length==sizeof(value))return PT_EDIT_INVALID;
    if(!strcmp(p->title,name))return PT_EDIT_OK;
    if(h->next_revision==UINT32_MAX)return PT_EDIT_CAPACITY;
    memset(value,0,sizeof(value));memcpy(value,name,length);
    command=reserve(h,0);command->kind=PT_COMMAND_TITLE;
    memcpy(command->data.titles[0],p->title,sizeof(p->title));memcpy(command->data.titles[1],value,sizeof(value));
    memcpy(p->title,value,sizeof(value));h->revision=command->after_revision;++h->count;h->cursor=h->count;return PT_EDIT_OK;
}
enum pt_edit_result pt_pattern_resource_apply(struct pt_project *p,struct pt_pattern_history *h,const struct pt_edit_resource *r)
{
    struct pt_pattern_command *command;struct pt_edit_resource copy;
    if(!valid(p,h) || !r || !r->context || !r->apply || !r->discard)return PT_EDIT_INVALID;
    if(h->next_revision==UINT32_MAX)return PT_EDIT_CAPACITY;
    copy=*r;
    if(!copy.apply(copy.context,p,1))return PT_EDIT_CONFLICT;
    command=reserve(h,0);command->kind=PT_COMMAND_RESOURCE;command->data.resource=copy;
    h->revision=command->after_revision;++h->count;h->cursor=h->count;return PT_EDIT_OK;
}
enum pt_edit_result pt_pattern_undo(struct pt_project *p,struct pt_pattern_history *h,int direction)
{
    struct pt_pattern_command *command;size_t i,n;
    if(!valid(p,h) || (direction!=-1 && direction!=1))return PT_EDIT_INVALID;
    if((direction<0 && !h->cursor) || (direction>0 && h->cursor==h->count))return PT_EDIT_END;
    command=&h->commands[direction<0?h->cursor-1:h->cursor];n=event_count(p);
    if(command->kind==PT_COMMAND_CHANNEL) {
        struct pt_channels next=p->channels;
        const struct pt_channel *expected=direction<0?&command->data.channels[1]:&command->data.channels[0];
        const struct pt_channel *replacement=direction<0?&command->data.channels[0]:&command->data.channels[1];
        if(memcmp(&p->channels.track[command->channel],expected,sizeof(*expected)))return PT_EDIT_CONFLICT;
        next.track[command->channel]=*replacement;
        if(pt_channels_validate(&next)!=PT_CHANNEL_OK)return PT_EDIT_CONFLICT;
        p->channels.track[command->channel]=*replacement;
    }
    if(command->kind==PT_COMMAND_TITLE) {
        const char *expected=command->data.titles[direction<0?1:0],*replacement=command->data.titles[direction<0?0:1];
        if(memcmp(p->title,expected,sizeof(p->title)))return PT_EDIT_CONFLICT;
        memcpy(p->title,replacement,sizeof(p->title));
    }
    if(command->kind==PT_COMMAND_RESOURCE && !command->data.resource.apply(command->data.resource.context,p,direction))return PT_EDIT_CONFLICT;
    for(i=0;i<command->count;++i) {
        struct pt_event_change *c=&h->changes[command->offset+i];
        const struct pt_event *expected=direction<0?&c->after:&c->before,*replacement=direction<0?&c->before:&c->after;
        if(c->index>=n || !equal(&p->events[c->index],expected) || !pt_project_event_valid(p,replacement))return PT_EDIT_CONFLICT;
    }
    for(i=0;i<command->count;++i) {
        struct pt_event_change *c=&h->changes[command->offset+i];p->events[c->index]=direction<0?c->before:c->after;
    }
    if(direction<0) {--h->cursor;h->revision=command->before_revision;}
    else {++h->cursor;h->revision=command->after_revision;}
    return PT_EDIT_OK;
}
void pt_pattern_mark_saved(struct pt_pattern_history *h) {if(h)h->saved_revision=h->revision;}
int pt_pattern_dirty(const struct pt_pattern_history *h) {return h && h->saved_revision!=h->revision;}
static int selection(const struct pt_project *p,unsigned pattern,unsigned first_row,unsigned end_row,unsigned first_channel,unsigned end_channel)
{return shape(p) && pattern<p->pattern_count && first_row<end_row && end_row<=64 &&
    first_channel<end_channel && end_channel<=p->channels.count;}
enum pt_edit_result pt_pattern_copy(const struct pt_project *p,unsigned pattern,unsigned first_row,unsigned end_row,
    unsigned first_channel,unsigned end_channel,struct pt_block *out)
{
    unsigned r,c;size_t n,i=0;
    if(!selection(p,pattern,first_row,end_row,first_channel,end_channel) || !out || !out->events)return PT_EDIT_INVALID;
    n=(size_t)(end_row-first_row)*(end_channel-first_channel);
    if(out->capacity<n)return PT_EDIT_CAPACITY;
    if(overlap(out->events,n*sizeof(*out->events),p->events,event_count(p)*sizeof(*p->events)) ||
       overlap(out->events,n*sizeof(*out->events),out,sizeof(*out)) ||
       overlap(out,sizeof(*out),p->events,event_count(p)*sizeof(*p->events)) ||
       overlap(out->events,n*sizeof(*out->events),p,sizeof(*p)) || overlap(out,sizeof(*out),p,sizeof(*p)))return PT_EDIT_ALIAS;
    for(r=first_row;r<end_row;++r)for(c=first_channel;c<end_channel;++c)
        out->events[i++]=p->events[((size_t)pattern*64+r)*p->channels.count+c];
    out->rows=(uint8_t)(end_row-first_row);out->channels=(uint8_t)(end_channel-first_channel);return PT_EDIT_OK;
}
enum pt_edit_result pt_pattern_paste(struct pt_project *p,struct pt_pattern_history *h,unsigned pattern,unsigned row,
    unsigned channel,const struct pt_block *block,struct pt_event_update *scratch,size_t capacity)
{
    unsigned r,c;size_t n,i=0;
    if(!valid(p,h) || !block || !block->events || !block->rows || block->rows>64 ||
       !block->channels || block->channels>16 || row>=64 || channel>=p->channels.count ||
       !selection(p,pattern,row,row+block->rows,channel,channel+block->channels))return PT_EDIT_INVALID;
    n=(size_t)block->rows*block->channels;
    if(!scratch || capacity<n || block->capacity<n)return PT_EDIT_CAPACITY;
    if(scratch_alias(p,h,scratch,n*sizeof(*scratch)) || overlap(scratch,n*sizeof(*scratch),block,sizeof(*block)) ||
       overlap(scratch,n*sizeof(*scratch),block->events,n*sizeof(*block->events)))return PT_EDIT_ALIAS;
    for(r=0;r<block->rows;++r)for(c=0;c<block->channels;++c) {
        scratch[i].index=(uint32_t)(((size_t)pattern*64+row+r)*p->channels.count+channel+c);
        scratch[i].event=block->events[i];++i;
    }
    return pt_pattern_apply(p,h,scratch,n);
}
enum pt_edit_result pt_pattern_transpose(struct pt_project *p,struct pt_pattern_history *h,unsigned pattern,
    unsigned first_row,unsigned end_row,unsigned first_channel,unsigned end_channel,int semitones,
    struct pt_event_update *scratch,size_t capacity)
{
    unsigned r,c;size_t n,i=0;
    if(!valid(p,h) || !selection(p,pattern,first_row,end_row,first_channel,end_channel) ||
       semitones < -127 || semitones>127)return PT_EDIT_INVALID;
    n=(size_t)(end_row-first_row)*(end_channel-first_channel);
    if(!scratch || capacity<n)return PT_EDIT_CAPACITY;
    if(scratch_alias(p,h,scratch,n*sizeof(*scratch)))return PT_EDIT_ALIAS;
    for(r=first_row;r<end_row;++r)for(c=first_channel;c<end_channel;++c) {
        struct pt_event *e;int note;
        scratch[i].index=(uint32_t)(((size_t)pattern*64+r)*p->channels.count+c);
        scratch[i].event=p->events[scratch[i].index];e=&scratch[i++].event;
        if(e->kind==PT_NOTE_MIDI) {note=(int)e->pitch+semitones;if(note<0 || note>127)return PT_EDIT_UNSUPPORTED;e->pitch=(uint16_t)note;}
        else if(e->kind==PT_NOTE_PERIOD) {
            for(note=0;note<36 && periods[note]!=e->pitch;++note) {}
            if(note==36 || note+semitones<0 || note+semitones>=36)return PT_EDIT_UNSUPPORTED;
            e->pitch=periods[note+semitones];
        }
    }
    return pt_pattern_apply(p,h,scratch,n);
}
enum pt_edit_result pt_pattern_clone(struct pt_project *p,struct pt_pattern_history *h,unsigned source,unsigned dest,
    struct pt_event_update *scratch,size_t capacity)
{
    size_t n,i;
    if(!valid(p,h) || source>=p->pattern_count || dest>=p->pattern_count)return PT_EDIT_INVALID;
    n=(size_t)64*p->channels.count;if(!scratch || capacity<n)return PT_EDIT_CAPACITY;
    if(scratch_alias(p,h,scratch,n*sizeof(*scratch)))return PT_EDIT_ALIAS;
    for(i=0;i<n;++i) {scratch[i].index=(uint32_t)(dest*n+i);scratch[i].event=p->events[source*n+i];}
    return pt_pattern_apply(p,h,scratch,n);
}
