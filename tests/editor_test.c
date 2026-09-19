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
static void blocks(struct pt_editor *e)
{
    struct pt_project *p=e->project;struct pt_editor_selection s;struct pt_event original[4],saved;
    size_t total=(size_t)p->pattern_count*64*p->channels.count,i;uint32_t revision;
    assert(p->channels.count==16 && p->pattern_count>=2);
    memset(p->events,0,total*sizeof(*p->events));p->channels.selected=0;
    for(i=0;i<4;++i) {
        struct pt_event *v=&p->events[(i/2)*16+i%2];v->kind=PT_NOTE_PERIOD;v->pitch=428;v->instrument=1;
        v->effect=12;v->parameter=(uint8_t)(24+i);original[i]=*v;
    }
    assert(pt_editor_init(e,p));
    /* Reverse marking across rows/channels, copy freezes the rectangle and
       navigation after copy leaves it intact. Clipboard is not a dirty edit. */
    e->row=1;p->channels.selected=1;pt_editor_key(e,0x35,8);
    pt_editor_key(e,0x4c,0);pt_editor_key(e,0x42,1);
    assert(pt_editor_selection(e,&s) && s.r0==0 && s.r1==2 && s.c0==0 && s.c1==2);
    pt_editor_key(e,0x33,8);assert(e->clipboard.rows==2 && e->clipboard.channels==2 && !e->selection.marking);
    assert(!pt_editor_dirty(e));for(i=0;i<4;++i)assert(!memcmp(&original[i],&e->clipboard.events[i],sizeof(saved)));
    e->row=10;p->channels.selected=4;assert(pt_editor_selection(e,&s) && s.r0==0 && s.c0==0 && s.r1==2);
    pt_editor_key(e,0x34,8);assert(e->history.count==1 && pt_editor_dirty(e));
    for(i=0;i<4;++i)assert(!memcmp(&original[i],&p->events[(10+i/2)*16+4+i%2],sizeof(saved)));
    pt_editor_key(e,0x31,8);assert(!pt_editor_dirty(e));for(i=0;i<4;++i)assert(p->events[(10+i/2)*16+4+i%2].kind==PT_NOTE_NONE);
    pt_editor_key(e,0x31,9);assert(pt_editor_dirty(e));revision=e->history.revision;
    e->row=63;pt_editor_key(e,0x34,8);assert(e->history.revision==revision && strstr(e->status,"EDGE"));
    e->row=10;p->channels.selected=15;pt_editor_key(e,0x34,8);assert(e->history.revision==revision);
    /* Frozen selection, not the destination cursor, drives transpose/clear. */
    pt_editor_key(e,0x0c,8);assert(p->events[0].pitch==404 && p->events[16].pitch==404);
    assert(p->events[10*16+4].pitch==428 && p->events[0].parameter==24);
    pt_editor_key(e,0x46,8);assert(p->events[0].kind==PT_NOTE_NONE && p->events[16].instrument==0);
    pt_editor_key(e,0x31,8);assert(p->events[0].pitch==404);
    pt_editor_key(e,0x31,8);assert(p->events[0].pitch==428);
    /* One unsupported event refuses the complete transpose without history. */
    p->events[0].pitch=113;assert(pt_editor_init(e,p));pt_editor_key(e,0x20,8);revision=e->history.revision;
    pt_editor_key(e,0x0c,8);assert(p->events[0].pitch==113 && p->events[1].pitch==428 && e->history.revision==revision);
    p->events[0].pitch=123;pt_editor_key(e,0x0b,8);assert(p->events[0].pitch==123 && p->events[1].pitch==428 && !pt_editor_dirty(e));
    /* Whole 16-channel patterns fit bounded storage and one undo transaction.
       Copy + explicitly selecting another pattern implements cloning. */
    for(i=0;i<1024;++i) {p->events[i]=original[i%4];p->events[1024+i]=(struct pt_event){0};}
    p->channels.selected=0;assert(pt_editor_init(e,p));pt_editor_key(e,0x20,8);pt_editor_key(e,0x33,8);
    assert(e->clipboard.rows==64 && e->clipboard.channels==16);
    pt_editor_key(e,0x5b,0);assert(e->pattern==1 && !e->selection.active);
    pt_editor_key(e,0x34,8);assert(!memcmp(p->events,p->events+1024,1024*sizeof(saved)) && e->history.count==1);
    pt_editor_key(e,0x20,8);pt_editor_key(e,0x46,8);assert(e->history.count==2 && e->history.used==2048);
    for(i=1024;i<2048;++i)assert(p->events[i].kind==PT_NOTE_NONE);
    pt_editor_key(e,0x31,8);assert(!memcmp(p->events,p->events+1024,1024*sizeof(saved)));
    pt_editor_key(e,0x31,8);assert(!pt_editor_dirty(e));
    for(i=1024;i<2048;++i)assert(p->events[i].kind==PT_NOTE_NONE);
    /* New-song controls are non-destructive until the caller commits a
       staged document. Other input cancels dirty-discard confirmation. */
    pt_editor_key(e,0x31,9);assert(pt_editor_dirty(e));
    pt_editor_key(e,0x36,8);assert(e->panel==3 && e->new_channels==16);
    assert(pt_editor_key(e,0x44,0)==PT_UI_NONE && e->new_pending);
    pt_editor_key(e,0x0b,0);assert(!e->new_pending && e->new_channels==15);
    assert(pt_editor_key(e,0x44,0)==PT_UI_NONE && e->new_pending);
    assert(pt_editor_key(e,0x44,0)==PT_UI_NEW && !e->new_pending);
    assert(pt_editor_key(e,0x45,0)==PT_UI_NONE && e->panel==0 && pt_editor_dirty(e));
    pt_editor_click(e,400,30);assert(e->panel==3);
    for(i=0;i<20;++i)pt_editor_click(e,250,28);assert(e->new_channels==1);
    for(i=0;i<20;++i)pt_editor_click(e,520,28);assert(e->new_channels==16);
    assert(pt_editor_click(e,250,70)==PT_UI_NONE && e->new_pending);
    pt_editor_click(e,520,70);assert(e->panel==0 && !e->new_pending);
    /* Native panel hit targets use the same actions, and loaded documents
       reset both the clipboard and selection. */
    assert(pt_editor_init(e,p));pt_editor_click(e,400,50);assert(e->panel==1);
    pt_editor_click(e,520,28);assert(e->selection.active && e->selection.marking);
    pt_editor_click(e,250,50);assert(e->clipboard.rows==1 && !e->selection.marking);
    pt_editor_click(e,520,70);assert(e->selection.r1==64 && e->selection.c1==16);
    pt_editor_click(e,250,87);assert(!e->selection.active);
    pt_editor_click(e,400,87);assert(e->panel==0);
    assert(pt_editor_init(e,p));pt_editor_key(e,0x34,8);assert(!e->clipboard.rows && strstr(e->status,"EMPTY") && !pt_editor_dirty(e));
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
    pt_editor_click(e,200,500);assert(doc.project.channels.selected==12);
    for(i=0;i<3;++i)pt_editor_key(e,0x42,0);assert(doc.project.channels.selected==15);
    pt_editor_click(e,494,258);assert(e->row==0 && e->field==0);
    pt_editor_key(e,0x31,0);assert(doc.project.events[15].kind==PT_NOTE_MIDI && doc.project.events[15].pitch==24);
    pt_editor_key(e,0x41,0);assert(doc.project.events[31].kind==PT_NOTE_OFF);
    pt_editor_key(e,0x4c,0);pt_editor_key(e,0x46,0);assert(doc.project.events[31].kind==PT_NOTE_NONE);
    /* A sample above the available count is refused without changing event. */
    pt_editor_click(e,38+3*150+65,258);assert(e->field==1);old=doc.project.events[15];
    pt_editor_key(e,1,0);assert(!memcmp(&old,&doc.project.events[15],sizeof(old)));
    assert(pt_editor_key(e,0x21,8)==PT_UI_SAVE);
    pt_editor_click(e,400,70);assert(e->panel==2);
    assert(pt_editor_click(e,500,40)==PT_UI_NONE && e->quit_pending);
    pt_editor_key(e,0x4d,0);assert(!e->quit_pending);
    assert(pt_editor_key(e,0x45,0)==PT_UI_NONE && pt_editor_key(e,0x45,0)==PT_UI_QUIT);
    e->quit_pending=0;assert(pt_editor_click(e,500,40)==PT_UI_NONE && pt_editor_click(e,500,40)==PT_UI_QUIT);
    pt_editor_click(e,400,80);assert(e->panel==0); /* Disk Op Back */
    pt_editor_click(e,218,30);assert(e->pattern==1);
    pt_editor_click(e,198,30);assert(e->pattern==0);
    pt_editor_click(e,218,88);assert(e->sample==2);
    pt_editor_click(e,198,88);assert(e->sample==1);
    pt_editor_click(e,400,70);assert(e->panel==2 && pt_editor_click(e,300,40)==PT_UI_SAVE_AS);
    pt_editor_click(e,400,80);assert(e->panel==0);
    assert(pt_editor_click(e,600,180)==PT_UI_NONE && e->load_pending);
    pt_editor_key(e,0x4c,0);assert(!e->load_pending);
    assert(pt_editor_key(e,0x18,8)==PT_UI_NONE && e->load_pending);
    assert(pt_editor_key(e,0x18,8)==PT_UI_LOAD && !e->load_pending);
    assert(pt_editor_key(e,0x21,9)==PT_UI_SAVE_AS);
    assert(pt_editor_key(e,0x37,9)==PT_UI_EXPORT_MOD);
    pt_editor_click(e,400,70);assert(e->panel==2);
    assert(pt_editor_click(e,400,65)==PT_UI_EXPORT_MOD);
    pt_editor_click(e,400,85);assert(e->panel==0);
    assert(pt_editor_click(e,250,10)==PT_UI_PLAY);
    assert(pt_editor_click(e,400,10)==PT_UI_STOP);
    assert(pt_editor_click(e,250,30)==PT_UI_PATTERN);
    assert(pt_editor_click(e,250,87)==PT_UI_AUDITION);
    assert(pt_editor_key(e,0x57,0)==PT_UI_PLAY && pt_editor_key(e,0x58,0)==PT_UI_PATTERN);
    e->playback.active=1;assert(pt_editor_key(e,0x40,0)==PT_UI_STOP);e->playback.active=0;
    for(i=0;i<4;++i) {canvas.planes[i]=malloc(PT_VIEW_PLANE_BYTES);assert(canvas.planes[i]);}
    pt_editor_status(e,"EDITOR DEVELOPMENT - AUDIO NOT CONNECTED");e->quit_pending=0;e->panel=0;
    for(i=0;i<4;++i) {pt_editor_key(e,0x50+i,0);pt_editor_draw(e,&canvas,font);}
    e->playback.active=1;e->playback.speed=6;e->playback.bpm=150;
    for(i=0;i<4;++i) {unsigned j;e->playback.volume[i]=64;for(j=0;j<81;++j)e->playback.wave[i][j]=j%2?-128:127;}
    pt_editor_key(e,0x50,0);pt_editor_draw(e,&canvas,font);pt_editor_draw_playback(e,&canvas,font);
    ppm(argv[3],&canvas);
    /* Incremental updates must produce the full renderer's exact pixels, and
       the reported rectangles must cover every changed byte/pixel. */
    {
        struct pt_view_cache cache={0};struct pt_canvas incremental,shown;
        struct pt_view_rect areas[PT_VIEW_DIRTY_MAX];unsigned step,p,j,y,x,n;
        static const unsigned actions[]={0x4d,0x4e,0x4f,0x42,0x4c,0x50,0x51,0x52,0x53,0x40,0x31,0x32,0x46,0x0c,0x0b,0x5a,0x5b};
        for(p=0;p<4;++p) {incremental.planes[p]=calloc(1,PT_VIEW_PLANE_BYTES);shown.planes[p]=calloc(1,PT_VIEW_PLANE_BYTES);assert(incremental.planes[p] && shown.planes[p]);}
        for(step=0;step<100;++step) {
            if(step)pt_editor_key(e,actions[(step-1)%(sizeof(actions)/sizeof(actions[0]))],0);
            if(step%13==0) {e->playback.row=step%64;e->playback.bpm=125+step;e->playback.wave[0][step%81]=(int8_t)step;}
            if(step==30) {pt_editor_click(e,400,70);cache.valid=0;}
            if(step==31)pt_editor_click(e,400,80);
            if(step%17==0)pt_editor_key(e,0x31,8);
            if(step%19==0)pt_editor_key(e,0x35,8);
            if(step%23==0)pt_editor_key(e,0x33,8);
            if(step==68)pt_editor_key(e,0x20,8);
            if(step==70)pt_editor_click(e,400,50);
            if(step==74)pt_editor_click(e,250,87);
            if(step==76)pt_editor_click(e,400,87);
            if(step==80)pt_editor_key(e,0x36,8);
            if(step==81)pt_editor_key(e,0x0b,0);
            if(step==82)pt_editor_key(e,0x44,0);
            if(step==84)pt_editor_key(e,0x45,0);
            pt_editor_draw(e,&canvas,font);n=pt_editor_draw_update(e,&incremental,font,&cache,areas);assert(n<=PT_VIEW_DIRTY_MAX);
            for(j=0;j<n;++j) {
                const struct pt_view_rect *a=&areas[j];assert(a->x+a->width<=640 && a->y+a->height<=512);
                for(y=a->y;y<a->y+a->height;++y)for(x=a->x;x<a->x+a->width;++x)for(p=0;p<4;++p) {
                    uint8_t mask=(uint8_t)(128>>(x&7));size_t offset=y*80+x/8;
                    shown.planes[p][offset]=(shown.planes[p][offset]&~mask)|(incremental.planes[p][offset]&mask);
                }
            }
            for(p=0;p<4;++p) {assert(!memcmp(canvas.planes[p],incremental.planes[p],PT_VIEW_PLANE_BYTES));assert(!memcmp(canvas.planes[p],shown.planes[p],PT_VIEW_PLANE_BYTES));}
        }
        n=pt_editor_draw_update(e,&incremental,font,&cache,areas);assert(!n);
        for(p=0;p<4;++p) {free(incremental.planes[p]);free(shown.planes[p]);}
    }

    blocks(e);
    for(i=0;i<4;++i)free(canvas.planes[i]);
    free(font);free(e);pt_document_release(&doc);
    puts("EDITOR PASS: bank/wrap/scroll, guarded note and nibble edits, OFF, undo/redo, save state, discard confirmation, atomic block copy/paste/clear/transpose/clone, all-page planar render");return 0;
}
