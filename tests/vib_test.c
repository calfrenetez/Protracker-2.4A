#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "pitch.h"
int main(void)
{
    static const unsigned wave_delta[3][8]={{0,21,29,21,0,21,29,21},{0,7,15,22,29,22,14,7},{29,29,29,29,29,29,29,29}};
    struct pt_project p;struct pt_flow f;struct pt_pitch s;struct pt_event events[16];uint16_t order=0;unsigned ch,phase;
    memset(&p,0,sizeof(p));pt_channels_init(&p.channels);assert(pt_channels_resize(&p.channels,16)==PT_CHANNEL_OK);
    memset(&f,0,sizeof(f));memset(events,0,sizeof(events));p.events=events;p.orders=&order;f.project=&p;
    for(phase=0;phase<8;++phase) {
        pt_pitch_init(&s);
        for(ch=0;ch<16;++ch) {
            s.channel[ch].period=428;s.channel[ch].vib_command=0x4f;s.channel[ch].vib_phase=(uint8_t)(phase*32);s.channel[ch].vib_control=(uint8_t)ch;
            f.effect[ch]=6;f.parameter[ch]=0xf1;
        }
        pt_pitch_tick(&s,&f,65535);
        for(ch=0;ch<16;++ch) {
            unsigned wave=ch&3,delta=wave_delta[wave>=2?2:wave][phase];
            assert(s.channel[ch].period==428 && s.channel[ch].output==(phase<4?428+delta:428-delta));
            assert(s.channel[ch].vib_command==0x4f && s.channel[ch].vib_phase==(uint8_t)(phase*32+16));
        }
    }
    for(ch=0;ch<16;++ch) {f.effect[ch]=4;f.parameter[ch]=(uint8_t)(ch<<4);}
    pt_pitch_tick(&s,&f,65535);
    for(ch=0;ch<16;++ch)assert(s.channel[ch].vib_command==(ch?(ch<<4)|15:0x4f));
    for(ch=0;ch<16;++ch)f.parameter[ch]=(uint8_t)ch;
    pt_pitch_tick(&s,&f,65535);
    for(ch=0;ch<16;++ch)assert(s.channel[ch].vib_command==(ch?(ch<<4)|ch:0x4f));
    /* A new note checks the old reset flag before executing its E4 command. */
    f.fresh=1;s.channel[15].vib_phase=92;s.channel[15].vib_control=4;
    events[15].kind=PT_NOTE_PERIOD;events[15].pitch=404;events[15].effect=f.effect[15]=14;events[15].parameter=f.parameter[15]=0x40;
    pt_pitch_tick(&s,&f,0x8000);assert(s.channel[15].vib_phase==92 && !s.channel[15].vib_control);
    f.parameter[15]=events[15].parameter=0x44;pt_pitch_tick(&s,&f,0x8000);assert(!s.channel[15].vib_phase && s.channel[15].vib_control==4);
    /* Tone targets do not reset vibrato, even when ordinary notes would. */
    s.channel[15].vib_control=0;s.channel[15].vib_phase=92;events[15].effect=f.effect[15]=3;events[15].pitch=214;
    pt_pitch_tick(&s,&f,0x8000);assert(s.channel[15].vib_phase==92);
    /* Native word arithmetic wraps without pitch clamping. */
    f.fresh=0;f.effect[15]=6;s.channel[15].vib_control=2;s.channel[15].vib_command=15;
    s.channel[15].period=65535;s.channel[15].vib_phase=0;pt_pitch_tick(&s,&f,0x8000);assert(s.channel[15].output==28);
    s.channel[15].period=1;s.channel[15].vib_phase=128;pt_pitch_tick(&s,&f,0x8000);assert(s.channel[15].output==65508);
    puts("VIBRATO boundaries PASS: 16 tracks, waveforms, independent nibble memory, combined-slide memory, reset ordering and word wrap");
    return 0;
}
