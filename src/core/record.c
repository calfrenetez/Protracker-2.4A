#include <string.h>
#include "record.h"
int pt_record_arm(struct pt_recorder *r,const struct pt_record_settings *settings,uint64_t position,
    void *context,int (*store)(void *,uint32_t,unsigned,const struct pt_event *))
{
    struct pt_record_settings copy;
    if(!r || !settings || !store || !settings->grid_rows || settings->grid_rows>64 ||
       settings->first_row>=settings->row_limit || settings->hold>1 || settings->velocity>1)return 0;
    copy=*settings;memset(r,0,sizeof(*r));r->settings=copy;r->context=context;r->store=store;
    r->origin=r->last_position=position;r->armed=1;r->started=(uint8_t)!copy.hold;return 1;
}
static enum pt_record_result quantize(const struct pt_recorder *r,uint64_t origin,uint64_t position,uint32_t *row)
{
    uint64_t units=(uint64_t)r->settings.grid_rows<<16,elapsed=position-origin,q=elapsed/units;
    if(elapsed%units>=units/2)++q; /* ties round to the next grid line */
    if(q>((uint64_t)r->settings.row_limit-1-r->settings.first_row)/r->settings.grid_rows)return PT_RECORD_CAPACITY;
    *row=r->settings.first_row+(uint32_t)q*r->settings.grid_rows;return PT_RECORD_OK;
}
enum pt_record_result pt_record_input(struct pt_recorder *r,uint64_t position,unsigned track,unsigned note,unsigned velocity,int on)
{
    struct pt_record_voice *v;struct pt_event event;uint64_t origin;uint32_t row;enum pt_record_result result;
    if(!r || !r->armed || !r->store || track>=16 || note>127 || velocity>127 || (on!=0 && on!=1))return PT_RECORD_INVALID;
    if(position<r->last_position)return PT_RECORD_TIME;
    r->last_position=position;
    if(!velocity)on=0;
    if(!r->started && !on)return PT_RECORD_IGNORED;
    v=&r->voices[track];
    /* Old key releases cannot cut the replacement note on a monophonic track. */
    if(!on && (!v->active || v->note!=note))return PT_RECORD_IGNORED;
    origin=r->started?r->origin:position;
    result=quantize(r,origin,position,&row);if(result!=PT_RECORD_OK)return result;
    if(!on && row<=v->on_row) {
        if(r->settings.grid_rows>=r->settings.row_limit-v->on_row)return PT_RECORD_CAPACITY;
        row=v->on_row+r->settings.grid_rows;
    }
    /* A note column cannot hold two events at one grid position. Do not hide
       losses by overwriting a prior accepted recording event. */
    if(v->written && row<=v->last_row)return PT_RECORD_COLLISION;
    memset(&event,0,sizeof(event));event.kind=on?PT_NOTE_MIDI:PT_NOTE_OFF;
    if(on) {event.pitch=(uint16_t)note;event.instrument=r->settings.instrument;
        if(r->settings.velocity) {event.flags=1;event.velocity=(uint8_t)velocity;}}
    if(!r->store(r->context,row,track,&event))return PT_RECORD_STORE_FAILED;
    r->origin=origin;r->last_position=position;r->started=1;
    v->last_row=row;v->written=1;
    if(on) {v->active=1;v->note=(uint8_t)note;v->on_row=row;}else v->active=0;
    return PT_RECORD_OK;
}
enum pt_record_result pt_record_stop(struct pt_recorder *r,uint64_t position)
{
    enum pt_record_result result=PT_RECORD_OK,one;unsigned i;
    if(!r || !r->armed)return PT_RECORD_INVALID;
    if(position<r->last_position)return PT_RECORD_TIME;
    for(i=0;i<16;++i)if(r->voices[i].active) {
        one=pt_record_input(r,position,i,r->voices[i].note,0,0);
        if(one!=PT_RECORD_OK)result=one;
    }
    if(result==PT_RECORD_OK)r->armed=0;
    return result;
}
void pt_record_cancel(struct pt_recorder *r)
{if(r) {r->armed=0;memset(r->voices,0,sizeof(r->voices));}}
