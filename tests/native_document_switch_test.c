#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "src/native/editor_document_switch.h"
#include "src/editor/sampler_lifetime_internal.h"

/* HOST accounting boundary only: genuine document/editor/history factories use
 * fresh malloc storage. This is not Exec, a native stop or a quiet receipt. */
struct account {
    size_t used,limit,calls,fail_at,allocations,releases,live;
    struct pt_native_document_switch *active;
    struct pt_document *old;
    unsigned reentry,reentered,mutate_old_once,quarantine_once,quarantined;
};
union charge {struct {size_t bytes;struct account *owner;} h;long double align;void *pointer;};
static void observe(struct account *a)
{
    struct pt_native_document_switch *c=a->active;
    if(c && a->reentry) {
        assert(pt_native_document_switch_state(c)==PT_NATIVE_DOCUMENT_SWITCH_BUSY);
        assert(pt_native_document_switch_cancel(c)==PT_NATIVE_DOCUMENT_SWITCH_BUSY);
        assert(pt_native_document_switch_admit_publication(c)==PT_NATIVE_DOCUMENT_SWITCH_BUSY);
        assert(pt_native_document_switch_new(c,c->capacity,&c->inputs,4,SIZE_MAX)==PT_NATIVE_DOCUMENT_SWITCH_BUSY);
        assert(!pt_native_document_switch_reset(c));++a->reentered;
    }
    if(a->mutate_old_once) {a->mutate_old_once=0;a->old->dirty^=1;}
}
static void *allocate(void *context,size_t bytes)
{
    struct account *a=context;union charge *p;size_t total;
    assert(a);++a->calls;observe(a);
    if(a->fail_at==a->calls || !bytes || bytes>SIZE_MAX-sizeof(*p))return NULL;
    total=sizeof(*p)+bytes;
    if(total>a->limit-a->used)return NULL;
    p=malloc(total);if(!p)return NULL;
    p->h.bytes=total;p->h.owner=a;a->used+=total;++a->allocations;++a->live;return p+1;
}
static void release(void *context,void *pointer)
{
    struct account *a=context;union charge *p;
    if(!pointer)return;
    p=(union charge *)pointer-1;assert(p->h.owner==a && a->used>=p->h.bytes && a->live);
    a->used-=p->h.bytes;--a->live;++a->releases;free(p);observe(a);
    if(a->quarantine_once) {
        a->quarantine_once=0;assert(a->active);
        assert(pt_document_release_checked(&a->active->candidate)==PT_DOCUMENT_RELEASE_REFUSED);
        ++a->quarantined;
    }
}
struct rig {
    struct account *a;
    struct pt_document *document,*before_document;
    struct pt_editor *editor,*before_editor;
    struct pt_native_document_switch *control;
    struct pt_native_document_switch_inputs in;
    uint8_t *saved;size_t saved_bytes,capacity;
    const int32_t *master;unsigned generation;
    size_t used,live;unsigned barrier_calls,packed;
};
static struct rig *retained;
static const uint8_t raw24[12]={1,0,0,255,255,127,0,0,128,254,255,255};
static const int32_t pcm24[4]={1,8388607,-8388608,-2};
static int veto(void *context)
{unsigned *calls=context;++*calls;return 0;}
static uint8_t *encoded(const struct pt_project *p,size_t *bytes)
{
    uint8_t *out;size_t written;
    assert(pt_project_size(p,bytes)==PT_PROJECT_OK);out=malloc(*bytes);assert(out);
    assert(pt_project_encode(p,out,*bytes,&written)==PT_PROJECT_OK && written==*bytes);return out;
}
static void tail(const struct rig *r)
{
    size_t i;if(r->packed)return;
    for(i=sizeof(*r->control);i<r->capacity;++i)assert(((const uint8_t *)r->control)[i]==0xa5);
}
static struct rig *create(unsigned packed)
{
    struct rig *r=calloc(1,sizeof(*r));struct pt_allocator a;size_t offset;
    assert(r);r->packed=packed;r->capacity=sizeof(*r->control)+64;
    if(packed) {
        /* Actual complete control allocation contains the original context in
         * its tail. Truthful full capacity must refuse this known overlap. */
        offset=(sizeof(*r->control)+sizeof(union charge)-1)/sizeof(union charge)*sizeof(union charge);
        r->capacity=offset+sizeof(*r->a);r->control=calloc(1,r->capacity);assert(r->control);
        r->a=(struct account *)((uint8_t *)r->control+offset);
    } else {
        r->a=calloc(1,sizeof(*r->a));r->control=calloc(1,r->capacity);assert(r->a && r->control);
        memset((uint8_t *)r->control+sizeof(*r->control),0xa5,64);
    }
    r->a->limit=32U*1024U*1024U;a=(struct pt_allocator){r->a,allocate,release};
    r->document=calloc(1,sizeof(*r->document));assert(r->document);
    pt_document_init(r->document,&a);assert(pt_document_new(r->document,4,SIZE_MAX)==PT_PROJECT_OK);
    r->editor=allocate(r->a,sizeof(*r->editor));assert(r->editor && pt_editor_init(r->editor,&r->document->project));
    pt_sampler_init(&r->editor->sampler,&a,(r->a->limit-r->a->used)/2);
    pt_document_init(&r->editor->sample_source,&a);
    pt_song_init(&r->editor->song,&a,(r->a->limit-r->a->used)/4);
    r->in=(struct pt_native_document_switch_inputs){r->document,sizeof(*r->document),r->editor,sizeof(*r->editor),r->a,sizeof(*r->a)};
    r->a->old=r->document;return r;
}
static void enrich(struct rig *r)
{
    struct pt_raw_format format={48000,24,1,1,0};
    assert(pt_sampler_import_raw(&r->editor->sampler,&r->document->project,&r->editor->history,
        0,raw24,sizeof(raw24),"switch24",&format)==PT_EDIT_OK);
    assert(pt_pattern_title_apply(&r->document->project,&r->editor->history,"original journal")==PT_EDIT_OK);
    assert(pt_sampler_attributes(&r->editor->sampler,&r->document->project,&r->editor->history,0,"kept24",43,-2)==PT_EDIT_OK);
    assert(pt_song_append(&r->editor->song,&r->document->project,&r->editor->history,0,0)==PT_EDIT_OK);
    assert(pt_pattern_undo(&r->document->project,&r->editor->history,-1)==PT_EDIT_OK);
    assert(pt_pattern_undo(&r->document->project,&r->editor->history,1)==PT_EDIT_OK);
    assert(r->editor->history.count>=4 && r->editor->history.cursor==r->editor->history.count);
    assert(pt_editor_change_barrier(r->editor,veto,&r->barrier_calls));
    r->master=r->document->project.samples[0].pcm.data;r->generation=r->editor->sampler.generation;
    assert(r->master && !memcmp(r->master,pcm24,sizeof(pcm24)));
    r->before_document=malloc(sizeof(*r->document));r->before_editor=malloc(sizeof(*r->editor));
    assert(r->before_document && r->before_editor);
    memcpy(r->before_document,r->document,sizeof(*r->document));memcpy(r->before_editor,r->editor,sizeof(*r->editor));
    r->saved=encoded(&r->document->project,&r->saved_bytes);r->used=r->a->used;r->live=r->a->live;
}
static void original_unchanged(struct rig *r)
{
    size_t bytes;uint8_t *out=encoded(&r->document->project,&bytes);
    assert(!memcmp(r->document,r->before_document,sizeof(*r->document)));
    assert(!memcmp(r->editor,r->before_editor,sizeof(*r->editor)));
    assert(bytes==r->saved_bytes && !memcmp(out,r->saved,bytes));free(out);
    assert(r->document->project.samples[0].pcm.data==r->master);
    assert(r->document->project.samples[0].pcm.bits==24 && r->document->project.samples[0].pcm.rate==48000);
    assert(!memcmp(r->master,pcm24,sizeof(pcm24)) && r->editor->sampler.generation==r->generation);
    assert(!r->barrier_calls);tail(r);
}
static void finish(struct rig *r)
{
    struct pt_native_document_switch *c=r->control;unsigned packed=r->packed;
    original_unchanged(r);assert(r->a->used==r->used && r->a->live==r->live);
    r->a->active=NULL;r->a->reentry=0;
    assert(pt_editor_change_barrier(r->editor,NULL,NULL));assert(pt_editor_dispose(r->editor));
    assert(pt_sampler_lifetime_storage_retired(&r->editor->sampler));
    assert(pt_document_release_checked(r->document)==PT_DOCUMENT_RELEASED);
    release(r->a,r->editor);assert(!r->a->used && !r->a->live && r->a->allocations==r->a->releases);
    free(r->document);free(r->before_document);free(r->before_editor);free(r->saved);
    if(!packed)free(r->a);
    /* Ended control status/reset cannot touch the now freed former sources. */
    if(!packed) {
        assert(pt_native_document_switch_state(c)==PT_NATIVE_DOCUMENT_SWITCH_CANCELED);
        assert(pt_native_document_switch_admit_publication(c)==PT_NATIVE_DOCUMENT_SWITCH_CANCELED);
        assert(pt_native_document_switch_cancel(c)==PT_NATIVE_DOCUMENT_SWITCH_CANCELED);
        assert(pt_native_document_switch_reset(c));assert(pt_native_document_switch_state(c)==PT_NATIVE_DOCUMENT_SWITCH_EMPTY);
        tail(r);
    }
    free(c);free(r);
}
static void prepared(struct rig *r)
{
    struct pt_native_document_switch snapshot=*r->control;
    original_unchanged(r);assert(r->control->candidate.loaded);
    assert(pt_native_document_switch_admit_publication(r->control)==PT_NATIVE_DOCUMENT_SWITCH_STOP_UNQUALIFIED);
    assert(!memcmp(&snapshot,r->control,sizeof(snapshot)));
    assert(pt_native_document_switch_state(r->control)==PT_NATIVE_DOCUMENT_SWITCH_PREPARED);
    assert(!pt_native_document_switch_reset(r->control));original_unchanged(r);
}
static void new_cancel_reuse(void)
{
    struct rig *r=create(0);unsigned channels,k;
    enrich(r);
    for(channels=4;channels<=16;channels*=2) {
        assert(pt_native_document_switch_state(r->control)==PT_NATIVE_DOCUMENT_SWITCH_EMPTY);
        assert(pt_native_document_switch_new(r->control,r->capacity,&r->in,channels,SIZE_MAX)==PT_NATIVE_DOCUMENT_SWITCH_PREPARED);
        prepared(r);assert(r->control->candidate.project.channels.count==channels);
        assert(r->control->candidate.project.sample_count==31 && r->control->candidate.project.order_count==1);
        for(k=0;k<channels;++k)assert(r->control->candidate.project.channels.track[k].route==(k<4?PT_PAULA:PT_AMIGUS));
        assert(pt_native_document_switch_cancel(r->control)==PT_NATIVE_DOCUMENT_SWITCH_CANCELED);
        original_unchanged(r);assert(r->a->used==r->used && r->a->live==r->live);
        if(channels<16)assert(pt_native_document_switch_reset(r->control));
    }
    finish(r);
}
static void load_roundtrip(void)
{
    struct rig *r=create(0);uint8_t *input,*copy;size_t bytes,n;const struct pt_sample *sample;
    enrich(r);input=encoded(&r->document->project,&bytes);
    assert(pt_native_document_switch_load(r->control,r->capacity,&r->in,input,bytes,SIZE_MAX)==PT_NATIVE_DOCUMENT_SWITCH_PREPARED);
    prepared(r);sample=&r->control->candidate.project.samples[0];
    assert(sample->pcm.data!=r->master && sample->pcm.bits==24 && sample->pcm.frames==4 && sample->pcm.rate==48000);
    assert(!memcmp(sample->pcm.data,pcm24,sizeof(pcm24)));
    copy=encoded(&r->control->candidate.project,&n);assert(n==bytes && !memcmp(copy,input,n));free(copy);free(input);
    assert(pt_native_document_switch_cancel(r->control)==PT_NATIVE_DOCUMENT_SWITCH_CANCELED);finish(r);
}
static void ordinary_failure(void)
{
    struct rig *r=create(0);unsigned i,n;size_t calls,bytes;uint8_t bad[8]={0},*input;
    struct pt_project_requirements need;
    enrich(r);
    for(i=1;i<=3;++i) {
        r->a->fail_at=r->a->calls+i;
        assert(pt_native_document_switch_new(r->control,r->capacity,&r->in,4,SIZE_MAX)==PT_NATIVE_DOCUMENT_SWITCH_FAILED);
        assert(r->control->preparation_result!=PT_PROJECT_OK);original_unchanged(r);
        assert(r->a->used==r->used && r->a->live==r->live);
        r->a->fail_at=0;assert(pt_native_document_switch_reset(r->control));
    }
    input=encoded(&r->document->project,&bytes);
    assert(pt_project_probe(input,bytes,&need)==PT_PROJECT_OK);
    n=(need.orders!=0)+(need.events!=0)+(need.samples!=0)+(need.pcm_values!=0)+
      (need.slices!=0)+(need.extensions!=0)+(need.extension_bytes!=0);
    assert(n>=4 && n<=7);
    for(i=1;i<=n;++i) {
        r->a->fail_at=r->a->calls+i;
        assert(pt_native_document_switch_load(r->control,r->capacity,&r->in,input,bytes,SIZE_MAX)==PT_NATIVE_DOCUMENT_SWITCH_FAILED);
        assert(r->control->preparation_result!=PT_PROJECT_OK);original_unchanged(r);
        assert(r->a->used==r->used && r->a->live==r->live);
        r->a->fail_at=0;assert(pt_native_document_switch_reset(r->control));
    }
    free(input);calls=r->a->calls;
    assert(pt_native_document_switch_new(r->control,r->capacity,&r->in,4,1)==PT_NATIVE_DOCUMENT_SWITCH_FAILED);
    assert(r->control->preparation_result==PT_PROJECT_CAPACITY && r->a->calls==calls);
    assert(pt_native_document_switch_reset(r->control));
    assert(pt_native_document_switch_load(r->control,r->capacity,&r->in,bad,sizeof(bad),SIZE_MAX)==PT_NATIVE_DOCUMENT_SWITCH_FAILED);
    assert(r->a->calls==calls && pt_native_document_switch_reset(r->control));original_unchanged(r);
    assert(pt_native_document_switch_new(r->control,r->capacity,&r->in,4,SIZE_MAX)==PT_NATIVE_DOCUMENT_SWITCH_PREPARED);
    assert(pt_native_document_switch_cancel(r->control)==PT_NATIVE_DOCUMENT_SWITCH_CANCELED);finish(r);
}
static void geometry(void)
{
    struct rig *r=create(0),*p;struct pt_native_document_switch_inputs in;size_t calls;uint8_t *before;
    enrich(r);calls=r->a->calls;before=malloc(r->capacity);assert(before);memcpy(before,r->control,r->capacity);
#define REFUSE(expression) do {assert((expression)==PT_NATIVE_DOCUMENT_SWITCH_INVALID);assert(!memcmp(before,r->control,r->capacity));assert(r->a->calls==calls);original_unchanged(r);} while(0)
    REFUSE(pt_native_document_switch_new(r->control,sizeof(*r->control)-1,&r->in,4,SIZE_MAX));
    REFUSE(pt_native_document_switch_new(r->control,r->capacity,&r->in,0,SIZE_MAX));
    in=r->in;in.editor_capacity=sizeof(*r->editor)-1;
    REFUSE(pt_native_document_switch_new(r->control,r->capacity,&in,4,SIZE_MAX));
    in=r->in;in.allocator_context_capacity=0;
    REFUSE(pt_native_document_switch_new(r->control,r->capacity,&in,4,SIZE_MAX));
    in=r->in;in.allocator_context=r;
    REFUSE(pt_native_document_switch_new(r->control,r->capacity,&in,4,SIZE_MAX));
    REFUSE(pt_native_document_switch_load(r->control,r->capacity,&r->in,(uint8_t *)r->control,1,SIZE_MAX));
    REFUSE(pt_native_document_switch_load(r->control,r->capacity,&r->in,(uint8_t *)r->document->storage.events,1,SIZE_MAX));
    r->control->result=PT_NATIVE_DOCUMENT_SWITCH_INVALID;memcpy(before,r->control,r->capacity);
    REFUSE(pt_native_document_switch_new(r->control,r->capacity,&r->in,4,SIZE_MAX));
    memset(r->control,0,sizeof(*r->control));free(before);
#undef REFUSE
    p=create(1);enrich(p);before=malloc(p->capacity);assert(before);memcpy(before,p->control,p->capacity);calls=p->a->calls;
    assert(pt_native_document_switch_new(p->control,p->capacity,&p->in,4,SIZE_MAX)==PT_NATIVE_DOCUMENT_SWITCH_INVALID);
    assert(!memcmp(before,p->control,p->capacity) && p->a->calls==calls);free(before);finish(p);
    assert(pt_native_document_switch_new(r->control,r->capacity,&r->in,4,SIZE_MAX)==PT_NATIVE_DOCUMENT_SWITCH_PREPARED);
    assert(pt_native_document_switch_cancel(r->control)==PT_NATIVE_DOCUMENT_SWITCH_CANCELED);finish(r);
}
static void reentry(void)
{
    struct rig *r=create(0);unsigned before;
    enrich(r);r->a->active=r->control;r->a->reentry=1;
    assert(pt_native_document_switch_new(r->control,r->capacity,&r->in,4,SIZE_MAX)==PT_NATIVE_DOCUMENT_SWITCH_PREPARED);
    assert(r->a->reentered==3);before=r->a->reentered;prepared(r);
    assert(pt_native_document_switch_cancel(r->control)==PT_NATIVE_DOCUMENT_SWITCH_CANCELED);
    assert(r->a->reentered==before+3);finish(r);
}
static void keep(struct rig *r)
{
    size_t released=r->a->releases;
    assert(pt_native_document_switch_state(r->control)==PT_NATIVE_DOCUMENT_SWITCH_HELD);
    assert(pt_native_document_switch_admit_publication(r->control)==PT_NATIVE_DOCUMENT_SWITCH_HELD);
    assert(pt_native_document_switch_cancel(r->control)==PT_NATIVE_DOCUMENT_SWITCH_HELD);
    assert(!pt_native_document_switch_reset(r->control) && r->a->releases==released);
    assert(r->document->loaded && r->editor->project==&r->document->project && r->a->used && r->a->live);tail(r);retained=r;
}
static void original_drift(void)
{
    struct rig *r=create(0);size_t releases;
    enrich(r);assert(pt_native_document_switch_new(r->control,r->capacity,&r->in,4,SIZE_MAX)==PT_NATIVE_DOCUMENT_SWITCH_PREPARED);
    releases=r->a->releases;r->editor->row=1;
    assert(pt_native_document_switch_state(r->control)==PT_NATIVE_DOCUMENT_SWITCH_HELD && r->a->releases==releases);keep(r);
}
static void callback_drift(void)
{
    struct rig *r=create(0);enrich(r);r->a->active=r->control;r->a->mutate_old_once=1;
    assert(pt_native_document_switch_new(r->control,r->capacity,&r->in,4,SIZE_MAX)==PT_NATIVE_DOCUMENT_SWITCH_HELD);
    assert(r->document->dirty!=r->before_document->dirty && r->control->candidate.loaded);keep(r);
}
static void candidate_drift(void)
{
    struct rig *r=create(0);enrich(r);
    assert(pt_native_document_switch_new(r->control,r->capacity,&r->in,4,SIZE_MAX)==PT_NATIVE_DOCUMENT_SWITCH_PREPARED);
    r->control->candidate.dirty^=1;assert(pt_native_document_switch_state(r->control)==PT_NATIVE_DOCUMENT_SWITCH_HELD);keep(r);
}
static void release_quarantine(void)
{
    struct rig *r=create(0);size_t released;enrich(r);
    assert(pt_native_document_switch_new(r->control,r->capacity,&r->in,4,SIZE_MAX)==PT_NATIVE_DOCUMENT_SWITCH_PREPARED);
    r->a->active=r->control;r->a->quarantine_once=1;released=r->a->releases;
    assert(pt_native_document_switch_cancel(r->control)==PT_NATIVE_DOCUMENT_SWITCH_HELD);
    assert(r->a->quarantined==1 && r->a->releases==released+1);
    assert(pt_document_release_checked(&r->control->candidate)==PT_DOCUMENT_RELEASE_UNCERTAIN);
    assert(r->a->releases==released+1);keep(r);
}
int main(int argc,char **argv)
{
    assert(argc==2);
    if(!strcmp(argv[1],"new_cancel_reuse"))new_cancel_reuse();
    else if(!strcmp(argv[1],"load_roundtrip"))load_roundtrip();
    else if(!strcmp(argv[1],"ordinary_failure"))ordinary_failure();
    else if(!strcmp(argv[1],"geometry"))geometry();
    else if(!strcmp(argv[1],"reentry"))reentry();
    else if(!strcmp(argv[1],"original_drift"))original_drift();
    else if(!strcmp(argv[1],"callback_drift"))callback_drift();
    else if(!strcmp(argv[1],"candidate_drift"))candidate_drift();
    else if(!strcmp(argv[1],"release_quarantine"))release_quarantine();
    else assert(!"unknown finite case");
    printf("DOCUMENT PREP HOST %s PASS %s\n",argv[1],retained?"RETAINED_DOMAIN":"COMPLETE_DOMAIN");return 0;
}
