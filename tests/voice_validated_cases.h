#include "../src/core/voice_internal.h"
/* The private path must match public setup and preserve all bounded refusals. */
static void validated_voice_fixture(void)
{
    int32_t data[16]={0,1,-2,3,-4,5,-6,7,8,-9,10,-11,12,-13,14,-15};
    struct pt_pcm p={data,16,8,48000,2,24},other;struct pt_voice a,b,before;
    unsigned bits,loop,i;int32_t left[2],right[2];
    for(bits=8;bits<=24;bits+=8) {
        p.bits=(uint8_t)bits;assert(pt_pcm_validate(&p)==PT_PCM_OK);
        for(loop=PT_VOICE_ONCE;loop<=PT_VOICE_PINGPONG;++loop) {
            uint32_t start=loop?2:0,end=loop?6:0;
            assert(pt_voice_init(&a,&p,0,8,(enum pt_voice_loop)loop,start,end,0x180000001ULL,1)==PT_PCM_OK);
            assert(pt_voice_init_validated(&b,&p,0,8,(enum pt_voice_loop)loop,start,end,0x180000001ULL,1)==PT_PCM_OK);
            assert(!memcmp(&a,&b,sizeof(a)));
            for(i=0;i<19;++i){assert(pt_voice_frame(&a,left)==PT_PCM_OK && pt_voice_frame(&b,right)==PT_PCM_OK);assert(!memcmp(left,right,sizeof(left)));}
            assert(!memcmp(&a,&b,sizeof(a)));
        }
        assert(pt_voice_init_segment(&a,&p,1,5,2,7,0x90000000ULL,1)==PT_PCM_OK);
        assert(pt_voice_init_segment_validated(&b,&p,1,5,2,7,0x90000000ULL,1)==PT_PCM_OK);
        other=p;assert(pt_voice_set_repeat_source(&a,&other,3,6)==PT_PCM_OK);
        assert(pt_voice_set_repeat_source_validated(&b,&other,3,6)==PT_PCM_OK);
        assert(!memcmp(&a,&b,sizeof(a)));
        for(i=0;i<19;++i){assert(pt_voice_frame(&a,left)==PT_PCM_OK && pt_voice_frame(&b,right)==PT_PCM_OK);assert(!memcmp(left,right,sizeof(left)));}
        assert(!memcmp(&a,&b,sizeof(a)));
    }
    before=b;p.capacity=1;
    assert(pt_voice_init_validated(&b,&p,0,8,PT_VOICE_ONCE,0,0,1,0)==PT_PCM_CAPACITY && !memcmp(&before,&b,sizeof(b)));
    assert(pt_voice_init_segment_validated(&b,&p,0,8,2,6,1,0)==PT_PCM_CAPACITY && !memcmp(&before,&b,sizeof(b)));
    other.capacity=1;
    assert(pt_voice_set_repeat_source_validated(&b,&other,2,6)==PT_PCM_CAPACITY && !memcmp(&before,&b,sizeof(b)));
    p.capacity=other.capacity=16;p.bits=7;
    assert(pt_voice_init_validated(&b,&p,0,8,PT_VOICE_ONCE,0,0,1,0)==PT_PCM_INVALID && !memcmp(&before,&b,sizeof(b)));
    p.bits=24;
    assert(pt_voice_init_validated(&b,&p,0,9,PT_VOICE_ONCE,0,0,1,0)==PT_PCM_INVALID && !memcmp(&before,&b,sizeof(b)));
    assert(pt_voice_init_segment_validated(&b,&p,0,8,2,9,1,0)==PT_PCM_INVALID && !memcmp(&before,&b,sizeof(b)));
    assert(pt_voice_set_repeat_source_validated(&b,&other,2,9)==PT_PCM_INVALID && !memcmp(&before,&b,sizeof(b)));
    {struct pt_pcm alias=p;alias.data=(int32_t *)&b;alias.frames=1;alias.capacity=2;
        assert(pt_voice_init_validated(&b,&alias,0,1,PT_VOICE_ONCE,0,0,1,0)==PT_PCM_ALIAS && !memcmp(&before,&b,sizeof(b)));
        assert(pt_voice_set_repeat_source_validated(&b,&alias,0,1)==PT_PCM_ALIAS && !memcmp(&before,&b,sizeof(b)));
    }
    /* New/untrusted source values still fail through every public API. */
    data[0]=8388608;
    assert(pt_voice_init(&b,&p,0,8,PT_VOICE_ONCE,0,0,1,0)==PT_PCM_INVALID && !memcmp(&before,&b,sizeof(b)));
    assert(pt_voice_init_segment(&b,&p,0,8,2,6,1,0)==PT_PCM_INVALID && !memcmp(&before,&b,sizeof(b)));
    assert(pt_voice_set_repeat_source(&b,&other,2,6)==PT_PCM_INVALID && !memcmp(&before,&b,sizeof(b)));
    data[0]=0;
    puts("VALIDATED VOICE PASS: 8/16/24-bit exact loop/segment/handoff parity, bounded refusal/alias atomicity, public value validation retained");
}
