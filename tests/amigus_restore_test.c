#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "amigus_render_voice.h"
static int restore_fixture_main(void)
{
    int32_t data[16]={1,257,-513,799,123,991,-777,27};
    struct pt_pcm pcm={data,16,16,48000,1,24};struct pt_voice v,initial;
    struct pt_playback_format f={16,0,0,0};uint32_t gains[2]={65536,12345};
    struct pt_amigus_restore_plan plan,sentinel;struct pt_amigus_voice_plan trigger;
    unsigned bits,loop,i;uint64_t expected;
    memset(&sentinel,0x5a,sizeof(sentinel));
    for(bits=8;bits<=16;bits+=8)for(loop=PT_VOICE_ONCE;loop<=PT_VOICE_FORWARD;++loop) {
        f.bits=(uint8_t)bits;
        assert(pt_voice_init(&v,&pcm,2,16,(enum pt_voice_loop)loop,loop?4:0,loop?14:0,0x90000000ULL,1)==PT_PCM_OK);
        initial=v;
        assert(pt_amigus_render_voice(&initial,48000,gains,&f,256,16*(bits/8),&trigger));
        for(i=0;i<64;++i) {
            if(v.active) {
                expected=v.phase+(v.looped?((uint64_t)v.loop_start<<32):0);
                assert(pt_amigus_render_restore(&v,48000,gains,&f,256,16*(bits/8),&plan));
                assert(plan.cursor_q32==((uint64_t)256<<32)+expected*(bits/8));
                assert(!memcmp(&plan.bounds,&trigger,sizeof(trigger)));
                /* Recover the EXACT original source frame/fraction independently
                   from the cache byte cursor; no phase bit may disappear. */
                assert((plan.cursor_q32-((uint64_t)256<<32))/(bits/8)==expected);
            } else {
                plan=sentinel;assert(!pt_amigus_render_restore(&v,48000,gains,&f,256,16*(bits/8),&plan));
                assert(!memcmp(&plan,&sentinel,sizeof(plan)));
            }
            assert(pt_voice_advance(&v,1,1)==PT_PCM_OK);
        }
    }
    f.bits=16;assert(pt_voice_init(&v,&pcm,2,16,PT_VOICE_FORWARD,4,14,0x90000000ULL,1)==PT_PCM_OK);
    initial=v;plan=sentinel;
#define REFUSE() do {assert(!pt_amigus_render_restore(&v,48000,gains,&f,256,32,&plan));assert(!memcmp(&plan,&sentinel,sizeof(plan)));v=initial;} while(0)
    v.phase=((uint64_t)4<<32);REFUSE();v.phase=((uint64_t)2<<32)-1;REFUSE();
    v.cycle++;REFUSE();v.looped=1;v.phase=v.cycle;REFUSE();v.active=2;REFUSE();
    v.segment=1;REFUSE();v.repeat_pcm=&pcm;REFUSE();v.loop=PT_VOICE_PINGPONG;REFUSE();
    v.looped=2;REFUSE();v.loop_end=17;REFUSE();v.step=0;REFUSE();
    pcm.channels=2;REFUSE();pcm.channels=1;
    assert(!pt_amigus_render_restore(NULL,48000,gains,&f,256,32,&plan));
    assert(!pt_amigus_render_restore(&v,48000,gains,&f,256,32,NULL));
    assert(!pt_amigus_render_restore(&v,48000,NULL,&f,256,32,&plan));
    assert(!pt_amigus_render_restore(&v,48000,gains,NULL,256,32,&plan));
    assert(!pt_amigus_render_restore(&v,48000,gains,&f,257,32,&plan));
    assert(!pt_amigus_render_restore(&v,48000,gains,&f,0x02000000-30,32,&plan));
    assert(!memcmp(&plan,&sentinel,sizeof(plan)));
    v.looped=1;v.phase=v.cycle-1;
    assert(pt_amigus_render_restore(&v,48000,gains,&f,0x02000000-34,32,&plan));
    assert(plan.cursor_q32<((uint64_t)PT_AMIGUS_RAM_ADDRESS_SPACE<<32));
    assert((uint32_t)plan.cursor_q32==0xfffffffeUL);
    /* Same state is deliberately refused by the ordinary initialized trigger. */
    assert(!pt_amigus_render_voice(&v,48000,gains,&f,256,32,&trigger));
    assert(pcm.bits==24 && data[1]==257 && data[2]==-513);
    puts("AMIGUS RESTORE PASS: exact fractional cache cursor, head/loop/one-shot bounds,8/16-bit cache and24-bit master preserved; software only");return 0;
}
#ifndef PT_AMIGUS_RESTORE_NATIVE
int main(void){return restore_fixture_main();}
#endif
