#include <proto/dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "document.h"
#include "../src/editor/view.h"
#include "pt_font.h"
#include "../src/native/present.h"
#include <graphics/gfxbase.h>
struct GfxBase *GfxBase;
static int present_test(struct pt_editor *e,struct pt_canvas *c)
{
    struct BitMap staging={0},target={0};struct RastPort port;
    struct pt_view_cache cache={0};struct pt_view_rect areas[PT_VIEW_DIRTY_MAX];
    unsigned frame=0,plane,j,n;int ok=0,scroll;
    GfxBase=(struct GfxBase *)OpenLibrary("graphics.library",36);if(!GfxBase)return 0;
    InitBitMap(&staging,4,640,512);InitBitMap(&target,4,640,512);
    for(plane=0;plane<4;++plane) {
        staging.Planes[plane]=AllocRaster(640,512);target.Planes[plane]=AllocRaster(640,512);
        if(!staging.Planes[plane] || !target.Planes[plane])goto done;
    }
    InitRastPort(&port);port.BitMap=&target;
    for(frame=0;frame<50;++frame) {
        e->first_row=frame<24?frame:frame<48?47-frame:frame==48?44:0;e->row=e->first_row+9;
        e->playback.active=frame%9!=0;e->playback.row=e->row;e->playback.bpm=frame%11?125:150;
        for(plane=0;plane<4;++plane) {
            e->playback.volume[plane]=(frame+plane)%65;
            for(j=0;j<81;++j)e->playback.wave[plane][j]=(int8_t)((frame*3+j*7+plane*11)%256-128);
        }
        scroll=(int)e->first_row-(int)cache.first_row;
        n=pt_editor_draw_update(e,c,pt_font,&cache,areas);
        pt_native_present(c,&staging,&port,areas,n,scroll);
        for(plane=0;plane<4;++plane)if(memcmp(target.Planes[plane],c->planes[plane],PT_VIEW_PLANE_BYTES))goto done;
        if(port.Mask!=255)goto done;
    }
    ok=1;
done:
    WaitBlit();
    for(plane=0;plane<4;++plane) {
        if(staging.Planes[plane])FreeRaster(staging.Planes[plane],640,512);
        if(target.Planes[plane])FreeRaster(target.Planes[plane],640,512);
    }
    CloseLibrary((struct Library *)GfxBase);GfxBase=NULL;
    memset(&e->playback,0,sizeof(e->playback));e->row=e->first_row=0;
    printf("VIEW PRESENT states=%u identical=%u mask_restored=%u\n",frame,ok,ok);return ok;
}
static void *allocate(void *c,size_t n) {(void)c;return malloc(n);}
static void release(void *c,void *p) {(void)c;free(p);}
static long elapsed(const struct DateStamp *a,const struct DateStamp *b)
{return (b->ds_Days-a->ds_Days)*4320000L+(b->ds_Minute-a->ds_Minute)*3000L+b->ds_Tick-a->ds_Tick;}
static uint32_t pixels_hash(const uint8_t *pixels)
{size_t j;uint32_t h=2166136261UL;for(j=0;j<4UL*PT_VIEW_PLANE_BYTES;++j) {h^=pixels[j];h*=16777619UL;}return h;}
int main(int argc,char **argv)
{
    struct pt_allocator allocator={NULL,allocate,release};struct pt_document doc;
    struct pt_editor *e=NULL;struct pt_canvas c;struct DateStamp a,b;FILE *f=NULL;
    struct pt_view_cache cache={0};struct pt_view_rect areas[PT_VIEW_DIRTY_MAX];long full_ticks,partial_ticks;
    unsigned plane,frame;uint8_t *input=NULL,*pixels=NULL;long n;uint32_t hash,full_hash;
    pt_document_init(&doc,&allocator);
    if(argc!=2 || !(f=fopen(argv[1],"rb")))return 20;
    if(fseek(f,0,SEEK_END) || (n=ftell(f))<=0 || !(input=malloc(n)))return 20;
    rewind(f);if(fread(input,1,n,f)!=(size_t)n)return 20;fclose(f);
    if(pt_document_load(&doc,input,n,SIZE_MAX)!=PT_PROJECT_OK || !(e=malloc(sizeof(*e))) || !pt_editor_init(e,&doc.project))return 20;
    pixels=malloc(4UL*PT_VIEW_PLANE_BYTES);if(!pixels)return 20;
    for(plane=0;plane<4;++plane)c.planes[plane]=pixels+plane*PT_VIEW_PLANE_BYTES;
    if(!present_test(e,&c))return 20;
    DateStamp(&a);for(frame=0;frame<3;++frame)pt_editor_draw(e,&c,pt_font);DateStamp(&b);
    hash=pixels_hash(pixels);
    printf("VIEW PASS frames=3 ticks50=%ld pixel_fnv=%08lx\n",elapsed(&a,&b),(unsigned long)hash);
    DateStamp(&a);for(frame=0;frame<6;++frame) {pt_editor_key(e,0x4d,0);pt_editor_draw(e,&c,pt_font);}DateStamp(&b);
    full_ticks=elapsed(&a,&b);full_hash=pixels_hash(pixels);
    pt_editor_init(e,&doc.project);pt_editor_draw_update(e,&c,pt_font,&cache,areas);
    DateStamp(&a);for(frame=0;frame<6;++frame) {pt_editor_key(e,0x4d,0);pt_editor_draw_update(e,&c,pt_font,&cache,areas);}DateStamp(&b);
    partial_ticks=elapsed(&a,&b);hash=pixels_hash(pixels);
    printf("VIEW CURSOR steps=6 full_ticks50=%ld partial_ticks50=%ld pixel_fnv=%08lx identical=%u\n",full_ticks,partial_ticks,(unsigned long)hash,hash==full_hash);
    if(hash!=full_hash || partial_ticks>=full_ticks)return 20;
    DateStamp(&a);for(frame=0;frame<24;++frame) {e->row=frame+19;e->first_row=frame;pt_editor_draw(e,&c,pt_font);}DateStamp(&b);
    full_ticks=elapsed(&a,&b);full_hash=pixels_hash(pixels);
    cache.valid=0;e->row=19;e->first_row=0;pt_editor_draw_update(e,&c,pt_font,&cache,areas);
    DateStamp(&a);for(frame=0;frame<24;++frame) {e->row=frame+19;e->first_row=frame;pt_editor_draw_update(e,&c,pt_font,&cache,areas);}DateStamp(&b);
    partial_ticks=elapsed(&a,&b);hash=pixels_hash(pixels);
    printf("VIEW SCROLL steps=24 full_ticks50=%ld partial_ticks50=%ld pixel_fnv=%08lx identical=%u\n",full_ticks,partial_ticks,(unsigned long)hash,hash==full_hash);
    free(pixels);free(e);free(input);pt_document_release(&doc);return hash==full_hash && partial_ticks<full_ticks?0:20;
}
