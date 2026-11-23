#include "amigus_voice_plan.h"
int pt_amigus_voice_plan_prepare(const struct pt_sample *s,const struct pt_playback_format *f,
    const struct pt_amigus_voice_request *r,uint32_t address,uint32_t bytes,
    struct pt_amigus_voice_plan *out)
{
    struct pt_amigus_voice_plan p={0};uint32_t width,end,loop;uint64_t size,rate,den;
    if(!s || !f || !r || !out || !s->pcm.frames ||
       (s->pcm.bits!=8 && s->pcm.bits!=16 && s->pcm.bits!=24) ||
       (s->pcm.channels!=1 && s->pcm.channels!=2) || f->channel>=s->pcm.channels ||
       (f->bits!=8 && f->bits!=16) || f->little_endian>1 || f->word_pad ||
       s->interpolation>1 || !r->rate_numerator || !r->rate_denominator ||
       r->volume>64 || r->pan>256 || address>=PT_AMIGUS_RAM_ADDRESS_SPACE)return 0;
    width=f->bits/8;size=(uint64_t)s->pcm.frames*width;
    if(size!=bytes || size>PT_AMIGUS_RAM_ADDRESS_SPACE-address)return 0;
    if(s->loop==PT_LOOP_NONE) {
        if(s->loop_start || s->loop_end || s->crossfade)return 0;
        end=s->pcm.frames;loop=0;
    } else if(s->loop==PT_LOOP_FORWARD) {
        if(s->crossfade || s->loop_start>=s->loop_end || s->loop_end>s->pcm.frames)return 0;
        end=s->loop_end;loop=s->loop_start;
    } else return 0;
    if(r->offset>=end)return 0;
    p.start=address+r->offset*width;p.loop=address+loop*width;
    p.end_exclusive=address+end*width;
    if(((p.start|p.loop|p.end_exclusive)&1) || p.end_exclusive>=PT_AMIGUS_RAM_ADDRESS_SPACE)return 0;
    den=(uint64_t)192000*r->rate_denominator;
    if(r->rate_numerator>den)return 0; /* Explicit supported limit, no clamping. */
    rate=(uint64_t)r->rate_numerator*0x40000000UL/den;
    if(!rate)return 0;
    p.rate=(uint32_t)rate;
    p.control=(uint16_t)(0x8000|(f->bits==16?1:0)|(s->loop==PT_LOOP_FORWARD?2:0)|
        (s->interpolation?4:0)|(f->bits==16 && f->little_endian?8:0));
    p.left=(uint16_t)(((uint64_t)65535*r->volume*(256-r->pan)+8192)/16384);
    p.right=(uint16_t)(((uint64_t)65535*r->volume*r->pan+8192)/16384);
    *out=p;return 1;
}

enum voice_register {
    VOICE_BANK = 0x1e, VOICE_CONTROL = 0x20, VOICE_START = 0x22, VOICE_LOOP = 0x26,
    VOICE_END = 0x2a, VOICE_RATE = 0x2e, VOICE_LEFT = 0x32, VOICE_RIGHT = 0x34
};

static void register_long(struct pt_amigus_voice_register_program *program, uint16_t offset, uint32_t value)
{
    program->write[program->count++] = (struct pt_amigus_voice_register_write){offset, (uint16_t)(value >> 16)};
    program->write[program->count++] = (struct pt_amigus_voice_register_write){offset + 2, (uint16_t)value};
}

int pt_amigus_voice_stop_registers(unsigned voice, struct pt_amigus_voice_register_program *out)
{
    if (!out || voice >= 32)
        return 0;
    *out = (struct pt_amigus_voice_register_program){2, {{VOICE_BANK, (uint16_t)voice}, {VOICE_CONTROL, 0}}};
    return 1;
}

int pt_amigus_voice_trigger_registers(unsigned voice, const struct pt_amigus_voice_plan *plan,
    struct pt_amigus_voice_register_program *out)
{
    struct pt_amigus_voice_register_program program;
    if (!plan || !out || plan->start >= plan->end_exclusive || plan->loop >= plan->end_exclusive ||
        plan->end_exclusive >= PT_AMIGUS_RAM_ADDRESS_SPACE || ((plan->start | plan->loop | plan->end_exclusive) & 1) ||
        !(plan->control & 0x8000) || (plan->control & ~0x800f) ||
        ((plan->control & 8) && !(plan->control & 1)) || !plan->rate || plan->rate > 0x40000000UL ||
        !pt_amigus_voice_stop_registers(voice, &program))
        return 0;
    if (plan->control & 2)
        register_long(&program, VOICE_LOOP, plan->loop);
    register_long(&program, VOICE_START, plan->start);
    register_long(&program, VOICE_END, plan->end_exclusive - 2);
    register_long(&program, VOICE_RATE, plan->rate);
    program.write[program.count++] = (struct pt_amigus_voice_register_write){VOICE_LEFT, plan->left};
    program.write[program.count++] = (struct pt_amigus_voice_register_write){VOICE_RIGHT, plan->right};
    program.write[program.count++] = (struct pt_amigus_voice_register_write){VOICE_CONTROL, plan->control};
    *out = program;
    return 1;
}
