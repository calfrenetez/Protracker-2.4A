/* Private SOURCE proposal. No native/Exec/output-quiet authority is modeled.
 * Reuse the genuine preparation fixture's factory/history/accounting boundary;
 * this file adds the real typed Paula song/cache/reader composition. */
#define main preparation_fixture_main
#include "native_document_switch_test.c"
#undef main
#include "src/editor/editor_paula.h"

struct commit_driver {
    struct pt_paula_voice_plan plan[4];unsigned reading[4],starts,stops,quiesces;
    int stop_result,quiesce_result;size_t chip_bytes;uint64_t ticks;
};
struct commit_rig;
struct commit_release {struct commit_rig *rig;unsigned calls,reenter,drift;int result;};
struct commit_rig {
    struct rig *r;struct pt_editor_paula *owner;
    struct pt_sampler_paula cache;struct pt_paula_voices voices;
    struct commit_driver driver;struct commit_release *release;
    struct pt_native_document_switch_commit_inputs inputs;
    struct pt_sample_version *outside_pin;
};
static struct commit_rig *commit_retained;
static void *commit_chip_allocate(void *context,size_t bytes)
{struct commit_driver *d=context;void *p=malloc(bytes);if(p)d->chip_bytes+=bytes;return p;}
static void commit_chip_release(void *context,void *p,size_t bytes)
{
    struct commit_driver *d=context;unsigned i;
    for(i=0;i<4;++i)assert(!d->reading[i]);
    assert(d->chip_bytes>=bytes);d->chip_bytes-=bytes;free(p);
}
static int commit_start(void *context,unsigned slot,const struct pt_paula_voice_plan *p)
{struct commit_driver *d=context;assert(slot<4 && !d->reading[slot]);d->plan[slot]=*p;d->reading[slot]=1;++d->starts;return 1;}
static int commit_stop(void *context,unsigned slot)
{struct commit_driver *d=context;assert(slot<4);++d->stops;if(d->stop_result==1)d->reading[slot]=0;return d->stop_result;}
static int commit_control(void *context,unsigned slot,uint16_t period,uint8_t volume)
{struct commit_driver *d=context;assert(slot<4 && d->reading[slot] && period && volume<=64);return 1;}
static int commit_quiesce(void *context)
{struct commit_driver *d=context;unsigned i;for(i=0;i<4;++i)assert(!d->reading[i]);++d->quiesces;return d->quiesce_result;}
static int commit_clock(void *context,uint64_t *ticks,uint32_t *frequency)
{struct commit_driver *d=context;*ticks=d->ticks;*frequency=48000;return 1;}
static int commit_forbidden_progress(void *context,uint32_t done,uint32_t total)
{(void)context;(void)done;(void)total;assert(!"drifted progress must never run");return 0;}
static int commit_release_barrier(void *context)
{
    struct commit_release *w=context;struct commit_rig *x=w->rig;struct rig *r=x->r;
    ++w->calls;
    /* This existing typed release boundary is not a replacement stop callback.
     * The genuine song/voices/cache must already have closed before it runs. */
    assert(!x->owner->song && !x->driver.chip_bytes);
    assert(r->editor->history.count==r->before_editor->history.count);
    assert(r->document->project.samples[0].pcm.data==r->master);
    assert(!memcmp(r->master,pcm24,sizeof(pcm24)));
    if(w->reenter) {
        assert(pt_native_document_switch_state(r->control)==PT_NATIVE_DOCUMENT_SWITCH_BUSY);
        assert(pt_native_document_switch_commit_paula(r->control,&x->inputs)==PT_NATIVE_DOCUMENT_SWITCH_BUSY);
        assert(pt_native_document_switch_cancel(r->control)==PT_NATIVE_DOCUMENT_SWITCH_BUSY);
        assert(!pt_native_document_switch_reset(r->control));
    }
    if(w->drift==1)r->document->dirty^=1;
    if(w->drift==2){r->editor->sampler.progress=commit_forbidden_progress;r->editor->sampler.progress_context=w;}
    if(w->drift==3)r->editor->project=&r->control->candidate.project;
    return w->result;
}
static void commit_capture(struct commit_rig *x)
{
    struct rig *r=x->r;
    memcpy(r->before_document,r->document,sizeof(*r->document));
    memcpy(r->before_editor,r->editor,sizeof(*r->editor));
    free(r->saved);r->saved=encoded(&r->document->project,&r->saved_bytes);
    r->master=r->document->project.samples[0].pcm.data;r->generation=r->editor->sampler.generation;
    r->used=r->a->used;r->live=r->a->live;
}
static void commit_attach(struct commit_rig *x,unsigned play)
{
    struct rig *r=x->r;struct pt_render_options options={0};
    /* Model-only capability envelope: this is not a native Paula clock claim. */
    struct pt_paula_render_caps caps={3546895,1,65535};
    struct pt_paula_voice_api api={&x->driver,commit_start,commit_stop,commit_control};
    struct pt_paula_preflight_report report;enum pt_paula_song_result state;unsigned n=0;
    assert(pt_editor_paula_attach(x->owner,r->editor));
    assert(pt_editor_paula_bind_release(x->owner,commit_release_barrier,x->release));
    assert(pt_editor_paula_attachment_current(x->owner,r->editor));
    assert(pt_sampler_paula_bind(&x->cache,&r->editor->sampler,&r->document->project,
        &x->driver,commit_chip_allocate,commit_chip_release,32));
    assert(pt_paula_voices_bind(&x->voices,&x->cache,&api));
    assert(pt_paula_voices_bind_quiesce(&x->voices,commit_quiesce,&x->driver));
    options.rate=48000;options.bits=24;options.tracks=1;options.gain_q16=65536;
    options.tick_limit=100;options.frame_limit=100000;
    assert(pt_editor_paula_begin(x->owner,&x->voices,&options,&caps)==PT_PAULA_SONG_PREPARING);
    if(play) {
        do {state=pt_editor_paula_prepare(x->owner,&report);assert(++n<4096);}while(state==PT_PAULA_SONG_PREPARING);
        assert(state==PT_PAULA_SONG_OK);
        uint64_t deadline=0;
        x->driver.ticks=0;
        assert(pt_editor_paula_clocked_begin(x->owner,1000,commit_clock,&x->driver)==PT_PAULA_SONG_OK);
        x->driver.ticks=999;
        do {state=pt_editor_paula_clocked_service(x->owner,&deadline);assert(++n<8192);}while(state==PT_PAULA_SONG_WAITING);
        assert(state==PT_PAULA_SONG_OK && deadline==1000 && !x->driver.reading[0]);
        x->driver.ticks=1000;
        assert(pt_editor_paula_clocked_service(x->owner,&deadline)==PT_PAULA_SONG_WAITING && x->driver.reading[0]);
    }
    commit_capture(x);
}
static struct commit_rig *commit_create(unsigned play,unsigned source)
{
    struct commit_rig *x=calloc(1,sizeof(*x));struct rig *r;
    assert(x);x->r=r=create(0);enrich(r);
    x->owner=calloc(1,sizeof(*x->owner));x->release=calloc(1,sizeof(*x->release));
    assert(x->owner && x->release);x->release->rig=x;x->release->result=1;
    x->driver.stop_result=x->driver.quiesce_result=1;
    x->inputs=(struct pt_native_document_switch_commit_inputs){x->owner,sizeof(*x->owner),x->release,sizeof(*x->release)};
    assert(pt_editor_change_barrier(r->editor,NULL,NULL));
    r->document->project.channels.track[0].pan=0;
    r->document->project.events[0]=(struct pt_event){428,0,PT_NOTE_PERIOD,1,0,0,0,0};
    r->document->project.events[4*3].effect=15;r->document->project.events[4*3].parameter=0;
    if(source) {
        assert(pt_document_new(&r->editor->sample_source,4,SIZE_MAX)==PT_PROJECT_OK);
        assert(r->editor->sampler.budget>=r->editor->sample_source.allocated_bytes);
        r->editor->sampler.budget-=r->editor->sample_source.allocated_bytes;r->editor->source_selected=1;
    }
    commit_attach(x,play);return x;
}
static void commit_prepare(struct commit_rig *x,unsigned loading)
{
    struct rig *r=x->r;
    assert((loading?pt_native_document_switch_load(r->control,r->capacity,&r->in,r->saved,r->saved_bytes,SIZE_MAX):
        pt_native_document_switch_new(r->control,r->capacity,&r->in,8,SIZE_MAX))==PT_NATIVE_DOCUMENT_SWITCH_PREPARED);
    original_unchanged(r);assert(pt_native_document_switch_admit_publication(r->control)==PT_NATIVE_DOCUMENT_SWITCH_STOP_UNQUALIFIED);
}
static void commit_finish(struct commit_rig *x)
{
    struct rig *r=x->r;r->a->active=NULL;r->a->reentry=0;
    assert(!x->owner->editor && !x->owner->song && !x->driver.chip_bytes);
    assert(pt_editor_dispose(r->editor));assert(pt_sampler_lifetime_storage_retired(&r->editor->sampler));
    assert(pt_document_release_checked(r->document)==PT_DOCUMENT_RELEASED);
    release(r->a,r->editor);assert(!r->a->used && !r->a->live && r->a->allocations==r->a->releases);
    free(r->document);free(r->before_document);free(r->before_editor);free(r->saved);free(r->a);
    /* Positive transfer ended control has no borrowed lifetime dependency. */
    assert(pt_native_document_switch_state(r->control)==PT_NATIVE_DOCUMENT_SWITCH_COMMITTED);
    assert(pt_native_document_switch_reset(r->control));tail(r);free(r->control);free(r);
    free(x->owner);free(x->release);free(x);
}
static void commit_success(unsigned loading,unsigned source,unsigned reenter)
{
    struct commit_rig *x=commit_create(1,source);struct rig *r=x->r;
    struct pt_editor *address=r->editor;struct pt_project *project=&r->document->project;
    struct pt_allocator sampler=r->editor->sampler.allocator,sa=r->editor->sample_source.allocator,song=r->editor->song.allocator;
    size_t budget=r->editor->sampler.budget+r->editor->sample_source.allocated_bytes,song_budget=r->editor->song.budget;
    unsigned n;commit_prepare(x,loading);x->release->reenter=reenter;
    if(reenter){r->a->active=r->control;r->a->reentry=1;}
    assert(pt_native_document_switch_commit_paula(r->control,&x->inputs)==PT_NATIVE_DOCUMENT_SWITCH_COMMITTED);
    r->a->reentry=0;
    assert(r->editor==address && r->editor->project==project);
    assert(!r->editor->change_ready && !r->editor->before_change && !r->editor->before_change_context);
    assert(r->editor->history.commands==r->editor->commands && r->editor->clipboard.events==r->editor->clipboard_events);
    assert(!r->editor->history.count && !r->editor->history.cursor && x->release->calls==1);
#define SAME(a,b) assert((a).context==(b).context && (a).allocate==(b).allocate && (a).release==(b).release)
    SAME(r->editor->sampler.allocator,sampler);SAME(r->editor->sample_source.allocator,sa);SAME(r->editor->song.allocator,song);
#undef SAME
    assert(r->editor->sampler.budget==budget && r->editor->song.budget==song_budget);
    assert(!r->editor->sample_source.loaded && !r->editor->sample_source.allocated_bytes);
    assert(!r->control->candidate.loaded && !r->control->candidate.allocated_bytes);
    if(loading) {
        uint8_t *out;size_t bytes;out=encoded(project,&bytes);
        assert(bytes==r->saved_bytes && !memcmp(out,r->saved,bytes));free(out);
        assert(project->samples[0].pcm.bits==24 && !memcmp(project->samples[0].pcm.data,pcm24,sizeof(pcm24)));
    } else {assert(project->channels.count==8);for(n=0;n<project->sample_count;++n)assert(!project->samples[n].pcm.frames);}
    assert(pt_native_document_switch_commit_paula(r->control,&x->inputs)==PT_NATIVE_DOCUMENT_SWITCH_COMMITTED);
    assert(x->release->calls==1);commit_finish(x);
}
static void commit_keep(struct commit_rig *x,unsigned unchanged)
{
    struct rig *r=x->r;size_t releases=r->a->releases;unsigned calls=x->release->calls;
    if(unchanged) {
        size_t bytes;uint8_t *out=encoded(&r->document->project,&bytes);
        assert(bytes==r->saved_bytes && !memcmp(out,r->saved,bytes));free(out);
        assert(r->editor->history.count==r->before_editor->history.count);
        assert(r->document->project.samples[0].pcm.data==r->master && !memcmp(r->master,pcm24,sizeof(pcm24)));
    }
    assert(pt_native_document_switch_state(r->control)==PT_NATIVE_DOCUMENT_SWITCH_HELD);
    assert(pt_native_document_switch_commit_paula(r->control,&x->inputs)==PT_NATIVE_DOCUMENT_SWITCH_HELD);
    assert(pt_native_document_switch_cancel(r->control)==PT_NATIVE_DOCUMENT_SWITCH_HELD);
    assert(!pt_native_document_switch_reset(r->control));
    assert(r->a->releases==releases && x->release->calls==calls);tail(r);
    commit_retained=x;retained=r; /* Both complete original domains remain rooted. */
}
static void commit_refusal(unsigned kind)
{
    struct commit_rig *x=commit_create(1,0);struct rig *r=x->r;struct pt_pcm pcm;
    if(kind==4) {
        assert(pt_sampler_pin(&r->editor->sampler,&r->document->project,0,r->editor->sampler.generation,&pcm,&x->outside_pin)==PT_EDIT_OK);
        commit_capture(x);
    }
    commit_prepare(x,1);
    if(kind==0)x->driver.stop_result=0;
    if(kind==1)x->driver.quiesce_result=0;
    if(kind==2)x->release->result=0;
    if(kind==3)x->release->drift=1;
    if(kind==5)x->release->drift=2;
    if(kind==6)x->release->drift=3;
    assert(pt_native_document_switch_commit_paula(r->control,&x->inputs)==PT_NATIVE_DOCUMENT_SWITCH_HELD);
    if(kind==0){assert(x->driver.reading[0] && x->owner->song && !x->release->calls);}
    if(kind==1){assert(!x->driver.reading[0] && x->owner->song && !x->release->calls);}
    if(kind==2){assert(!x->owner->song && x->owner->release_pending && x->release->calls==1);}
    if(kind==4){assert(x->outside_pin && !r->editor->history.count && !pt_sampler_lifetime_storage_retired(&r->editor->sampler));}
    commit_keep(x,kind!=4);
}
static void commit_repeated(void)
{
    struct commit_rig *x=commit_create(1,0);struct rig *r=x->r;
    struct pt_editor *address=r->editor;unsigned i;
    for(i=0;i<3;++i) {
        if(i) {
            assert(pt_native_document_switch_reset(r->control));
            free(r->before_document);free(r->before_editor);free(r->saved);
            /* A loaded document already has "original journal". Force a real
             * title command before reusing enrich's fresh-title precondition. */
            assert(!r->editor->history.count && !r->editor->history.cursor);
            assert(strcmp(r->document->project.title,"repeat journal"));
            assert(pt_pattern_title_apply(&r->document->project,&r->editor->history,"repeat journal")==PT_EDIT_OK);
            assert(r->editor->history.count==1 && r->editor->history.cursor==1);
            enrich(r);assert(pt_editor_change_barrier(r->editor,NULL,NULL));
            r->document->project.channels.track[0].pan=0;
            r->document->project.events[0]=(struct pt_event){428,0,PT_NOTE_PERIOD,1,0,0,0,0};
            r->document->project.events[r->document->project.channels.count*3].effect=15;
            r->document->project.events[r->document->project.channels.count*3].parameter=0;
            commit_attach(x,1);
        }
        commit_prepare(x,i==1);
        assert(pt_native_document_switch_commit_paula(r->control,&x->inputs)==PT_NATIVE_DOCUMENT_SWITCH_COMMITTED);
        assert(r->editor==address && r->editor->project==&r->document->project);
        assert(!r->editor->history.count && !r->editor->history.cursor && x->release->calls==i+1);
        if(i==1) {
            size_t bytes;uint8_t *out=encoded(&r->document->project,&bytes);
            assert(bytes==r->saved_bytes && !memcmp(out,r->saved,bytes));free(out);
            assert(r->document->project.samples[0].pcm.bits==24 &&
                !memcmp(r->document->project.samples[0].pcm.data,pcm24,sizeof(pcm24)));
        } else {
            unsigned slot;assert(r->document->project.channels.count==8);
            for(slot=0;slot<r->document->project.sample_count;++slot)
                assert(!r->document->project.samples[slot].pcm.frames);
        }
    }
    commit_finish(x);
}
static void commit_invalid_owner(void)
{
    struct commit_rig *x=commit_create(0,0);struct rig *r=x->r;
    struct pt_editor_paula wrong={0},copied;struct pt_native_document_switch_commit_inputs in=x->inputs;
    struct pt_native_document_switch before;commit_prepare(x,0);memcpy(&before,r->control,sizeof(before));
    in.owner=&wrong;in.owner_capacity=sizeof(wrong);
    assert(pt_native_document_switch_commit_paula(r->control,&in)==PT_NATIVE_DOCUMENT_SWITCH_INVALID);
    memcpy(&copied,x->owner,sizeof(copied));in=x->inputs;in.owner=&copied;in.owner_capacity=sizeof(copied);
    assert(pt_native_document_switch_commit_paula(r->control,&in)==PT_NATIVE_DOCUMENT_SWITCH_INVALID);
    in=x->inputs;in.owner_capacity=sizeof(*x->owner)-1;
    assert(pt_native_document_switch_commit_paula(r->control,&in)==PT_NATIVE_DOCUMENT_SWITCH_INVALID);
    in=x->inputs;in.release_context_capacity=0;
    assert(pt_native_document_switch_commit_paula(r->control,&in)==PT_NATIVE_DOCUMENT_SWITCH_INVALID);
    assert(!memcmp(&before,r->control,sizeof(before)) && !x->release->calls);original_unchanged(r);
    assert(pt_native_document_switch_commit_paula(r->control,&x->inputs)==PT_NATIVE_DOCUMENT_SWITCH_COMMITTED);
    commit_finish(x);
}
int main(int argc,char **argv)
{
    unsigned held=0;assert(argc==2);
    if(!strcmp(argv[1],"new_commit"))commit_success(0,0,0);
    else if(!strcmp(argv[1],"load_commit"))commit_success(1,0,0);
    else if(!strcmp(argv[1],"source_policy"))commit_success(1,1,0);
    else if(!strcmp(argv[1],"callback_reentry"))commit_success(0,0,1);
    else if(!strcmp(argv[1],"invalid_owner"))commit_invalid_owner();
    else if(!strcmp(argv[1],"repeated_cycles"))commit_repeated();
    else if(!strcmp(argv[1],"stop_refusal")){held=1;commit_refusal(0);}
    else if(!strcmp(argv[1],"quiesce_refusal")){held=1;commit_refusal(1);}
    else if(!strcmp(argv[1],"release_refusal")){held=1;commit_refusal(2);}
    else if(!strcmp(argv[1],"callback_drift")){held=1;commit_refusal(3);}
    else if(!strcmp(argv[1],"retained_master")){held=1;commit_refusal(4);}
    else if(!strcmp(argv[1],"progress_drift")){held=1;commit_refusal(5);}
    else if(!strcmp(argv[1],"project_drift")){held=1;commit_refusal(6);}
    else assert(!"unknown commit case");
    printf("DOCUMENT COMMIT HOST %s PASS %s\n",argv[1],held?"RETAINED_DOMAIN":"COMPLETE_DOMAIN");return 0;
}
