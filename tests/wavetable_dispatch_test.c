#define PT_WAVETABLE_VOICES_NATIVE
#include "wavetable_voices_test.c"
#include "../src/editor/wavetable_dispatch.h"
#include "../src/core/amigus_render_voice.h"
struct dispatch_bus {
    struct fixture *f;struct pt_wavetable_voices *owner;
    unsigned starts,stops,controls,fail_start,fail_control;
    unsigned active[16];int stop_result[16];struct pt_amigus_voice_plan plan[16];
};
static int dispatch_start(void *ctx,unsigned ch,const struct pt_amigus_voice_plan *p)
{
    struct dispatch_bus *b=ctx;assert(!b->active[ch] && b->owner->voice[ch].held);
    assert(pt_cache_data(&b->f->cache.cache,b->owner->voice[ch].lease));
    b->active[ch]=1;b->plan[ch]=*p;++b->starts;return b->starts!=b->fail_start;
}
static int dispatch_stop(void *ctx,unsigned ch)
{
    struct dispatch_bus *b=ctx;assert(b->active[ch] && b->owner->voice[ch].held);
    assert(pt_cache_data(&b->f->cache.cache,b->owner->voice[ch].lease));++b->stops;
    if(b->stop_result[ch]==1)b->active[ch]=0;
    return b->stop_result[ch];
}
static int dispatch_control(void *ctx,unsigned ch,uint32_t rate,uint16_t left,uint16_t right)
{
    struct dispatch_bus *b=ctx;assert(b->active[ch] && b->owner->voice[ch].held);
    b->plan[ch].rate=rate;b->plan[ch].left=left;b->plan[ch].right=right;++b->controls;
    return b->controls!=b->fail_control;
}
static int dispatch_fixture_main(void)
{
    struct fixture *f=malloc(sizeof(*f));struct dispatch_bus *bus=malloc(sizeof(*bus));
    struct pt_allocator allocator={NULL,allocate_master,release_master};
    struct pt_document d;struct pt_sampler sampler;struct pt_sampler_wavetable bridge={0};struct pt_wavetable_voices owner={0};
    struct pt_wavetable_voice_api api={bus,dispatch_start,dispatch_stop,dispatch_control};
    struct pt_render_command_state state;struct pt_render_plan plan,bad;
    struct pt_flow flow={0};struct pt_pitch pitch={0};struct pt_render_range ranges[16]={{0}};
    struct pt_render_options options={0};struct pt_playback_format format={16,0,0,0};uint8_t staging[3];
    int32_t data[]={257,-513,1025,-2049,17,31,47,63};unsigned ch,i,starts,stops,controls;uint64_t version;
    uint8_t *saved;size_t size,used;
    assert(f && bus);assert(voices_fixture_main()==0);memset(bus,0,sizeof(*bus));
    init(f,PT_AMIGUS_WAVETABLE);assert(pt_amigus_wavetable_cache_attach(&f->cache,&f->reservation,16,112,112,f,bus_owned,bus_write));
    pt_document_init(&d,&allocator);assert(pt_document_new(&d,16,SIZE_MAX)==PT_PROJECT_OK);
    d.project.samples[0].pcm=(struct pt_pcm){data,8,8,48000,1,24};d.project.samples[0].volume=64;
    d.project.samples[0].loop=PT_LOOP_FORWARD;d.project.samples[0].loop_end=8;
    for(ch=0;ch<16;++ch){struct pt_event *e=d.project.events+ch;e->kind=PT_NOTE_PERIOD;e->pitch=428;e->instrument=1;pitch.channel[ch].output=428;bus->stop_result[ch]=1;}
    assert(pt_project_validate(&d.project,NULL)==PT_PROJECT_OK);
    assert(pt_project_size(&d.project,&size)==PT_PROJECT_OK);saved=malloc(size);assert(saved);
    assert(pt_project_encode(&d.project,saved,size,&used)==PT_PROJECT_OK && used==size);
    pt_sampler_init(&sampler,&allocator,1024*1024);assert(pt_sampler_wavetable_bind(&bridge,&sampler,&d.project,&f->cache));
    assert(pt_wavetable_voices_bind(&owner,&bridge,&api));bus->owner=&owner;bus->f=f;version=bridge.version;
    options.rate=48000;options.bits=24;options.gain_q16=4096;options.tracks=65535;flow.project=&d.project;flow.fresh=1;
    pt_render_commands_init(&state);
    assert(pt_render_commands_plan(&d.project,&options,&flow,&pitch,ranges,0,&state,&plan)==PT_RENDER_OK);
    assert(plan.count==32);
    /* Entire batch preflight: even a late unsupported/foreign operation must
     * not partially start earlier valid channels. */
    bad=plan;bad.action[31].kind=PT_RENDER_REPEAT;
    assert(pt_wavetable_dispatch(&owner,version,48000,&bad,&format,staging,3)==0 && !bus->starts);
    bad=plan;bad.action[0].voice.pcm=(const struct pt_pcm *)(uintptr_t)1;
    assert(pt_wavetable_dispatch(&owner,version,48000,&bad,&format,staging,3)==0 && !bus->starts);
    bad=plan;bad.action[0].kind=PT_RENDER_SEGMENT;
    assert(pt_wavetable_dispatch(&owner,version,48000,&bad,&format,staging,3)==0 && !bus->starts);
    assert(pt_wavetable_dispatch(&owner,version+1,48000,&plan,&format,staging,3)==0 && !bus->starts);
    assert(pt_wavetable_dispatch(&owner,version,48000,&plan,&format,staging,3)==1);
    assert(bus->starts==16 && bus->controls==16 && pins(f)==16 && !bus->stops);
    for(i=0;i<plan.count;++i)if(plan.action[i].kind==PT_RENDER_CONTROL){
        const struct pt_render_action *a=plan.action+i;const struct pt_amigus_voice_plan *p=bus->plan+a->channel;
        assert(p->rate==0x10000000 && p->start==16 && p->loop==16 && p->end_exclusive==32);
        assert(p->left==((uint64_t)a->gain[0]*65535+32768)/65536);
        assert(p->right==((uint64_t)a->gain[1]*65535+32768)/65536);
    }
    exact_save(&d.project,saved,size);starts=bus->starts;stops=bus->stops;
    /* Slide/control-only ticks preserve lease and phase: no start/stop callback. */
    flow.fresh=0;flow.counter=1;for(ch=0;ch<16;++ch)pitch.channel[ch].output=214;
    assert(pt_render_commands_plan(&d.project,&options,&flow,&pitch,ranges,0,&state,&plan)==PT_RENDER_OK && plan.count==16);
    assert(pt_wavetable_dispatch(&owner,version,48000,&plan,&format,staging,3)==1);
    assert(bus->starts==starts && bus->stops==stops && pins(f)==16);
    for(ch=0;ch<16;++ch)assert(bus->plan[ch].rate==0x20000000);
    /* Preserve independently resolved side gains, including silent phase. */
    for(i=0;i<plan.count;++i){plan.action[i].gain[0]=0;plan.action[i].gain[1]=65536;}
    assert(pt_wavetable_dispatch(&owner,version,48000,&plan,&format,staging,3)==1);
    for(ch=0;ch<16;++ch)assert(!bus->plan[ch].left && bus->plan[ch].right==65535);
    bad=plan;bad.action[15].gain[1]=65537;controls=bus->controls;
    assert(pt_wavetable_dispatch(&owner,version,48000,&bad,&format,staging,3)==0 && bus->controls==controls);
    /* Explicit Stop releases all leases; a subsequent note starts afresh. */
    bad.count=16;for(ch=0;ch<16;++ch){bad.action[ch].kind=PT_RENDER_STOP;bad.action[ch].channel=ch;}
    assert(pt_wavetable_dispatch(&owner,version,48000,&bad,&format,staging,3)==1 && !pins(f));
    flow.fresh=1;flow.counter=0;
    assert(pt_render_commands_plan(&d.project,&options,&flow,&pitch,ranges,0,&state,&plan)==PT_RENDER_OK);
    assert(pt_wavetable_dispatch(&owner,version,48000,&plan,&format,staging,3)==1 && pins(f)==16);
    flow.fresh=0;flow.counter=1;
    assert(pt_render_commands_plan(&d.project,&options,&flow,&pitch,ranges,0,&state,&plan)==PT_RENDER_OK);
    /* Runtime control uncertainty poisons the session and stops other voices,
     * but a pending stop keeps its device lease and reservation. */
    bus->fail_control=bus->controls+2;bus->stop_result[1]=0;
    assert(pt_wavetable_dispatch(&owner,version,48000,&plan,&format,staging,3)==-1);
    assert(owner.closing && pins(f)==1 && owner.voice[1].held && owner.voice[1].uncertain);
    assert(!pt_amigus_reservation_close(&f->reservation));controls=bus->controls;
    assert(pt_wavetable_dispatch(&owner,version,48000,&plan,&format,staging,3)==0 && bus->controls==controls);
    assert(!pt_wavetable_voices_close(&owner));bus->stop_result[1]=1;assert(pt_wavetable_voices_close(&owner));
    assert(pt_amigus_reservation_close(&f->reservation));exact_save(&d.project,saved,size);
    /* A later batch fails after two successful starts and an uncertain third.
     * Only the unconfirmed third lease remains; no later start is issued. */
    init(f,PT_AMIGUS_WAVETABLE);assert(pt_amigus_wavetable_cache_attach(&f->cache,&f->reservation,16,112,112,f,bus_owned,bus_write));
    assert(pt_sampler_wavetable_bind(&bridge,&sampler,&d.project,&f->cache));
    assert(pt_wavetable_voices_bind(&owner,&bridge,&api));version=bridge.version;
    bus->fail_control=0;bus->fail_start=bus->starts+3;bus->stop_result[2]=0;starts=bus->starts;
    flow.fresh=1;flow.counter=0;pt_render_commands_init(&state);
    assert(pt_render_commands_plan(&d.project,&options,&flow,&pitch,ranges,0,&state,&plan)==PT_RENDER_OK);
    assert(pt_wavetable_dispatch(&owner,version,48000,&plan,&format,staging,3)==-1);
    assert(bus->starts==starts+3 && pins(f)==1 && owner.voice[2].uncertain && owner.closing);
    bus->stop_result[2]=1;assert(pt_wavetable_voices_close(&owner));assert(pt_amigus_reservation_close(&f->reservation));
    exact_save(&d.project,saved,size);free(saved);
    pt_sampler_release(&sampler);pt_document_release(&d);assert(!allocations && data[0]==257);
    /* Fixed-point rates: no truncation to integer Hz; rejection is atomic. */
    {uint32_t rate=99;uint16_t left=99,right=99;uint32_t gain[2]={65536,1};uint64_t step=((uint64_t)1<<32)+1234567;
        assert(pt_amigus_render_control(step,44100,gain,&rate,&left,&right));
        assert((uint64_t)rate*768000<=step*44100 && (uint64_t)(rate+1)*768000>step*44100);
        assert(left==65535 && right==1);rate=99;
        assert(!pt_amigus_render_control(UINT64_MAX,48000,gain,&rate,&left,&right) && rate==99);
    }
    {struct pt_pcm pcm={NULL,0,12,48000,1,24};struct pt_voice voice={0};
        struct pt_amigus_voice_plan command,unchanged;uint32_t gain[2]={65536,0};
        voice.pcm=&pcm;voice.active=1;voice.start=2;voice.end=7;voice.phase=(uint64_t)2<<32;voice.step=(uint64_t)1<<32;
        assert(pt_amigus_render_voice(&voice,48000,gain,&format,256,24,&command));
        assert(command.start==260 && command.end_exclusive==270 && command.left==65535 && !command.right);
        unchanged=command;pcm.channels=2;
        assert(!pt_amigus_render_voice(&voice,48000,gain,&format,256,24,&command) && !memcmp(&command,&unchanged,sizeof(command)));pcm.channels=1;
        voice.phase++;assert(!pt_amigus_render_voice(&voice,48000,gain,&format,256,24,&command));voice.phase--;
        voice.segment=1;assert(!pt_amigus_render_voice(&voice,48000,gain,&format,256,24,&command));voice.segment=0;
        voice.repeat_pcm=&pcm;assert(!pt_amigus_render_voice(&voice,48000,gain,&format,256,24,&command));voice.repeat_pcm=NULL;
        voice.loop=PT_VOICE_PINGPONG;assert(!pt_amigus_render_voice(&voice,48000,gain,&format,256,24,&command));
    }
    free(bus);free(f);puts("WAVETABLE DISPATCH PASS: resolved16-channel triggers, phase-preserving controls, atomic refusal, uncertain-stop retention; injected only");return 0;
}
#ifndef PT_WAVETABLE_DISPATCH_NATIVE
int main(void){return dispatch_fixture_main();}
#endif
