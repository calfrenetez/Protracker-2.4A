#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "pitch.h"
int main(void)
{
    struct pt_project p;struct pt_flow f;struct pt_pitch s,expected;struct pt_event events[16];uint16_t order=0;unsigned ch,param,fresh;
    memset(&p,0,sizeof(p));memset(&f,0,sizeof(f));memset(events,0,sizeof(events));
    pt_channels_init(&p.channels);assert(pt_channels_resize(&p.channels,16)==PT_CHANNEL_OK);p.events=events;p.orders=&order;f.project=&p;
    for(fresh=0;fresh<2;++fresh)for(param=0;param<256;++param) {
        pt_pitch_init(&s);f.fresh=(uint8_t)fresh;f.counter=fresh?0:2;
        for(ch=0;ch<16;++ch) {
            struct pt_pitch_channel *v=s.channel+ch;
            v->period=65394;v->output=3986;v->empty=0;v->sounding=v->instrument=1;
            v->vib_phase=92;v->vib_command=0x4f;v->vib_control=2;v->gliss=1;
            events[ch].effect=f.effect[ch]=(ch&1)?14:8;
            events[ch].parameter=f.parameter[ch]=(uint8_t)((ch&1)?0x80|(param&15):param);
        }
        expected=s;for(ch=0;ch<16;ch+=2)expected.channel[ch].output=expected.channel[ch].period;
        pt_pitch_tick(&s,&f,65535);assert(!memcmp(&s,&expected,sizeof(s)));
    }
    /* An E8 row still inherits the old-packed-empty PerNop shortcut. */
    f.fresh=1;f.counter=0;pt_pitch_init(&s);
    for(ch=0;ch<16;++ch) {s.channel[ch].period=428;s.channel[ch].output=450;events[ch].effect=f.effect[ch]=14;events[ch].parameter=f.parameter[ch]=0x8f;}
    pt_pitch_tick(&s,&f,65535);
    for(ch=0;ch<16;++ch)assert(s.channel[ch].output==428 && !s.channel[ch].empty);
    puts("UNUSED PASS: all8xx/E8x parameters across16 tracks, stored/output restoration versus hold, no modulation-state mutation");return 0;
}
