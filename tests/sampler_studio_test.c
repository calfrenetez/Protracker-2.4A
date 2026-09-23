#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "../src/editor/sampler_studio.h"
static unsigned live,refuse;
static void *alloc(void *c,size_t n) {void *p;(void)c;if(refuse)return NULL;p=malloc(n);if(p)++live;return p;}
static void drop(void *c,void *p) {(void)c;if(p){assert(live);--live;free(p);}}
int main(void)
{
    struct pt_allocator a={NULL,alloc,drop};struct pt_document d;struct pt_sampler s;
    struct pt_pattern_history h;struct pt_pattern_command commands[2];struct pt_event_change changes[2];
    struct pt_sampler_studio provider={&s,&d.project};struct pt_studio_source source=pt_sampler_studio_source(&provider);
    struct pt_studio_mix *mix;struct pt_studio_note note={1,0,1ULL<<32,0,4,0,4,{65536,65536},PT_VOICE_FORWARD,0};
    int32_t original[]={1,257,-513,1025},out[8];struct pt_pcm output={out,8,4,48000,2,24};uint64_t clips;
    pt_document_init(&d,&a);assert(pt_document_new(&d,4,SIZE_MAX)==PT_PROJECT_OK);
    d.project.samples[0].pcm=(struct pt_pcm){original,4,4,48000,1,24};
    pt_sampler_init(&s,&a,1024*1024);assert(pt_pattern_history_init(&h,&d.project,commands,2,changes,2)==PT_EDIT_OK);
    mix=pt_studio_open(&a,&source,2);assert(mix);
    s.budget=0;assert(pt_studio_trigger(mix,0,&note)==PT_PCM_CAPACITY && !s.bytes);s.budget=1024*1024;
    refuse=1;assert(pt_studio_trigger(mix,0,&note)==PT_PCM_CAPACITY);refuse=0;
    assert(!s.bytes && d.project.samples[0].pcm.data==original);
    assert(pt_studio_trigger(mix,0,&note)==PT_PCM_OK);
    assert(d.project.samples[0].pcm.data!=original && s.bytes);
    assert(pt_sampler_edit(&s,&d.project,&h,0,PT_PCM_GAIN,0,4,2000)==PT_EDIT_OK);
    assert(pt_studio_trigger(mix,1,&note)==PT_PCM_CAPACITY); /* stale generation */
    assert(pt_studio_read(mix,&output,&clips)==PT_PCM_OK && out[2]==257);
    assert(pt_pattern_undo(&d.project,&h,-1)==PT_EDIT_OK);
    assert(pt_pattern_undo(&d.project,&h,1)==PT_EDIT_OK);
    /* Repeated edits evict old journal ownership; active voice remains exact. */
    assert(pt_sampler_edit(&s,&d.project,&h,0,PT_PCM_GAIN,0,4,2000)==PT_EDIT_OK);
    assert(pt_sampler_edit(&s,&d.project,&h,0,PT_PCM_GAIN,0,4,2000)==PT_EDIT_OK);
    assert(pt_studio_read(mix,&output,&clips)==PT_PCM_OK && out[2]==257);
    note.version=s.generation;assert(pt_studio_trigger(mix,1,&note)==PT_PCM_OK);
    assert(pt_studio_read(mix,&output,&clips)==PT_PCM_OK && out[2]==257*9);
    pt_pattern_history_release(&h);pt_sampler_release(&s);pt_document_release(&d);
    assert(s.bytes); /* pins independently own both versions */
    assert(pt_studio_read(mix,&output,&clips)==PT_PCM_OK && out[2]==257*9);
    pt_studio_close(mix);assert(!s.bytes && !live && original[1]==257);
    puts("SAMPLER STUDIO PASS: budgeted promotion, stale refusal, edit/undo/eviction pins and final release");return 0;
}
