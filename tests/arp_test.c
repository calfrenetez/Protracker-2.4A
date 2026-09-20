#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "pitch.h"
int main(void)
{
    static const unsigned high[]={113,0,850,802,757,715,674,637,601,567,535,505,477,450,425,401};
    struct pt_project p;struct pt_flow f;struct pt_pitch s;unsigned ch;
    memset(&p,0,sizeof(p));pt_channels_init(&p.channels);assert(pt_channels_resize(&p.channels,16)==PT_CHANNEL_OK);
    memset(&f,0,sizeof(f));f.project=&p;pt_pitch_init(&s);
    for(ch=0;ch<16;++ch) {s.channel[ch].period=113;f.parameter[ch]=(uint8_t)(((15-ch)<<4)|ch);}
    f.counter=1;pt_pitch_tick(&s,&f,65535);
    for(ch=0;ch<16;++ch)assert(s.channel[ch].period==113 && s.channel[ch].output==high[15-ch]);
    f.counter=2;pt_pitch_tick(&s,&f,65535);
    for(ch=0;ch<16;++ch)assert(s.channel[ch].period==113 && s.channel[ch].output==high[ch]);
    /* Zero parameter is no effect: it does not write back the base period. */
    memset(f.parameter,0,sizeof(f.parameter));f.counter=3;pt_pitch_tick(&s,&f,65535);
    for(ch=0;ch<16;++ch)assert(s.channel[ch].output==high[ch]);
    f.parameter[15]=0xff;s.channel[15].period=0;f.counter=1;pt_pitch_tick(&s,&f,0x8000);
    assert(s.channel[15].output==379);for(ch=0;ch<15;++ch)assert(s.channel[ch].output==high[ch]);
    s.channel[15].period=426;f.parameter[15]=0x10;f.counter=2;pt_pitch_tick(&s,&f,0x8000);
    assert(s.channel[15].period==426 && s.channel[15].output==404);
    f.counter=3;pt_pitch_tick(&s,&f,0x8000);assert(s.channel[15].output==426);
    f.parameter[15]=0xff;s.channel[15].period=65394;f.counter=3;pt_pitch_tick(&s,&f,0x8000);assert(s.channel[15].output==65394);
    f.counter=33;pt_pitch_tick(&s,&f,0x8000);assert(s.channel[15].output==360);
    puts("ARPEGGIO boundaries PASS: 16 tracks, nibble phases, sentinel/adjacent table, mask, inactive base and wrapped counter");
    return 0;
}
