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
#include "../editor/view.h"
#include "../platform/file_save.h"
#include "pt_font.h"
#include "paula.h"
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
    ok=pt_document_load(d,bytes,(size_t)n,SIZE_MAX)==PT_PROJECT_OK;
done:
    free(bytes);if(fclose(f))ok=0;return ok;
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
int main(int argc,char **argv)
{
    struct pt_paula audio={0};
    struct pt_allocator allocator={NULL,allocate,release};struct pt_document doc;
    struct pt_editor *editor=NULL;struct Screen *screen=NULL;struct Window *window=NULL;
    struct BitMap bitmap;struct pt_canvas canvas;unsigned plane;uint8_t *pixels=NULL;int rc=20,running=1,redraw=1;
    memset(&bitmap,0,sizeof(bitmap));memset(&canvas,0,sizeof(canvas));pt_document_init(&doc,&allocator);
    if(argc<2 || argc>3) {puts("Usage: PT24GEdit INPUT [NEW_OUTPUT]\nDevelopment editor; classic Paula playback; existing output is never replaced.");goto done;}
    if(!load(&doc,argv[1]) || !(editor=malloc(sizeof(*editor))) || !pt_editor_init(editor,&doc.project)) {
        puts("EDITOR: input invalid or allocation failed");goto done;
    }
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
            pt_editor_draw(editor,&canvas,pt_font);
            for(plane=0;plane<4;++plane)CopyMem(canvas.planes[plane],bitmap.Planes[plane],PT_VIEW_PLANE_BYTES);
            BltBitMapRastPort(&bitmap,0,0,window->RPort,0,0,640,512,0xc0);WaitBlit();WaitTOF();WaitTOF();redraw=0;
            printf("EDITOR FRAME row=%u channel=%u revision=%lu dirty=%u status=%s\n",editor->row,
                doc.project.channels.selected,(unsigned long)editor->history.revision,pt_editor_dirty(editor),editor->status);fflush(stdout);
        }
        WaitPort(window->UserPort);
        while((message=(struct IntuiMessage *)GetMsg(window->UserPort))) {
            ULONG kind=message->Class;UWORD code=message->Code,qualifier=message->Qualifier;
            WORD mx=message->MouseX,my=message->MouseY;enum pt_editor_action action=PT_UI_NONE;
            unsigned long revision=editor->history.revision;const char *error=NULL;
            ReplyMsg((struct Message *)message);
            if(kind==IDCMP_INTUITICKS) {
                unsigned was_active=editor->playback.active;
                pt_paula_poll(&audio,&editor->playback);
                if(was_active && !editor->playback.active) {pt_editor_status(editor,"PLAYBACK ENDED - AUDIO RELEASED");redraw=1;}
                if(was_active || editor->playback.active) {
                    pt_editor_draw_playback(editor,&canvas,pt_font);
                    for(plane=0;plane<4;++plane) {
                        CopyMem(canvas.planes[plane]+116*80,bitmap.Planes[plane]+116*80,39*80);
                        CopyMem(canvas.planes[plane]+216*80,bitmap.Planes[plane]+216*80,21*80);
                        CopyMem(canvas.planes[plane]+495*80,bitmap.Planes[plane]+495*80,17*80);
                    }
                    BltBitMapRastPort(&bitmap,230,116,window->RPort,230,116,408,39,0xc0);
                    BltBitMapRastPort(&bitmap,14,216,window->RPort,14,216,80,18,0xc0);
                    BltBitMapRastPort(&bitmap,610,227,window->RPort,610,227,24,10,0xc0);
                    BltBitMapRastPort(&bitmap,248,495,window->RPort,248,495,168,17,0xc0);WaitBlit();
                    printf("EDITOR REPLAY active=%u ticks=%lu order=%u pattern=%u row=%u bpm=%u speed=%u period=%u,%u,%u,%u volume=%u,%u,%u,%u\n",
                        editor->playback.active,(unsigned long)editor->playback.ticks,editor->playback.order,editor->playback.pattern,
                        editor->playback.row,editor->playback.bpm,editor->playback.speed,
                        editor->playback.period[0],editor->playback.period[1],editor->playback.period[2],editor->playback.period[3],
                        editor->playback.volume[0],editor->playback.volume[1],editor->playback.volume[2],editor->playback.volume[3]);fflush(stdout);
                }
                continue;
            }
            if(kind==IDCMP_RAWKEY && (code&0x80 || code>=0x60))continue;
            if(kind==IDCMP_RAWKEY)action=pt_editor_key(editor,code,qualifier);
            else if(kind==IDCMP_MOUSEBUTTONS && code==SELECTDOWN)action=pt_editor_click(editor,mx,my);
            else if(kind==IDCMP_REFRESHWINDOW) {BeginRefresh(window);EndRefresh(window,TRUE);}
            else if(kind==IDCMP_INACTIVEWINDOW)editor->quit_pending=0;
            if(action==PT_UI_PLAY || action==PT_UI_PATTERN) {
                error=pt_paula_play(&audio,editor->project,action==PT_UI_PATTERN,editor->position,editor->pattern);
                pt_editor_status(editor,error?error:action==PT_UI_PATTERN?"PLAYING PATTERN - PAULA CIA":"PLAYING SONG - PAULA CIA");
            }
            if(action==PT_UI_AUDITION) {
                error=pt_paula_audition(&audio,editor->project,editor->sample,856U>>editor->octave);
                pt_editor_status(editor,error?error:"SAMPLE AUDITION - PAULA; STOP TO RELEASE");
            }
            if(action==PT_UI_STOP) {pt_paula_stop(&audio);pt_editor_status(editor,"STOPPED - AUDIO RELEASED");}
            if(editor->history.revision!=revision && audio.started && audio.mode!=2) {
                error=pt_paula_sync(&audio,editor->project);if(error)pt_editor_status(editor,error);
            }
            if(error || action==PT_UI_PLAY || action==PT_UI_PATTERN || action==PT_UI_AUDITION || action==PT_UI_STOP)
                pt_paula_poll(&audio,&editor->playback);
            if(action==PT_UI_SAVE)save(editor,argc==3?argv[2]:NULL);
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
    free(editor);pt_document_release(&doc);return rc;
}
