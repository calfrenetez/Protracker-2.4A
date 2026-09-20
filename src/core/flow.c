#include "flow.h"
#include <string.h>

enum pt_flow_result pt_flow_init(struct pt_flow *out,const struct pt_project *p,
                               enum pt_flow_mode mode,unsigned start,uint32_t limit)
{
    struct pt_flow next;
    if(!out || !p || !limit || (mode!=PT_FLOW_CLASSIC128 && mode!=PT_FLOW_EXTENDED256) ||
       pt_project_validate(p,NULL)!=PT_PROJECT_OK || start>=p->order_count)
        return PT_FLOW_INVALID;
    if(mode==PT_FLOW_CLASSIC128 && (p->channels.count!=4 || p->order_count>128 || p->speed!=6 || p->bpm!=125))
        return PT_FLOW_INVALID;
    memset(&next,0,sizeof(next));next.project=p;next.mode=(uint8_t)mode;
    next.order=next.played_order=(uint16_t)start;next.speed=p->speed;next.bpm=p->bpm;
    next.limit=limit;next.active=1;*out=next;return PT_FLOW_TICK;
}

static void extended_effect(struct pt_flow *s,unsigned ch)
{
    unsigned parameter=s->parameter[ch],value=parameter&15;
    if(s->counter)return;
    if((parameter>>4)==6) {
        if(!value)s->loop_start[ch]=(uint8_t)(s->row&63);
        else {
            if(!s->loop_count[ch])s->loop_count[ch]=(uint8_t)value;
            else if(!--s->loop_count[ch])return;
            s->break_row=s->loop_start[ch];s->loop_break=1;
        }
    } else if((parameter>>4)==14 && !s->delay)s->pending_delay=(uint8_t)(value+1);
}

static void new_effect(struct pt_flow *s,unsigned ch)
{
    unsigned parameter=s->parameter[ch],row;
    switch(s->effect[ch]) {
    case 11:s->order=(uint8_t)(parameter-1);s->break_row=0;s->jump=1;break;
    case 13:
        row=(parameter>>4)*10+(parameter&15);
        s->break_row=(uint8_t)(row>63?0:row);s->jump=1;break;
    case 14:extended_effect(s,ch);break;
    case 15:
        if(!parameter)s->active=0;
        else if(parameter<32) {s->counter=0;s->speed=(uint8_t)parameter;}
        else s->bpm=(uint16_t)parameter;
        break;
    default:break; /* Audio/voice effects belong to the subsequent voice core. */
    }
}

static void next_position(struct pt_flow *s,unsigned from)
{
    s->row=s->break_row;s->break_row=s->jump=0;
    s->order=(uint16_t)((s->order+1)&(s->mode==PT_FLOW_CLASSIC128?127:255));
    if(s->order>=s->project->order_count)s->order=0;
    ++s->positions;if(s->order<=from)++s->returns;
}

enum pt_flow_result pt_flow_tick(struct pt_flow *s)
{
    const struct pt_project *p;unsigned ch,from;int row_tick;
    if(!s || !s->project || !s->limit)return PT_FLOW_INVALID;
    if(!s->active)return PT_FLOW_STOPPED;
    if(s->ticks>=s->limit)return PT_FLOW_LIMIT;
    p=s->project;from=s->order;++s->ticks;s->fresh=s->delayed=0;
    ++s->counter;row_tick=s->counter>=s->speed;
    if(row_tick)s->counter=0;
    if(row_tick && !s->delay) {
        const struct pt_event *events=p->events+((size_t)p->orders[s->order]*64+s->row)*p->channels.count;
        s->played_order=s->order;s->played_row=s->row;s->fresh=1;++s->fetches;
        for(ch=0;ch<p->channels.count;++ch) {
            s->effect[ch]=events[ch].effect;s->parameter[ch]=events[ch].parameter;
            new_effect(s,ch);
        }
    } else {
        s->delayed=(uint8_t)row_tick;
        for(ch=0;ch<p->channels.count;++ch)if(s->effect[ch]==14)extended_effect(s,ch);
    }
    if(row_tick) {
        ++s->row;
        if(s->pending_delay) {s->delay=s->pending_delay;s->pending_delay=0;}
        if(s->delay && --s->delay)--s->row;
        if(s->loop_break) {s->loop_break=0;s->row=s->break_row;s->break_row=0;}
        if(s->row>=64)next_position(s,from);
    }
    if(s->jump)next_position(s,from);
    return PT_FLOW_TICK;
}
