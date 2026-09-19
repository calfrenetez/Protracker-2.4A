#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "document.h"
#include "../src/editor/view.h"
static void *allocate(void *c,size_t n) {(void)c;return malloc(n);}
static void release(void *c,void *p) {(void)c;free(p);}
static uint8_t *readfile(const char *path,size_t *n)
{
    FILE *f=fopen(path,"rb");long z;uint8_t *b;assert(f);assert(!fseek(f,0,SEEK_END));z=ftell(f);assert(z>0);
    rewind(f);*n=(size_t)z;b=malloc(*n);assert(b);assert(fread(b,1,*n,f)==*n);assert(!fclose(f));return b;
}
static void ppm(const char *path,const struct pt_canvas *canvas)
{
    unsigned x,y,p,pen;FILE *f=fopen(path,"wb");assert(f);fprintf(f,"P6\n640 512\n255\n");
    for(y=0;y<512;++y)for(x=0;x<640;++x) {
        pen=0;for(p=0;p<4;++p)if(canvas->planes[p][y*80+x/8]&(128>>(x&7)))pen|=1U<<p;
        fputc(((pt_view_palette[pen]>>8)&15)*17,f);fputc(((pt_view_palette[pen]>>4)&15)*17,f);fputc((pt_view_palette[pen]&15)*17,f);
    }
    assert(!fclose(f));
}
int main(int argc,char **argv)
{
    struct pt_allocator a={NULL,allocate,release};struct pt_document doc;struct pt_editor *e;
    struct pt_canvas canvas;struct pt_event old;uint8_t *input,*font;size_t n,fn;unsigned i;
    assert(argc==4);input=readfile(argv[1],&n);font=readfile(argv[2],&fn);assert(fn==580);
    pt_document_init(&doc,&a);assert(pt_document_load(&doc,input,n,SIZE_MAX)==PT_PROJECT_OK);free(input);
    e=malloc(sizeof(*e));assert(e && pt_editor_init(e,&doc.project));assert(!pt_editor_dirty(e));
    /* Bank selection, wrap, row scrolling and classic/MIDI note entry use the
       same controller as actual native Intuition events. */
    pt_editor_key(e,0x50,0);assert(doc.project.channels.selected==0);
    pt_editor_key(e,0x42,1);assert(doc.project.channels.selected==15);
    pt_editor_key(e,0x42,0);assert(doc.project.channels.selected==0);
    pt_editor_key(e,0x4c,0);assert(e->row==63 && e->first_row==44);
    pt_editor_key(e,0x4d,0);assert(e->row==0 && e->first_row==0);
    old=doc.project.events[0];pt_editor_key(e,0x31,0);assert(!memcmp(&old,&doc.project.events[0],sizeof(old)));
    pt_editor_key(e,0x40,0);pt_editor_key(e,0x31,0);
    assert(doc.project.events[0].kind==PT_NOTE_PERIOD && doc.project.events[0].pitch==428 && pt_editor_dirty(e));
    pt_editor_key(e,0x31,8);assert(!memcmp(&old,&doc.project.events[0],sizeof(old)) && !pt_editor_dirty(e));
    pt_editor_key(e,0x31,9);assert(pt_editor_dirty(e));
    pt_editor_saved(e);assert(!pt_editor_dirty(e));
    pt_editor_click(e,305,500);assert(doc.project.channels.selected==12);
    for(i=0;i<3;++i)pt_editor_key(e,0x42,0);assert(doc.project.channels.selected==15);
    pt_editor_click(e,490,250);assert(e->row==0 && e->field==0);
    pt_editor_key(e,0x31,0);assert(doc.project.events[15].kind==PT_NOTE_MIDI && doc.project.events[15].pitch==24);
    pt_editor_key(e,0x41,0);assert(doc.project.events[31].kind==PT_NOTE_OFF);
    pt_editor_key(e,0x4c,0);pt_editor_key(e,0x46,0);assert(doc.project.events[31].kind==PT_NOTE_NONE);
    /* A sample above the available count is refused without changing event. */
    pt_editor_click(e,36+3*150+53,250);assert(e->field==1);old=doc.project.events[15];
    pt_editor_key(e,1,0);assert(!memcmp(&old,&doc.project.events[15],sizeof(old)));
    assert(pt_editor_key(e,0x21,8)==PT_UI_SAVE);
    assert(pt_editor_click(e,580,500)==PT_UI_NONE && e->quit_pending);
    pt_editor_key(e,0x4d,0);assert(!e->quit_pending);
    assert(pt_editor_key(e,0x45,0)==PT_UI_NONE && pt_editor_key(e,0x45,0)==PT_UI_QUIT);
    e->quit_pending=0;assert(pt_editor_click(e,580,500)==PT_UI_NONE && pt_editor_click(e,580,500)==PT_UI_QUIT);
    for(i=0;i<4;++i) {canvas.planes[i]=malloc(PT_VIEW_PLANE_BYTES);assert(canvas.planes[i]);}
    pt_editor_status(e,"EDITOR DEVELOPMENT - AUDIO NOT CONNECTED");e->quit_pending=0;
    for(i=0;i<4;++i) {pt_editor_key(e,0x50+i,0);pt_editor_draw(e,&canvas,font);}
    ppm(argv[3],&canvas);
    for(i=0;i<4;++i)free(canvas.planes[i]);
    free(font);free(e);pt_document_release(&doc);
    puts("EDITOR PASS: bank/wrap/scroll, guarded note and nibble edits, OFF, undo/redo, save state, discard confirmation, all-page planar render");return 0;
}
