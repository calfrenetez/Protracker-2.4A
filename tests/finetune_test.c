#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "pitch.h"
int main(void)
{
    static const unsigned expected[]={428,425,422,419,416,413,410,407,453,450,447,444,441,437,434,431};
    struct pt_project p;struct pt_sample samples[16];struct pt_event events[64*16];struct pt_flow f;struct pt_pitch state;uint16_t order=0;unsigned ch;
    memset(&p,0,sizeof(p));memset(samples,0,sizeof(samples));memset(events,0,sizeof(events));memset(&f,0,sizeof(f));pt_pitch_init(&state);
    pt_channels_init(&p.channels);assert(pt_channels_resize(&p.channels,16)==PT_CHANNEL_OK);
    p.samples=samples;p.sample_count=16;p.events=events;p.orders=&order;p.order_count=p.pattern_count=1;f.project=&p;f.fresh=1;
    for(ch=0;ch<16;++ch) {
        samples[ch].finetune=(int8_t)(ch<8?(int)ch:(int)ch-16);
        events[ch].kind=PT_NOTE_PERIOD;events[ch].pitch=428;events[ch].instrument=(uint8_t)(ch+1);
    }
    pt_pitch_tick(&state,&f,65535);
    for(ch=0;ch<16;++ch)assert(state.channel[ch].period==expected[ch] && state.channel[ch].finetune==ch);
    events[0].effect=f.effect[0]=14;events[0].parameter=f.parameter[0]=0x57;
    pt_pitch_tick(&state,&f,1);assert(state.channel[0].period==407);
    events[0].effect=f.effect[0]=0;events[0].parameter=f.parameter[0]=0;
    pt_pitch_tick(&state,&f,1);assert(state.channel[0].period==428 && !state.channel[0].finetune);
    events[0].instrument=0;events[0].effect=f.effect[0]=14;events[0].parameter=f.parameter[0]=0x5f;
    pt_pitch_tick(&state,&f,1);assert(state.channel[0].period==431);
    events[0].kind=PT_NOTE_NONE;events[0].parameter=f.parameter[0]=0x57;
    pt_pitch_tick(&state,&f,1);assert(state.channel[0].period==431 && state.channel[0].output==431);
    events[0].kind=PT_NOTE_PERIOD;events[0].pitch=427;events[0].effect=f.effect[0]=0;events[0].parameter=f.parameter[0]=0;
    pt_pitch_tick(&state,&f,1);assert(state.channel[0].period==384);
    events[8].instrument=0;events[8].pitch=214;events[8].effect=f.effect[8]=3;events[8].parameter=f.parameter[8]=7;
    pt_pitch_tick(&state,&f,256);assert(state.channel[8].target==226 && state.channel[8].period==453);
    events[15].pitch=113;pt_pitch_tick(&state,&f,32768);assert(state.channel[15].period==114);
    f.fresh=0;f.counter=1;f.parameter[15]=255;pt_pitch_tick(&state,&f,32768);assert(state.channel[15].output==12851);
    puts("FINETUNE PASS: 16 tables, E5 ordering/memory/reload, ordinary quantization, negative tone target and final overflow words");return 0;
}
