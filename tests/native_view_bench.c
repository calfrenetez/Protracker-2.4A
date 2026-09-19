#include <proto/dos.h>
#include <stdio.h>
#include <stdlib.h>
#include "document.h"
#include "../src/editor/view.h"
#include "pt_font.h"
static void *allocate(void *c,size_t n) {(void)c;return malloc(n);}
static void release(void *c,void *p) {(void)c;free(p);}
static long elapsed(const struct DateStamp *a,const struct DateStamp *b)
{return (b->ds_Days-a->ds_Days)*4320000L+(b->ds_Minute-a->ds_Minute)*3000L+b->ds_Tick-a->ds_Tick;}
int main(int argc,char **argv)
{
    struct pt_allocator allocator={NULL,allocate,release};struct pt_document doc;
    struct pt_editor *e=NULL;struct pt_canvas c;struct DateStamp a,b;FILE *f=NULL;
    unsigned plane,frame;uint8_t *input=NULL,*pixels=NULL;long n;uint32_t hash=2166136261UL;size_t j;
    pt_document_init(&doc,&allocator);
    if(argc!=2 || !(f=fopen(argv[1],"rb")))return 20;
    if(fseek(f,0,SEEK_END) || (n=ftell(f))<=0 || !(input=malloc(n)))return 20;
    rewind(f);if(fread(input,1,n,f)!=(size_t)n)return 20;fclose(f);
    if(pt_document_load(&doc,input,n,SIZE_MAX)!=PT_PROJECT_OK || !(e=malloc(sizeof(*e))) || !pt_editor_init(e,&doc.project))return 20;
    pixels=malloc(4UL*PT_VIEW_PLANE_BYTES);if(!pixels)return 20;
    for(plane=0;plane<4;++plane)c.planes[plane]=pixels+plane*PT_VIEW_PLANE_BYTES;
    DateStamp(&a);for(frame=0;frame<3;++frame)pt_editor_draw(e,&c,pt_font);DateStamp(&b);
    for(j=0;j<4UL*PT_VIEW_PLANE_BYTES;++j) {hash^=pixels[j];hash*=16777619UL;}
    printf("VIEW PASS frames=3 ticks50=%ld pixel_fnv=%08lx\n",elapsed(&a,&b),(unsigned long)hash);
    free(pixels);free(e);free(input);pt_document_release(&doc);return 0;
}
