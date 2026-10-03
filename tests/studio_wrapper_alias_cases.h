/* Shared public-owner regressions; included by existing host/native fixtures.
 * The allocator observer owns every recorded span; no freed output is tested. */
#include <string.h>
#include "../src/editor/sampler_song.h"
#include "../src/editor/sampler_internal.h"
#ifdef PT_STUDIO_ALIAS_WITH_INVERT
#include "../src/editor/sampler_invert_song.h"
#endif
#ifdef PT_STUDIO_ALIAS_WITH_EDITOR
#include "../src/editor/editor_studio.h"
#endif
struct studio_alias_allocation {void *pointer;size_t bytes;unsigned active;};
struct studio_alias_memory {const struct pt_allocator *base;struct studio_alias_allocation allocation[256];unsigned count,live;};
static void *studio_alias_allocate(void *context,size_t bytes)
{
    struct studio_alias_memory *m=context;void *pointer=m->base->allocate(m->base->context,bytes);
    if(pointer) {assert(m->count<256);m->allocation[m->count++]=(struct studio_alias_allocation){pointer,bytes,1};++m->live;}
    return pointer;
}
static void studio_alias_release(void *context,void *pointer)
{
    struct studio_alias_memory *m=context;unsigned i;
    if(!pointer)return;
    for(i=0;i<m->count;++i)if(m->allocation[i].active && m->allocation[i].pointer==pointer)break;
    assert(i<m->count && m->live);m->allocation[i].active=0;--m->live;
    m->base->release(m->base->context,pointer);
}
struct studio_alias_handle {
    struct pt_sampler_song *ordinary;
#ifdef PT_STUDIO_ALIAS_WITH_INVERT
    struct pt_sampler_invert_song *invert;
#endif
#ifdef PT_STUDIO_ALIAS_WITH_EDITOR
    struct pt_editor_studio *editor;
#endif
    unsigned private_bank;
};
static enum pt_render_result studio_alias_begin(struct studio_alias_handle *h,struct pt_sampler *s,
    struct pt_project *p,const struct pt_render_options *o,const struct pt_allocator *a)
{
#ifdef PT_STUDIO_ALIAS_WITH_EDITOR
    if(h->editor)return h->private_bank?pt_editor_studio_begin_invert(h->editor,o,SIZE_MAX):pt_editor_studio_begin(h->editor,o);
#endif
#ifdef PT_STUDIO_ALIAS_WITH_INVERT
    if(h->private_bank)return pt_sampler_invert_song_begin(s,p,o,SIZE_MAX,a,&h->invert);
#endif
#ifndef PT_STUDIO_ALIAS_INVERT_ONLY
    return pt_sampler_song_begin(s,p,o,a,&h->ordinary);
#else
    return PT_RENDER_INVALID;
#endif
}
static enum pt_render_result studio_alias_prepare(struct studio_alias_handle *h,unsigned *ready)
{
#ifdef PT_STUDIO_ALIAS_WITH_EDITOR
    if(h->editor)return pt_editor_studio_prepare(h->editor,ready);
#endif
#ifdef PT_STUDIO_ALIAS_WITH_INVERT
    if(h->private_bank)return pt_sampler_invert_song_prepare(h->invert,ready);
#endif
#ifndef PT_STUDIO_ALIAS_INVERT_ONLY
    return pt_sampler_song_prepare(h->ordinary,ready);
#else
    return PT_RENDER_INVALID;
#endif
}
static enum pt_render_result studio_alias_pull(struct studio_alias_handle *h,const struct pt_pcm **pcm,unsigned *done)
{
#ifdef PT_STUDIO_ALIAS_WITH_EDITOR
    if(h->editor)return pt_editor_studio_pull(h->editor,1,pcm,done);
#endif
#ifdef PT_STUDIO_ALIAS_WITH_INVERT
    if(h->private_bank)return pt_sampler_invert_song_pull(h->invert,1,pcm,done);
#endif
#ifndef PT_STUDIO_ALIAS_INVERT_ONLY
    return pt_sampler_song_pull(h->ordinary,1,pcm,done);
#else
    return PT_RENDER_INVALID;
#endif
}
static void studio_alias_stop(struct studio_alias_handle *h)
{
#ifdef PT_STUDIO_ALIAS_WITH_EDITOR
    if(h->editor) {pt_editor_studio_stop(h->editor);return;}
#endif
#ifdef PT_STUDIO_ALIAS_WITH_INVERT
    if(h->private_bank) {pt_sampler_invert_song_stop(h->invert);return;}
#endif
#ifndef PT_STUDIO_ALIAS_INVERT_ONLY
    pt_sampler_song_stop(h->ordinary);
#endif
}
static void studio_alias_close(struct studio_alias_handle *h)
{
#ifdef PT_STUDIO_ALIAS_WITH_EDITOR
    if(h->editor) {pt_editor_studio_stop(h->editor);return;}
#endif
#ifdef PT_STUDIO_ALIAS_WITH_INVERT
    if(h->private_bank) {pt_sampler_invert_song_close(h->invert);h->invert=NULL;return;}
#endif
#ifndef PT_STUDIO_ALIAS_INVERT_ONLY
    pt_sampler_song_close(h->ordinary);h->ordinary=NULL;
#endif
}
static void studio_alias_refuse_span(struct studio_alias_handle *h,void *storage,size_t bytes,
    const struct pt_allocator *a)
{
    unsigned ready=77,done=77;const struct pt_pcm *pcm=(const struct pt_pcm *)1;
    unsigned char *image=a->allocate(a->context,bytes);unsigned *tail;
    assert(image && bytes>=sizeof(unsigned));memcpy(image,storage,bytes);
    tail=(unsigned *)((unsigned char *)storage+((bytes-sizeof(unsigned))/sizeof(unsigned))*sizeof(unsigned));
    assert(studio_alias_prepare(h,storage)==PT_RENDER_INVALID);
    assert(!memcmp(image,storage,bytes));
    assert(studio_alias_prepare(h,tail)==PT_RENDER_INVALID && !memcmp(image,storage,bytes));
    assert(studio_alias_pull(h,&pcm,tail)==PT_RENDER_INVALID && pcm==(const struct pt_pcm *)1 && !memcmp(image,storage,bytes));
    if(bytes>=sizeof(pcm)) {
        assert(studio_alias_pull(h,(const struct pt_pcm **)storage,&done)==PT_RENDER_INVALID && done==77 && !memcmp(image,storage,bytes));
    }
    (void)ready;a->release(a->context,image);
}
static void studio_alias_owner_cases(const struct pt_allocator *base,unsigned private_bank,unsigned editor_case)
{
    struct studio_alias_memory memory={0};struct pt_allocator a={&memory,studio_alias_allocate,studio_alias_release};
    struct pt_document document;struct pt_sampler sampler,*s=&sampler;
    struct studio_alias_handle h={0};struct pt_render_options options={0};
    struct pt_project saved;struct pt_sample saved_samples[2];int32_t *master,*unused;uint32_t markers[2]={0,4096};
    unsigned i,phase,ready,done,first_allocation,steps,generation;const struct pt_pcm *pcm;
#ifdef PT_STUDIO_ALIAS_WITH_EDITOR
    struct pt_editor *editor=NULL;struct pt_editor_studio owner={0};
#endif
    memory.base=base;h.private_bank=private_bank;
    master=base->allocate(base->context,8208*sizeof(*master));unused=base->allocate(base->context,20*sizeof(*unused));assert(master && unused);
    for(i=0;i<8208;++i)master[i]=(int)(i%127)-63;
    for(i=0;i<20;++i)unused[i]=(int)i-10;
    pt_document_init(&document,&a);assert(pt_document_new(&document,4,SIZE_MAX)==PT_PROJECT_OK);
#ifdef PT_STUDIO_ALIAS_WITH_EDITOR
    if(editor_case) {editor=base->allocate(base->context,sizeof(*editor));assert(editor);memset(editor,0,sizeof(*editor));assert(pt_editor_init(editor,&document.project));
        s=&editor->sampler;h.editor=&owner;assert(pt_editor_studio_attach(&owner,editor));}
#else
    assert(!editor_case);
#endif
    pt_sampler_init(s,&a,1024*1024);
    document.project.samples[0].pcm=(struct pt_pcm){master,8208,8192,48000,1,private_bank?8:24};
    document.project.samples[0].volume=64;document.project.samples[0].loop=PT_LOOP_FORWARD;document.project.samples[0].loop_end=8192;
    document.project.samples[0].slices=markers;document.project.samples[0].slice_count=2;
    document.project.samples[1].pcm=(struct pt_pcm){unused,20,4,48000,1,private_bank?8:24};
    document.project.samples[1].volume=64;
    document.project.events[0].kind=PT_NOTE_PERIOD;document.project.events[0].pitch=428;document.project.events[0].instrument=1;
    document.project.events[4].effect=15;
    options.rate=48000;options.bits=24;options.tracks=1;options.gain_q16=65536;options.tick_limit=1000;options.frame_limit=1000000;
    saved=document.project;memcpy(saved_samples,document.project.samples,sizeof(saved_samples));generation=s->generation;
    /* External handle publication into authoritative aligned PCM is refused. */
    if(!editor_case)for(phase=0;phase<4;++phase) {
        enum pt_render_result result;unsigned live_before=memory.live;int32_t *publication=master+(phase>=2?8204:0);
#ifdef PT_STUDIO_ALIAS_WITH_INVERT
        if(private_bank)result=(phase&1)?pt_sampler_invert_song_open(s,&document.project,&options,SIZE_MAX,&a,(struct pt_sampler_invert_song **)publication):
            pt_sampler_invert_song_begin(s,&document.project,&options,SIZE_MAX,&a,(struct pt_sampler_invert_song **)publication);
        else
#endif
#ifndef PT_STUDIO_ALIAS_INVERT_ONLY
        result=(phase&1)?pt_sampler_song_open(s,&document.project,&options,&a,(struct pt_sampler_song **)publication):
            pt_sampler_song_begin(s,&document.project,&options,&a,(struct pt_sampler_song **)publication);
#else
        result=PT_RENDER_INVALID;
#endif
        assert(result==PT_RENDER_INVALID && !s->bytes && s->generation==generation && !s->current[0] && !s->current[1]);
        assert(memory.live==live_before && document.project.samples==saved.samples &&
            !memcmp(document.project.samples,saved_samples,sizeof(saved_samples)));
        for(i=0;i<8208;++i)assert(master[i]==(int)(i%127)-63);
        /* Refusal precedes promotion and preserves the complete original span. */
        if(!private_bank) {pt_sampler_release(s);pt_sampler_init(s,&a,1024*1024);document.project=saved;memcpy(document.project.samples,saved_samples,sizeof(saved_samples));}
    }
    if(!editor_case) {
        union {struct pt_render_options options;void *aligned;} argument;
        struct pt_allocator argument_allocator=a;unsigned calls=memory.count;enum pt_render_result result;
        unsigned char option_image[sizeof(argument)],allocator_image[sizeof(argument_allocator)];
        memset(&argument,0,sizeof(argument));argument.options=options;
        memcpy(option_image,&argument,sizeof(argument));memcpy(allocator_image,&argument_allocator,sizeof(argument_allocator));
        for(phase=0;phase<4;++phase) {
            void *output=phase<2?(void *)&argument:(void *)&argument_allocator;
#ifdef PT_STUDIO_ALIAS_WITH_INVERT
            if(private_bank)result=(phase&1)?pt_sampler_invert_song_open(s,&document.project,&argument.options,SIZE_MAX,&argument_allocator,output):
                pt_sampler_invert_song_begin(s,&document.project,&argument.options,SIZE_MAX,&argument_allocator,output);
            else
#endif
#ifndef PT_STUDIO_ALIAS_INVERT_ONLY
            result=(phase&1)?pt_sampler_song_open(s,&document.project,&argument.options,&argument_allocator,output):
                pt_sampler_song_begin(s,&document.project,&argument.options,&argument_allocator,output);
#else
            result=PT_RENDER_INVALID;
#endif
            assert(result==PT_RENDER_INVALID && memory.count==calls && !memcmp(option_image,&argument,sizeof(argument)) &&
                !memcmp(allocator_image,&argument_allocator,sizeof(argument_allocator)));
        }
    }
    first_allocation=memory.count;assert(studio_alias_begin(&h,s,&document.project,&options,&a)==PT_RENDER_OK);
    studio_alias_refuse_span(&h,master,8208*sizeof(*master),base);
    studio_alias_refuse_span(&h,unused,20*sizeof(*unused),base);
    studio_alias_refuse_span(&h,markers,sizeof(markers),base);
    studio_alias_refuse_span(&h,s,sizeof(*s),base);
    studio_alias_refuse_span(&h,&document.project,sizeof(document.project),base);
#ifdef PT_STUDIO_ALIAS_WITH_EDITOR
    if(editor_case) {studio_alias_refuse_span(&h,&owner,sizeof(owner),base);studio_alias_refuse_span(&h,editor,sizeof(*editor),base);}
#endif
    /* Check every complete newly owned allocation and its last aligned word,
     * both pending and after a private copy/promotion allocation exists. */
    for(phase=0;phase<3;++phase) {
        for(i=first_allocation;i<memory.count;++i)if(memory.allocation[i].active)
            studio_alias_refuse_span(&h,memory.allocation[i].pointer,memory.allocation[i].bytes,base);
        {union {const struct pt_pcm *pcm;unsigned done;} both;unsigned char image[sizeof(both)];
            both.pcm=(const struct pt_pcm *)1;memcpy(image,&both,sizeof(both));
            assert(studio_alias_pull(&h,&both.pcm,&both.done)==PT_RENDER_INVALID && !memcmp(image,&both,sizeof(both)));}
        if(phase==0) {
            for(steps=0;steps<1000;++steps) {ready=0;assert(studio_alias_prepare(&h,&ready)==PT_RENDER_OK);
                if(private_bank || s->bytes)break;}
            assert(steps<1000 && !ready);
        }else if(phase==1) {
            ready=0;for(steps=0;steps<1000 && !ready;++steps)assert(studio_alias_prepare(&h,&ready)==PT_RENDER_OK);
            assert(ready && steps<1000);
        }
    }
    pcm=NULL;done=0;for(steps=0;steps<1000 && !pcm;++steps)assert(studio_alias_pull(&h,&pcm,&done)==PT_RENDER_OK && !done);
    assert(pcm && steps<1000);studio_alias_stop(&h);
    /* Stop permits actual release/reinitialization of former table owners. */
    pt_sampler_release(s);pt_sampler_init(s,&a,1024*1024);document.project=saved;memcpy(document.project.samples,saved_samples,sizeof(saved_samples));
    saved=document.project;document.project.samples=NULL;document.project.events=NULL;document.project.orders=NULL;
    ready=77;assert(studio_alias_prepare(&h,&ready)==PT_RENDER_INVALID && ready==77);
    pcm=(const struct pt_pcm *)1;done=77;assert(studio_alias_pull(&h,&pcm,&done)==PT_RENDER_OK && !pcm && done==1);
    document.project=saved;studio_alias_close(&h);
    /* Stale header/generation refusal does not write either caller output,
     * including a still-live original master supplied as the output span. */
    for(phase=0;phase<3;++phase) {
        assert(studio_alias_begin(&h,s,&document.project,&options,&a)==PT_RENDER_OK);
        if(phase==0)++s->generation;
        else if(phase==1)document.project.samples=NULL;
#ifdef PT_STUDIO_ALIAS_WITH_EDITOR
        else if(editor_case)editor->project=NULL;
#endif
        else document.project.events=NULL;
        pcm=(const struct pt_pcm *)1;done=77;
        assert(studio_alias_pull(&h,&pcm,(unsigned *)master)==PT_RENDER_INVALID && pcm==(const struct pt_pcm *)1 && master[0]==-63);
        ready=77;assert(studio_alias_prepare(&h,&ready)==PT_RENDER_INVALID && ready==77);
        if(!editor_case)assert(studio_alias_pull(&h,&pcm,&done)==PT_RENDER_INVALID && pcm==(const struct pt_pcm *)1 && done==77);
        else assert(studio_alias_pull(&h,&pcm,&done)==PT_RENDER_OK && !pcm && done==1);
        studio_alias_close(&h);s->generation=generation;document.project=saved;
#ifdef PT_STUDIO_ALIAS_WITH_EDITOR
        if(editor_case)editor->project=&document.project;
#endif
    }
#ifdef PT_STUDIO_ALIAS_WITH_EDITOR
    if(editor_case) {
        struct pt_studio_queue *queue=pt_studio_queue_open(&a,2);uint64_t ticket;
        assert(queue && (private_bank?pt_editor_studio_begin_invert_queued(&owner,&options,SIZE_MAX,queue):
            pt_editor_studio_begin_queued(&owner,&options,queue))==PT_RENDER_OK);
        editor->project=NULL;
        assert(pt_editor_studio_step(&owner,1)==PT_PUMP_ERROR && !owner.song && !owner.invert_song && !owner.queue);
        pcm=(const struct pt_pcm *)1;assert(pt_studio_queue_acquire(queue,&pcm,&ticket)==PT_QUEUE_DONE && pt_studio_queue_close(queue)==PT_QUEUE_OK);
        editor->project=&document.project;
    }
#endif
    for(i=0;i<8208;++i)assert(master[i]==(int)(i%127)-63);
    for(i=0;i<20;++i)assert(unused[i]==(int)i-10);
    assert(markers[0]==0 && markers[1]==4096);
    pt_sampler_release(s);
#ifdef PT_STUDIO_ALIAS_WITH_EDITOR
    if(editor_case) {pt_editor_studio_detach(&owner);pt_editor_dispose(editor);base->release(base->context,editor);}
#endif
    pt_document_release(&document);assert(!memory.live);
    base->release(base->context,master);base->release(base->context,unused);
    puts(private_bank?"STUDIO PRIVATE OWNER ALIAS PASS: capacity/padding, private allocations, pairwise outputs, stale cancellation and former-table Stop":
        "STUDIO MASTER OWNER ALIAS PASS: pre-promotion publication refusal, job/pin capacities, pairwise outputs, stale cancellation and former-table Stop");
}
#ifndef PT_STUDIO_ALIAS_INVERT_ONLY
static void studio_alias_adopted_cases(const struct pt_allocator *base)
{
    struct studio_alias_memory memory={0};struct pt_allocator a={&memory,studio_alias_allocate,studio_alias_release};
    struct pt_document d;struct pt_sampler s;struct pt_pattern_history history;
    struct pt_pattern_command commands[4];struct pt_event_change changes[4];
    struct studio_alias_handle h={0};struct pt_render_options options={0};struct pt_pcm owned;
    struct pt_sample_version *backing;unsigned slot,i,ready=0;
    memory.base=base;pt_document_init(&d,&a);assert(pt_document_new(&d,4,SIZE_MAX)==PT_PROJECT_OK);
    pt_sampler_init(&s,&a,1024*1024);assert(pt_pattern_history_init(&history,&d.project,commands,4,changes,4)==PT_EDIT_OK);
    owned=(struct pt_pcm){a.allocate(a.context,32*sizeof(int32_t)),32,4,48000,1,24};assert(owned.data);
    for(i=0;i<32;++i)owned.data[i]=(int)i-16;
    assert(pt_sampler_append_owned(&s,&d.project,&history,&owned,&a,"owned")==PT_EDIT_OK && !owned.data);
    for(slot=0;slot<d.project.sample_count && !s.current[slot];++slot){}assert(slot<d.project.sample_count);
    backing=s.current[slot];assert(pt_sampler_attributes(&s,&d.project,&history,slot,"metadata",64,0)==PT_EDIT_OK && s.current[slot]!=backing);
    d.project.events[0].kind=PT_NOTE_PERIOD;d.project.events[0].pitch=428;d.project.events[0].instrument=slot+1;d.project.events[4].effect=15;
    options.rate=48000;options.bits=24;options.tracks=1;options.gain_q16=65536;options.tick_limit=1000;options.frame_limit=1000000;
    assert(studio_alias_begin(&h,&s,&d.project,&options,&a)==PT_RENDER_OK);
    studio_alias_refuse_span(&h,d.project.samples[slot].pcm.data,32*sizeof(int32_t),base);
    for(i=0;i<memory.count;++i)if(memory.allocation[i].active &&
       (memory.allocation[i].pointer==s.current[slot] || memory.allocation[i].pointer==backing || memory.allocation[i].pointer==s.table))
        studio_alias_refuse_span(&h,memory.allocation[i].pointer,memory.allocation[i].bytes,base);
    for(i=0;i<1000 && !ready;++i)assert(studio_alias_prepare(&h,&ready)==PT_RENDER_OK);
    assert(ready && i<1000);
    studio_alias_refuse_span(&h,d.project.samples[slot].pcm.data,32*sizeof(int32_t),base);
    studio_alias_close(&h);pt_pattern_history_release(&history);pt_sampler_release(&s);pt_document_release(&d);assert(!memory.live);
    puts("STUDIO ADOPTED OWNER ALIAS PASS: full owned PCM capacity, separate version headers, shared backing and table storage");
}
#endif
