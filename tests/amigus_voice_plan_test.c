#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "amigus_voice_plan.h"

static void check_voice_registers(void)
{
    struct pt_amigus_voice_plan plan = {
        .start = 0x10002, .loop = 0x10004, .end_exclusive = 0x20000, .rate = 0x12345678,
        .control = 0x800f, .left = 4321, .right = 1234
    };
    static const struct pt_amigus_voice_register_write looped[] = {
        {0x1e,31}, {0x20,0}, {0x26,1}, {0x28,4}, {0x22,1}, {0x24,2},
        {0x2a,1}, {0x2c,0xfffe}, {0x2e,0x1234}, {0x30,0x5678}, {0x32,4321}, {0x34,1234}, {0x20,0x800f}
    };
    struct pt_amigus_voice_register_program program, previous;
    unsigned i;
    assert(pt_amigus_voice_trigger_registers(31, &plan, &program));
    assert(program.count == sizeof(looped) / sizeof(*looped));
    for (i = 0; i < program.count; ++i) {
        assert(program.write[i].offset == looped[i].offset);
        assert(program.write[i].value == looped[i].value);
    }
    assert(plan.end_exclusive == 0x20000);

    plan.start = 0x1fffe;
    plan.control = 0x8000;
    assert(pt_amigus_voice_trigger_registers(0, &plan, &program));
    assert(program.count == 11 && program.write[2].offset == 0x22);
    assert(program.write[3].offset == 0x24 && program.write[3].value == 0xfffe);
    assert(program.write[5].offset == 0x2c && program.write[5].value == 0xfffe);
    assert(program.write[10].offset == 0x20 && program.write[10].value == 0x8000);
    assert(pt_amigus_voice_stop_registers(5, &program));
    assert(program.count == 2 && program.write[0].offset == 0x1e && program.write[0].value == 5);
    assert(program.write[1].offset == 0x20 && !program.write[1].value);
    for (i = program.count; i < PT_AMIGUS_VOICE_REGISTER_WRITES; ++i)
        assert(!program.write[i].offset && !program.write[i].value);
    memcpy(&previous, &program, sizeof(program));
    assert(!pt_amigus_voice_stop_registers(32, &program));
    assert(!pt_amigus_voice_trigger_registers(32, &plan, &program));
    --plan.start;
    assert(!pt_amigus_voice_trigger_registers(5, &plan, &program));
    ++plan.start;
    plan.control |= 8;
    assert(!pt_amigus_voice_trigger_registers(5, &plan, &program));
    plan.control = 0x8000;
    plan.end_exclusive = plan.start;
    assert(!pt_amigus_voice_trigger_registers(5, &plan, &program));
    assert(!memcmp(&program, &previous, sizeof(program)));
}

static int voice_plan_fixture_main(void)
{
    struct pt_sample s={0};struct pt_playback_format f={16,0,0,0};
    struct pt_amigus_voice_request r={48000,1,0,64,128};
    struct pt_amigus_voice_plan p,old;unsigned i;
    s.pcm.frames=12;s.pcm.bits=24;s.pcm.channels=2;s.interpolation=1;
    /* Metadata-only planner: no source pointer needed, and stereo conversion
     * is one selected channel (24 bytes, not48 interleaved bytes). */
    assert(pt_amigus_voice_plan_prepare(&s,&f,&r,256,24,&p));
    assert(p.start==256 && p.loop==256 && p.end_exclusive==280);
    assert(p.rate==0x10000000 && p.control==0x8005 && p.left==32768 && p.right==32768);
    f.channel=1;f.little_endian=1;s.loop=PT_LOOP_FORWARD;s.loop_start=3;s.loop_end=9;r.offset=1;
    assert(pt_amigus_voice_plan_prepare(&s,&f,&r,256,24,&p));
    assert(p.start==258 && p.loop==262 && p.end_exclusive==274 && p.control==0x800f);
    r.rate_numerator=3546895;r.rate_denominator=428;
    assert(pt_amigus_voice_plan_prepare(&s,&f,&r,256,24,&p));assert(p.rate==46345033);
    r.rate_numerator=192000;r.rate_denominator=1;r.volume=64;r.pan=0;
    assert(pt_amigus_voice_plan_prepare(&s,&f,&r,256,24,&p));assert(p.rate==0x40000000 && p.left==65535 && !p.right);
    r.pan=256;assert(pt_amigus_voice_plan_prepare(&s,&f,&r,256,24,&p));assert(!p.left && p.right==65535);
    r.volume=0;assert(pt_amigus_voice_plan_prepare(&s,&f,&r,256,24,&p));assert(!p.left && !p.right);
    old=p;
#define REFUSE() do {assert(!pt_amigus_voice_plan_prepare(&s,&f,&r,256,24,&p));assert(!memcmp(&p,&old,sizeof(p)));} while(0)
    r.rate_numerator=192001;REFUSE();r.rate_numerator=1;r.rate_denominator=UINT32_MAX;REFUSE();
    r.rate_denominator=0;REFUSE();r.rate_denominator=1;r.rate_numerator=0;REFUSE();r.rate_numerator=48000;
    r.volume=65;REFUSE();r.volume=64;r.pan=257;REFUSE();r.pan=128;
    r.offset=9;REFUSE();r.offset=0;s.loop=PT_LOOP_PINGPONG;REFUSE();s.loop=PT_LOOP_CROSSFADE;REFUSE();s.loop=PT_LOOP_FORWARD;
    s.loop_start=9;REFUSE();s.loop_start=3;s.loop_end=13;REFUSE();s.loop_end=9;s.crossfade=1;REFUSE();s.crossfade=0;
    f.bits=24;REFUSE();f.bits=16;f.channel=2;REFUSE();f.channel=0;f.word_pad=1;REFUSE();f.word_pad=0;
    f.little_endian=2;REFUSE();f.little_endian=0;s.interpolation=2;REFUSE();s.interpolation=1;
    assert(!pt_amigus_voice_plan_prepare(&s,&f,&r,256,48,&p));assert(!memcmp(&p,&old,sizeof(p)));
    s.loop=PT_LOOP_NONE;REFUSE();s.loop_start=s.loop_end=0;
    assert(!pt_amigus_voice_plan_prepare(&s,&f,&r,257,24,&p));
    assert(!pt_amigus_voice_plan_prepare(&s,&f,&r,0x02000000,24,&p));
    assert(!pt_amigus_voice_plan_prepare(&s,&f,&r,0xfffffff0,24,&p));
    assert(!pt_amigus_voice_plan_prepare(&s,&f,&r,0x02000000-24,24,&p)); /* Exclusive end not representable. */
    assert(pt_amigus_voice_plan_prepare(&s,&f,&r,0x02000000-26,24,&p));assert(p.end_exclusive==0x01fffffe);
    f.bits=8;
    assert(pt_amigus_voice_plan_prepare(&s,&f,&r,256,12,&p));assert(p.end_exclusive==268 && p.control==0x8004);
    r.offset=1;assert(!pt_amigus_voice_plan_prepare(&s,&f,&r,256,12,&p));r.offset=2;
    assert(pt_amigus_voice_plan_prepare(&s,&f,&r,256,12,&p));assert(p.start==258);
    s.loop=PT_LOOP_FORWARD;s.loop_start=3;s.loop_end=8;
    assert(!pt_amigus_voice_plan_prepare(&s,&f,&r,256,12,&p));s.loop_start=4;
    assert(pt_amigus_voice_plan_prepare(&s,&f,&r,256,12,&p));
    s.loop_end=9;assert(!pt_amigus_voice_plan_prepare(&s,&f,&r,256,12,&p));
    s.loop=PT_LOOP_NONE;s.loop_start=s.loop_end=0;s.pcm.frames=11;
    assert(!pt_amigus_voice_plan_prepare(&s,&f,&r,256,11,&p));s.pcm.frames=UINT32_MAX;
    assert(!pt_amigus_voice_plan_prepare(&s,&f,&r,0,UINT32_MAX,&p));
    s.pcm.frames=12;r.offset=0;
    /* Independent rational frequency oracle and exact bounds across rates. */
    for(i=1;i<=192000;i+=193) {
        r.rate_numerator=i;
        assert(pt_amigus_voice_plan_prepare(&s,&f,&r,256,12,&p));
        assert((uint64_t)p.rate*192000<=(uint64_t)i*1073741824);
        assert((uint64_t)(p.rate+1)*192000>(uint64_t)i*1073741824);
    }
    check_voice_registers();
    puts("AMIGUS VOICE PLAN PASS: byte bounds, alignment, loops, rational rate, volume, refusal atomicity; no I/O");return 0;
}
#ifndef PT_VOICE_PLAN_NATIVE
int main(void){return voice_plan_fixture_main();}
#endif
