#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "pitch.h"
int main(void)
{
    struct pt_project p;struct pt_event events[64*16];struct pt_flow f;struct pt_pitch state;uint16_t order=0;unsigned ch;
    memset(&p,0,sizeof(p));memset(events,0,sizeof(events));memset(&f,0,sizeof(f));pt_pitch_init(&state);
    pt_channels_init(&p.channels);assert(pt_channels_resize(&p.channels,16)==PT_CHANNEL_OK);
    p.events=events;p.orders=&order;p.order_count=p.pattern_count=1;f.project=&p;f.fresh=1;
    for(ch=0;ch<16;++ch) {
        state.channel[ch].period=state.channel[ch].output=428;state.channel[ch].instrument=state.channel[ch].sounding=1;
        events[ch].effect=f.effect[ch]=14;events[ch].parameter=f.parameter[ch]=(uint8_t)(0x31+ch%15);
    }
    pt_pitch_tick(&state,&f,65535);
    for(ch=0;ch<16;++ch) {
        assert(state.channel[ch].gliss==1+ch%15);
        events[ch].kind=PT_NOTE_PERIOD;events[ch].pitch=214;events[ch].effect=f.effect[ch]=3;events[ch].parameter=f.parameter[ch]=(uint8_t)(ch+1);
    }
    pt_pitch_tick(&state,&f,65535);f.fresh=0;f.counter=1;pt_pitch_tick(&state,&f,65535);
    for(ch=0;ch<16;++ch)assert(state.channel[ch].period==427-ch && state.channel[ch].output==404);
    f.counter=2;pt_pitch_tick(&state,&f,65535);
    for(ch=0;ch<16;++ch)assert(state.channel[ch].period==426-2*ch && state.channel[ch].output==(426-2*ch>=404?404:381));
    f.fresh=1;f.counter=0;
    for(ch=0;ch<16;++ch) {events[ch].kind=PT_NOTE_NONE;events[ch].effect=f.effect[ch]=14;events[ch].parameter=f.parameter[ch]=0x30;}
    pt_pitch_tick(&state,&f,65535);f.fresh=0;f.counter=1;
    for(ch=0;ch<16;++ch) {f.effect[ch]=3;f.parameter[ch]=0;}
    pt_pitch_tick(&state,&f,65535);
    for(ch=0;ch<16;++ch)assert(!state.channel[ch].gliss && state.channel[ch].period==425-3*ch && state.channel[ch].output==425-3*ch);
    puts("GLISS PASS: 16 independent controls, smooth stored periods, stepped output and disable preserves speed/target");return 0;
}
