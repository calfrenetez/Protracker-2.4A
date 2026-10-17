#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "record_channel.h"
#include "record_pattern.h"

#define ROW(n) ((uint64_t)(n)<<16)
#define EVENTS (2U*64U*16U)
struct fixture {
    struct pt_project project;
    struct pt_pattern_history history;
    struct pt_pattern_command commands[32];
    struct pt_event_change changes[64];
    struct pt_recorder recorder;
    struct pt_record_pattern store;
    struct pt_event events[EVENTS];
    struct pt_sample sample;
    int32_t pcm[4];uint32_t slices[2];uint16_t orders[2];
};
static struct fixture *make(uint32_t first,uint32_t limit,unsigned grid,unsigned hold)
{
    struct fixture *f=calloc(1,sizeof(*f));
    struct pt_record_settings settings={first,limit,(uint8_t)grid,(uint8_t)hold,1,0};
    assert(f);pt_channels_init(&f->project.channels);
    assert(pt_channels_resize(&f->project.channels,16)==PT_CHANNEL_OK);
    assert(pt_channels_route(&f->project.channels,14,PT_MIDI)==PT_CHANNEL_OK);
    assert(pt_channels_route(&f->project.channels,15,PT_MIDI)==PT_CHANNEL_OK);
    f->project.channels.selected=15;f->project.channels.track[15].midi_channel=3;
    strcpy(f->project.channels.track[15].name,"Recorded");
    strcpy(f->project.title,"Channel input fixture");
    strcpy(f->project.midi_input,"Configured input");
    strcpy(f->project.midi_output[15],"External output");
    f->project.bpm=125;f->project.speed=6;f->project.orders=f->orders;
    f->project.order_count=2;f->project.pattern_count=2;f->orders[1]=1;
    f->project.events=f->events;f->project.samples=&f->sample;f->project.sample_count=1;
    f->pcm[0]=8388607;f->pcm[1]=1;f->pcm[2]=-1;f->pcm[3]=-8388608;
    f->sample.pcm=(struct pt_pcm){f->pcm,4,4,48000,1,24};f->sample.volume=64;
    f->slices[1]=2;f->sample.slices=f->slices;f->sample.slice_count=2;
    assert(pt_project_validate(&f->project,NULL)==PT_PROJECT_OK);
    assert(pt_pattern_history_init(&f->history,&f->project,f->commands,32,f->changes,64)==PT_EDIT_OK);
    f->store.project=&f->project;f->store.history=&f->history;
    assert(pt_record_arm(&f->recorder,&settings,0,&f->store,pt_record_pattern_store));
    return f;
}
static void drop(struct fixture *f)
{
    assert(!f->recorder.armed);pt_pattern_history_release(&f->history);free(f);
}
static enum pt_record_result message(struct fixture *f,uint64_t position,unsigned track,
    unsigned filter,unsigned status,unsigned note,unsigned velocity)
{
    uint8_t bytes[3]={(uint8_t)status,(uint8_t)note,(uint8_t)velocity};
    return pt_record_channel_message(&f->recorder,position,track,filter,bytes,sizeof(bytes));
}
static void unchanged(struct fixture *f,unsigned track,unsigned filter,const uint8_t *data,
    size_t bytes,enum pt_record_result expected)
{
    struct fixture *before=malloc(sizeof(*before));assert(before);memcpy(before,f,sizeof(*before));
    assert(pt_record_channel_message(&f->recorder,UINT64_MAX,track,filter,data,bytes)==expected);
    assert(!memcmp(before,f,sizeof(*f)));free(before);
}
static void invalid_and_filtered(void)
{
    struct fixture *f=make(0,128,1,1);unsigned channel,kind;
    const uint8_t on[3]={0x9f,60,100},program[2]={0xcf,7},pressure[2]={0xdf,127};
    const uint8_t status_low[3]={0x7f,60,100},system[3]={0xf0,60,100};
    const uint8_t bad_note[3]={0x9f,128,100},bad_velocity[3]={0x9f,60,128};
    const uint8_t malformed_filtered[3]={0x90,60,255};
    unchanged(f,15,16,NULL,3,PT_RECORD_INVALID);
    unchanged(f,15,16,on,0,PT_RECORD_INVALID);unchanged(f,15,16,on,1,PT_RECORD_INVALID);
    unchanged(f,15,16,on,2,PT_RECORD_INVALID);unchanged(f,15,16,on,4,PT_RECORD_INVALID);
    unchanged(f,15,16,on,SIZE_MAX,PT_RECORD_INVALID);
    unchanged(f,15,16,(const uint8_t *)(uintptr_t)UINTPTR_MAX,3,PT_RECORD_INVALID);
    unchanged(f,16,16,on,3,PT_RECORD_INVALID);unchanged(f,UINT_MAX,16,on,3,PT_RECORD_INVALID);
    unchanged(f,15,0,on,3,PT_RECORD_INVALID);unchanged(f,15,17,on,3,PT_RECORD_INVALID);
    unchanged(f,15,UINT_MAX,on,3,PT_RECORD_INVALID);
    unchanged(f,15,16,status_low,3,PT_RECORD_INVALID);unchanged(f,15,16,system,3,PT_RECORD_INVALID);
    unchanged(f,15,16,bad_note,3,PT_RECORD_INVALID);unchanged(f,15,16,bad_velocity,3,PT_RECORD_INVALID);
    unchanged(f,15,16,malformed_filtered,3,PT_RECORD_INVALID);
    unchanged(f,15,16,program,2,PT_RECORD_IGNORED);unchanged(f,15,16,pressure,2,PT_RECORD_IGNORED);
    unchanged(f,15,16,program,3,PT_RECORD_INVALID);unchanged(f,15,16,pressure,3,PT_RECORD_INVALID);
    for(channel=0;channel<16;++channel){
        uint8_t data[3]={(uint8_t)(0x90|channel),60,100};
        if(channel!=15)unchanged(f,15,16,data,3,PT_RECORD_IGNORED);
        for(kind=0xa0;kind<=0xe0;kind+=0x10){
            data[0]=(uint8_t)(kind|channel);
            unchanged(f,15,16,data,kind==0xc0||kind==0xd0?2:3,PT_RECORD_IGNORED);
        }
    }
    assert(!f->recorder.started&&!f->history.count);
    assert(pt_record_channel_message(NULL,0,15,16,on,3)==PT_RECORD_INVALID);
    f->recorder.store=NULL;unchanged(f,15,16,on,3,PT_RECORD_INVALID);
    f->recorder.store=pt_record_pattern_store;pt_record_cancel(&f->recorder);
    unchanged(f,15,16,on,3,PT_RECORD_INVALID);drop(f);
}
static void save_roundtrip(const struct fixture *f)
{
    struct fixture *decoded=calloc(1,sizeof(*decoded));struct pt_project_storage storage;
    struct pt_project_requirements need;uint8_t *encoded,*again;size_t size,written=0,rewritten=0;
    assert(decoded);assert(pt_project_size(&f->project,&size)==PT_PROJECT_OK);
    encoded=malloc(size);again=malloc(size);assert(encoded&&again);
    assert(pt_project_encode(&f->project,encoded,size,&written)==PT_PROJECT_OK&&written==size);
    assert(pt_project_probe(encoded,size,&need)==PT_PROJECT_OK);
    assert(need.events==EVENTS&&need.orders==2&&need.samples==1&&need.pcm_values==4&&need.slices==2);
    memset(&storage,0,sizeof(storage));storage.orders=decoded->orders;storage.order_capacity=2;
    storage.events=decoded->events;storage.event_capacity=EVENTS;
    storage.samples=&decoded->sample;storage.sample_capacity=1;
    storage.pcm=decoded->pcm;storage.pcm_capacity=4;storage.slices=decoded->slices;storage.slice_capacity=2;
    assert(pt_project_decode(encoded,size,&storage,&decoded->project)==PT_PROJECT_OK);
    assert(!memcmp(decoded->events,f->events,sizeof(f->events)));
    assert(!memcmp(decoded->pcm,f->pcm,sizeof(f->pcm)));
    assert(!memcmp(decoded->project.channels.track,f->project.channels.track,sizeof(f->project.channels.track)));
    assert(!memcmp(decoded->project.midi_input,f->project.midi_input,PT_MIDI_ENDPOINT));
    assert(!memcmp(decoded->project.midi_output,f->project.midi_output,sizeof(f->project.midi_output)));
    assert(decoded->sample.pcm.bits==24&&decoded->sample.pcm.rate==48000);
    assert(pt_project_encode(&decoded->project,again,size,&rewritten)==PT_PROJECT_OK&&rewritten==size);
    assert(!memcmp(encoded,again,size));free(again);free(encoded);free(decoded);
}
static void hold_boundary_save_undo(void)
{
    struct fixture *f=make(63,128,1,1);struct pt_event *on=&f->events[63*16+15];
    struct pt_event *off=&f->events[64*16+15];struct pt_channels routes=f->project.channels;
    const uint8_t unselected[3]={0x90,60,90};
    on->effect=14;on->parameter=0xc3;on->instrument=1;on->slice=2;on->flags=1;on->velocity=13;
    assert(pt_project_validate(&f->project,NULL)==PT_PROJECT_OK);
    unchanged(f,15,16,unselected,3,PT_RECORD_IGNORED);
    assert(message(f,ROW(104),15,16,0x8f,60,127)==PT_RECORD_IGNORED);
    assert(!f->recorder.started&&f->recorder.last_position==ROW(104)&&!f->history.count);
    assert(message(f,ROW(105),15,16,0x9f,60,91)==PT_RECORD_OK);
    assert(f->recorder.started&&f->recorder.origin==ROW(105));
    assert(on->kind==PT_NOTE_MIDI&&on->pitch==60&&on->velocity==91&&on->flags==1);
    assert(on->effect==14&&on->parameter==0xc3&&on->instrument==1&&!on->slice);
    assert(message(f,ROW(105)+1,15,16,0x9f,60,0)==PT_RECORD_OK);
    assert(off->kind==PT_NOTE_OFF&&!off->pitch&&!f->recorder.voices[15].active);
    assert(f->history.count==2&&f->history.cursor==2&&!memcmp(&routes,&f->project.channels,sizeof(routes)));
    assert(pt_record_stop(&f->recorder,ROW(106))==PT_RECORD_OK);save_roundtrip(f);
    pt_pattern_mark_saved(&f->history);assert(!pt_pattern_dirty(&f->history));
    assert(pt_pattern_undo(&f->project,&f->history,-1)==PT_EDIT_OK&&off->kind==PT_NOTE_NONE);
    assert(pt_pattern_dirty(&f->history));
    assert(pt_pattern_undo(&f->project,&f->history,-1)==PT_EDIT_OK&&on->kind==PT_NOTE_NONE);
    assert(on->effect==14&&on->parameter==0xc3&&on->instrument==1&&on->slice==2&&on->velocity==13);
    assert(pt_pattern_undo(&f->project,&f->history,1)==PT_EDIT_OK&&on->kind==PT_NOTE_MIDI);
    assert(pt_pattern_undo(&f->project,&f->history,1)==PT_EDIT_OK&&off->kind==PT_NOTE_OFF);
    assert(!pt_pattern_dirty(&f->history));save_roundtrip(f);drop(f);
}
static void collision_and_stale_release(void)
{
    struct fixture *f=make(0,128,1,0);size_t cursor;
    assert(message(f,0,15,16,0x9f,60,100)==PT_RECORD_OK);
    assert(message(f,0,15,16,0x9f,62,90)==PT_RECORD_COLLISION);
    assert(f->recorder.voices[15].note==60&&f->events[15].pitch==60&&f->history.cursor==1);
    assert(message(f,ROW(1),15,16,0x9f,62,90)==PT_RECORD_OK);cursor=f->history.cursor;
    assert(message(f,ROW(2),15,16,0x8f,60,127)==PT_RECORD_IGNORED);
    assert(f->recorder.voices[15].active&&f->recorder.voices[15].note==62);
    assert(f->recorder.last_position==ROW(2)&&f->history.cursor==cursor);
    assert(message(f,ROW(3),15,16,0x8f,62,127)==PT_RECORD_OK);
    assert(f->events[3*16+15].kind==PT_NOTE_OFF&&!f->recorder.voices[15].active);
    assert(f->events[2*16+15].kind==PT_NOTE_NONE);
    assert(pt_record_stop(&f->recorder,ROW(4))==PT_RECORD_OK);drop(f);
}
static void route_and_occupied_store_refusal(void)
{
    struct fixture *f=make(0,128,1,1),*before=malloc(sizeof(*before));assert(before);
    memcpy(before,f,sizeof(*f));before->recorder.last_position=ROW(1);
    assert(message(f,ROW(1),0,16,0x9f,60,100)==PT_RECORD_STORE_FAILED);
    assert(!memcmp(before,f,sizeof(*f))&&!f->recorder.started);
    memcpy(before,f,sizeof(*f));before->recorder.last_position=ROW(2);
    assert(message(f,ROW(2),5,16,0x9f,60,100)==PT_RECORD_STORE_FAILED);
    assert(!memcmp(before,f,sizeof(*f))&&!f->recorder.started);
    f->events[15].kind=PT_NOTE_MIDI;f->events[15].pitch=71;
    assert(pt_project_validate(&f->project,NULL)==PT_PROJECT_OK);
    memcpy(before,f,sizeof(*f));before->recorder.last_position=ROW(3);
    assert(message(f,ROW(3),15,16,0x9f,60,100)==PT_RECORD_STORE_FAILED);
    assert(!memcmp(before,f,sizeof(*f))&&!f->recorder.started);
    assert(pt_record_stop(&f->recorder,ROW(4))==PT_RECORD_OK);free(before);drop(f);
}
static void time_and_release_capacity(void)
{
    struct fixture *f=make(0,128,1,0),*before=malloc(sizeof(*before));assert(before);
    assert(message(f,ROW(10),15,1,0x90,0,1)==PT_RECORD_OK);memcpy(before,f,sizeof(*f));
    assert(message(f,ROW(9),15,1,0x80,0,127)==PT_RECORD_TIME);
    assert(!memcmp(before,f,sizeof(*f)));
    assert(pt_record_stop(&f->recorder,ROW(11))==PT_RECORD_OK);drop(f);
    f=make(127,128,1,0);
    assert(message(f,0,15,1,0x90,127,127)==PT_RECORD_OK);
    assert(message(f,1,15,1,0x80,127,127)==PT_RECORD_CAPACITY);
    assert(f->recorder.armed&&f->recorder.voices[15].active&&f->history.cursor==1);
    assert(pt_record_stop(&f->recorder,ROW(1))==PT_RECORD_CAPACITY&&f->recorder.armed);
    /* Explicit local cancel is not an external MIDI/audio quiet receipt. */
    pt_record_cancel(&f->recorder);free(before);drop(f);
}
static void all_channels_and_explicit_track_binding(void)
{
    unsigned channel;struct fixture *f;
    for(channel=1;channel<=16;++channel){
        f=make(0,128,1,1);
        assert(message(f,0,15,channel,0x90|(channel-1),channel==1?0:127,127)==PT_RECORD_OK);
        assert(message(f,1,15,channel,0x80|(channel-1),channel==1?0:127,127)==PT_RECORD_OK);
        assert(f->project.channels.track[15].midi_channel==3&&f->history.count==2);
        assert(pt_record_stop(&f->recorder,ROW(2))==PT_RECORD_OK);drop(f);
    }
    f=make(0,128,1,0);
    assert(message(f,0,15,16,0x9f,64,80)==PT_RECORD_OK);
    assert(message(f,ROW(1),14,16,0x8f,64,64)==PT_RECORD_IGNORED);
    assert(f->recorder.voices[15].active&&!f->recorder.voices[14].active&&f->history.cursor==1);
    assert(message(f,ROW(2),15,16,0x8f,64,64)==PT_RECORD_OK);
    assert(!f->recorder.voices[15].active&&f->history.cursor==2);
    assert(pt_record_stop(&f->recorder,ROW(3))==PT_RECORD_OK);drop(f);
}
int main(void)
{
    invalid_and_filtered();hold_boundary_save_undo();collision_and_stale_release();
    route_and_occupied_store_refusal();time_and_release_capacity();all_channels_and_explicit_track_binding();
    puts("RECORD CHANNEL HOST PASS:6 groups;complete messages;filter/Hold;row63/zeroOff;real project/save/undo;collision/stale;explicit tracks;NO CAMD/TIMING/OUTPUT");
    return 0;
}
