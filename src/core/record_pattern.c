#include "record_pattern.h"
int pt_record_pattern_store(void *context,uint32_t row,unsigned track,const struct pt_event *event)
{
    struct pt_record_pattern *r=context;struct pt_project *p;struct pt_event_update u;unsigned pattern;
    if(!r || !(p=r->project) || !r->history || !event || !p->orders || !p->events ||
       track>=p->channels.count || p->channels.track[track].route!=PT_MIDI ||
       row/64>=p->order_count || (event->kind!=PT_NOTE_MIDI && event->kind!=PT_NOTE_OFF))return 0;
    pattern=p->orders[row/64];if(pattern>=p->pattern_count)return 0;
    u.index=(pattern*64+row%64)*p->channels.count+track;u.event=p->events[u.index];
    if(u.event.kind!=PT_NOTE_NONE)return 0;
    u.event.kind=event->kind;u.event.pitch=event->pitch;u.event.slice=0;
    u.event.flags=event->flags;u.event.velocity=event->velocity;
    if(event->instrument)u.event.instrument=event->instrument;
    return pt_pattern_apply(p,r->history,&u,1)==PT_EDIT_OK;
}
