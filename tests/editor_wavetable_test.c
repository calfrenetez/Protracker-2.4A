#define PT_WAVETABLE_DISPATCH_NATIVE
#include "wavetable_dispatch_test.c"
#include "../src/editor/editor_wavetable.h"
#include "../src/editor/editor_studio.h"
#include "../src/platform/sample_import.h"
#include "studio_preparation_cases.h"
static void editor_song_start(struct pt_editor_wavetable *o,struct fixture *f,struct dispatch_bus *bus,
    struct pt_sampler_wavetable *bridge,struct pt_wavetable_voices *voices)
{
    struct pt_render_options options={0};struct pt_playback_format format={16,0,0,0};
    struct pt_wavetable_preflight_report report;struct pt_render_interval span;
    options.rate=48000;options.bits=24;options.tracks=1;options.gain_q16=65536;
    options.tick_limit=100;options.frame_limit=100000;
    song_bind(f,bridge,voices,bus,&o->editor->sampler,o->editor->project);
    assert(pt_editor_wavetable_start(o,voices,&options,&format,&report)==PT_WAVETABLE_SONG_OK);
    do {
        assert(pt_editor_wavetable_next(o,&span)==PT_WAVETABLE_SONG_OK && !span.frames);
        assert(pt_editor_wavetable_complete(o)==PT_WAVETABLE_SONG_OK);
    }while(!bus->starts);
    bus->stop_result[0]=0;
}
static void editor_prepare_cancel_fixture(struct pt_editor *e,struct pt_editor_wavetable *owner,
    struct fixture *f,struct dispatch_bus *bus,struct pt_sampler_wavetable *bridge,struct pt_wavetable_voices *voices)
{
    struct pt_render_options o={0};struct pt_playback_format format={16,0,0,0};
    struct pt_wavetable_preflight_report report;unsigned mode;
    o.rate=48000;o.bits=24;o.tracks=1;o.gain_q16=65536;o.tick_limit=100;o.frame_limit=100000;
    for(mode=0;mode<3;++mode) {
        song_bind(f,bridge,voices,bus,&e->sampler,e->project);
        assert(pt_editor_wavetable_begin(owner,voices,&o,&format,&report)==PT_WAVETABLE_SONG_PREPARING);
        assert(pt_editor_wavetable_prepare(owner,&report)==PT_WAVETABLE_SONG_PREPARING);
        if(mode==0) {
            e->editing=1;e->row=0;e->project->channels.selected=0;
            pt_editor_key(e,0x46,0);assert(!owner->song && e->project->events[0].kind==PT_NOTE_NONE);
            pt_editor_key(e,0x31,8);assert(e->project->events[0].kind==PT_NOTE_PERIOD);
        }else if(mode==1)assert(pt_editor_wavetable_stop(owner) && !owner->song);
        else assert(pt_editor_dispose(e) && !owner->song);
        assert(!f->writes && !bus->starts && !bus->stops && !bus->restores && !voices->song_owner);
        assert(pt_wavetable_voices_close(voices) && pt_amigus_reservation_close(&f->reservation));
    }
    puts("EDITOR PREPARING PASS: edit, Stop and dispose cancel before source/output ownership");
}
static int editor_wavetable_fixture(void)
{
    struct pt_editor *e=malloc(sizeof(*e));struct fixture *f=malloc(sizeof(*f));struct dispatch_bus *bus=malloc(sizeof(*bus));
    struct pt_document d;struct pt_allocator a={NULL,allocate_master,release_master};
    struct pt_editor_wavetable owner={0},other={0};struct pt_editor_studio studio={0};
    struct pt_sampler_wavetable bridge={0};struct pt_wavetable_voices voices={0};
    int32_t data[8]={257,-513,1025,-2049,17,31,47,63};size_t size,used;uint8_t *saved;
    unsigned revision,generation;struct pt_render_interval span;
    studio_preparation_fixture(&a);
    assert(dispatch_fixture_main()==0);assert(e && f && bus);
    pt_document_init(&d,&a);assert(pt_document_new(&d,4,SIZE_MAX)==PT_PROJECT_OK);
    d.project.samples[0].pcm=(struct pt_pcm){data,8,8,48000,1,24};d.project.samples[0].volume=64;
    d.project.samples[0].loop=PT_LOOP_FORWARD;d.project.samples[0].loop_end=8;
    d.project.events[0]=(struct pt_event){428,0,PT_NOTE_PERIOD,1,0,0,0,0};d.project.events[8].effect=15;
    assert(pt_editor_init(e,&d.project));e->sampler.allocator=a;
    assert(pt_project_size(&d.project,&size)==PT_PROJECT_OK);saved=malloc(size);assert(saved);
    assert(pt_project_encode(&d.project,saved,size,&used)==PT_PROJECT_OK && used==size);
    assert(pt_editor_studio_attach(&studio,e));assert(!pt_editor_wavetable_attach(&owner,e));pt_editor_studio_detach(&studio);
    assert(pt_editor_wavetable_attach(&owner,e));assert(!pt_editor_studio_attach(&studio,e) && !pt_editor_wavetable_attach(&other,e));
    editor_song_start(&owner,f,bus,&bridge,&voices);
    /* Navigation, including selected-channel changes, leaves playback valid. */
    pt_editor_key(e,0x4d,0);pt_editor_key(e,0x42,0);assert(!bus->stops);
    assert(pt_editor_wavetable_next(&owner,&span)==PT_WAVETABLE_SONG_OK);
    e->project->channels.selected=0;e->row=0;e->editing=1;
    revision=e->history.revision;generation=e->sampler.generation;
    pt_editor_key(e,0x46,0); /* Delete note. */
    assert(e->history.revision==revision && d.project.events[0].kind==PT_NOTE_PERIOD && owner.song && pins(f)==1);
    /* Pending/failed stop blocks sample processing, external imports, undo,
     * storage replacement preparation, disposal and detach alike. */
    e->panel=5;pt_editor_sample_all(e);pt_editor_key(e,0x13,0);
    assert(e->sampler.generation==generation && e->history.revision==revision);
    assert(pt_editor_sample_file_import(e,"no-file.raw",1,0,NULL)==PT_EDIT_CONFLICT);
    pt_editor_key(e,0x31,8);assert(e->history.revision==revision);
    bus->stop_result[0]=-1;assert(!pt_editor_prepare_change(e) && !pt_editor_dispose(e));
    assert(!pt_editor_wavetable_detach(&owner) && e->change_ready && owner.editor==e && owner.song);
    assert(!pt_amigus_reservation_close(&f->reservation));exact_save(&d.project,saved,size);
    /* Confirmation permits the requested edit, then normal undo/redo. */
    bus->stop_result[0]=1;pt_editor_key(e,0x13,0);
    assert(!owner.song && !pins(f) && e->sampler.generation!=generation && e->history.revision!=revision);
    assert(d.project.samples[0].pcm.data[0]==63 && pt_amigus_reservation_close(&f->reservation));
    pt_editor_key(e,0x31,8);exact_save(&d.project,saved,size);
    /* Pending undo and redo preserve journal state and master data. */
    pt_editor_key(e,0x31,9);assert(d.project.samples[0].pcm.data[0]==63);
    editor_song_start(&owner,f,bus,&bridge,&voices);revision=e->history.revision;
    pt_editor_key(e,0x31,8);assert(e->history.revision==revision && d.project.samples[0].pcm.data[0]==63);
    bus->stop_result[0]=1;pt_editor_key(e,0x31,8);exact_save(&d.project,saved,size);
    assert(pt_amigus_reservation_close(&f->reservation));
    editor_song_start(&owner,f,bus,&bridge,&voices);revision=e->history.revision;
    pt_editor_key(e,0x31,9);assert(e->history.revision==revision && d.project.samples[0].pcm.data[0]==257);
    bus->stop_result[0]=1;pt_editor_key(e,0x31,9);assert(d.project.samples[0].pcm.data[0]==63);
    assert(pt_amigus_reservation_close(&f->reservation));pt_editor_key(e,0x31,8);exact_save(&d.project,saved,size);
    /* Replacement/disposal contract: no document ownership can be freed until
     * prepare/dispose confirms stop. Successful dispose precedes detach/free. */
    editor_song_start(&owner,f,bus,&bridge,&voices);
    assert(!pt_editor_dispose(e) && e->sampler.bytes && owner.song);exact_save(&d.project,saved,size);
    bus->stop_result[0]=1;assert(pt_editor_prepare_change(e) && !owner.song);
    assert(pt_editor_dispose(e) && !e->sampler.bytes && pt_amigus_reservation_close(&f->reservation));
    assert(pt_editor_wavetable_detach(&owner) && !e->change_ready);
    /* Start from the original borrowed source after disposed sampler storage. */
    d.project.samples[0].pcm=(struct pt_pcm){data,8,8,48000,1,24};
    assert(pt_editor_init(e,&d.project));e->sampler.allocator=a;
    assert(pt_editor_wavetable_attach(&owner,e));
    editor_prepare_cancel_fixture(e,&owner,f,bus,&bridge,&voices);
    assert(pt_editor_wavetable_detach(&owner));
    free(saved);free(e);pt_document_release(&d);free(bus);free(f);assert(!allocations);
    puts("EDITOR WAVETABLE PASS: pending/failed stop vetoes edits, imports, undo/redo, replacement and disposal; navigation retained; injected only");return 0;
}
#ifndef PT_EDITOR_WAVETABLE_NATIVE
int main(void){return editor_wavetable_fixture();}
#endif
