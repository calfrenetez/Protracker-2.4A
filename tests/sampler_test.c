#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/editor/sampler.h"
#include "wav.h"
static size_t live,calls,fail;
static void *allocate(void *c,size_t n) {(void)c;if(++calls==fail)return NULL;void *p=malloc(n);if(p)++live;return p;}
static void release(void *c,void *p) {(void)c;if(p) {assert(live);--live;free(p);}}
static void loops_and_slices(void)
{
    struct pt_allocator a={NULL,allocate,release};struct pt_document d,reopened;struct pt_sampler s;
    struct pt_pattern_history h;struct pt_pattern_command commands[16];struct pt_event_change changes[16];
    int32_t values[8]={-100,80,20,-30,100,-80,40,-50},baked[8];
    struct pt_pcm pcm={values,8,8,8287,1,8},cross={baked,8,8,8287,1,8};uint8_t wav[100],*encoded;
    uint32_t markers[3]={0,2,6},retarget[3]={0,1,2},next,revision;size_t length,n,w,allocations,i;
    struct pt_event_update event={0};
    pt_document_init(&d,&a);pt_document_init(&reopened,&a);assert(pt_document_new(&d,4,SIZE_MAX)==PT_PROJECT_OK);
    pt_sampler_init(&s,&a,1024*1024);assert(pt_pattern_history_init(&h,&d.project,commands,16,changes,16)==PT_EDIT_OK);
    assert(pt_wav_encode(&pcm,wav,sizeof(wav),&length)==PT_WAV_OK);
    assert(pt_sampler_import(&s,&d.project,&h,0,wav,length,"loops")==PT_EDIT_OK);
    for(i=1;i<=2;++i) {
        allocations=live;revision=h.revision;fail=calls+i;
        assert(pt_sampler_loop(&s,&d.project,&h,0,PT_LOOP_FORWARD,0,8,0)==PT_EDIT_CAPACITY);
        assert(live==allocations && revision==h.revision && !d.project.samples[0].loop);
    }
    fail=0;
    assert(pt_sampler_loop(&s,&d.project,&h,0,PT_LOOP_FORWARD,0,8,0)==PT_EDIT_OK);
    assert(pt_sampler_loop(&s,&d.project,&h,0,PT_LOOP_PINGPONG,2,6,0)==PT_EDIT_OK);
    assert(pt_pattern_undo(&d.project,&h,-1)==PT_EDIT_OK);
    revision=h.revision;
    assert(pt_sampler_loop(&s,&d.project,&h,0,PT_LOOP_CROSSFADE,0,8,5)==PT_EDIT_INVALID);
    assert(pt_sampler_loop(&s,&d.project,&h,0,PT_LOOP_FORWARD,0,8,0)==PT_EDIT_OK && h.revision==revision);
    assert(pt_pattern_undo(&d.project,&h,1)==PT_EDIT_OK && d.project.samples[0].loop==PT_LOOP_PINGPONG);
    memcpy(baked,values,sizeof(baked));assert(pt_pcm_crossfade_loop(&cross,0,8,2,&next)==PT_PCM_OK && next==2);
    assert(pt_sampler_loop(&s,&d.project,&h,0,PT_LOOP_CROSSFADE,0,8,2)==PT_EDIT_OK);
    assert(d.project.samples[0].loop==PT_LOOP_FORWARD && d.project.samples[0].loop_start==2 && !d.project.samples[0].crossfade);
    assert(!memcmp(d.project.samples[0].pcm.data,baked,sizeof(baked)));
    assert(pt_pattern_undo(&d.project,&h,-1)==PT_EDIT_OK && !memcmp(d.project.samples[0].pcm.data,values,sizeof(values)));
    assert(d.project.samples[0].loop==PT_LOOP_PINGPONG && d.project.samples[0].loop_end==6);
    assert(pt_sampler_slices(&s,&d.project,&h,0,markers,2)==PT_EDIT_OK);
    assert(!memcmp(d.project.samples[0].pcm.data,values,sizeof(values)));
    event.event.kind=PT_NOTE_PERIOD;event.event.pitch=428;event.event.instrument=1;event.event.slice=2;
    assert(pt_pattern_apply(&d.project,&h,&event,1)==PT_EDIT_OK);revision=h.revision;allocations=live;
    assert(pt_sampler_slices(&s,&d.project,&h,0,retarget,3)==PT_EDIT_UNSUPPORTED);
    assert(pt_sampler_slices(&s,&d.project,&h,0,NULL,0)==PT_EDIT_UNSUPPORTED);
    assert(h.revision==revision && live==allocations && d.project.events[0].slice==2);
    for(i=1;i<=2;++i) {
        fail=calls+i;assert(pt_sampler_slices(&s,&d.project,&h,0,markers,3)==PT_EDIT_CAPACITY);
        assert(h.revision==revision && live==allocations && d.project.samples[0].slice_count==2);
    }
    fail=0;assert(pt_sampler_slices(&s,&d.project,&h,0,markers,3)==PT_EDIT_OK);
    d.project.events[0].slice=3;revision=h.revision;
    assert(pt_pattern_undo(&d.project,&h,-1)==PT_EDIT_CONFLICT && h.revision==revision);
    d.project.events[0].slice=2;assert(pt_pattern_undo(&d.project,&h,-1)==PT_EDIT_OK);
    assert(pt_sampler_slices(&s,&d.project,&h,0,markers,2)==PT_EDIT_OK);
    assert(pt_pattern_undo(&d.project,&h,1)==PT_EDIT_OK);
    /* Same count can still retarget a note; redo must reject that case too. */
    assert(pt_pattern_undo(&d.project,&h,-1)==PT_EDIT_OK);
    assert(pt_pattern_undo(&d.project,&h,-1)==PT_EDIT_OK); /* note */
    assert(pt_sampler_slices(&s,&d.project,&h,0,retarget,2)==PT_EDIT_OK);
    assert(pt_pattern_undo(&d.project,&h,-1)==PT_EDIT_OK);
    d.project.events[0]=event.event;revision=h.revision;
    assert(pt_pattern_undo(&d.project,&h,1)==PT_EDIT_CONFLICT && h.revision==revision);
    d.project.events[0].slice=0;assert(pt_pattern_undo(&d.project,&h,1)==PT_EDIT_OK);
    pt_pattern_mark_saved(&h);assert(pt_project_size(&d.project,&n)==PT_PROJECT_OK);encoded=malloc(n);assert(encoded);
    assert(pt_project_encode(&d.project,encoded,n,&w)==PT_PROJECT_OK && w==n);
    assert(pt_document_load(&reopened,encoded,n,SIZE_MAX)==PT_PROJECT_OK);free(encoded);
    assert(reopened.project.samples[0].loop==PT_LOOP_PINGPONG && reopened.project.samples[0].loop_start==2 && reopened.project.samples[0].loop_end==6);
    assert(reopened.project.samples[0].slice_count==2 && !memcmp(reopened.project.samples[0].slices,retarget,8));
    assert(!memcmp(reopened.project.samples[0].pcm.data,values,sizeof(values)));
    assert(pt_sampler_loop(&s,&d.project,&h,0,PT_LOOP_NONE,0,0,0)==PT_EDIT_OK);
    assert(!d.project.samples[0].loop && !d.project.samples[0].loop_start && !d.project.samples[0].loop_end);
    assert(pt_pattern_undo(&d.project,&h,-1)==PT_EDIT_OK && !pt_pattern_dirty(&h));
    pt_pattern_history_release(&h);pt_sampler_release(&s);assert(!s.bytes);
    pt_document_release(&d);pt_document_release(&reopened);assert(!live);
    puts("LOOP/SLICE PASS: metadata, atomic bake undo, proposal commit, non-destructive PCM, retarget refusal, undo/redo conflicts, allocation rollback and exact persistence");
}
static int cancel_conversion(void *context,uint32_t done,uint32_t total)
{(void)total;return done<*(uint32_t *)context;}
static void conversion(void)
{
    struct pt_allocator a={NULL,allocate,release};struct pt_document d,reopened;struct pt_sampler s;
    struct pt_pattern_history h;struct pt_pattern_command commands[12];struct pt_event_change changes[4];
    int32_t values[8]={-8388608,8388607,-65536,65536,0,0,8388607,-8388608};
    int32_t expected[16]={-32768,32767,-16512,16512,-256,256,-128,128,0,0,16384,-16384,32767,-32768,32767,-32768};
    uint32_t markers[2]={0,2},revision;struct pt_event_update event={0};
    struct pt_pcm pcm={values,8,4,44100,2,24};uint8_t wav[100],*encoded;size_t length,allocations,n,w;unsigned i;
    pt_document_init(&d,&a);pt_document_init(&reopened,&a);assert(pt_document_new(&d,4,SIZE_MAX)==PT_PROJECT_OK);
    pt_sampler_init(&s,&a,1024*1024);assert(pt_pattern_history_init(&h,&d.project,commands,12,changes,4)==PT_EDIT_OK);
    assert(pt_wav_encode(&pcm,wav,sizeof(wav),&length)==PT_WAV_OK);
    assert(pt_sampler_import(&s,&d.project,&h,0,wav,length,"precision")==PT_EDIT_OK);
    assert(pt_sampler_slices(&s,&d.project,&h,0,markers,2)==PT_EDIT_OK);
    assert(pt_sampler_loop(&s,&d.project,&h,0,PT_LOOP_FORWARD,1,4,0)==PT_EDIT_OK);
    event.event.kind=PT_NOTE_PERIOD;event.event.pitch=428;event.event.instrument=1;event.event.slice=2;
    assert(pt_pattern_apply(&d.project,&h,&event,1)==PT_EDIT_OK);
    revision=h.revision;allocations=live;
    for(i=1;i<=2;++i) {
        fail=calls+i;assert(pt_sampler_convert(&s,&d.project,&h,0,16,88200)==PT_EDIT_CAPACITY);
        assert(live==allocations && h.revision==revision && !memcmp(d.project.samples[0].pcm.data,values,sizeof(values)));
    }
    fail=0;assert(pt_sampler_convert(&s,&d.project,&h,0,16,88200)==PT_EDIT_OK);
    assert(d.project.samples[0].pcm.bits==16 && d.project.samples[0].pcm.frames==8 && d.project.samples[0].pcm.rate==88200);
    assert(!memcmp(d.project.samples[0].pcm.data,expected,sizeof(expected)));
    assert(d.project.samples[0].slices[1]==4 && d.project.samples[0].loop_start==2 && d.project.samples[0].loop_end==8 && d.project.events[0].slice==2);
    pt_pattern_mark_saved(&h);assert(pt_pattern_undo(&d.project,&h,-1)==PT_EDIT_OK);
    assert(d.project.samples[0].pcm.bits==24 && d.project.samples[0].slices[1]==2 && d.project.samples[0].loop_start==1);
    assert(!memcmp(d.project.samples[0].pcm.data,values,sizeof(values)));
    revision=h.revision;allocations=live;
    assert(pt_sampler_convert(&s,&d.project,&h,0,24,44100)==PT_EDIT_OK && h.revision==revision && live==allocations);
    assert(pt_sampler_convert(&s,&d.project,&h,0,8,1)==PT_EDIT_UNSUPPORTED && h.revision==revision && live==allocations);
    assert(pt_sampler_convert(&s,&d.project,&h,0,12,44100)==PT_EDIT_INVALID);
    assert(pt_sampler_convert(&s,&d.project,&h,0,16,192001)==PT_EDIT_INVALID);
    assert(pt_pattern_undo(&d.project,&h,1)==PT_EDIT_OK && !pt_pattern_dirty(&h));
    assert(pt_project_size(&d.project,&n)==PT_PROJECT_OK);encoded=malloc(n);assert(encoded);
    assert(pt_project_encode(&d.project,encoded,n,&w)==PT_PROJECT_OK && pt_document_load(&reopened,encoded,n,SIZE_MAX)==PT_PROJECT_OK);free(encoded);
    assert(!memcmp(reopened.project.samples[0].pcm.data,expected,sizeof(expected)) && reopened.project.samples[0].slices[1]==4);
    assert(pt_sampler_convert(&s,&d.project,&h,0,8,88200)==PT_EDIT_OK && d.project.samples[0].pcm.data[0]==-128 && d.project.samples[0].pcm.data[1]==127);
    assert(pt_pattern_undo(&d.project,&h,-1)==PT_EDIT_OK && !pt_pattern_dirty(&h));
    {uint32_t cancel_at=0;
        revision=h.revision;allocations=live;s.progress=cancel_conversion;s.progress_context=&cancel_at;
        assert(pt_sampler_convert_quality(&s,&d.project,&h,0,16,44100,1)==PT_EDIT_CANCELLED);
        assert(h.revision==revision && live==allocations && !memcmp(d.project.samples[0].pcm.data,expected,sizeof(expected)));
        cancel_at=1;
        assert(pt_sampler_convert_quality(&s,&d.project,&h,0,16,44100,1)==PT_EDIT_CANCELLED);
        assert(h.revision==revision && live==allocations && !memcmp(d.project.samples[0].pcm.data,expected,sizeof(expected)));
        s.progress=NULL;s.progress_context=NULL;
    }
    assert(pt_sampler_convert_quality(&s,&d.project,&h,0,16,44100,1)==PT_EDIT_OK);
    assert(d.project.samples[0].pcm.frames==4 && d.project.samples[0].slices[1]==2 && d.project.samples[0].loop_start==1 && d.project.events[0].slice==2);
    assert(pt_pattern_undo(&d.project,&h,-1)==PT_EDIT_OK && !pt_pattern_dirty(&h));
    assert(!memcmp(d.project.samples[0].pcm.data,expected,sizeof(expected)));
    revision=h.revision;allocations=live;
    assert(pt_sampler_convert_quality(&s,&d.project,&h,0,16,1,1)==PT_EDIT_UNSUPPORTED);
    assert(pt_sampler_convert_quality(&s,&d.project,&h,0,16,44100,2)==PT_EDIT_INVALID);
    assert(h.revision==revision && live==allocations && pt_pattern_undo(&d.project,&h,1)==PT_EDIT_OK);
    pt_pattern_history_release(&h);pt_sampler_release(&s);assert(!s.bytes);pt_document_release(&d);pt_document_release(&reopened);assert(!live);
    puts("CONVERSION PASS: stereo precision/rate, scaled loops and referenced slice ordinals, exact undo, collapse refusal, allocation rollback, redo and persistence");
}
static void iff_samples(void)
{
    struct pt_allocator a={NULL,allocate,release};struct pt_document d,reopened;struct pt_sampler s;
    struct pt_pattern_history h;struct pt_pattern_command commands[8];struct pt_event_change changes[8];
    int32_t values[5]={-128,-7,0,12,127};struct pt_pcm pcm={values,5,5,8287,1,8};
    struct pt_svx_info info={0};uint8_t bytes[100],out[100],*project;size_t n,w,i,allocated;uint32_t revision;
    struct pt_sample *sample;struct pt_event_update event={0};uint32_t marker=0;
    info.loop_start=1;info.loop_end=4;info.volume=32768;strcpy(info.name,"IFF NAME");
    assert(pt_svx_encode(&pcm,&info,bytes,sizeof(bytes),&n)==PT_SVX_OK);
    pt_document_init(&d,&a);pt_document_init(&reopened,&a);assert(pt_document_new(&d,4,SIZE_MAX)==PT_PROJECT_OK);
    pt_sampler_init(&s,&a,1024*1024);assert(pt_pattern_history_init(&h,&d.project,commands,8,changes,8)==PT_EDIT_OK);
    for(i=1;i<=3;++i) {
        allocated=live;fail=calls+i;
        assert(pt_sampler_import(&s,&d.project,&h,0,bytes,n,"fallback")==PT_EDIT_CAPACITY);
        assert(live==allocated && !s.bytes && !h.revision && !d.project.samples[0].pcm.frames);
    }
    fail=0;assert(pt_sampler_import(&s,&d.project,&h,0,bytes,n,"fallback")==PT_EDIT_OK);sample=&d.project.samples[0];
    assert(!strcmp(sample->name,"IFF NAME") && sample->volume==32 && sample->loop==PT_LOOP_FORWARD && sample->loop_start==1 && sample->loop_end==4);
    assert(!memcmp(sample->pcm.data,values,sizeof(values)));
    revision=h.revision;allocated=live;assert(pt_sampler_svx_encode(sample,out,sizeof(out),&w)==PT_SVX_OK && w==n && !memcmp(bytes,out,n));
    assert(revision==h.revision && allocated==live);
    assert(pt_pattern_undo(&d.project,&h,-1)==PT_EDIT_OK && !sample->pcm.frames);
    bytes[35]=2;assert(pt_sampler_import(&s,&d.project,&h,0,bytes,n,"bad")==PT_EDIT_UNSUPPORTED && !sample->pcm.frames);
    bytes[35]=0;assert(pt_pattern_undo(&d.project,&h,1)==PT_EDIT_OK && h.revision==revision);
    assert(pt_sampler_loop(&s,&d.project,&h,0,PT_LOOP_PINGPONG,1,4,0)==PT_EDIT_OK);
    assert(pt_sampler_svx_encode(sample,out,sizeof(out),&w)==PT_SVX_UNSUPPORTED);
    assert(pt_pattern_undo(&d.project,&h,-1)==PT_EDIT_OK);
    assert(pt_sampler_slices(&s,&d.project,&h,0,&marker,1)==PT_EDIT_OK);
    assert(pt_sampler_svx_encode(sample,out,sizeof(out),&w)==PT_SVX_UNSUPPORTED);
    event.event.kind=PT_NOTE_PERIOD;event.event.pitch=428;event.event.instrument=1;event.event.slice=1;
    assert(pt_pattern_apply(&d.project,&h,&event,1)==PT_EDIT_OK);revision=h.revision;allocated=live;
    assert(pt_sampler_import(&s,&d.project,&h,0,bytes,n,"replace")==PT_EDIT_UNSUPPORTED && h.revision==revision && allocated==live);
    assert(pt_pattern_undo(&d.project,&h,-1)==PT_EDIT_OK);assert(pt_pattern_undo(&d.project,&h,-1)==PT_EDIT_OK);
    assert(pt_project_size(&d.project,&n)==PT_PROJECT_OK);project=malloc(n);assert(project);
    assert(pt_project_encode(&d.project,project,n,&w)==PT_PROJECT_OK && w==n);
    assert(pt_document_load(&reopened,project,n,SIZE_MAX)==PT_PROJECT_OK);free(project);
    assert(pt_sampler_svx_encode(&reopened.project.samples[0],out,sizeof(out),&w)==PT_SVX_OK && w==94 && !memcmp(bytes,out,w));
    assert(pt_pattern_undo(&d.project,&h,-1)==PT_EDIT_OK && !sample->pcm.frames);
    /* Compressed import: seed 120 then +21,+21,-34,0 with defined signed wrap. */
    bytes[4]=bytes[5]=bytes[6]=0;bytes[7]=84;bytes[20]=bytes[21]=bytes[22]=0;bytes[23]=4;memset(bytes+24,0,4);bytes[35]=1;bytes[87]=4;
    bytes[88]=0;bytes[89]=120;bytes[90]=255;bytes[91]=8;
    assert(pt_sampler_import(&s,&d.project,&h,0,bytes,92,"compressed")==PT_EDIT_OK);
    assert(sample->pcm.frames==4 && sample->pcm.data[0]==-115 && sample->pcm.data[1]==-94 && sample->pcm.data[2]==-128 && sample->pcm.data[3]==-128);
    pt_pattern_history_release(&h);pt_sampler_release(&s);pt_document_release(&d);pt_document_release(&reopened);assert(!live);
    puts("IFF SAMPLER PASS: name/volume/loop/PCM, compressed import, exact export, allocation rollback, unsupported refusal, redo and project persistence");
}

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
    loops_and_slices();conversion();iff_samples();
    puts("SAMPLER PASS: exact stereo24 import/range edit, unified history, dirty state, no-op redo, allocation rollback, bounded memory, eviction, save/reopen and release");return 0;
}
