#ifndef PT_NATIVE_PRESENT_H
#define PT_NATIVE_PRESENT_H
#include <proto/exec.h>
#include <proto/graphics.h>
#include "../editor/view.h"
/* Called after the previous blit completed. The cached canvas is authoritative;
   staging rows not included in this update may be stale. */
static inline void pt_native_present(const struct pt_canvas *canvas,struct BitMap *bitmap,
    struct RastPort *destination,const struct pt_view_rect *areas,unsigned n,int scroll)
{
    unsigned i,plane;
    for(i=0;i<n;++i) {
        const struct pt_view_rect *area=&areas[i];
        UBYTE mask=destination->Mask;
        if(area->x==0 && area->y==PT_EDITOR_PATTERN_Y && area->height==PT_EDITOR_ROWS*12 &&
           scroll && scroll>-PT_EDITOR_ROWS && scroll<PT_EDITOR_ROWS) {
            /* Move the displayed rows with the blitter. Following dirty
               row rectangles repair exposed rows, edits and highlights.
               Avoid copying the entire pattern through Chip RAM again. */
            ClipBlit(destination,0,PT_EDITOR_PATTERN_Y+(scroll>0?scroll:0)*12,
                     destination,0,PT_EDITOR_PATTERN_Y+(scroll<0?-scroll:0)*12,
                     640,(PT_EDITOR_ROWS-(scroll<0?-scroll:scroll))*12,0xc0);
            WaitBlit();continue;
        }
        /* Scope animation changes only the yellow (0 and 2) planes.
           BltBitMapRastPort honours the destination's write mask. */
        if(area->y==PT_EDITOR_SCOPE_Y && area->height==PT_EDITOR_SCOPE_BOTTOM-PT_EDITOR_SCOPE_Y)
            destination->Mask=mask&5;
        for(plane=0;plane<4;++plane)if(destination->Mask&(1U<<plane))CopyMem(canvas->planes[plane]+area->y*80,
            bitmap->Planes[plane]+area->y*80,area->height*80);
        BltBitMapRastPort(bitmap,area->x,area->y,destination,area->x,area->y,area->width,area->height,0xc0);WaitBlit();
        destination->Mask=mask;
    }
}
#endif
