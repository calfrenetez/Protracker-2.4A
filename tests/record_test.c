#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "record.h"
#include "record_pattern.h"
static struct pt_event events[256][16];
static unsigned fail,calls;
static int store(void *ctx,uint32_t row,unsigned track,const struct pt_event *e)
{
    (void)ctx;assert(row<256 && track<16);++calls;
    if(calls==fail || events[row][track].kind!=PT_NOTE_NONE)return 0;
    events[row][track]=*e;return 1;
}
#define ROW(n) ((uint64_t)(n)<<16)
static void pattern_recording(void)
{
    struct pt_project p;struct pt_pattern_history history;struct pt_pattern_command commands[4];
    struct pt_event_change changes[8];struct pt_recorder recorder;struct pt_record_settings settings={63,128,1,0,1,0};
    struct pt_record_pattern adapter;uint16_t orders[2]={0,1};
    memset(&p,0,sizeof(p));memset(events,0,sizeof(events));pt_channels_init(&p.channels);
    assert(pt_channels_resize(&p.channels,16)==PT_CHANNEL_OK && pt_channels_route(&p.channels,15,PT_MIDI)==PT_CHANNEL_OK);
    p.events=&events[0][0];p.pattern_count=2;p.orders=orders;p.order_count=2;p.speed=6;p.bpm=125;
    events[63][15].effect=14;events[63][15].parameter=0xc3;
    assert(pt_project_validate(&p,NULL)==PT_PROJECT_OK);
    assert(pt_pattern_history_init(&history,&p,commands,4,changes,8)==PT_EDIT_OK);
    adapter.project=&p;adapter.history=&history;
    assert(pt_record_arm(&recorder,&settings,0,&adapter,pt_record_pattern_store));
    assert(pt_record_input(&recorder,0,0,60,90,1)==PT_RECORD_STORE_FAILED); /* Paula route */
    assert(pt_record_input(&recorder,0,15,60,90,1)==PT_RECORD_OK);
    assert(events[63][15].kind==PT_NOTE_MIDI && events[63][15].effect==14 && events[63][15].parameter==0xc3);
    assert(pt_record_input(&recorder,ROW(1),15,60,0,0)==PT_RECORD_OK && events[64][15].kind==PT_NOTE_OFF);
    assert(pt_pattern_undo(&p,&history,-1)==PT_EDIT_OK && events[64][15].kind==PT_NOTE_NONE);
    assert(pt_pattern_undo(&p,&history,-1)==PT_EDIT_OK && events[63][15].kind==PT_NOTE_NONE && events[63][15].effect==14);
    assert(pt_pattern_undo(&p,&history,1)==PT_EDIT_OK && events[63][15].kind==PT_NOTE_MIDI);
    assert(pt_record_arm(&recorder,&settings,0,&adapter,pt_record_pattern_store));
    assert(pt_record_input(&recorder,0,15,72,80,1)==PT_RECORD_STORE_FAILED && events[63][15].pitch==60);
    assert(pt_project_validate(&p,NULL)==PT_PROJECT_OK);
}
int main(void)
{
    struct pt_recorder r;struct pt_record_settings settings={0,256,1,1,1,0};unsigned i;
    assert(pt_record_arm(&r,&settings,ROW(100),NULL,store));
    assert(pt_record_input(&r,ROW(101),0,60,0,0)==PT_RECORD_IGNORED && !r.started && !calls);
    fail=1;assert(pt_record_input(&r,ROW(102),0,60,100,1)==PT_RECORD_STORE_FAILED && !r.started);
    fail=0;assert(pt_record_input(&r,ROW(104),0,60,100,1)==PT_RECORD_OK && r.started && r.origin==ROW(104));
    assert(events[0][0].kind==PT_NOTE_MIDI && events[0][0].velocity==100);
    assert(pt_record_input(&r,ROW(104)+100,0,61,100,1)==PT_RECORD_COLLISION && r.voices[0].note==60);
    /* A short note's release moves to the next representable row, preserving ON. */
    assert(pt_record_input(&r,ROW(104)+200,0,60,0,0)==PT_RECORD_OK && events[1][0].kind==PT_NOTE_OFF);
    assert(pt_record_input(&r,ROW(104)+300,0,62,100,1)==PT_RECORD_COLLISION);
    assert(pt_record_input(&r,ROW(106),0,64,100,1)==PT_RECORD_OK);
    assert(pt_record_input(&r,ROW(107),0,67,90,1)==PT_RECORD_OK);
    assert(pt_record_input(&r,ROW(107)+16384,0,64,0,0)==PT_RECORD_IGNORED && r.voices[0].active);
    assert(pt_record_input(&r,ROW(107)-1,0,67,0,0)==PT_RECORD_TIME);
    assert(pt_record_input(&r,ROW(107)+32768,0,67,0,1)==PT_RECORD_OK && events[4][0].kind==PT_NOTE_OFF);
    assert(pt_record_stop(&r,ROW(109))==PT_RECORD_OK && !r.armed);
    /* Pattern boundary: last-row note and next-pattern OFF; no silent wrapping. */
    memset(events,0,sizeof(events));settings.first_row=63;settings.hold=0;settings.velocity=0;
    assert(pt_record_arm(&r,&settings,0,NULL,store));
    assert(pt_record_input(&r,0,15,0,127,1)==PT_RECORD_OK && !events[63][15].flags);
    assert(pt_record_input(&r,1,15,0,0,0)==PT_RECORD_OK && events[64][15].kind==PT_NOTE_OFF);
    settings.first_row=255;assert(pt_record_arm(&r,&settings,0,NULL,store));
    assert(pt_record_input(&r,0,15,127,1,1)==PT_RECORD_OK);
    assert(pt_record_stop(&r,ROW(1))==PT_RECORD_CAPACITY && r.armed && r.voices[15].active);
    pt_record_cancel(&r);assert(!r.armed && !r.voices[15].active);
    /* Coarser grid, simultaneous tracks and partial stop failure are explicit. */
    memset(events,0,sizeof(events));settings.first_row=0;settings.grid_rows=4;
    assert(pt_record_arm(&r,&settings,0,NULL,store));
    for(i=0;i<16;++i)assert(pt_record_input(&r,ROW(2),i,60+i,80,1)==PT_RECORD_OK && events[4][i].kind==PT_NOTE_MIDI);
    fail=calls+3;assert(pt_record_stop(&r,ROW(3))==PT_RECORD_STORE_FAILED && r.armed && r.voices[2].active);
    for(i=0;i<16;++i)assert(r.voices[i].active==(i==2));
    fail=0;assert(pt_record_stop(&r,ROW(3))==PT_RECORD_OK && events[8][2].kind==PT_NOTE_OFF && !r.armed);
    assert(pt_record_arm(&r,&settings,0,NULL,store));
    assert(pt_record_input(&r,UINT64_MAX,0,60,100,1)==PT_RECORD_CAPACITY);
    assert(pt_record_input(&r,0,16,60,100,1)==PT_RECORD_INVALID);
    settings.grid_rows=0;assert(!pt_record_arm(&r,&settings,0,NULL,store));
    pattern_recording();
    puts("RECORD PASS: Hold Record, staged start, velocity, grid rounding, short-note OFF, collisions, stale release, monotonic time, capacity, retryable stop and undoable project-pattern recording with effect retention");return 0;
}
