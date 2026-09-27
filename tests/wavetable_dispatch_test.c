#define PT_WAVETABLE_VOICES_NATIVE
#include "wavetable_voices_test.c"
#include "../src/editor/wavetable_dispatch.h"
#include "../src/core/amigus_render_voice.h"
#include <limits.h>
struct preflight_alloc {unsigned calls,fail_at;};
static void *preflight_allocate(void *ctx,size_t bytes)
{
    struct preflight_alloc *a=ctx;
    if(++a->calls==a->fail_at)return NULL;
    return allocate_master(NULL,bytes);
}
static void preflight_fixture(void)
{
    struct pt_project p={0};struct pt_sample samples[2];
    struct pt_event events[64*4]={{0}};uint16_t orders[1]={0};
    int32_t pcm[16]={1,257,-513,799,123,991,-777,27},classic[8]={1,2,3,4,5,6,7,8},silent[8]={0,0,3,4,5,6,7,8};
    struct pt_render_options o={0};struct pt_playback_format format={16,0,0,0};
    struct preflight_alloc memory={0};struct pt_allocator a={&memory,preflight_allocate,release_master};
    struct pt_wavetable_preflight_report report;struct pt_render_report measured;
    unsigned baseline=allocations,mode;
    memset(samples,0,sizeof(samples));pt_channels_init(&p.channels);p.samples=samples;p.sample_count=2;p.events=events;
    p.orders=orders;p.order_count=p.pattern_count=1;p.speed=3;p.bpm=125;
    samples[0].pcm=(struct pt_pcm){pcm,16,8,48000,1,24};samples[0].volume=64;
    samples[0].loop=PT_LOOP_FORWARD;samples[0].loop_end=8;samples[1]=samples[0];
    events[0]=(struct pt_event){428,0,PT_NOTE_PERIOD,1,0,0,0,0};
    events[4].effect=15;events[4].parameter=131;
    events[8].effect=14;events[8].parameter=0xe1;
    events[12].effect=10;events[12].parameter=1;
    events[16]=(struct pt_event){320,0,PT_NOTE_PERIOD,1,0,0,0,0};
    events[20].effect=15;
    o.rate=48000;o.bits=24;o.tracks=1;o.gain_q16=65536;o.tick_limit=1000;o.frame_limit=1000000;
    /* Whole song, lead-in and selected-range pre-roll all traverse the same
     * audited phases. Range analysis counts silent frames as well. */
    for(mode=0;mode<3;++mode) {
        o.include_lead_in=mode==1;o.pattern_only=o.row_range=mode==2;o.row_first=2;o.row_end=5;
        assert(pt_render_measure(&p,&o,NULL,NULL,&measured)==PT_RENDER_OK);
        assert(pt_wavetable_preflight(&p,&o,&format,1,&a,&report)==PT_WAVETABLE_COMPATIBLE);
        assert(report.render_result==PT_RENDER_OK && report.intervals && report.action==UINT_MAX);
        assert(mode==2?report.frames>measured.frames:report.frames==measured.frames);
        assert(allocations==baseline);
    }
    o.include_lead_in=o.pattern_only=o.row_range=0;
    /* A later valid stereo note passes ordinary rendering, but refuses the
     * mono device path only after earlier valid notes/control ticks. */
    samples[1].pcm.channels=2;events[16].instrument=2;
    assert(pt_render_measure(&p,&o,NULL,NULL,&measured)==PT_RENDER_OK);
    assert(pt_wavetable_preflight(&p,&o,&format,1,&a,&report)==PT_WAVETABLE_GEOMETRY);
    assert(report.intervals>4 && report.frames>0 && report.channel==0 && report.kind==PT_RENDER_TRIGGER);
    /* Unsupported source in pre-roll is still found before range playback. */
    events[20].effect=0;events[24].effect=15;
    o.pattern_only=o.row_range=1;o.row_first=5;o.row_end=7;
    assert(pt_wavetable_preflight(&p,&o,&format,1,&a,&report)==PT_WAVETABLE_GEOMETRY);
    events[20].effect=15;events[24].effect=0;
    o.pattern_only=o.row_range=0;samples[1].pcm.channels=1;events[16].instrument=1;
    assert(pt_wavetable_preflight(&p,&o,&format,0,&a,&report)==PT_WAVETABLE_CONTROL);
    assert(report.kind==PT_RENDER_CONTROL && report.channel==0);
    /* No allocation, even on an empty/invalid project, for invalid format. */
    {unsigned before=memory.calls;format.word_pad=1;
        assert(pt_wavetable_preflight(&p,&o,&format,1,&a,&report)==PT_WAVETABLE_FORMAT && memory.calls==before);
        format.word_pad=0;}
    /* Both workspace and internal sequence allocation failures clean up. */
    for(mode=1;mode<=2;++mode) {
        memory.calls=0;memory.fail_at=mode;
        assert(pt_wavetable_preflight(&p,&o,&format,1,&a,&report)==PT_WAVETABLE_MEMORY && allocations==baseline);
    }
    memory.fail_at=0;o.tick_limit=1;
    assert(pt_wavetable_preflight(&p,&o,&format,1,&a,&report)==PT_WAVETABLE_RENDER && report.render_result==PT_RENDER_TICK_LIMIT);
    o.tick_limit=1000;o.frame_limit=1;
    assert(pt_wavetable_preflight(&p,&o,&format,1,&a,&report)==PT_WAVETABLE_RENDER && report.render_result==PT_RENDER_FRAME_LIMIT);
    o.frame_limit=1000000;events[12].effect=14;events[12].parameter=0xf1;
    assert(pt_wavetable_preflight(&p,&o,&format,1,&a,&report)==PT_WAVETABLE_RENDER && report.render_result==PT_RENDER_EFFECT);
    /* Classic later instrument-only handoff emits a REPEAT. Its first note
     * is an ordinary forward trigger, so refusal must occur later. */
    memset(events,0,sizeof(events));
    samples[0].pcm.data=samples[1].pcm.data=classic;
    samples[0].pcm.capacity=samples[1].pcm.capacity=8;
    samples[0].pcm.bits=samples[1].pcm.bits=8;
    events[0]=(struct pt_event){428,0,PT_NOTE_PERIOD,1,0,0,0,0};
    events[8].instrument=2;events[16].effect=15;
    assert(pt_render_measure(&p,&o,NULL,NULL,&measured)==PT_RENDER_OK);
    assert(pt_wavetable_preflight(&p,&o,&format,1,&a,&report)==PT_WAVETABLE_OPERATION);
    assert(report.kind==PT_RENDER_REPEAT && report.intervals>3 && report.frames>0);
    /* A later classic silent-tail one-shot requires SEGMENT semantics. */
    samples[1].loop=PT_LOOP_NONE;samples[1].loop_end=0;samples[1].pcm.data=silent;
    events[8].kind=PT_NOTE_PERIOD;events[8].pitch=428;
    assert(pt_render_measure(&p,&o,NULL,NULL,&measured)==PT_RENDER_OK);
    assert(pt_wavetable_preflight(&p,&o,&format,1,&a,&report)==PT_WAVETABLE_OPERATION);
    assert(report.kind==PT_RENDER_SEGMENT && report.intervals>3 && report.frames>0);
    assert(allocations==baseline && pcm[0]==1 && pcm[1]==257 && pcm[2]==-513);
}
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

#include "wavetable_song_cases.h"
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
    assert(f && bus);assert(voices_fixture_main()==0);preflight_fixture();song_fixture();memset(bus,0,sizeof(*bus));
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
    free(bus);free(f);puts("WAVETABLE DISPATCH PASS: whole-sequence preflight, resolved16-channel triggers, phase-preserving controls, atomic refusal, uncertain-stop retention; injected only");return 0;
}
#ifndef PT_WAVETABLE_DISPATCH_NATIVE
int main(void){return dispatch_fixture_main();}
#endif
