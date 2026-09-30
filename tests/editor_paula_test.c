#define main ownership_fixture_main
#include "paula_voices_test.c"
#undef main
#include "../src/editor/editor_paula.h"
#include "../src/platform/sample_import.h"
static void editor_start(struct pt_editor_paula *o,struct pt_sampler_paula *cache,
    struct pt_paula_voices *voices,struct driver *d,unsigned prepare,unsigned play)
{
    struct pt_render_options options={0};struct pt_paula_render_caps caps={3546895,124,65535};
    struct pt_paula_voice_api api={d,start,stop,control};struct pt_paula_preflight_report report;
    struct pt_render_interval span;enum pt_paula_song_result result;unsigned n=0;
    d->start_result=d->control_result=d->quiesce_result=1;
    for(n=0;n<4;++n)d->stop_result[n]=1;
    assert(pt_sampler_paula_bind(cache,&o->editor->sampler,o->editor->project,d,chip_alloc,chip_free,32));
    assert(pt_paula_voices_bind(voices,cache,&api));assert(pt_paula_voices_bind_quiesce(voices,quiesce,d));
    options.rate=48000;options.bits=24;options.tracks=1U<<4;options.gain_q16=65536;
    options.tick_limit=100;options.frame_limit=100000;
    assert(pt_editor_paula_begin(o,voices,&options,&caps)==PT_PAULA_SONG_PREPARING);
    if(!prepare)return;
    n=0;do{result=pt_editor_paula_prepare(o,&report);assert(++n<100);}while(result==PT_PAULA_SONG_PREPARING);
    assert(result==PT_PAULA_SONG_OK && !d->reading[0]);
    if(!play)return;
    for(n=0;!d->reading[0];++n) {
        assert(n<20 && pt_editor_paula_next(o,&span)==PT_PAULA_SONG_OK);
        while(span.frames){uint32_t block=span.frames>256?256:span.frames;
            assert(pt_editor_paula_consume(o,block)==PT_PAULA_SONG_OK);span.frames-=block;}
        assert(pt_editor_paula_stage(o)==PT_PAULA_SONG_OK);
        assert(pt_editor_paula_complete(o)==PT_PAULA_SONG_OK);
    }
}
static int occupied(void *context) {(void)context;return 1;}
static void exact_save(struct pt_project *p,const uint8_t *saved,size_t bytes)
{
    size_t n,used;uint8_t *out;assert(pt_project_size(p,&n)==PT_PROJECT_OK && n==bytes);
    out=malloc(n);assert(out && pt_project_encode(p,out,n,&used)==PT_PROJECT_OK && used==n);
    assert(!memcmp(out,saved,n));free(out);
}
static void editor_fixture(unsigned bits)
{
    struct pt_allocator a={NULL,fast_alloc,fast_free};struct pt_document doc;struct pt_editor *e=malloc(sizeof(*e));
    struct pt_editor_paula owner={0},other={0};struct pt_sampler_paula cache={0};struct pt_paula_voices voices={0};
    struct driver d={0};int32_t pcm[16];unsigned i,revision,generation,stops;size_t retained,size,used;uint8_t *saved;
    struct pt_render_interval span={99,99,99};
    assert(e);for(i=0;i<16;++i)pcm[i]=(int32_t)i+1;
    pt_document_init(&doc,&a);assert(pt_document_new(&doc,16,SIZE_MAX)==PT_PROJECT_OK);
    for(i=0;i<16;++i)doc.project.channels.track[i].route=PT_AMIGUS;
    doc.project.channels.track[4].route=PT_PAULA;doc.project.channels.track[4].pan=0;
    doc.project.samples[0].pcm=doc.project.samples[1].pcm=(struct pt_pcm){pcm,16,16,8000,1,(uint8_t)bits};doc.project.samples[0].volume=64;
    doc.project.events[4]=(struct pt_event){428,0,PT_NOTE_PERIOD,1,0,0,0,0};
    doc.project.events[16*3+15].effect=15;doc.project.events[16*3+15].parameter=0;
    doc.project.channels.selected=4;assert(pt_editor_init(e,&doc.project));e->sampler.allocator=a;
    assert(pt_project_size(&doc.project,&size)==PT_PROJECT_OK);saved=malloc(size);assert(saved);
    assert(pt_project_encode(&doc.project,saved,size,&used)==PT_PROJECT_OK && used==size);
    assert(pt_editor_change_barrier(e,occupied,NULL));assert(!pt_editor_paula_attach(&owner,e));
    assert(pt_editor_change_barrier(e,NULL,NULL));assert(pt_editor_paula_attach(&owner,e));
    assert(!pt_editor_paula_attach(&owner,e) && !pt_editor_paula_attach(&other,e));
    assert(!pt_editor_change_barrier(e,occupied,NULL));
    assert(pt_editor_paula_next(&other,&span)==PT_PAULA_SONG_INVALID && span.frames==99);
    editor_start(&owner,&cache,&voices,&d,1,1);d.stop_result[0]=0;
    /* Navigation survives; pattern/sample mutations, imports, undo and disposal
     * are vetoed while a real external simulated reader remains active. */
    stops=d.stops;pt_editor_key(e,0x4d,0);assert(d.stops==stops && owner.song);
    e->project->channels.selected=4;e->row=0;e->editing=1;e->panel=0;
    revision=e->history.revision;generation=e->sampler.generation;retained=e->sampler.bytes;
    pt_editor_key(e,0x46,0);assert(doc.project.events[4].kind==PT_NOTE_PERIOD && e->history.revision==revision);
    e->panel=5;pt_editor_sample_all(e);pt_editor_key(e,0x13,0);
    assert(e->sampler.generation==generation && e->sampler.bytes==retained);
    assert(pt_editor_sample_file_import(e,"no-file.raw",1,0,NULL)==PT_EDIT_CONFLICT);
    pt_editor_key(e,0x31,8);assert(e->history.revision==revision);
    d.stop_result[0]=-1;assert(!pt_editor_prepare_change(e) && !pt_editor_dispose(e));
    assert(!pt_editor_paula_detach(&owner) && owner.song && owner.editor==e && e->change_ready);
    exact_save(&doc.project,saved,size);
    /* Confirmed stop is still insufficient while callbacks retain context. */
    d.stop_result[0]=1;d.quiesce_result=0;
    assert(!pt_editor_prepare_change(e) && !d.reading[0] && owner.song && e->sampler.bytes==retained);
    d.quiesce_result=2;assert(!pt_editor_dispose(e) && !pt_editor_paula_stop(&owner));
    d.quiesce_result=1;pt_editor_key(e,0x13,0);
    assert(!owner.song && !d.live && e->sampler.generation!=generation && e->history.revision!=revision);
    assert(doc.project.samples[0].pcm.data[0]==16);
    pt_editor_key(e,0x31,8);exact_save(&doc.project,saved,size);
    /* Undo/redo wait for ownership too; retry produces normal history behavior. */
    pt_editor_key(e,0x31,9);assert(doc.project.samples[0].pcm.data[0]==16);
    editor_start(&owner,&cache,&voices,&d,1,1);d.stop_result[0]=0;revision=e->history.revision;
    pt_editor_key(e,0x31,8);assert(e->history.revision==revision && owner.song);
    d.stop_result[0]=1;pt_editor_key(e,0x31,8);exact_save(&doc.project,saved,size);
    /* Preparation cancellation must quiesce even before any voice starts. */
    editor_start(&owner,&cache,&voices,&d,0,0);stops=d.starts;d.quiesce_result=0;
    assert(pt_editor_paula_prepare(&owner,NULL)==PT_PAULA_SONG_PREPARING);
    assert(!pt_editor_prepare_change(e) && owner.song && d.starts==stops);
    assert(!pt_editor_dispose(e));d.quiesce_result=1;
    assert(pt_editor_prepare_change(e) && !owner.song && !voices.bridge);
    /* Partial unpublished copy remains charged while adapter context is held. */
    doc.project.events[4].instrument=2;retained=e->sampler.bytes;
    editor_start(&owner,&cache,&voices,&d,0,0);
    assert(pt_editor_paula_prepare(&owner,NULL)==PT_PAULA_SONG_PREPARING);
    assert(pt_editor_paula_prepare(&owner,NULL)==PT_PAULA_SONG_PREPARING);
    assert(e->sampler.bytes>retained && !e->sampler.current[1]);retained=e->sampler.bytes;d.quiesce_result=0;
    assert(!pt_editor_paula_detach(&owner) && e->sampler.bytes==retained);
    d.quiesce_result=1;assert(pt_editor_paula_stop(&owner) && !owner.song && e->sampler.bytes<retained && !e->sampler.current[1]);
    doc.project.events[4].instrument=1;
    editor_start(&owner,&cache,&voices,&d,1,1);d.stop_result[0]=0;
    assert(!pt_editor_dispose(e) && owner.song);d.stop_result[0]=1;
    assert(pt_editor_dispose(e) && !owner.song && !e->sampler.bytes && !d.live);
    assert(pt_editor_paula_detach(&owner) && !e->change_ready && !owner.editor);
    free(saved);free(e);pt_document_release(&doc);
}
int editor_paula_fixture(void)
{editor_fixture(8);editor_fixture(16);editor_fixture(24);puts("EDITOR PAULA PASS: edits, imports, undo, replacement and disposal wait for reader stops and quiescence");return 0;}
#ifndef PT_TEST_EDITOR_PAULA_EXEC
int main(void) {return editor_paula_fixture();}
#endif
