#define PT_SAMPLER_WAVETABLE_NATIVE
#include "sampler_wavetable_test.c"
#include "../src/editor/wavetable_voices.h"
#define PT_VOICE_PLAN_NATIVE
#include "amigus_voice_plan_test.c"
struct voice_bus {
    struct fixture *f;struct pt_wavetable_voices *owner;
    int start_result,stop_result[PT_WAVETABLE_VOICES];
    unsigned starts,stops,active[PT_WAVETABLE_VOICES];
    uint32_t address[PT_WAVETABLE_VOICES];uint8_t held[PT_WAVETABLE_VOICES][8];
    int quiesce_result;
    unsigned quiesce_calls;
};
static int voice_quiesce(void *context)
{
    struct voice_bus *b=context;unsigned i;
    assert(b->owner->bridge && b->f->reservation.access && b->f->cache.reservation);
    for(i=0;i<PT_WAVETABLE_VOICES;++i)assert(!b->active[i] && !b->owner->voice[i].held);
    ++b->quiesce_calls;return b->quiesce_result;
}
static int voice_start(void *ctx,unsigned id,const struct pt_amigus_voice_plan *plan)
{
    struct voice_bus *b=ctx;struct pt_wavetable_voice *v=&b->owner->voice[id];
    uint32_t address=plan->start;
    assert(!b->active[id] && v->held && v->uncertain);
    assert(pt_cache_data(&b->f->cache.cache,v->lease));assert(plan->end_exclusive-address==8 && (plan->control&1));
    assert(plan->rate==0x10000000 && plan->left==32768 && plan->right==32768);
    b->active[id]=1;b->address[id]=address;memcpy(b->held[id],b->f->ram+address,8);++b->starts;
    return b->start_result;
}
static int voice_stop(void *ctx,unsigned id)
{
    struct voice_bus *b=ctx;struct pt_wavetable_voice *v=&b->owner->voice[id];
    assert(b->active[id] && v->held && pt_cache_data(&b->f->cache.cache,v->lease));
    assert(!memcmp(b->held[id],b->f->ram+b->address[id],8));++b->stops;
    if(b->stop_result[id]==1)b->active[id]=0;
    return b->stop_result[id];
}
static enum pt_voice_result trigger(struct pt_wavetable_voices *v,unsigned id)
{uint8_t staging[3];struct pt_playback_format format={16,0,0,0};struct pt_amigus_voice_request request={48000,1,0,64,128};return pt_wavetable_voices_trigger(v,id,0,&format,&request,staging,sizeof(staging));}
static unsigned pins(struct fixture *f)
{unsigned i,n=0;for(i=0;i<PT_CACHE_SLOTS;++i)n+=f->cache.cache.entry[i].pins;return n;}
static int voices_fixture_main(void)
{
    struct fixture *f=malloc(sizeof(*f));struct voice_bus *bus=malloc(sizeof(*bus));
    struct pt_allocator allocator={NULL,allocate_master,release_master};
    struct pt_document document;struct pt_sampler sampler;struct pt_sampler_wavetable bridge={0};
    struct pt_wavetable_voices voices={0};struct pt_wavetable_voice_api api={bus,voice_start,voice_stop,NULL,NULL};
    struct pt_pattern_history history;struct pt_pattern_command commands[8];struct pt_event_change changes[8];
    int32_t data[]={257,-513,1025,-2049};struct pt_cache_lease lease;unsigned i,n;
    uint8_t *saved;size_t size,used;
    assert(f && bus);memset(bus,0,sizeof(*bus));assert(sampler_fixture_main()==0);assert(voice_plan_fixture_main()==0);init(f,PT_AMIGUS_WAVETABLE);
    assert(pt_amigus_wavetable_cache_attach(&f->cache,&f->reservation,16,112,112,f,bus_owned,bus_write));
    pt_document_init(&document,&allocator);assert(pt_document_new(&document,4,SIZE_MAX)==PT_PROJECT_OK);
    document.project.samples[0].pcm=(struct pt_pcm){data,4,4,48000,1,24};
    assert(pt_project_size(&document.project,&size)==PT_PROJECT_OK);saved=malloc(size);assert(saved);
    assert(pt_project_encode(&document.project,saved,size,&used)==PT_PROJECT_OK && used==size);
    pt_sampler_init(&sampler,&allocator,1024*1024);
    assert(pt_pattern_history_init(&history,&document.project,commands,8,changes,8)==PT_EDIT_OK);
    assert(pt_sampler_wavetable_bind(&bridge,&sampler,&document.project,&f->cache));
    /* Refuse adoption while another caller still owns a lease. */
    assert(sample_load(&bridge,0,&lease)==PT_CACHE_LOAD);
    assert(!pt_wavetable_voices_bind(&voices,&bridge,&api));assert(pt_sampler_wavetable_unpin(&bridge,lease));
    assert(pt_wavetable_voices_bind(&voices,&bridge,&api));assert(!pt_wavetable_voices_bind(&voices,&bridge,&api));
    bus->f=f;bus->owner=&voices;bus->start_result=1;
    for(i=0;i<PT_WAVETABLE_VOICES;++i)bus->stop_result[i]=1;
    assert(!pt_wavetable_voices_bind_quiesce(&voices,NULL,bus));
    assert(pt_wavetable_voices_bind_quiesce(&voices,voice_quiesce,bus));
    assert(!pt_wavetable_voices_bind_quiesce(&voices,voice_quiesce,bus));
    assert(trigger(&voices,16)==PT_VOICE_REFUSED && pt_wavetable_voices_stop(&voices,16)==-1);
    for(i=0;i<PT_WAVETABLE_VOICES;++i)assert(trigger(&voices,i)==PT_VOICE_ACTIVE);
    assert(pins(f)==16 && f->cache.cache.bytes==8);exact_save(&document.project,saved,size);
    /* Invalid commands refuse before cache upload or old-voice stop. */
    {
        struct pt_playback_format bad={24,0,0,0};
        struct pt_amigus_voice_request request={48000,1,0,64,128};
        uint8_t staging[4];unsigned writes=f->writes,stops=bus->stops;
        assert(pt_wavetable_voices_trigger(&voices,0,0,&bad,&request,staging,4)==PT_VOICE_REFUSED);
        bad.bits=16;request.pan=257;
        assert(pt_wavetable_voices_trigger(&voices,0,0,&bad,&request,staging,4)==PT_VOICE_REFUSED);
        assert(f->writes==writes && bus->stops==stops && pins(f)==16);
    }
    /* Retrigger takes a second pin before confirmed stop, even on a cache HIT. */
    assert(trigger(&voices,0)==PT_VOICE_ACTIVE && pins(f)==16 && bus->stops==1);
    bus->stop_result[0]=0;n=bus->starts;
    assert(trigger(&voices,0)==PT_VOICE_STOP_PENDING && pins(f)==16 && bus->starts==n);
    bus->stop_result[0]=-1;
    assert(trigger(&voices,0)==PT_VOICE_STOP_FAILED && pins(f)==16 && bus->starts==n);
    /* Edited/retired data cannot be evicted under any active voice. */
    assert(pt_sampler_edit(&sampler,&document.project,&history,0,PT_PCM_GAIN,0,4,2000)==PT_EDIT_OK);
    f->cache.cache.budget=8;n=bus->stops;
    assert(trigger(&voices,0)==PT_VOICE_REFUSED && bus->stops==n && pins(f)==16);
    f->cache.cache.budget=112;f->fail=f->writes+2;
    assert(trigger(&voices,0)==PT_VOICE_REFUSED && bus->stops==n && pins(f)==16);f->fail=0;
    bus->stop_result[0]=0;
    assert(trigger(&voices,0)==PT_VOICE_STOP_PENDING && pins(f)==16);
    assert(pt_cache_trim(&f->cache.cache,112)==8 && f->cache.cache.bytes==8);
    /* Retry uses the latest master, not the unstarted candidate from above. */
    assert(pt_pattern_undo(&document.project,&history,-1)==PT_EDIT_OK);
    bus->stop_result[0]=1;assert(trigger(&voices,0)==PT_VOICE_ACTIVE);
    assert(!memcmp(bus->held[0],bus->held[1],8));exact_save(&document.project,saved,size);
    /* Both pending and failed start may have touched device state: keep pins. */
    bus->start_result=0;assert(trigger(&voices,0)==PT_VOICE_UNCERTAIN && voices.voice[0].uncertain && pins(f)==16);
    bus->stop_result[0]=-1;assert(pt_wavetable_voices_stop(&voices,0)==-1 && pins(f)==16);
    bus->stop_result[0]=1;bus->start_result=-1;
    assert(trigger(&voices,0)==PT_VOICE_UNCERTAIN && voices.voice[0].held && pins(f)==16);
    /* Lost ownership blocks new triggers; only explicit stop proof releases. */
    f->healthy=0;n=bus->starts;assert(trigger(&voices,1)==PT_VOICE_REFUSED && bus->starts==n && pins(f)==16);
    bus->stop_result[0]=0;bus->stop_result[1]=-1;n=bus->stops;
    assert(!pt_wavetable_voices_close(&voices) && bus->stops==n+16 && pins(f)==2);
    assert(!bus->quiesce_calls);
    assert(voices.closing && bridge.backend && f->reservation.access);
    assert(!pt_amigus_reservation_close(&f->reservation));assert(trigger(&voices,2)==PT_VOICE_REFUSED);
    n=bus->stops;assert(!pt_wavetable_voices_close(&voices) && bus->stops==n+2 && pins(f)==2);
    bus->stop_result[0]=1;assert(!pt_wavetable_voices_close(&voices) && pins(f)==1);
    bus->stop_result[1]=1;assert(!pt_wavetable_voices_close(&voices) && !pins(f));
    assert(bus->quiesce_calls==1 && voices.bridge && bridge.backend && f->cache.cache.bytes);
    n=bus->stops;bus->quiesce_result=-1;
    assert(!pt_wavetable_voices_close(&voices) && bus->stops==n && bus->quiesce_calls==2);
    bus->quiesce_result=2;assert(!pt_wavetable_voices_close(&voices) && f->cache.cache.bytes);
    assert(!pt_amigus_reservation_close(&f->reservation));
    /* Even a positive adapter acknowledgement cannot override an IRQ owner. */
    f->reservation.interrupt=1;bus->quiesce_result=1;
    assert(!pt_wavetable_voices_close(&voices) && !voices.quiesced && f->cache.cache.bytes);
    f->reservation.interrupt=0;
    assert(pt_wavetable_voices_close(&voices) && !pins(f) && !voices.bridge && !bridge.backend);
    assert(bus->stops==n && bus->quiesce_calls==5);
    assert(pt_wavetable_voices_close(&voices));assert(pt_amigus_reservation_close(&f->reservation));
    exact_save(&document.project,saved,size);free(saved);
    pt_pattern_history_release(&history);pt_sampler_release(&sampler);assert(!sampler.bytes);
    pt_document_release(&document);assert(!allocations && data[0]==257);
    free(bus);free(f);puts("WAVETABLE VOICES PASS: 16 voices, retrigger, uncertain start, confirmed stop and quiescence, bounded close, master preserved");return 0;
}
#ifndef PT_WAVETABLE_VOICES_NATIVE
int main(void){return voices_fixture_main();}
#endif
