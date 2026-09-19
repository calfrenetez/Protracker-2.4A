#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/editor/sampler.h"
#include "wav.h"
static size_t live,calls,fail;
static void *allocate(void *c,size_t n) {(void)c;if(++calls==fail)return NULL;void *p=malloc(n);if(p)++live;return p;}
static void release(void *c,void *p) {(void)c;if(p) {assert(live);--live;free(p);}}
int main(void)
{
    struct pt_allocator a={NULL,allocate,release};struct pt_document d,reopened;struct pt_sampler s;
    struct pt_pattern_history h;struct pt_pattern_command commands[4];struct pt_event_change changes[4];
    int32_t values[8]={-8388608,8388607,-1234567,7654321,1,-1,123,456};
    struct pt_pcm pcm={values,8,4,44100,2,24};uint8_t wav[100];size_t length,n,w;unsigned i;
    struct pt_event_update update={0};struct pt_channel route;uint32_t revision;size_t allocation_count;
    pt_document_init(&d,&a);pt_document_init(&reopened,&a);assert(pt_document_new(&d,4,SIZE_MAX)==PT_PROJECT_OK);
    pt_sampler_init(&s,&a,1024*1024);assert(pt_pattern_history_init(&h,&d.project,commands,4,changes,4)==PT_EDIT_OK);
    assert(pt_wav_encode(&pcm,wav,sizeof(wav),&length)==PT_WAV_OK);
    /* Every failed allocation leaves the project, journal and ownership intact. */
    for(i=1;i<=3;++i) {
        allocation_count=live;fail=calls+i;
        assert(pt_sampler_import(&s,&d.project,&h,0,wav,length,"stereo24")==PT_EDIT_CAPACITY);
        assert(live==allocation_count && !s.bytes && !h.count && !d.project.samples[0].pcm.frames);
    }
    fail=0;assert(pt_sampler_import(&s,&d.project,&h,0,wav,length,"stereo24")==PT_EDIT_OK);
    assert(!memcmp(d.project.samples[0].pcm.data,values,sizeof(values)) && s.generation==1);
    update.event.kind=PT_NOTE_PERIOD;update.event.pitch=428;update.event.instrument=1;
    assert(pt_pattern_apply(&d.project,&h,&update,1)==PT_EDIT_OK);
    assert(pt_sampler_edit(&s,&d.project,&h,0,PT_PCM_REVERSE,1,4,0)==PT_EDIT_OK);
    assert(d.project.samples[0].pcm.data[2]==123 && d.project.samples[0].pcm.data[3]==456);
    assert(d.project.samples[0].pcm.data[0]==-8388608);
    route=d.project.channels.track[0];route.muted=1;assert(pt_pattern_channel_apply(&d.project,&h,0,&route)==PT_EDIT_OK);
    pt_pattern_mark_saved(&h);assert(!pt_pattern_dirty(&h));
    assert(pt_pattern_undo(&d.project,&h,-1)==PT_EDIT_OK && !d.project.channels.track[0].muted);
    assert(pt_pattern_undo(&d.project,&h,-1)==PT_EDIT_OK && !memcmp(d.project.samples[0].pcm.data,values,sizeof(values)));
    revision=h.revision;allocation_count=live;
    assert(pt_sampler_edit(&s,&d.project,&h,0,PT_PCM_GAIN,0,4,1000)==PT_EDIT_OK);
    assert(h.revision==revision && h.count==4 && live==allocation_count);
    assert(pt_pattern_undo(&d.project,&h,-1)==PT_EDIT_OK && !d.project.events[0].instrument);
    assert(pt_pattern_undo(&d.project,&h,-1)==PT_EDIT_OK && !d.project.samples[0].pcm.frames);
    for(i=0;i<4;++i)assert(pt_pattern_undo(&d.project,&h,1)==PT_EDIT_OK);
    assert(!pt_pattern_dirty(&h));
    assert(pt_project_size(&d.project,&n)==PT_PROJECT_OK);uint8_t *encoded=malloc(n);assert(encoded);
    assert(pt_project_encode(&d.project,encoded,n,&w)==PT_PROJECT_OK && w==n);
    assert(pt_document_load(&reopened,encoded,n,SIZE_MAX)==PT_PROJECT_OK);free(encoded);
    assert(!memcmp(reopened.project.samples[0].pcm.data,d.project.samples[0].pcm.data,sizeof(values)));
    assert(reopened.project.samples[0].pcm.bits==24 && reopened.project.samples[0].pcm.channels==2);
    /* Eviction and redo truncation must retain the active version. */
    for(i=0;i<12;++i)assert(pt_sampler_edit(&s,&d.project,&h,0,PT_PCM_REVERSE,0,4,0)==PT_EDIT_OK);
    assert(h.count==4 && pt_project_validate(&d.project,NULL)==PT_PROJECT_OK);
    assert(pt_pattern_undo(&d.project,&h,-1)==PT_EDIT_OK);
    assert(pt_sampler_edit(&s,&d.project,&h,0,PT_PCM_GAIN,0,4,500)==PT_EDIT_OK);
    assert(pt_pattern_undo(&d.project,&h,1)==PT_EDIT_END);
    revision=h.revision;allocation_count=live;s.budget=s.bytes;
    assert(pt_sampler_edit(&s,&d.project,&h,0,PT_PCM_REVERSE,0,4,0)==PT_EDIT_CAPACITY);
    assert(h.revision==revision && live==allocation_count);
    assert(pt_sampler_import(&s,&d.project,&h,0,wav,12,"bad")==PT_EDIT_UNSUPPORTED);
    assert(h.revision==revision);
    /* Native New/Load commits the staged document before releasing the old
       editor journal. Its independent sample versions must survive until then. */
    {
        int32_t *active=d.project.samples[0].pcm.data;
        fail=calls+1;assert(pt_document_new(&d,8,SIZE_MAX)==PT_PROJECT_CAPACITY);
        assert(d.project.samples[0].pcm.data==active && h.revision==revision);
        fail=0;assert(pt_document_new(&d,8,SIZE_MAX)==PT_PROJECT_OK);
        assert(d.project.channels.count==8 && !d.project.samples[0].pcm.frames);
    }
    pt_pattern_history_release(&h);pt_sampler_release(&s);assert(!s.bytes);
    /* Replacing a sliced sample refuses dangling event references. Undo keeps
       marker metadata; redo must also recheck references changed externally. */
    {
        uint32_t marker=1;
        reopened.project.samples[0].slices=&marker;reopened.project.samples[0].slice_count=1;
        reopened.project.events[0].slice=1;s.budget=1024*1024;
        assert(pt_pattern_history_init(&h,&reopened.project,commands,4,changes,4)==PT_EDIT_OK);
        assert(pt_sampler_import(&s,&reopened.project,&h,0,wav,length,"replacement")==PT_EDIT_UNSUPPORTED);
        assert(!h.count && !s.bytes);
        reopened.project.events[0].slice=0;
        assert(pt_sampler_import(&s,&reopened.project,&h,0,wav,length,"replacement")==PT_EDIT_OK);
        assert(!reopened.project.samples[0].slice_count);
        assert(pt_pattern_undo(&reopened.project,&h,-1)==PT_EDIT_OK);
        assert(reopened.project.samples[0].slice_count==1 && reopened.project.samples[0].slices[0]==1);
        reopened.project.events[0].slice=1;revision=h.revision;
        assert(pt_pattern_undo(&reopened.project,&h,1)==PT_EDIT_CONFLICT && h.revision==revision);
        pt_pattern_history_release(&h);pt_sampler_release(&s);assert(!s.bytes);
    }
    pt_document_release(&d);pt_document_release(&reopened);assert(!live);
    puts("SAMPLER PASS: exact stereo24 import/range edit, unified history, dirty state, no-op redo, allocation rollback, bounded memory, eviction, save/reopen and release");return 0;
}
