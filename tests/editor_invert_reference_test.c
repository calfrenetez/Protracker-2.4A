#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/editor/editor_studio.h"
#define INVERT_RECORD_BYTES 208
#ifndef INVERT_FIRST_PULL
#define INVERT_FIRST_PULL 1
#endif
#include "invert_reference_oracle.h"
static unsigned live;
static void *allocate(void *c,size_t n){void *p;(void)c;p=malloc(n);if(p)++live;return p;}
static void release(void *c,void *p){(void)c;if(p){assert(live);--live;free(p);}}
static unsigned char *saved;static size_t saved_size;
static void unchanged(struct pt_project *p)
{
    size_t n,written;unsigned char *bytes;
    assert(pt_project_size(p,&n)==PT_PROJECT_OK && n==saved_size);
    bytes=malloc(n);assert(bytes);
    assert(pt_project_encode(p,bytes,n,&written)==PT_PROJECT_OK && written==n && !memcmp(bytes,saved,n));free(bytes);
}
struct stop_context {struct pt_editor_studio *owner;struct pt_project *project;unsigned count;};
static void stopped(void *c)
{
    struct stop_context *s=c;
    /* Stop must close source ownership before this callback and before the edit. */
    assert(!s->owner->invert_song && !s->owner->song && !s->owner->queue);
    unchanged(s->project);++s->count;
}
static struct pt_studio_queue *start(struct pt_editor_studio *owner,struct pt_render_options *options,
                                    struct pt_allocator *a,struct stop_context *stop,struct oracle *o)
{
    struct pt_studio_queue *q=pt_studio_queue_open(a,2);
    assert(q && pt_editor_studio_start_invert_queued(owner,options,SIZE_MAX,q)==PT_RENDER_OK);
    assert(pt_editor_studio_bind_output_stop(owner,stopped,stop));reset(o);return q;
}
static void congest(struct pt_editor_studio *owner,const struct pt_pcm *held,unsigned block)
{
    unsigned i;int32_t copy[512],pending[512];unsigned frames;
    memcpy(copy,held->data,held->frames*2*sizeof(int32_t));
    for(i=0;i<6;++i)assert(pt_editor_studio_step(owner,block)!=PT_PUMP_ERROR);
    if(owner->pump.held) {
        frames=owner->pump.pending.frames;memcpy(pending,owner->pump.data,frames*2*sizeof(int32_t));
        for(i=0;i<4;++i)assert(pt_editor_studio_step(owner,256)==PT_PUMP_BLOCKED);
        assert(owner->pump.pending.frames==frames && !memcmp(pending,owner->pump.data,frames*2*sizeof(int32_t)));
    }
    assert(!memcmp(copy,held->data,held->frames*2*sizeof(int32_t)));
}
static void complete(struct pt_editor_studio *owner,struct pt_render_options *options,
                     struct pt_allocator *a,struct stop_context *stop,struct oracle *o,unsigned block)
{
    struct pt_studio_queue *q=start(owner,options,a,stop,o);unsigned i,blocked=0,before=stop->count;
    for(i=0;i<300000;++i) {
        const struct pt_pcm *pcm;uint64_t ticket;enum pt_queue_result qr;
        assert(pt_editor_studio_step(owner,block)!=PT_PUMP_ERROR);
        qr=pt_studio_queue_acquire(q,&pcm,&ticket);
        if(qr==PT_QUEUE_OK) {
            congest(owner,pcm,block);blocked+=owner->pump.held!=0;
            assert(receive(o,pcm,o->frames));assert(pt_studio_queue_release(q,ticket)==PT_QUEUE_OK);
        } else if(qr==PT_QUEUE_DONE)break;
        else assert(qr==PT_QUEUE_EMPTY);
    }
    assert(i<300000 && blocked && o->frames==o->tick_end[o->ticks-1] && o->changes==2);
    assert(!owner->invert_song && owner->queue==q && owner->output_stop && stop->count==before);
    unchanged(stop->project);pt_editor_studio_stop(owner);assert(stop->count==before+1);
    assert(pt_studio_queue_close(q)==PT_QUEUE_OK);
}
int main(int argc,char **argv)
{
    struct pt_allocator a={NULL,allocate,release};struct pt_document d,copy;
    struct pt_editor *e=malloc(sizeof(*e));struct pt_editor_studio owner={0};
    struct pt_render_options options={0};struct oracle o;struct stop_context stop;
    FILE *f;long size;unsigned i,action,partitions[]={INVERT_FIRST_PULL,17,256};uint64_t clock=0;size_t written;
    unsigned char record[208];assert(argc==4 && e);memset(e,0,sizeof(*e));memset(&o,0,sizeof(o));
    f=fopen(argv[1],"rb");assert(f && !fseek(f,0,SEEK_END));size=ftell(f);assert(size>0);rewind(f);
    o.size=(size_t)size;o.initial=malloc(o.size);o.data=malloc(o.size);assert(o.initial && o.data);
    assert(fread(o.initial,1,o.size,f)==o.size && !fclose(f));pt_document_init(&d,&a);
    assert(pt_document_load(&d,o.initial,o.size,SIZE_MAX)==PT_PROJECT_OK);
    for(i=0;i<d.project.sample_count;++i)d.project.samples[i].pcm.rate=48000;
    d.project.channels.track[0].pan=0;
    assert(pt_editor_init(e,&d.project));pt_sampler_init(&e->sampler,&a,1024*1024);
    assert(pt_editor_studio_attach(&owner,e));stop=(struct stop_context){&owner,&d.project,0};
    assert(pt_project_size(&d.project,&saved_size)==PT_PROJECT_OK);saved=malloc(saved_size);assert(saved);
    assert(pt_project_encode(&d.project,saved,saved_size,&written)==PT_PROJECT_OK && written==saved_size);
    f=fopen(argv[2],"rb");assert(f);
    while(fread(record,1,208,f)==208) {
        if(!record[14] || !word(record+28))continue;
        assert(o.ticks<100 && word(record+12)>=32);memcpy(o.record[o.ticks],record,208);
        clock+=(120000ULL<<32)/word(record+12);o.tick_end[o.ticks++]=clock>>32;
    }
    assert(feof(f) && !ferror(f) && !fclose(f) && o.ticks==(unsigned)strtoul(argv[3],NULL,10));
    options.rate=48000;options.bits=24;options.tracks=1;options.gain_q16=65536;options.tick_limit=100;options.frame_limit=100000;
    assert(pt_editor_studio_start_invert(&owner,&options,1)==PT_RENDER_MEMORY && !owner.invert_song);unchanged(&d.project);
    for(i=0;i<3;++i)if(i!=1 || INVERT_FIRST_PULL!=17)complete(&owner,&options,&a,&stop,&o,partitions[i]);
    for(action=0;action<2;++action) {
        struct pt_studio_queue *q=start(&owner,&options,&a,&stop,&o);
        const struct pt_pcm *held=NULL;uint64_t ticket=0;int32_t retained[512];unsigned frames=0,before=stop.count;
        for(i=0;i<10000;++i) {
            enum pt_queue_result qr;assert(pt_editor_studio_step(&owner,17)!=PT_PUMP_ERROR);
            qr=pt_studio_queue_acquire(q,&held,&ticket);
            if(qr==PT_QUEUE_OK) {
                assert(receive(&o,held,o.frames));
                if(o.frames>=12500)break;
                assert(pt_studio_queue_release(q,ticket)==PT_QUEUE_OK);
            } else assert(qr==PT_QUEUE_EMPTY);
        }
        assert(i<10000 && held);frames=held->frames;memcpy(retained,held->data,frames*2*sizeof(int32_t));
        congest(&owner,held,17);assert(owner.pump.held);
        if(action==0) {e->panel=0;e->row=0;e->editing=1;pt_editor_key(e,0x31,0);}
        else {e->panel=5;e->sample=1;pt_editor_sample_all(e);pt_editor_key(e,0x24,0);}
        assert(!owner.invert_song && !owner.queue && !owner.pump.held && stop.count==before+1);
        assert(!memcmp(retained,held->data,frames*2*sizeof(int32_t)));
        assert(pt_studio_queue_close(q)==PT_QUEUE_BUSY);pt_editor_studio_stop(&owner);assert(stop.count==before+1);
        assert(pt_studio_queue_release(q,ticket)==PT_QUEUE_OK);
        assert(pt_studio_queue_acquire(q,&held,&ticket)==PT_QUEUE_DONE && pt_studio_queue_close(q)==PT_QUEUE_OK);
        assert(pt_pattern_dirty(&e->history));pt_editor_key(e,0x31,8);unchanged(&d.project);
        /* Undo restored the master/event representation; restart must reset all clocks. */
        complete(&owner,&options,&a,&stop,&o,256);
    }
    pt_document_init(&copy,&a);assert(pt_document_load(&copy,saved,saved_size,SIZE_MAX)==PT_PROJECT_OK);
    unchanged(&copy.project);pt_document_release(&copy);
    pt_editor_studio_detach(&owner);pt_editor_dispose(e);pt_document_release(&d);
    free(saved);free(o.initial);free(o.data);free(e);assert(!live);
    puts("EDITOR EFx REFERENCE PASS: exact queued PCM, backpressure, guarded edits, held leases, undo/restart, master roundtrip, no leaks");return 0;
}
