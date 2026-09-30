#define main ownership_fixture_main
#include "paula_voices_test.c"
#undef main
#include "../src/editor/paula_dispatch.h"
static void action(struct pt_render_action *a,const struct pt_pcm *pcm,unsigned track,unsigned sample)
{
    memset(a,0,sizeof(*a));a->kind=PT_RENDER_TRIGGER;a->channel=track;
    a->voice.pcm=pcm+sample;a->voice.active=1;a->voice.end=4;
    a->voice.step=((uint64_t)8000<<32)/48000;
}
static void batch_fixture(unsigned bits)
{
    struct pt_allocator alloc={NULL,fast_alloc,fast_free};struct pt_document doc;struct pt_sampler sampler;
    struct pt_sampler_paula cache={0};struct pt_paula_voices owner={0};struct driver d={0};
    struct pt_paula_voice_api api={&d,start,stop,control};struct pt_paula_render_caps caps={3546895,124,65535};
    struct pt_render_plan p={0};struct pt_paula_batch work;unsigned i,before,starts;
    int32_t pcm[3][4]={{1,2,3,4},{5,6,7,8},{9,10,11,12}},saved[3][4];
    memcpy(saved,pcm,sizeof(pcm));d.start_result=d.control_result=d.quiesce_result=1;
    for(i=0;i<4;++i)d.stop_result[i]=1;
    pt_document_init(&doc,&alloc);assert(pt_document_new(&doc,16,SIZE_MAX)==PT_PROJECT_OK);
    for(i=0;i<16;++i)doc.project.channels.track[i].route=PT_AMIGUS;
    doc.project.channels.track[4].route=doc.project.channels.track[7].route=PT_PAULA;
    for(i=0;i<3;++i)doc.project.samples[i].pcm=(struct pt_pcm){pcm[i],4,4,8000,1,(uint8_t)bits};
    pt_sampler_init(&sampler,&alloc,1024*1024);
    assert(pt_sampler_paula_bind(&cache,&sampler,&doc.project,&d,chip_alloc,chip_free,8));
    assert(pt_paula_voices_bind(&owner,&cache,&api));
    p.count=2;action(&p.action[0],&doc.project.samples[0].pcm,4,0);
    p.action[1]=p.action[0];p.action[1].kind=PT_RENDER_CONTROL;p.action[1].gain[0]=65536;
    assert(pt_paula_dispatch(&owner,cache.version,48000,&p,&caps,&work)==1);
    assert(d.starts==1 && d.controls==1 && d.reading[0] && d.plan[0].volume==64);
    /* Acquire distinct replacements for two slots while old reader pins four
     * bytes: second acquisition cannot fit. No stop/start/control may happen. */
    p.count=2;action(&p.action[0],&doc.project.samples[1].pcm,4,0);
    action(&p.action[1],&doc.project.samples[2].pcm,7,0);
    before=d.stops;starts=d.starts;
    assert(pt_paula_dispatch(&owner,cache.version,48000,&p,&caps,&work)==0);
    assert(d.stops==before && d.starts==starts && d.reading[0] && !d.reading[1]);
    assert(!work.entry[0].held && !work.entry[1].held);
    cache.cache.budget=12;
    assert(pt_paula_dispatch(&owner,cache.version,48000,&p,&caps,&work)==1);
    assert(d.reading[0] && d.reading[1]);
    before=d.stops;starts=d.starts;
    p.action[1].voice.pcm=(const struct pt_pcm *)(uintptr_t)1;
    assert(!pt_paula_dispatch(&owner,cache.version,48000,&p,&caps,&work));
    assert(d.stops==before && d.starts==starts);
    p.action[1].voice.pcm=&doc.project.samples[2].pcm;
    assert(!pt_paula_dispatch(&owner,cache.version+1,48000,&p,&caps,&work));
    doc.project.channels.track[4].route=PT_AMIGUS;doc.project.channels.track[2].route=PT_PAULA;
    assert(!pt_paula_dispatch(&owner,cache.version,48000,&p,&caps,&work));
    assert(d.stops==before && d.starts==starts && d.reading[0]);
    doc.project.channels.track[4].route=PT_PAULA;doc.project.channels.track[2].route=PT_AMIGUS;
    assert(pt_sampler_paula_sync(&cache));
    /* A late invalid control refuses before any earlier trigger callback. */
    p.action[1].kind=PT_RENDER_CONTROL;p.action[1].gain[0]=65536;p.action[1].gain[1]=0;
    assert(!pt_paula_dispatch(&owner,cache.version,48000,&p,&caps,&work));
    assert(d.stops==before && d.starts==starts);
    p.action[1].gain[0]=0;p.action[1].gain[1]=65536;
    d.control_result=0;d.stop_result[1]=0;
    assert(pt_paula_dispatch(&owner,cache.version,48000,&p,&caps,&work)==-1);
    assert(owner.closing && !d.reading[0] && d.reading[1] && owner.voice[1].held && owner.voice[1].uncertain);
    before=d.stops;starts=d.starts;
    assert(!pt_paula_dispatch(&owner,cache.version,48000,&p,&caps,&work));
    assert(d.stops==before && d.starts==starts);
    assert(!pt_paula_voices_close(&owner));d.stop_result[1]=1;
    assert(pt_paula_voices_close(&owner) && !d.live);
    /* Uncertain first start retains its reader, releases the unstarted second
     * candidate, and forbids retry even when a later driver call would succeed. */
    assert(pt_sampler_paula_bind(&cache,&sampler,&doc.project,&d,chip_alloc,chip_free,8));
    assert(pt_paula_voices_bind(&owner,&cache,&api));
    p.count=2;action(&p.action[0],&doc.project.samples[0].pcm,4,0);
    action(&p.action[1],&doc.project.samples[1].pcm,7,0);
    d.start_result=0;d.stop_result[0]=0;starts=d.starts;
    assert(pt_paula_dispatch(&owner,cache.version,48000,&p,&caps,&work)==-1);
    assert(d.starts==starts+1 && d.reading[0] && !d.reading[1]);
    assert(owner.voice[0].held && owner.voice[0].uncertain && !work.entry[1].held);
    before=d.stops;d.start_result=1;
    assert(!pt_paula_dispatch(&owner,cache.version,48000,&p,&caps,&work));
    assert(d.stops==before && d.starts==starts+1);
    d.stop_result[0]=1;assert(pt_paula_voices_close(&owner) && !d.live);
    for(i=0;i<3;++i)assert(!memcmp(doc.project.samples[i].pcm.data,saved[i],sizeof(saved[i])));
    pt_sampler_release(&sampler);pt_document_release(&doc);
}
int paula_dispatch_fixture(void)
{batch_fixture(8);batch_fixture(16);batch_fixture(24);puts("Paula prepared batch ownership OK");return 0;}

#ifndef PT_TEST_DISPATCH_EXEC
int main(void) {return paula_dispatch_fixture();}
#endif
