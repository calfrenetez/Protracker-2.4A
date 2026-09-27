#include "editor_wavetable.h"
int pt_editor_wavetable_stop(struct pt_editor_wavetable *o)
{return o && pt_wavetable_song_close(&o->song);}
static int barrier(void *context){return pt_editor_wavetable_stop(context);}
static int attached(const struct pt_editor_wavetable *o)
{return o && o->editor && o->editor->change_ready==barrier && o->editor->before_change_context==o;}
int pt_editor_wavetable_attach(struct pt_editor_wavetable *o,struct pt_editor *e)
{
    if(!o || o->editor || o->song || !pt_editor_change_barrier(e,barrier,o))return 0;
    o->editor=e;return 1;
}
int pt_editor_wavetable_detach(struct pt_editor_wavetable *o)
{
    if(!o || !pt_editor_wavetable_stop(o))return 0;
    if(attached(o))pt_editor_change_barrier(o->editor,NULL,NULL);
    o->editor=NULL;return 1;
}
enum pt_wavetable_song_result pt_editor_wavetable_start(struct pt_editor_wavetable *o,struct pt_wavetable_voices *v,
    const struct pt_render_options *options,const struct pt_playback_format *format,struct pt_wavetable_preflight_report *report)
{
    if(!attached(o) || o->song || !v || !v->bridge || v->bridge->sampler!=&o->editor->sampler || v->bridge->project!=o->editor->project)
        return PT_WAVETABLE_SONG_INVALID;
    return pt_wavetable_song_open(v,options,format,&o->editor->sampler.allocator,report,&o->song);
}
enum pt_wavetable_song_result pt_editor_wavetable_begin(struct pt_editor_wavetable *o,struct pt_wavetable_voices *v,
    const struct pt_render_options *options,const struct pt_playback_format *format,struct pt_wavetable_preflight_report *report)
{
    if(!attached(o) || o->song || !v || !v->bridge || v->bridge->sampler!=&o->editor->sampler || v->bridge->project!=o->editor->project)
        return PT_WAVETABLE_SONG_INVALID;
    return pt_wavetable_song_begin(v,options,format,&o->editor->sampler.allocator,report,&o->song);
}
enum pt_wavetable_song_result pt_editor_wavetable_prepare(struct pt_editor_wavetable *o,struct pt_wavetable_preflight_report *report)
{return attached(o)?pt_wavetable_song_prepare(o->song,report):PT_WAVETABLE_SONG_INVALID;}
enum pt_wavetable_song_result pt_editor_wavetable_next(struct pt_editor_wavetable *o,struct pt_render_interval *out)
{return attached(o)?pt_wavetable_song_next(o->song,out):PT_WAVETABLE_SONG_INVALID;}
enum pt_wavetable_song_result pt_editor_wavetable_consume(struct pt_editor_wavetable *o,uint32_t frames)
{return attached(o)?pt_wavetable_song_consume(o->song,frames):PT_WAVETABLE_SONG_INVALID;}
enum pt_wavetable_song_result pt_editor_wavetable_complete(struct pt_editor_wavetable *o)
{return attached(o)?pt_wavetable_song_complete(o->song):PT_WAVETABLE_SONG_INVALID;}

enum pt_wavetable_song_result pt_editor_wavetable_next_step(struct pt_editor_wavetable *o,struct pt_render_interval *out)
{return attached(o)?pt_wavetable_song_next_step(o->song,out):PT_WAVETABLE_SONG_INVALID;}
enum pt_wavetable_song_result pt_editor_wavetable_complete_step(struct pt_editor_wavetable *o)
{return attached(o)?pt_wavetable_song_complete_step(o->song):PT_WAVETABLE_SONG_INVALID;}

enum pt_wavetable_song_result pt_editor_wavetable_prefetch(struct pt_editor_wavetable *o)
{return attached(o)?pt_wavetable_song_prefetch(o->song):PT_WAVETABLE_SONG_INVALID;}

enum pt_wavetable_song_result pt_editor_wavetable_clock_arm(struct pt_editor_wavetable *o,uint64_t start)
{return attached(o)?pt_wavetable_song_clock_arm(o->song,start):PT_WAVETABLE_SONG_INVALID;}
enum pt_wavetable_song_result pt_editor_wavetable_clock_service(struct pt_editor_wavetable *o,uint64_t now)
{return attached(o)?pt_wavetable_song_clock_service(o->song,now):PT_WAVETABLE_SONG_INVALID;}

enum pt_wavetable_song_result pt_editor_wavetable_next_prepare(struct pt_editor_wavetable *o,struct pt_render_interval *out)
{return attached(o)?pt_wavetable_song_next_prepare(o->song,out):PT_WAVETABLE_SONG_INVALID;}
enum pt_wavetable_song_result pt_editor_wavetable_next_commit(struct pt_editor_wavetable *o)
{return attached(o)?pt_wavetable_song_next_commit(o->song):PT_WAVETABLE_SONG_INVALID;}

enum pt_wavetable_song_result pt_editor_wavetable_schedule_begin(struct pt_editor_wavetable *o,uint64_t start)
{return attached(o)?pt_wavetable_song_schedule_begin(o->song,start):PT_WAVETABLE_SONG_INVALID;}
enum pt_wavetable_song_result pt_editor_wavetable_schedule_step(struct pt_editor_wavetable *o,uint64_t now,uint64_t *deadline)
{return attached(o)?pt_wavetable_song_schedule_step(o->song,now,deadline):PT_WAVETABLE_SONG_INVALID;}

enum pt_wavetable_song_result pt_editor_wavetable_clocked_begin(struct pt_editor_wavetable *o,uint64_t delay,pt_wavetable_clock_read read,void *context)
{return attached(o)?pt_wavetable_song_clocked_begin(o->song,delay,read,context):PT_WAVETABLE_SONG_INVALID;}
enum pt_wavetable_song_result pt_editor_wavetable_clocked_service(struct pt_editor_wavetable *o,uint64_t *deadline)
{return attached(o)?pt_wavetable_song_clocked_service(o->song,deadline):PT_WAVETABLE_SONG_INVALID;}

enum pt_wavetable_song_result pt_editor_wavetable_clocked_deadline(struct pt_editor_wavetable *o,uint64_t *ticks)
{return attached(o)?pt_wavetable_song_clocked_deadline(o->song,ticks):PT_WAVETABLE_SONG_INVALID;}
