/* Finite, no-argument, offscreen-only profile-1 diagnostic. No device ownership. */
#include <proto/dos.h>
#include <proto/exec.h>
#include <dos/dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "document.h"
#include "../src/editor/view.h"
#include "pt_font.h"
static void *allocate(void *c,size_t n) {(void)c;return malloc(n);}
static void release(void *c,void *p) {(void)c;free(p);}
static long elapsed(const struct DateStamp *a,const struct DateStamp *b)
{return (b->ds_Days-a->ds_Days)*4320000L+(b->ds_Minute-a->ds_Minute)*3000L+b->ds_Tick-a->ds_Tick;}
static int interrupted(void) {return (SetSignal(0,0)&SIGBREAKF_CTRL_C)!=0;}
int main(void)
{
    struct pt_allocator allocator={NULL,allocate,release};struct pt_document doc;
    struct pt_editor *e=NULL;struct pt_canvas canvas;struct DateStamp a,b;
    struct pt_view_cache cache={0};struct pt_view_rect areas[PT_VIEW_DIRTY_MAX];
    unsigned plane,frame;uint8_t *pixels=NULL,*reference=NULL;int live=0,rc=20;
    long full_ticks=0,partial_ticks=0;unsigned char mod[2108]={0};
    pt_document_init(&doc,&allocator);
    memcpy(mod,"VIEW OFFSCREEN PROBE",20);mod[950]=1;memcpy(mod+1080,"M.K.",4);
    /* Distinct visible notes, without samples, files or playback. */
    for(frame=0;frame<64;frame++)for(plane=0;plane<4;plane++) {
        unsigned at=1084+frame*16+plane*4;mod[at]=1;mod[at+1]=(frame&1)?172:200;
    }
    if(pt_document_load(&doc,mod,sizeof(mod),SIZE_MAX)!=PT_PROJECT_OK)goto done;
    e=calloc(1,sizeof(*e));pixels=malloc(4UL*PT_VIEW_PLANE_BYTES);
    reference=malloc(4UL*PT_VIEW_PLANE_BYTES);
    if(!e || !pixels || !reference || !pt_editor_init(e,&doc.project))goto done;
    live=1;for(plane=0;plane<4;plane++)canvas.planes[plane]=pixels+plane*PT_VIEW_PLANE_BYTES;
    DateStamp(&a);
    for(frame=0;frame<24;frame++) {
        if(interrupted())goto done;
        pt_editor_key(e,0x4d,0);pt_editor_draw(e,&canvas,pt_font);
    }
    DateStamp(&b);full_ticks=elapsed(&a,&b);memcpy(reference,pixels,4UL*PT_VIEW_PLANE_BYTES);
    pt_editor_dispose(e);live=0;
    if(!pt_editor_init(e,&doc.project))goto done;
    live=1;pt_editor_draw_update(e,&canvas,pt_font,&cache,areas);DateStamp(&a);
    for(frame=0;frame<24;frame++) {
        if(interrupted())goto done;
        pt_editor_key(e,0x4d,0);pt_editor_draw_update(e,&canvas,pt_font,&cache,areas);
    }
    DateStamp(&b);partial_ticks=elapsed(&a,&b);
    if(full_ticks<0 || partial_ticks<0 || memcmp(reference,pixels,4UL*PT_VIEW_PLANE_BYTES))goto done;
    /* Performance is a measurement, not an arbitrary pass/fail threshold. */
    printf("PT24G VIEW steps=24 full_ticks50=%ld partial_ticks50=%ld identical=1\n",full_ticks,partial_ticks);
    rc=0;
done:
    if(live)pt_editor_dispose(e);
    free(reference);free(pixels);free(e);pt_document_release(&doc);
    puts(rc?"PT24G VIEW FAIL":"PT24G VIEW PASS");return rc;
}
