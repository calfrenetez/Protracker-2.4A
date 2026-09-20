/* Native enhanced editor and owned classic Paula replay integration. */
#include <exec/memory.h>
#include <graphics/gfxbase.h>
#include <graphics/modeid.h>
#include <intuition/intuitionbase.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "document.h"
#include "wav.h"
#include "mod_project.h"
#include "../editor/view.h"
#include "../platform/file_save.h"
#include "pt_font.h"
#include "paula.h"
#include "file_request.h"
struct IntuitionBase *IntuitionBase;
struct GfxBase *GfxBase;
static void *allocate(void *ctx,size_t n) {(void)ctx;return malloc(n);}
static void release(void *ctx,void *p) {(void)ctx;free(p);}
static int load(struct pt_document *d,const char *path)
{
    FILE *f=fopen(path,"rb");long n;uint8_t *bytes=NULL;int ok=0;
    if(!f)return 0;
    if(fseek(f,0,SEEK_END) || (n=ftell(f))<0 || n>64L*1024*1024)goto done;
    rewind(f);bytes=malloc(n?(size_t)n:1);if(!bytes)goto done;
    if(fread(bytes,1,(size_t)n,f)!=(size_t)n || ferror(f))goto done;
    if(fclose(f)) {f=NULL;goto done;}f=NULL;
    ok=pt_document_load(d,bytes,(size_t)n,SIZE_MAX)==PT_PROJECT_OK;
done:
    free(bytes);if(f)fclose(f);return ok;
}
static enum pt_edit_result load_sample(struct pt_editor *e,const char *path)
{
    FILE *f=fopen(path,"rb");long n;uint8_t *bytes=NULL;enum pt_edit_result result=PT_EDIT_INVALID;
    const char *name=path,*part;
    if(!f || !e->sample) {if(f)fclose(f);return PT_EDIT_INVALID;}
    if(fseek(f,0,SEEK_END) || (n=ftell(f))<=0 || n>64L*1024*1024)goto done;
    rewind(f);bytes=malloc((size_t)n);if(!bytes) {result=PT_EDIT_CAPACITY;goto done;}
    if(fread(bytes,1,(size_t)n,f)!=(size_t)n || ferror(f))goto done;
    if(fclose(f)) {f=NULL;goto done;}f=NULL;
    for(part=path;*part;++part)if(*part=='/' || *part==':')name=part+1;
    result=pt_sampler_import(&e->sampler,e->project,&e->history,e->sample-1,bytes,(size_t)n,name);
done:
    if(f)fclose(f);
    free(bytes);return result;
}
static void save_sample(struct pt_editor *e,const char *path)
{
    size_t n,w;uint8_t *bytes;enum pt_save_result result;
    const struct pt_pcm *pcm=e->sample && e->sample<=e->project->sample_count?&e->project->samples[e->sample-1].pcm:NULL;
    if(!pcm || !pcm->frames || pt_wav_size(pcm,&n)!=PT_WAV_OK) {pt_editor_status(e,"WAV EXPORT: SELECT A NONEMPTY SAMPLE");return;}
    bytes=malloc(n);if(!bytes) {pt_editor_status(e,"WAV EXPORT: OUT OF MEMORY");return;}
    if(pt_wav_encode(pcm,bytes,n,&w)!=PT_WAV_OK || w!=n) {free(bytes);pt_editor_status(e,"WAV EXPORT FAILED - SAMPLE PRESERVED");return;}
    result=pt_file_save_new(path,bytes,n);free(bytes);
    pt_editor_status(e,result==PT_SAVE_OK?"WAV EXPORTED AND VERIFIED - PROJECT STATE UNCHANGED":"WAV EXPORT REFUSED OR FAILED - DESTINATION PRESERVED");
    printf("EDITOR WAV result=%u dirty=%u\n",result,pt_editor_dirty(e));fflush(stdout);
}
static void save(struct pt_editor *e,const char *path)
{
    size_t n,w;uint8_t *bytes;enum pt_save_result result;
    if(!path) {pt_editor_status(e,"START WITH INPUT AND NEW_OUTPUT PATH TO ENABLE SAVE");return;}
    if(pt_project_size(e->project,&n)!=PT_PROJECT_OK || !(bytes=malloc(n))) {
        pt_editor_status(e,"SAVE: INVALID PROJECT OR OUT OF MEMORY");return;
    }
    if(pt_project_encode(e->project,bytes,n,&w)!=PT_PROJECT_OK || w!=n) {
        free(bytes);pt_editor_status(e,"SAVE: ENCODE FAILED; CURRENT EDITS PRESERVED");return;
    }
    result=pt_file_save_new(path,bytes,n);free(bytes);
    if(result==PT_SAVE_OK)pt_editor_saved(e);
    else pt_editor_status(e,result==PT_SAVE_PUBLISH?"SAVE REFUSED: DESTINATION EXISTS OR CANNOT BE PUBLISHED":"SAVE FAILED: CURRENT EDITS AND DESTINATION PRESERVED");
    printf("EDITOR SAVE result=%u dirty=%u\n",result,pt_editor_dirty(e));fflush(stdout);
}
static int mod_eligible(struct pt_editor *e,struct pt_mod_export_report *r)
{
    const char *reason;
    if(pt_mod_export_analyse(e->project,r)!=PT_PROJECT_OK) {pt_editor_status(e,"MOD EXPORT REFUSED: INVALID PROJECT");return 0;}
    if(!r->issues)return 1;
    if(r->issues&PT_EXPORT_MIDI_AUDIO)reason="MOD EXPORT REFUSED: EXTERNAL MIDI AUDIO IS NOT IN A MOD";
    else if(r->issues&PT_EXPORT_CHANNELS)reason="MOD EXPORT REFUSED: MORE THAN FOUR CHANNELS; SAVE PROJECT";
    else if(r->issues&PT_EXPORT_ROUTING)reason="MOD EXPORT REFUSED: NEEDS CLASSIC PAULA ROUTING";
    else if(r->issues&(PT_EXPORT_PRECISION|PT_EXPORT_STEREO|PT_EXPORT_RATE))reason="MOD EXPORT REFUSED: SAMPLE FORMAT NEEDS CONVERSION";
    else if(r->issues&(PT_EXPORT_LOOPS|PT_EXPORT_SLICES))reason="MOD EXPORT REFUSED: UNSUPPORTED SAMPLE LOOPS OR SLICES";
    else if(r->issues&(PT_EXPORT_OFF|PT_EXPORT_NOTES|PT_EXPORT_VELOCITY))reason="MOD EXPORT REFUSED: ENHANCED NOTES; SAVE PROJECT TO KEEP THEM";
    else if(r->issues&PT_EXPORT_TEMPO)reason="MOD EXPORT REFUSED: INITIAL TEMPO NEEDS CONVERSION";
    else if(r->issues&PT_EXPORT_PANNING)reason="MOD EXPORT REFUSED: ENHANCED PANNING; SAVE PROJECT";
    else reason="MOD EXPORT REFUSED: ENHANCED DATA OR CLASSIC FORMAT LIMITS";
    pt_editor_status(e,reason);printf("EDITOR MOD refused issues=0x%lx dirty=%u\n",(unsigned long)r->issues,pt_editor_dirty(e));fflush(stdout);return 0;
}
static void save_mod(struct pt_editor *e,const char *path,size_t n)
{
    uint8_t *bytes=malloc(n);size_t written;enum pt_save_result result;
    if(!bytes) {pt_editor_status(e,"MOD EXPORT: OUT OF MEMORY - PROJECT PRESERVED");return;}
    if(pt_mod_export_direct(e->project,bytes,n,&written)!=PT_PROJECT_OK || written!=n) {
        free(bytes);pt_editor_status(e,"MOD EXPORT FAILED - PROJECT PRESERVED");return;
    }
    result=pt_file_save_new(path,bytes,n);free(bytes);
    if(result==PT_SAVE_OK)pt_editor_status(e,pt_editor_dirty(e)?"MOD EXPORTED AND VERIFIED - PROJECT STILL UNSAVED":"MOD EXPORTED AND VERIFIED");
    else pt_editor_status(e,result==PT_SAVE_PUBLISH?"MOD EXPORT REFUSED: DESTINATION EXISTS OR CANNOT BE PUBLISHED":"MOD EXPORT FAILED: PROJECT AND DESTINATION PRESERVED");
    /* Export does not mark the richer project saved or consume undo history. */
    printf("EDITOR MOD result=%u dirty=%u\n",result,pt_editor_dirty(e));fflush(stdout);
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
        snprintf(status,sizeof(status),"FILTERING %u%% - ESC CANCEL; ORIGINAL SAMPLE PRESERVED",percent);
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
int main(int argc,char **argv)
{
    struct pt_view_cache view_cache={0};
    struct pt_paula audio={0};char load_path[1024]="",save_path[1024]="new-project.ptg",mod_path[1024]="new-module.mod",sample_path[1024]="",wav_path[1024]="new-sample.wav",chosen_path[1024];
    struct pt_allocator allocator={NULL,allocate,release};struct pt_document doc;
    struct pt_editor *editor=NULL;struct Screen *screen=NULL;struct Window *window=NULL;
    struct BitMap bitmap;struct pt_canvas canvas;unsigned plane;uint8_t *pixels=NULL;int rc=20,running=1,redraw=1;
    memset(&bitmap,0,sizeof(bitmap));memset(&canvas,0,sizeof(canvas));pt_document_init(&doc,&allocator);
    if(argc<1 || argc>3) {puts("Usage: PT24GEdit [INPUT [NEW_OUTPUT]]\nDevelopment editor; classic Paula playback; existing output is never replaced.");goto done;}
    if((argc>1?!load(&doc,argv[1]):pt_document_new(&doc,4,SIZE_MAX)!=PT_PROJECT_OK) || !(editor=calloc(1,sizeof(*editor))) || !pt_editor_init(editor,&doc.project)) {
        puts("EDITOR: input invalid or allocation failed");goto done;
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
        WA_IDCMP,IDCMP_RAWKEY|IDCMP_MOUSEBUTTONS|IDCMP_REFRESHWINDOW|IDCMP_INACTIVEWINDOW|IDCMP_INTUITICKS,TAG_DONE);
    if(!window)goto done;
    printf("EDITOR READY channels=%u patterns=%u bitmap_bytes=%lu history_bytes=%lu\n",
        doc.project.channels.count,doc.project.pattern_count,4UL*PT_VIEW_PLANE_BYTES,(unsigned long)sizeof(*editor));fflush(stdout);
    while(running) {
        struct IntuiMessage *message;
        if(redraw) {
            struct pt_view_rect areas[PT_VIEW_DIRTY_MAX];unsigned i,n;
            n=pt_editor_draw_update(editor,&canvas,pt_font,&view_cache,areas);
            for(i=0;i<n;++i) {
                struct pt_view_rect *area=&areas[i];
                for(plane=0;plane<4;++plane)CopyMem(canvas.planes[plane]+area->y*80,
                    bitmap.Planes[plane]+area->y*80,area->height*80);
                BltBitMapRastPort(&bitmap,area->x,area->y,window->RPort,area->x,area->y,area->width,area->height,0xc0);WaitBlit();
            }
            if(n) {WaitTOF();WaitTOF();}redraw=0;
            printf("EDITOR FRAME row=%u channel=%u revision=%lu dirty=%u status=%s panel=%u\n",editor->row,
                doc.project.channels.selected,(unsigned long)editor->history.revision,pt_editor_dirty(editor),editor->status,editor->panel);fflush(stdout);
        }
        WaitPort(window->UserPort);
        while((message=(struct IntuiMessage *)GetMsg(window->UserPort))) {
            ULONG kind=message->Class;UWORD code=message->Code,qualifier=message->Qualifier;
            WORD mx=message->MouseX,my=message->MouseY;enum pt_editor_action action=PT_UI_NONE;
            struct conversion_ui conversion={editor,window,&canvas,&bitmap,&view_cache,0};
            unsigned generation=editor->sampler.generation;
            unsigned long revision=editor->history.revision;const char *error=NULL;
            ReplyMsg((struct Message *)message);
            if(kind==IDCMP_INTUITICKS) {
                unsigned was_active=editor->playback.active;
                pt_paula_poll(&audio,&editor->playback);
                if(was_active && !editor->playback.active) {pt_editor_status(editor,"PLAYBACK ENDED - AUDIO RELEASED");redraw=1;}
                if(was_active || editor->playback.active) {
                    unsigned i;
                    pt_editor_draw_playback(editor,&canvas,pt_font);
                    for(i=0;i<PT_VIEW_PLAYBACK_AREAS;++i) {
                        const struct pt_view_rect *area=&pt_view_playback_areas[i];
                        for(plane=0;plane<4;++plane)CopyMem(canvas.planes[plane]+area->y*80,
                            bitmap.Planes[plane]+area->y*80,area->height*80);
                        BltBitMapRastPort(&bitmap,area->x,area->y,window->RPort,
                            area->x,area->y,area->width,area->height,0xc0);WaitBlit();
                    }
                    printf("EDITOR REPLAY active=%u ticks=%lu order=%u pattern=%u row=%u bpm=%u speed=%u period=%u,%u,%u,%u volume=%u,%u,%u,%u\n",
                        editor->playback.active,(unsigned long)editor->playback.ticks,editor->playback.order,editor->playback.pattern,
                        editor->playback.row,editor->playback.bpm,editor->playback.speed,
                        editor->playback.period[0],editor->playback.period[1],editor->playback.period[2],editor->playback.period[3],
                        editor->playback.volume[0],editor->playback.volume[1],editor->playback.volume[2],editor->playback.volume[3]);fflush(stdout);
                }
                continue;
            }
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
                error=pt_paula_audition(&audio,editor->project,editor->sample,856U>>editor->octave);
                pt_editor_status(editor,error?error:"SAMPLE AUDITION - PAULA; STOP TO RELEASE");
            }
            if(action==PT_UI_STOP) {pt_paula_stop(&audio);pt_editor_status(editor,"STOPPED - AUDIO RELEASED");}
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
            if(action==PT_UI_EXPORT_MOD) {
                struct pt_mod_export_report report;
                if(mod_eligible(editor,&report)) {
                    int selected;printf("EDITOR REQUEST mod\n");fflush(stdout);
                    selected=pt_file_request(window,2,mod_path,chosen_path,sizeof(chosen_path));view_cache.valid=0;
                    if(selected==1) {strcpy(mod_path,chosen_path);save_mod(editor,mod_path,report.bytes);}
                    else pt_editor_status(editor,selected==0?"MOD EXPORT CANCELLED - PROJECT PRESERVED":"MOD REQUESTER UNAVAILABLE OR PATH TOO LONG");
                }
            }
            if(action==PT_UI_SAMPLE_LOAD || action==PT_UI_SAMPLE_SAVE) {
                int importing=action==PT_UI_SAMPLE_LOAD,selected;
                printf("EDITOR REQUEST %s\n",importing?"sample":"wav");fflush(stdout);
                selected=pt_file_request(window,importing?3:4,importing?sample_path:wav_path,chosen_path,sizeof(chosen_path));view_cache.valid=0;
                if(selected==1) {
                    if(importing) {
                        enum pt_edit_result result=load_sample(editor,chosen_path);
                        pt_editor_sample_result(editor,result);
                        if(result==PT_EDIT_OK) {
                            strcpy(sample_path,chosen_path);pt_editor_sample_all(editor);
                            if(editor->sampler.generation!=generation) {pt_paula_stop(&audio);pt_paula_poll(&audio,&editor->playback);}
                        }
                        printf("EDITOR SAMPLE result=%u revision=%lu dirty=%u\n",result,(unsigned long)editor->history.revision,pt_editor_dirty(editor));fflush(stdout);
                    } else {strcpy(wav_path,chosen_path);save_sample(editor,chosen_path);}
                } else pt_editor_status(editor,selected==0?"SAMPLE FILE REQUEST CANCELLED - EDITS PRESERVED":"SAMPLE FILE REQUEST FAILED - EDITS PRESERVED");
            }
            if(action==PT_UI_NEW) {
                unsigned channels=editor->new_channels;
                if(pt_document_new(&doc,channels,SIZE_MAX)==PT_PROJECT_OK) {
                    pt_paula_stop(&audio);pt_editor_dispose(editor);pt_editor_init(editor,&doc.project);load_path[0]=0;
                    pt_editor_status(editor,"NEW SONG READY - EMPTY SAMPLE SLOTS");view_cache.valid=0;
                    printf("EDITOR NEW channels=%u patterns=%u\n",doc.project.channels.count,doc.project.pattern_count);fflush(stdout);
                } else pt_editor_status(editor,"NEW SONG FAILED - CURRENT PROJECT AND EDITS PRESERVED");
            }
            if(action==PT_UI_LOAD) {
                {
                    int selected;printf("EDITOR REQUEST load\n");fflush(stdout);
                    selected=pt_file_request(window,0,load_path,chosen_path,sizeof(chosen_path));view_cache.valid=0;
                    if(selected==1) {
                        if(load(&doc,chosen_path)) {
                            pt_paula_stop(&audio);pt_editor_dispose(editor);pt_editor_init(editor,&doc.project);strcpy(load_path,chosen_path);
                            pt_editor_status(editor,"PROJECT LOADED");
                            printf("EDITOR LOAD success channels=%u patterns=%u\n",doc.project.channels.count,doc.project.pattern_count);fflush(stdout);
                        } else pt_editor_status(editor,"LOAD FAILED - CURRENT PROJECT AND EDITS PRESERVED");
                    } else pt_editor_status(editor,selected==0?"LOAD CANCELLED - EDITS PRESERVED":"LOAD REQUESTER UNAVAILABLE OR PATH TOO LONG");
                }
            }
            if(action==PT_UI_QUIT)running=0;
            redraw=1;
        }
    }
    puts("EDITOR EXIT clean");rc=0;
done:
    pt_paula_stop(&audio);
    if(window)CloseWindow(window);
    if(screen)CloseScreen(screen);
    if(GfxBase) {WaitBlit();for(plane=0;plane<4;++plane)if(bitmap.Planes[plane])FreeRaster(bitmap.Planes[plane],640,512);}
    if(pixels)FreeMem(pixels,4UL*PT_VIEW_PLANE_BYTES);
    if(GfxBase)CloseLibrary((struct Library *)GfxBase);
    if(IntuitionBase)CloseLibrary((struct Library *)IntuitionBase);
    pt_editor_dispose(editor);free(editor);pt_document_release(&doc);return rc;
}
