/* Native enhanced editor and owned classic Paula replay integration. */
#include <exec/memory.h>
#include <devices/timer.h>
#include <graphics/gfxbase.h>
#include <graphics/modeid.h>
#include <intuition/intuitionbase.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/dos.h>
#include <dos/var.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "document.h"
#include "master_memory.h"
#include "wav.h"
#include "mod_project.h"
#include "../editor/view.h"
#include "../platform/file_save.h"
#include "../platform/sample_file.h"
#include "../platform/project_file.h"
#include "../platform/mod_file.h"
#include "../platform/raw_import.h"
#include "../platform/mod_import.h"
#include "../platform/project_import.h"
#include "../platform/pp20_import.h"
#include "../platform/file_load.h"
#include "pt_font.h"
#include "paula.h"
#include "present.h"
#include "file_request.h"
#include "../platform/render_file.h"
#include "../platform/stem_file.h"
#include "../editor/bounce.h"
#include "../platform/recent_file.h"
struct IntuitionBase *IntuitionBase;
struct GfxBase *GfxBase;
static struct pt_master_memory master_memory;
static const struct pt_allocator render_allocator={&master_memory,pt_master_allocate,pt_master_release};
static struct pt_recent recent_projects;
static char recent_override[PT_RECENT_PATH];
static const char *recent_prefix="ENVARC:ProTracker2.4G/recent";
static void recent_persist(struct pt_editor *e)
{
    int ok=pt_recent_file_save_allocated(recent_prefix,&recent_projects,&render_allocator);
    if(!ok)pt_editor_status(e,"RECENTS: SESSION ONLY");
    ++e->sample_ui;
    printf("EDITOR RECENT saved=%u count=%u\n",ok,recent_projects.count);fflush(stdout);
}
static void recent_success(struct pt_editor *e,const char *path)
{
    char resolved[PT_RECENT_PATH];BPTR lock=Lock((STRPTR)path,ACCESS_READ);int ok=0;
    if(lock) {ok=NameFromLock(lock,(STRPTR)resolved,sizeof(resolved));UnLock(lock);}
    if(ok && pt_recent_remember(&recent_projects,resolved)==PT_RECENT_OK)recent_persist(e);
    else {pt_editor_status(e,"RECENT PATH UNAVAILABLE");puts("EDITOR RECENT path unavailable");fflush(stdout);}
}
static int editor_init_memory(struct pt_editor *e,struct pt_project *p)
{
    struct pt_allocator a={&master_memory,pt_master_allocate,pt_master_release};
    size_t available;
    memset(e,0,sizeof(*e));
    if(!pt_editor_init(e,p))return 0;
    available=pt_master_memory_available(&master_memory);
    pt_sampler_init(&e->sampler,&a,available/2);
    pt_document_init(&e->sample_source,&a);
    pt_song_init(&e->song,&a,available/4);
    return 1;
}
static int load(struct pt_document *d,const char *path)
{
    size_t n;uint8_t *bytes;int ok;
    if(pt_pp20_file_candidate(path))return pt_pp20_file_load(d,path,64UL*1024*1024,SIZE_MAX)==PT_PROJECT_OK;
    if(pt_project_file_candidate(path))return pt_project_file_load(d,path,64UL*1024*1024,SIZE_MAX)==PT_PROJECT_OK;
    if(pt_mod_file_candidate(path))return pt_mod_file_load(d,path,64UL*1024*1024,SIZE_MAX)==PT_PROJECT_OK;
    if(pt_file_load(path,64UL*1024*1024,&render_allocator,&bytes,&n)!=PT_LOAD_OK)return 0;
    ok=pt_document_load(d,bytes,n,SIZE_MAX)==PT_PROJECT_OK;
    pt_master_release(&master_memory,bytes);return ok;
}
static enum pt_project_result load_mod_source(void *context,struct pt_document *d,size_t budget)
{return pt_pp20_file_candidate((const char *)context)?pt_pp20_file_load(d,(const char *)context,64UL*1024*1024,budget):pt_mod_file_load(d,(const char *)context,64UL*1024*1024,budget);}
static enum pt_edit_result load_sample(struct pt_editor *e,const char *path,int raw,int source_only,int *preview)
{
    size_t n;uint8_t *bytes=NULL;enum pt_edit_result result=PT_EDIT_INVALID;
    enum pt_load_result loaded;
    const char *name=path,*part;
    if(preview)*preview=0;
    if(!e->sample)return PT_EDIT_INVALID;
    if(raw) {
        for(part=path;*part;++part)if(*part=='/' || *part==':')name=part+1;
        pt_editor_prepare_change(e);
        return pt_raw_file_import(path,64UL*1024*1024,&e->sampler,e->project,&e->history,e->sample-1,name,&e->raw_format);
    }
    if(!source_only && pt_wav_file_candidate(path)) {
        for(part=path;*part;++part)if(*part=='/' || *part==':')name=part+1;
        pt_editor_prepare_change(e);
        return pt_wav_file_import(path,64UL*1024*1024,&e->sampler,e->project,&e->history,e->sample-1,name);
    }
    if(!source_only && pt_svx_file_candidate(path)) {
        for(part=path;*part;++part)if(*part=='/' || *part==':')name=part+1;
        pt_editor_prepare_change(e);
        return pt_svx_file_import(path,64UL*1024*1024,&e->sampler,e->project,&e->history,e->sample-1,name);
    }
    if(pt_mod_file_candidate(path) || pt_pp20_file_candidate(path)) {
        if(preview)*preview=1;
        return pt_editor_source_load_with(e,load_mod_source,(void *)path);
    }
    loaded=pt_file_load(path,64UL*1024*1024,&render_allocator,&bytes,&n);
    if(loaded!=PT_LOAD_OK)return loaded==PT_LOAD_MEMORY?PT_EDIT_CAPACITY:PT_EDIT_INVALID;
    if(!n)goto done;
    for(part=path;*part;++part)if(*part=='/' || *part==':')name=part+1;
    if(!raw) {
        struct pt_project_requirements need;
        if(source_only || ((size_t)n>=4 && !memcmp(bytes,"PP20",4)) || pt_mod_project_probe(bytes,(size_t)n,&need)==PT_PROJECT_OK) {
            if(preview)*preview=1;
            result=pt_editor_source_load(e,bytes,(size_t)n);goto done;
        }
    }
    pt_editor_prepare_change(e);
    result=raw?pt_sampler_import_raw(&e->sampler,e->project,&e->history,e->sample-1,bytes,(size_t)n,name,&e->raw_format):pt_sampler_import(&e->sampler,e->project,&e->history,e->sample-1,bytes,(size_t)n,name);
done:
    pt_master_release(&master_memory,bytes);return result;
}
static void save_sample(struct pt_editor *e,const char *path)
{
    size_t n;enum pt_save_result result;
    const struct pt_pcm *pcm=e->sample && e->sample<=e->project->sample_count?&e->project->samples[e->sample-1].pcm:NULL;
    if(!pcm || !pcm->frames || pt_wav_size(pcm,&n)!=PT_WAV_OK) {pt_editor_status(e,"WAV EXPORT: SELECT A NONEMPTY SAMPLE");return;}
    result=pt_sample_wav_save(path,pcm,&render_allocator);
    pt_editor_status(e,result==PT_SAVE_MEMORY?"SAVE: OUT OF MEMORY":result==PT_SAVE_OK?"WAV EXPORTED AND VERIFIED - PROJECT STATE UNCHANGED":"WAV EXPORT REFUSED OR FAILED - DESTINATION PRESERVED");
    printf("EDITOR WAV result=%u dirty=%u\n",result,pt_editor_dirty(e));fflush(stdout);
}
static int raw_eligible(struct pt_editor *e)
{
    size_t n;
    if(!e->sample || e->sample>e->project->sample_count || !e->project->samples[e->sample-1].pcm.frames ||
        pt_raw_size(&e->project->samples[e->sample-1].pcm,&e->raw_format,&n)!=PT_RAW_OK) {
        pt_editor_status(e,"RAW EXPORT NEEDS MATCHING BITS/CHANNELS/RATE; USE FORMAT");return 0;
    }
    return 1;
}
static void save_raw(struct pt_editor *e,const char *path)
{
    size_t n;enum pt_save_result result;
    if(!raw_eligible(e) || pt_raw_size(&e->project->samples[e->sample-1].pcm,&e->raw_format,&n)!=PT_RAW_OK)return;
    result=pt_sample_raw_save(path,&e->project->samples[e->sample-1].pcm,&e->raw_format,&render_allocator);
    pt_editor_status(e,result==PT_SAVE_MEMORY?"SAVE: OUT OF MEMORY":result==PT_SAVE_OK?"RAW PCM EXPORTED AND VERIFIED - PROJECT STATE UNCHANGED":"RAW EXPORT REFUSED OR FAILED - DESTINATION PRESERVED");
    printf("EDITOR RAW result=%u dirty=%u\n",result,pt_editor_dirty(e));fflush(stdout);
}
static int svx_eligible(struct pt_editor *e)
{
    size_t size;
    if(!e->sample || e->sample>e->project->sample_count || pt_sampler_svx_size(&e->project->samples[e->sample-1],&size)!=PT_SVX_OK) {
        pt_editor_status(e,"IFF NEEDS MONO8 <=65535HZ; NO SLICES/FINE/PINGPONG");return 0;
    }
    return 1;
}
static void save_svx(struct pt_editor *e,const char *path)
{
    size_t n;struct pt_svx_info info={0};enum pt_save_result result;const struct pt_sample *sample=&e->project->samples[e->sample-1];
    if(!svx_eligible(e) || pt_sampler_svx_size(sample,&n)!=PT_SVX_OK)return;
    memcpy(info.name,sample->name,sizeof(info.name));info.loop_start=sample->loop_start;
    info.loop_end=sample->loop_end;info.volume=(uint32_t)sample->volume*1024;
    result=pt_sample_svx_save(path,&sample->pcm,&info,&render_allocator);
    pt_editor_status(e,result==PT_SAVE_MEMORY?"SAVE: OUT OF MEMORY":result==PT_SAVE_OK?"IFF EXPORTED AND VERIFIED - PROJECT STATE UNCHANGED":"IFF EXPORT REFUSED OR FAILED - DESTINATION PRESERVED");
    printf("EDITOR IFF result=%u dirty=%u\n",result,pt_editor_dirty(e));fflush(stdout);
}
static void save(struct pt_editor *e,const char *path)
{
    enum pt_save_result result;
    if(!path) {pt_editor_status(e,"START WITH INPUT AND NEW_OUTPUT PATH TO ENABLE SAVE");return;}
    result=pt_project_file_save(path,e->project,&render_allocator);
    if(result==PT_SAVE_OK) {pt_editor_saved(e);recent_success(e,path);}
    else pt_editor_status(e,result==PT_SAVE_MEMORY?"SAVE: OUT OF MEMORY":result==PT_SAVE_PUBLISH?"SAVE REFUSED: DESTINATION EXISTS OR CANNOT BE PUBLISHED":"SAVE FAILED: CURRENT EDITS AND DESTINATION PRESERVED");
    printf("EDITOR SAVE result=%u dirty=%u\n",result,pt_editor_dirty(e));fflush(stdout);
}
static int mod_eligible(struct pt_editor *e,struct pt_mod_export_report *r,unsigned round8)
{
    const char *reason;uint32_t issues;
    if((round8?pt_mod_export_analyse_round8(e->project,r):pt_mod_export_analyse(e->project,r))!=PT_PROJECT_OK) {pt_editor_status(e,"MOD EXPORT REFUSED: INVALID PROJECT");return 0;}
    issues=r->issues&~(round8?PT_EXPORT_PRECISION:0U);
    if(!issues)return 1;
    if(issues&PT_EXPORT_MIDI_AUDIO)reason="MOD EXPORT REFUSED: EXTERNAL MIDI AUDIO IS NOT IN A MOD";
    else if(issues&PT_EXPORT_CHANNELS)reason="MOD EXPORT REFUSED: MORE THAN FOUR CHANNELS; SAVE PROJECT";
    else if(issues&PT_EXPORT_ROUTING)reason="MOD EXPORT REFUSED: NEEDS CLASSIC PAULA ROUTING";
    else if(issues&(PT_EXPORT_PRECISION|PT_EXPORT_STEREO|PT_EXPORT_RATE))reason="MOD EXPORT REFUSED: SAMPLE FORMAT NEEDS CONVERSION";
    else if(issues&(PT_EXPORT_LOOPS|PT_EXPORT_SLICES))reason="MOD EXPORT REFUSED: UNSUPPORTED SAMPLE LOOPS OR SLICES";
    else if(issues&(PT_EXPORT_OFF|PT_EXPORT_NOTES|PT_EXPORT_VELOCITY))reason="MOD EXPORT REFUSED: ENHANCED NOTES; SAVE PROJECT TO KEEP THEM";
    else if(issues&PT_EXPORT_TEMPO)reason="MOD EXPORT REFUSED: INITIAL TEMPO NEEDS CONVERSION";
    else if(issues&PT_EXPORT_PANNING)reason="MOD EXPORT REFUSED: ENHANCED PANNING; SAVE PROJECT";
    else reason="MOD EXPORT REFUSED: ENHANCED DATA OR CLASSIC FORMAT LIMITS";
    pt_editor_status(e,reason);printf("EDITOR MOD refused issues=0x%lx dirty=%u\n",(unsigned long)r->issues,pt_editor_dirty(e));fflush(stdout);return 0;
}
static void save_mod(struct pt_editor *e,const char *path,size_t n,unsigned round8,unsigned converted)
{
    enum pt_save_result result;
    (void)n; /* Eligibility/UI size was computed before the explicit export action. */
    result=pt_mod_file_save(path,e->project,round8,&render_allocator);
    if(result==PT_SAVE_OK) {pt_editor_status(e,converted?(pt_editor_dirty(e)?"MOD CONVERTED - UNSAVED":"MOD CONVERTED - SOURCE KEPT"):(pt_editor_dirty(e)?"MOD EXPORTED AND VERIFIED - PROJECT STILL UNSAVED":"MOD EXPORTED AND VERIFIED"));recent_success(e,path);}
    else pt_editor_status(e,result==PT_SAVE_MEMORY?"SAVE: OUT OF MEMORY":result==PT_SAVE_PUBLISH?"MOD EXPORT REFUSED: DESTINATION EXISTS OR CANNOT BE PUBLISHED":"MOD EXPORT FAILED: PROJECT AND DESTINATION PRESERVED");
    /* Export does not mark the richer project saved or consume undo history. */
    printf("EDITOR MOD result=%u dirty=%u dither=%s\n",result,pt_editor_dirty(e),round8==2?"tpdf-fixed":"none");fflush(stdout);
}
struct conversion_ui {
    struct pt_editor *editor;struct Window *window;struct pt_canvas *canvas;
    struct BitMap *bitmap;struct pt_view_cache *cache;
    unsigned percent;
};
static int conversion_progress(void *context,uint32_t done,uint32_t total)
{
    struct conversion_ui *ui=context;struct IntuiMessage *message;unsigned percent=total?(unsigned)((uint64_t)done*100/total):100;
    int cancelled=0;
    /* Conversion is modal. Drain/reply events so the window stays responsive,
       accepting Escape only; never mutate the project while staging PCM. */
    while((message=(struct IntuiMessage *)GetMsg(ui->window->UserPort))) {
        ULONG kind=message->Class;UWORD code=message->Code;
        ReplyMsg((struct Message *)message);
        if(kind==IDCMP_RAWKEY && code==0x45)cancelled=1;
        if(kind==IDCMP_REFRESHWINDOW) {BeginRefresh(ui->window);EndRefresh(ui->window,TRUE);ui->cache->valid=0;}
    }
    if(cancelled) {printf("EDITOR CONVERSION cancelled=%lu/%lu\n",(unsigned long)done,(unsigned long)total);fflush(stdout);return 0;}
    if(!done || percent/5!=ui->percent/5 || !ui->cache->valid) {
        struct pt_view_rect areas[PT_VIEW_DIRTY_MAX];unsigned i,n,plane;char status[76];
        snprintf(status,sizeof(status),"FILTERING %u%% - ESC CANCEL",percent);
        pt_editor_status(ui->editor,status);n=pt_editor_draw_update(ui->editor,ui->canvas,pt_font,ui->cache,areas);
        for(i=0;i<n;++i) {
            struct pt_view_rect *area=&areas[i];
            for(plane=0;plane<4;++plane)CopyMem(ui->canvas->planes[plane]+area->y*80,ui->bitmap->Planes[plane]+area->y*80,area->height*80);
            BltBitMapRastPort(ui->bitmap,area->x,area->y,ui->window->RPort,area->x,area->y,area->width,area->height,0xc0);WaitBlit();
        }
        ui->percent=percent;printf("EDITOR CONVERSION progress=%u\n",percent);fflush(stdout);
    }
    return 1;
}
struct render_ui {struct conversion_ui display;uint64_t total;unsigned phase;const char *target;};
static int render_progress(void *context,enum pt_render_phase phase,uint32_t ticks,uint64_t frames)
{
    struct render_ui *ui=context;struct conversion_ui *d=&ui->display;struct IntuiMessage *message;
    unsigned percent=ui->total?(unsigned)(frames*100/ui->total):0;int cancelled=0;
    while((message=(struct IntuiMessage *)GetMsg(d->window->UserPort))) {
        ULONG kind=message->Class;UWORD code=message->Code;ReplyMsg((struct Message *)message);
        if(kind==IDCMP_RAWKEY && code==0x45)cancelled=1;
        if(kind==IDCMP_REFRESHWINDOW) {BeginRefresh(d->window);EndRefresh(d->window,TRUE);d->cache->valid=0;}
    }
    if(cancelled) {puts("EDITOR RENDER cancelled");fflush(stdout);return 0;}
    if(ui->phase!=(unsigned)phase || (phase!=PT_RENDER_ANALYSE && percent/5!=d->percent/5) || !d->cache->valid) {
        struct pt_view_rect areas[PT_VIEW_DIRTY_MAX];unsigned i,n,plane;char status[76];
        const char *name=phase==PT_RENDER_ANALYSE?"CHECKING":phase==PT_RENDER_MIX?"RENDERING":"VERIFYING";
        if(phase==PT_RENDER_ANALYSE)snprintf(status,sizeof(status),"%s %s - ESC CANCEL",name,ui->target);
        else snprintf(status,sizeof(status),"%s %s %u%% - ESC CANCEL",name,ui->target,percent);
        pt_editor_status(d->editor,status);n=pt_editor_draw_update(d->editor,d->canvas,pt_font,d->cache,areas);
        for(i=0;i<n;++i) {
            struct pt_view_rect *a=&areas[i];
            for(plane=0;plane<4;++plane)CopyMem(d->canvas->planes[plane]+a->y*80,d->bitmap->Planes[plane]+a->y*80,a->height*80);
            BltBitMapRastPort(d->bitmap,a->x,a->y,d->window->RPort,a->x,a->y,a->width,a->height,0xc0);WaitBlit();
        }
        ui->phase=(unsigned)phase;d->percent=percent;
        printf("EDITOR RENDER progress=%s percent=%u ticks=%lu\n",name,percent,(unsigned long)ticks);fflush(stdout);
    }
    return 1;
}
static const char *render_error(enum pt_render_result result)
{
    switch(result) {
    case PT_RENDER_MEMORY:return "RENDER: OUT OF MEMORY";
    case PT_RENDER_EMPTY_RANGE:return "NO SELECTED ROWS REACHED - NOTHING RENDERED";
    case PT_RENDER_ROUTE:return "WAV REFUSED: SELECTED MIDI TRACK NEEDS SUPPLIED AUDIO";
    case PT_RENDER_EFFECT:return "WAV REFUSED: NOTE OR EFFECT NOT SUPPORTED BY REFERENCE RENDERER";
    case PT_RENDER_SAMPLE:return "WAV REFUSED: SAMPLE FORMAT OR LOOP NOT SUPPORTED";
    case PT_RENDER_TICK_LIMIT:case PT_RENDER_FRAME_LIMIT:return "WAV LIMIT REACHED - NO FILE PUBLISHED";
    case PT_RENDER_CANCELLED:return "WAV CANCELLED - EDITS PRESERVED";
    default:return "WAV REFUSED - CHECK SETTINGS; PROJECT PRESERVED";
    }
}
static void render_wav(struct conversion_ui *display,struct pt_paula *audio)
{
    struct pt_editor *e=display->editor;struct pt_render_options options;struct pt_render_report plan,report;
    struct render_ui ui={*display,0,~0U,"WAV"};enum pt_render_result detail;enum pt_render_file_result result;
    char path[1024],status[76];int selected;
    pt_paula_stop(audio);pt_paula_poll(audio,&e->playback);pt_editor_render_options(e,&options);
    detail=pt_render_measure_allocated(e->project,&options,render_progress,&ui,&plan,&render_allocator);
    if(detail!=PT_RENDER_OK) {pt_editor_status(e,render_error(detail));printf("EDITOR RENDER preflight=%u dirty=%u\n",detail,pt_editor_dirty(e));fflush(stdout);return;}
    ui.total=plan.frames;puts("EDITOR REQUEST render");fflush(stdout);
    selected=pt_file_request(display->window,9,"new-render.wav",path,sizeof(path));display->cache->valid=0;
    if(selected!=1) {pt_editor_status(e,selected==0?"WAV REQUEST CANCELLED - EDITS PRESERVED":"WAV REQUEST FAILED - EDITS PRESERVED");return;}
    result=pt_render_file_new_allocated(path,e->project,&options,render_progress,&ui,&report,&detail,&render_allocator);
    if(result==PT_RENDER_FILE_OK) {
        if(report.clipped) {snprintf(status,sizeof(status),"WAV VERIFIED - %lu CLIPS; REDUCE GAIN",(unsigned long)report.clipped);pt_editor_status(e,status);}
        else pt_editor_status(e,pt_editor_dirty(e)?"WAV VERIFIED - PROJECT STILL UNSAVED":"WAV VERIFIED - PROJECT UNCHANGED");
    } else if(detail!=PT_RENDER_OK)pt_editor_status(e,render_error(detail));
    else pt_editor_status(e,result==PT_RENDER_FILE_BEGIN || result==PT_RENDER_FILE_PUBLISH?"WAV REFUSED: TARGET EXISTS OR CANNOT BE CREATED":"WAV FAILED - PROJECT AND DESTINATION PRESERVED");
    printf("EDITOR RENDER result=%u detail=%u dirty=%u frames=%lu\n",result,detail,pt_editor_dirty(e),result==PT_RENDER_FILE_OK?(unsigned long)report.frames:0UL);fflush(stdout);
}
static void render_stems(struct conversion_ui *display,struct pt_paula *audio)
{
    struct pt_editor *e=display->editor;struct pt_render_options options;struct pt_render_report plan;
    struct pt_stem_report report;struct pt_stem_plan stems;struct render_ui ui={*display,0,~0U,"STEMS"};
    enum pt_render_result detail;enum pt_render_file_result result;enum pt_stem_result planned;
    char path[1024],status[76];int selected;unsigned i,clipped=0;
    pt_paula_stop(audio);pt_paula_poll(audio,&e->playback);pt_editor_render_options(e,&options);
    planned=pt_stems_plan(&e->project->channels,options.tracks,e->render_groups,&stems);
    if(planned!=PT_STEM_OK) {pt_editor_status(e,render_error(planned==PT_STEM_MIDI?PT_RENDER_ROUTE:PT_RENDER_INVALID));return;}
    detail=pt_render_measure_allocated(e->project,&options,render_progress,&ui,&plan,&render_allocator);
    if(detail!=PT_RENDER_OK) {pt_editor_status(e,render_error(detail));return;}
    ui.total=plan.frames;puts("EDITOR REQUEST stems");fflush(stdout);
    selected=pt_file_request(display->window,10,"new-stems",path,sizeof(path));display->cache->valid=0;
    if(selected!=1) {pt_editor_status(e,selected==0?"STEMS REQUEST CANCELLED - EDITS PRESERVED":"STEMS REQUEST FAILED - EDITS PRESERVED");return;}
    result=pt_stem_file_new_allocated(path,e->project,&options,e->render_groups,render_progress,&ui,&report,&detail,&render_allocator);
    if(result==PT_RENDER_FILE_OK) {
        for(i=0;i<report.plan.count;++i)if(report.audio[i].clipped)++clipped;
        if(clipped)snprintf(status,sizeof(status),"%u STEMS VERIFIED - %u CLIPPED; REDUCE GAIN",report.plan.count,clipped);
        else snprintf(status,sizeof(status),"%u STEMS VERIFIED - PROJECT %s",report.plan.count,pt_editor_dirty(e)?"STILL UNSAVED":"UNCHANGED");
        pt_editor_status(e,status);
    } else if(detail==PT_RENDER_CANCELLED)pt_editor_status(e,"STEMS CANCELLED - EDITS PRESERVED");
    else if(detail!=PT_RENDER_OK)pt_editor_status(e,render_error(detail));
    else pt_editor_status(e,"STEMS REFUSED - TARGET EXISTS OR FOLDER CANNOT BE CREATED");
    printf("EDITOR STEMS result=%u detail=%u dirty=%u count=%u\n",result,detail,pt_editor_dirty(e),result==PT_RENDER_FILE_OK?report.plan.count:0);fflush(stdout);
}
static void bounce_sample(struct conversion_ui *display,struct pt_paula *audio)
{
    struct pt_editor *e=display->editor;struct pt_render_options options;struct pt_render_report plan,report;
    struct render_ui ui={*display,0,~0U,"SAMPLE"};enum pt_render_result detail;enum pt_edit_result result;
    char name[PT_PROJECT_NAME],status[76];
    pt_paula_stop(audio);pt_paula_poll(audio,&e->playback);pt_editor_render_options(e,&options);
    detail=pt_render_measure_allocated(e->project,&options,render_progress,&ui,&plan,&render_allocator);
    if(detail!=PT_RENDER_OK)result=detail==PT_RENDER_CANCELLED?PT_EDIT_CANCELLED:detail==PT_RENDER_MEMORY?PT_EDIT_CAPACITY:PT_EDIT_UNSUPPORTED;
    else {
        ui.total=plan.frames;
        if(options.row_range)snprintf(name,sizeof(name),"BOUNCE ROWS %02X-%02X",options.row_first,options.row_end-1);
        else if(options.pattern_only)snprintf(name,sizeof(name),"BOUNCE PATTERN %03u",options.pattern);
        else strcpy(name,"BOUNCE SONG");
        pt_editor_prepare_change(e);
        result=pt_sampler_bounce(&e->sampler,e->project,&e->history,&options,name,render_progress,&ui,&report,&detail);
    }
    if(result==PT_EDIT_OK) {
        e->sample=e->project->sample_count;pt_editor_sample_all(e);
        if(report.clipped)snprintf(status,sizeof(status),"BOUNCED TO SAMPLE %03u - %lu CLIPS; LOWER GAIN",e->sample,(unsigned long)report.clipped);
        else snprintf(status,sizeof(status),"BOUNCED TO SAMPLE %03u - CONTROL-Z UNDO",e->sample);
        pt_editor_status(e,status);
    } else pt_editor_status(e,result==PT_EDIT_CANCELLED?"BOUNCE CANCELLED - PROJECT AND REDO PRESERVED":
        result==PT_EDIT_CAPACITY?"BOUNCE REFUSED: SAMPLE LIMIT OR MEMORY/HISTORY BUDGET":
        result==PT_EDIT_UNSUPPORTED?"BOUNCE REFUSED: CHECK EFFECTS, ROUTES AND FINETUNE":
        "BOUNCE REFUSED - PROJECT AND REDO PRESERVED");
    printf("EDITOR BOUNCE result=%u detail=%u revision=%lu dirty=%u samples=%u frames=%lu\n",
        result,detail,(unsigned long)e->history.revision,pt_editor_dirty(e),e->project->sample_count,
        result==PT_EDIT_OK?(unsigned long)report.frames:0UL);fflush(stdout);
}
int main(int argc,char **argv)
{
    struct pt_view_cache view_cache={0};
    struct pt_paula audio={0};char load_path[1024]="",save_path[1024]="new-project.ptg",mod_path[1024]="new-module.mod",sample_path[1024]="",wav_path[1024]="new-sample.wav",svx_path[1024]="new-sample.iff",raw_path[1024]="new-sample.raw",raw_input[1024]="",chosen_path[1024];
    struct pt_allocator allocator={&master_memory,pt_master_allocate,pt_master_release};struct pt_document doc;
    struct pt_editor *editor=NULL;struct Screen *screen=NULL;struct Window *window=NULL;
    struct MsgPort *clock_port=NULL;struct timerequest *clock_request=NULL;
    int clock_open=0,clock_pending=0;unsigned frame_log=1,frames=0,peak_gap=0;
    struct DateStamp frame_start,frame_last;
    struct BitMap bitmap;struct pt_canvas canvas;unsigned plane;uint8_t *pixels=NULL;int rc=20,running=1,redraw=1;
    pt_master_memory_init(&master_memory);
    memset(&bitmap,0,sizeof(bitmap));memset(&canvas,0,sizeof(canvas));pt_document_init(&doc,&allocator);
    if(argc<1 || argc>3) {puts("Usage: PT24GEdit [INPUT [NEW_OUTPUT]]\nDevelopment editor; classic Paula playback; existing output is never replaced.");goto done;}
    if((argc>1?!load(&doc,argv[1]):pt_document_new(&doc,4,SIZE_MAX)!=PT_PROJECT_OK) || !(editor=pt_master_allocate(&master_memory,sizeof(*editor))) || !editor_init_memory(editor,&doc.project)) {
        puts("EDITOR: input invalid or allocation failed");goto done;
    }
    {
        LONG override=GetVar((STRPTR)"PT24G_RECENT_PREFIX",(STRPTR)recent_override,sizeof(recent_override),GVF_GLOBAL_ONLY);BPTR directory;
        if(override>0 && override<(LONG)sizeof(recent_override))recent_prefix=recent_override;
        else {directory=CreateDir((STRPTR)"ENVARC:ProTracker2.4G");if(directory)UnLock(directory);}
        pt_recent_file_load_allocated(recent_prefix,&recent_projects,&render_allocator);editor->recent=&recent_projects;
        printf("EDITOR RECENT loaded=%u prefix=%s\n",recent_projects.count,recent_prefix);fflush(stdout);
        if(argc>1)recent_success(editor,argv[1]);
    }
    if(argc>1)snprintf(load_path,sizeof(load_path),"%s",argv[1]);
    if(argc==3)snprintf(save_path,sizeof(save_path),"%s",argv[2]);
    IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",36);
    GfxBase=(struct GfxBase *)OpenLibrary("graphics.library",36);
    if(!IntuitionBase || !GfxBase)goto done;
    pixels=AllocMem(4UL*PT_VIEW_PLANE_BYTES,MEMF_PUBLIC|MEMF_FAST);
    if(!pixels)pixels=AllocMem(4UL*PT_VIEW_PLANE_BYTES,MEMF_PUBLIC);
    if(!pixels)goto done;
    InitBitMap(&bitmap,4,640,512);
    for(plane=0;plane<4;++plane) {
        bitmap.Planes[plane]=AllocRaster(640,512);if(!bitmap.Planes[plane])goto done;
        canvas.planes[plane]=pixels+plane*PT_VIEW_PLANE_BYTES;
    }
    screen=OpenScreenTags(NULL,SA_Width,640,SA_Height,512,SA_Depth,4,
        SA_DisplayID,PAL_MONITOR_ID|HIRESLACE_KEY,SA_Type,CUSTOMSCREEN,
        SA_Quiet,TRUE,SA_ShowTitle,FALSE,SA_Title,(ULONG)"ProTracker 2.4G Enhanced Editor",TAG_DONE);
    if(!screen) {puts("EDITOR: PAL 640x512 four-plane screen unavailable");goto done;}
    LoadRGB4(&screen->ViewPort,(UWORD *)pt_view_palette,16);
    window=OpenWindowTags(NULL,WA_CustomScreen,(ULONG)screen,WA_Left,0,WA_Top,0,
        WA_Width,640,WA_Height,512,WA_Borderless,TRUE,WA_Backdrop,TRUE,WA_Activate,TRUE,
        WA_RMBTrap,TRUE,WA_SimpleRefresh,TRUE,
        WA_IDCMP,IDCMP_RAWKEY|IDCMP_MOUSEBUTTONS|IDCMP_REFRESHWINDOW|IDCMP_INACTIVEWINDOW,TAG_DONE);
    if(!window)goto done;
    clock_port=CreateMsgPort();if(!clock_port)goto done;
    clock_request=(struct timerequest *)CreateIORequest(clock_port,sizeof(*clock_request));
    if(!clock_request || OpenDevice(TIMERNAME,UNIT_VBLANK,(struct IORequest *)clock_request,0))goto done;
    clock_open=1;DateStamp(&frame_start);frame_last=frame_start;
    printf("EDITOR READY channels=%u patterns=%u bitmap_bytes=%lu history_bytes=%lu\n",
        doc.project.channels.count,doc.project.pattern_count,4UL*PT_VIEW_PLANE_BYTES,(unsigned long)sizeof(*editor));fflush(stdout);
    while(running) {
        struct IntuiMessage *message;
        /* Rearm before drawing so rendering consumes the interval instead of
           adding to it. Never queue catch-up frames or busy-wait. */
        if(!clock_pending) {
            clock_request->tr_node.io_Command=TR_ADDREQUEST;
            clock_request->tr_time.tv_secs=0;clock_request->tr_time.tv_micro=1;
            SendIO((struct IORequest *)clock_request);clock_pending=1;
        }
        if(redraw) {
            struct pt_view_rect areas[PT_VIEW_DIRTY_MAX];unsigned n;
            int scroll=(int)editor->first_row-(int)view_cache.first_row;
            n=pt_editor_draw_update(editor,&canvas,pt_font,&view_cache,areas);
            pt_native_present(&canvas,&bitmap,window->RPort,areas,n,scroll);
            redraw=0;
            if(n && editor->playback.active) {
                struct DateStamp now;long span,gap;DateStamp(&now);
                span=(now.ds_Days-frame_start.ds_Days)*4320000L+(now.ds_Minute-frame_start.ds_Minute)*3000L+now.ds_Tick-frame_start.ds_Tick;
                gap=(now.ds_Days-frame_last.ds_Days)*4320000L+(now.ds_Minute-frame_last.ds_Minute)*3000L+now.ds_Tick-frame_last.ds_Tick;
                if(frames && gap>0 && (unsigned long)gap>peak_gap)peak_gap=(unsigned)gap;
                frame_last=now;++frames;
                if(span>=100) {
                    printf("EDITOR CADENCE frames=%u ticks50=%ld peak_gap_ticks50=%u\n",frames,span,peak_gap);fflush(stdout);
                    frame_start=now;frames=peak_gap=0;
                }
            } else if(!editor->playback.active) {DateStamp(&frame_start);frame_last=frame_start;frames=peak_gap=0;}
            if(frame_log) {
                printf("EDITOR FRAME row=%u channel=%u revision=%lu dirty=%u status=%s panel=%u\n",editor->row,
                    doc.project.channels.selected,(unsigned long)editor->history.revision,pt_editor_dirty(editor),editor->status,editor->panel);fflush(stdout);
                frame_log=0;
            }
        }
        Wait((1UL<<window->UserPort->mp_SigBit)|(1UL<<clock_port->mp_SigBit));
        if(CheckIO((struct IORequest *)clock_request)) {
            unsigned was_active=editor->playback.active,old_row=editor->playback.row;
            WaitIO((struct IORequest *)clock_request);clock_pending=0;
            pt_paula_poll(&audio,&editor->playback);pt_editor_follow_playback(editor);
            if(was_active && !editor->playback.active)pt_editor_status(editor,"PLAYBACK ENDED - AUDIO RELEASED");
            if(was_active || editor->playback.active)redraw=1;
            if(was_active!=editor->playback.active || old_row!=editor->playback.row) {
                frame_log=1;
                printf("EDITOR REPLAY active=%u ticks=%lu order=%u pattern=%u row=%u bpm=%u speed=%u period=%u,%u,%u,%u volume=%u,%u,%u,%u\n",
                    editor->playback.active,(unsigned long)editor->playback.ticks,editor->playback.order,editor->playback.pattern,
                    editor->playback.row,editor->playback.bpm,editor->playback.speed,
                    editor->playback.period[0],editor->playback.period[1],editor->playback.period[2],editor->playback.period[3],
                    editor->playback.volume[0],editor->playback.volume[1],editor->playback.volume[2],editor->playback.volume[3]);fflush(stdout);
            }
        }
        while((message=(struct IntuiMessage *)GetMsg(window->UserPort))) {
            ULONG kind=message->Class;UWORD code=message->Code,qualifier=message->Qualifier;
            WORD mx=message->MouseX,my=message->MouseY;enum pt_editor_action action=PT_UI_NONE;
            struct conversion_ui conversion={editor,window,&canvas,&bitmap,&view_cache,0};
            unsigned generation=editor->sampler.generation;
            unsigned long revision=editor->history.revision;const char *error=NULL;
            ReplyMsg((struct Message *)message);
            if(kind==IDCMP_RAWKEY && (code&0x80 || code>=0x60))continue;
            editor->sampler.progress=conversion_progress;editor->sampler.progress_context=&conversion;
            if(kind==IDCMP_RAWKEY)action=pt_editor_key(editor,code,qualifier);
            else if(kind==IDCMP_MOUSEBUTTONS && code==SELECTDOWN)action=pt_editor_click(editor,mx,my);
            else if(kind==IDCMP_REFRESHWINDOW) {BeginRefresh(window);EndRefresh(window,TRUE);view_cache.valid=0;}
            else if(kind==IDCMP_INACTIVEWINDOW) {editor->quit_pending=0;editor->load_pending=0;}
            editor->sampler.progress=NULL;editor->sampler.progress_context=NULL;
            if(action==PT_UI_PLAY || action==PT_UI_PATTERN) {
                error=pt_paula_play(&audio,editor->project,action==PT_UI_PATTERN,editor->position,editor->pattern);
                pt_editor_status(editor,error?error:action==PT_UI_PATTERN?"PLAYING PATTERN - PAULA CIA":"PLAYING SONG - PAULA CIA");
            }
            if(action==PT_UI_AUDITION) {
                error=pt_paula_audition_progress(&audio,editor->project,editor->sample,856U>>editor->octave,conversion_progress,&conversion);
                pt_editor_status(editor,error?error:"SAMPLE AUDITION - PAULA; STOP TO RELEASE");
            }
            if(action==PT_UI_STOP) {pt_editor_prepare_change(editor);pt_paula_stop(&audio);pt_editor_status(editor,"STOPPED - AUDIO RELEASED");}
            if(editor->history.revision!=revision && audio.started) {
                if(audio.mode==2 || editor->sampler.generation!=generation)pt_paula_stop(&audio);
                else {error=pt_paula_sync(&audio,editor->project);if(error)pt_editor_status(editor,error);}
                pt_paula_poll(&audio,&editor->playback);
            }
            if(error || action==PT_UI_PLAY || action==PT_UI_PATTERN || action==PT_UI_AUDITION || action==PT_UI_STOP)
                pt_paula_poll(&audio,&editor->playback);
            if(action==PT_UI_SAVE || action==PT_UI_SAVE_AS) {
                if(action==PT_UI_SAVE && argc==3)save(editor,argv[2]);
                else {
                    int selected;printf("EDITOR REQUEST save\n");fflush(stdout);
                    selected=pt_file_request(window,1,save_path,chosen_path,sizeof(chosen_path));view_cache.valid=0;
                    if(selected==1) {strcpy(save_path,chosen_path);save(editor,save_path);}
                    else pt_editor_status(editor,selected==0?"SAVE CANCELLED - EDITS PRESERVED":"SAVE REQUESTER UNAVAILABLE OR PATH TOO LONG");
                }
            }
            if(action==PT_UI_EXPORT_MOD || action==PT_UI_EXPORT_MOD8) {
                struct pt_mod_export_report report;
                unsigned round8=action==PT_UI_EXPORT_MOD8?(editor->export_dither?2:1):0;
                if(mod_eligible(editor,&report,round8)) {
                    int selected;printf("EDITOR REQUEST %s\n",round8?"mod8":"mod");fflush(stdout);
                    selected=pt_file_request(window,2,mod_path,chosen_path,sizeof(chosen_path));view_cache.valid=0;
                    if(selected==1) {strcpy(mod_path,chosen_path);save_mod(editor,mod_path,report.bytes,round8,(report.issues&PT_EXPORT_PRECISION)!=0);}
                    else pt_editor_status(editor,selected==0?"MOD EXPORT CANCELLED - PROJECT PRESERVED":"MOD REQUESTER UNAVAILABLE OR PATH TOO LONG");
                }
            }
            if(action==PT_UI_RENDER)render_wav(&conversion,&audio);
            if(action==PT_UI_STEMS)render_stems(&conversion,&audio);
            if(action==PT_UI_BOUNCE)bounce_sample(&conversion,&audio);
            if(action==PT_UI_RAW_LOAD || (action==PT_UI_RAW_SAVE && raw_eligible(editor))) {
                int importing=action==PT_UI_RAW_LOAD,selected;printf("EDITOR REQUEST %s\n",importing?"rawload":"rawsave");fflush(stdout);
                selected=pt_file_request(window,importing?6:7,importing?raw_input:raw_path,chosen_path,sizeof(chosen_path));view_cache.valid=0;
                if(selected==1) {
                    if(importing) {
                        unsigned generation=editor->sampler.generation;enum pt_edit_result result=load_sample(editor,chosen_path,1,0,NULL);
                        pt_editor_sample_result(editor,result);
                        if(result==PT_EDIT_OK) {
                            strcpy(raw_input,chosen_path);pt_editor_sample_all(editor);
                            if(editor->sampler.generation!=generation) {pt_paula_stop(&audio);pt_paula_poll(&audio,&editor->playback);}
                        }
                        printf("EDITOR RAWLOAD result=%u revision=%lu dirty=%u\n",result,(unsigned long)editor->history.revision,pt_editor_dirty(editor));fflush(stdout);
                    } else {strcpy(raw_path,chosen_path);save_raw(editor,chosen_path);}
                } else pt_editor_status(editor,selected==0?"RAW REQUEST CANCELLED - EDITS PRESERVED":"RAW REQUEST FAILED - EDITS PRESERVED");
            }
            if(action==PT_UI_SAMPLE_SVX && svx_eligible(editor)) {
                int selected;printf("EDITOR REQUEST iff\n");fflush(stdout);
                selected=pt_file_request(window,5,svx_path,chosen_path,sizeof(chosen_path));view_cache.valid=0;
                if(selected==1) {strcpy(svx_path,chosen_path);save_svx(editor,chosen_path);}
                else pt_editor_status(editor,selected==0?"IFF EXPORT CANCELLED - EDITS PRESERVED":"IFF REQUESTER UNAVAILABLE OR PATH TOO LONG");
            }
            if(action==PT_UI_SAMPLE_LOAD || action==PT_UI_SOURCE_LOAD || action==PT_UI_SAMPLE_SAVE) {
                int importing=action!=PT_UI_SAMPLE_SAVE,selected,source_only=action==PT_UI_SOURCE_LOAD;
                printf("EDITOR REQUEST %s\n",source_only?"source":importing?"sample":"wav");fflush(stdout);
                selected=pt_file_request(window,source_only?8:importing?3:4,importing?sample_path:wav_path,chosen_path,sizeof(chosen_path));view_cache.valid=0;
                if(selected==1) {
                    if(importing) {
                        int preview=0;enum pt_edit_result result=load_sample(editor,chosen_path,0,source_only,&preview);
                        if(!preview || result!=PT_EDIT_OK)pt_editor_sample_result(editor,result);
                        if(result==PT_EDIT_OK)strcpy(sample_path,chosen_path);
                        if(result==PT_EDIT_OK && !preview) {
                            pt_editor_sample_all(editor);
                            if(editor->sampler.generation!=generation) {pt_paula_stop(&audio);pt_paula_poll(&audio,&editor->playback);}
                        }
                        printf("EDITOR SAMPLE result=%u revision=%lu dirty=%u preview=%u\n",result,(unsigned long)editor->history.revision,pt_editor_dirty(editor),preview);fflush(stdout);
                    } else {strcpy(wav_path,chosen_path);save_sample(editor,chosen_path);}
                } else pt_editor_status(editor,selected==0?"SAMPLE FILE REQUEST CANCELLED - EDITS PRESERVED":"SAMPLE FILE REQUEST FAILED - EDITS PRESERVED");
            }
            if(action==PT_UI_NEW) {
                unsigned channels=editor->new_channels;
                pt_editor_prepare_change(editor);
                if(pt_document_new(&doc,channels,SIZE_MAX)==PT_PROJECT_OK) {
                    pt_paula_stop(&audio);pt_editor_dispose(editor);editor_init_memory(editor,&doc.project);editor->recent=&recent_projects;load_path[0]=0;
                    pt_editor_status(editor,"NEW SONG READY - EMPTY SAMPLE SLOTS");view_cache.valid=0;
                    printf("EDITOR NEW channels=%u patterns=%u\n",doc.project.channels.count,doc.project.pattern_count);fflush(stdout);
                } else pt_editor_status(editor,"NEW SONG FAILED - CURRENT PROJECT AND EDITS PRESERVED");
            }
            if(action==PT_UI_RECENT_REMOVE || action==PT_UI_RECENT_CLEAR) {
                if(action==PT_UI_RECENT_CLEAR)pt_recent_init(&recent_projects);
                else pt_recent_remove(&recent_projects,editor->recent_selected);
                if(editor->recent_selected>=recent_projects.count)editor->recent_selected=recent_projects.count?recent_projects.count-1:0;
                pt_editor_status(editor,"RECENT LIST UPDATED");recent_persist(editor);
            }
            if(action==PT_UI_LOAD || action==PT_UI_RECENT_LOAD) {
                {
                    int selected;printf("EDITOR REQUEST load\n");fflush(stdout);
                    if(action==PT_UI_RECENT_LOAD && editor->recent_selected<recent_projects.count) {
                        strcpy(chosen_path,recent_projects.path[editor->recent_selected]);selected=1;
                    } else selected=pt_file_request(window,0,load_path,chosen_path,sizeof(chosen_path));
                    view_cache.valid=0;
                    if(selected==1) {
                        pt_editor_prepare_change(editor);
                        if(load(&doc,chosen_path)) {
                            pt_paula_stop(&audio);pt_editor_dispose(editor);editor_init_memory(editor,&doc.project);editor->recent=&recent_projects;strcpy(load_path,chosen_path);
                            pt_editor_status(editor,"PROJECT LOADED");recent_success(editor,chosen_path);
                            printf("EDITOR LOAD success channels=%u patterns=%u\n",doc.project.channels.count,doc.project.pattern_count);fflush(stdout);
                        } else pt_editor_status(editor,"LOAD FAILED - CURRENT PROJECT AND EDITS PRESERVED");
                    } else pt_editor_status(editor,selected==0?"LOAD CANCELLED - EDITS PRESERVED":"LOAD REQUESTER UNAVAILABLE OR PATH TOO LONG");
                }
            }
            if(action==PT_UI_QUIT)running=0;
            redraw=1;frame_log=1;
        }
    }
    puts("EDITOR EXIT clean");rc=0;
done:
    pt_paula_stop(&audio);
    if(clock_pending) {AbortIO((struct IORequest *)clock_request);WaitIO((struct IORequest *)clock_request);}
    if(clock_open)CloseDevice((struct IORequest *)clock_request);
    if(clock_request)DeleteIORequest((struct IORequest *)clock_request);
    if(clock_port)DeleteMsgPort(clock_port);
    if(window)CloseWindow(window);
    if(screen)CloseScreen(screen);
    if(GfxBase) {WaitBlit();for(plane=0;plane<4;++plane)if(bitmap.Planes[plane])FreeRaster(bitmap.Planes[plane],640,512);}
    if(pixels)FreeMem(pixels,4UL*PT_VIEW_PLANE_BYTES);
    if(GfxBase)CloseLibrary((struct Library *)GfxBase);
    if(IntuitionBase)CloseLibrary((struct Library *)IntuitionBase);
    pt_editor_dispose(editor);pt_master_release(&master_memory,editor);pt_document_release(&doc);return rc;
}
