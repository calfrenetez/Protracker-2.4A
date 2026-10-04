#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/editor/view.h"
static void *allocate(void *c,size_t n) {(void)c;return malloc(n);}
static void release(void *c,void *p) {(void)c;free(p);}
static void image(const char *folder,const char *name,const struct pt_canvas *c)
{
    char path[1024];unsigned x,y,p,pen;FILE *f;
    assert(snprintf(path,sizeof(path),"%s/%s.ppm",folder,name)>0);f=fopen(path,"wb");assert(f);
    fprintf(f,"P6\n640 512\n255\n");
    for(y=0;y<512;++y)for(x=0;x<640;++x) {
        pen=0;for(p=0;p<4;++p)if(c->planes[p][y*80+x/8]&(128>>(x&7)))pen|=1U<<p;
        fputc(((pt_view_palette[pen]>>8)&15)*17,f);fputc(((pt_view_palette[pen]>>4)&15)*17,f);fputc((pt_view_palette[pen]&15)*17,f);
    }assert(!fclose(f));
}
static void idle(struct pt_editor *e) {unsigned i;for(i=0;i<1000;++i)if(!pt_editor_workflow_idle(e) && !e->workflow.scanning && !e->workflow.wave_building && !e->workflow.resolving)return;assert(!"view service bound");}
int main(int argc,char **argv)
{
    struct pt_allocator a={NULL,allocate,release};struct pt_document d;struct pt_editor *e=calloc(1,sizeof(*e));
    struct pt_canvas c;uint8_t font[580];int32_t pcm[512];unsigned i;FILE *f;
    assert(argc==3 && e);f=fopen(argv[1],"rb");assert(f && fread(font,1,sizeof(font),f)==sizeof(font) && !fclose(f));
    for(i=0;i<4;++i) {c.planes[i]=calloc(1,PT_VIEW_PLANE_BYTES);assert(c.planes[i]);}
    pt_document_init(&d,&a);assert(pt_document_new(&d,16,SIZE_MAX)==PT_PROJECT_OK);
    for(i=0;i<512;++i)pcm[i]=(int32_t)(i%64)*262143-8388607;
    d.project.samples[0].pcm=(struct pt_pcm){pcm,512,512,44100,1,24};strcpy(d.project.samples[0].name,"24BIT TEST MASTER");
    d.project.samples[0].loop=PT_LOOP_FORWARD;d.project.samples[0].loop_start=128;d.project.samples[0].loop_end=512;
    strcpy(d.project.samples[1].name,"UNUSED NAMED SLOT");d.project.events[0].instrument=1;d.project.events[0].kind=PT_NOTE_PERIOD;d.project.events[0].pitch=428;
    assert(pt_editor_init(e,&d.project));
    pt_editor_key(e,0x28,9);idle(e);pt_editor_draw(e,&c,font);image(argv[2],"manager",&c);
    pt_editor_key(e,0x27,9);pt_editor_key(e,0x16,0);idle(e);pt_editor_draw(e,&c,font);image(argv[2],"toolbox",&c);
    e->panel=1;e->note_details=1;pt_editor_draw(e,&c,font);image(argv[2],"event-navigation",&c);
    pt_editor_key(e,0x44,9);idle(e);pt_editor_draw(e,&c,font);image(argv[2],"opened-master",&c);
    assert(pt_editor_dispose(e));pt_document_release(&d);for(i=0;i<4;++i)free(c.planes[i]);free(e);
    puts("WORKFLOW VIEW PASS: four planar pages generated, no backend or target");return 0;
}
