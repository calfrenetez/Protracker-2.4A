#include "editor_paula.h"
int pt_editor_paula_stop(struct pt_editor_paula *o)
{return o && pt_paula_song_close(&o->song);}
static int barrier(void *context){return pt_editor_paula_stop(context);}
static int attached(const struct pt_editor_paula *o)
{return o && o->editor && o->editor->change_ready==barrier && o->editor->before_change_context==o;}
int pt_editor_paula_attach(struct pt_editor_paula *o,struct pt_editor *e)
{
    if(!o || o->editor || o->song || !pt_editor_change_barrier(e,barrier,o))return 0;
    o->editor=e;return 1;
}
int pt_editor_paula_detach(struct pt_editor_paula *o)
{
    if(!o || !pt_editor_paula_stop(o))return 0;
    if(attached(o) && !pt_editor_change_barrier(o->editor,NULL,NULL))return 0;
    o->editor=NULL;return 1;
}
enum pt_paula_song_result pt_editor_paula_begin(struct pt_editor_paula *o,struct pt_paula_voices *v,
    const struct pt_render_options *options,const struct pt_paula_render_caps *caps)
{
    if(!attached(o) || o->song || !v || !v->bridge ||
       v->bridge->sampler!=&o->editor->sampler || v->bridge->project!=o->editor->project)
        return PT_PAULA_SONG_INVALID;
    return pt_paula_song_begin(v,options,caps,&o->editor->sampler.allocator,&o->song);
}
enum pt_paula_song_result pt_editor_paula_prepare(struct pt_editor_paula *o,struct pt_paula_preflight_report *report)
{return attached(o)?pt_paula_song_prepare(o->song,report):PT_PAULA_SONG_INVALID;}
enum pt_paula_song_result pt_editor_paula_next(struct pt_editor_paula *o,struct pt_render_interval *out)
{return attached(o)?pt_paula_song_next(o->song,out):PT_PAULA_SONG_INVALID;}
enum pt_paula_song_result pt_editor_paula_consume(struct pt_editor_paula *o,uint32_t frames)
{return attached(o)?pt_paula_song_consume(o->song,frames):PT_PAULA_SONG_INVALID;}
enum pt_paula_song_result pt_editor_paula_stage(struct pt_editor_paula *o)
{return attached(o)?pt_paula_song_stage(o->song):PT_PAULA_SONG_INVALID;}
enum pt_paula_song_result pt_editor_paula_complete(struct pt_editor_paula *o)
{return attached(o)?pt_paula_song_complete(o->song):PT_PAULA_SONG_INVALID;}

enum pt_paula_song_result pt_editor_paula_prefetch(struct pt_editor_paula *o)
{return attached(o)?pt_paula_song_prefetch(o->song):PT_PAULA_SONG_INVALID;}

enum pt_paula_song_result pt_editor_paula_clock_arm(struct pt_editor_paula *o,uint64_t start)
{return attached(o)?pt_paula_song_clock_arm(o->song,start):PT_PAULA_SONG_INVALID;}
enum pt_paula_song_result pt_editor_paula_clock_service(struct pt_editor_paula *o,uint64_t now)
{return attached(o)?pt_paula_song_clock_service(o->song,now):PT_PAULA_SONG_INVALID;}

enum pt_paula_song_result pt_editor_paula_schedule_begin(struct pt_editor_paula *o,uint64_t start)
{return attached(o)?pt_paula_song_schedule_begin(o->song,start):PT_PAULA_SONG_INVALID;}
enum pt_paula_song_result pt_editor_paula_schedule_step(struct pt_editor_paula *o,uint64_t now,uint64_t *deadline)
{return attached(o)?pt_paula_song_schedule_step(o->song,now,deadline):PT_PAULA_SONG_INVALID;}

enum pt_paula_song_result pt_editor_paula_clocked_begin(struct pt_editor_paula *o,uint64_t delay,pt_paula_clock_read read,void *context)
{return attached(o)?pt_paula_song_clocked_begin(o->song,delay,read,context):PT_PAULA_SONG_INVALID;}
enum pt_paula_song_result pt_editor_paula_clocked_service(struct pt_editor_paula *o,uint64_t *deadline)
{return attached(o)?pt_paula_song_clocked_service(o->song,deadline):PT_PAULA_SONG_INVALID;}
enum pt_paula_song_result pt_editor_paula_clocked_deadline(struct pt_editor_paula *o,uint64_t *ticks)
{return attached(o)?pt_paula_song_clocked_deadline(o->song,ticks):PT_PAULA_SONG_INVALID;}
