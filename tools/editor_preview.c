/* Real project data for layout review, not audio or simulated live scopes. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "document.h"
#include "../src/editor/view.h"
static void *allocate(void *c,size_t n) {(void)c;return malloc(n);}
static void release(void *c,void *p) {(void)c;free(p);}
static unsigned char *read_file(const char *path,size_t *size)
{
    FILE *f=fopen(path,"rb");long n;unsigned char *data;
    if(!f)return NULL;
    if(fseek(f,0,SEEK_END) || (n=ftell(f))<=0) {fclose(f);return NULL;}
    rewind(f);data=malloc((size_t)n);if(!data) {fclose(f);return NULL;}
    if(fread(data,1,(size_t)n,f)!=(size_t)n) {free(data);fclose(f);return NULL;}
    fclose(f);*size=(size_t)n;return data;
}
int main(int argc,char **argv)
{
    struct pt_allocator a={NULL,allocate,release};struct pt_document d;struct pt_editor *e;
    struct pt_canvas canvas;unsigned char *data,*font;size_t n,fn,w;unsigned r,c,i,x,y,pen;FILE *file;
    static const unsigned periods[12]={428,381,339,320,285,254,226,214,190,170,160,143};
    if(argc!=5)return 20;
    data=read_file(argv[1],&n);font=read_file(argv[2],&fn);if(!data || !font || fn!=580)return 20;
    pt_document_init(&d,&a);if(pt_document_load(&d,data,n,SIZE_MAX)!=PT_PROJECT_OK)return 20;free(data);
    if(d.project.channels.count!=16 || d.project.sample_count<3)return 20;
    strcpy(d.project.title,"PROTRACKER 2.4G LAYOUT STUDY");
    strcpy(d.project.samples[0].name,"SYNTHETIC DISPLAY FIXTURE");
    memset(d.project.events,0,(size_t)d.project.pattern_count*64*16*sizeof(*d.project.events));
    for(r=0;r<20;++r)for(c=0;c<16;++c) {
        struct pt_event *event=&d.project.events[r*16+c];
        if(r%3!=1) {event->kind=PT_NOTE_PERIOD;event->pitch=(uint16_t)periods[(r/2+c*3)%12];event->instrument=(uint8_t)(1+c%3);}
    }
    d.project.events[0].effect=12;d.project.events[0].parameter=48;d.project.channels.selected=0;
    if(pt_project_size(&d.project,&n)!=PT_PROJECT_OK || !(data=malloc(n)))return 20;
    if(pt_project_encode(&d.project,data,n,&w)!=PT_PROJECT_OK || n!=w)return 20;
    file=fopen(argv[3],"wb");if(!file || fwrite(data,1,n,file)!=n || fclose(file))return 20;free(data);
    e=malloc(sizeof(*e));if(!e || !pt_editor_init(e,&d.project))return 20;
    for(i=0;i<4;++i) {canvas.planes[i]=malloc(PT_VIEW_PLANE_BYTES);if(!canvas.planes[i])return 20;}
    pt_editor_draw(e,&canvas,font);file=fopen(argv[4],"wb");if(!file)return 20;
    fprintf(file,"P6\n640 512\n255\n");
    for(y=0;y<512;++y)for(x=0;x<640;++x) {
        pen=0;for(i=0;i<4;++i)if(canvas.planes[i][y*80+x/8]&(128>>(x&7)))pen|=1U<<i;
        fputc(((pt_view_palette[pen]>>8)&15)*17,file);fputc(((pt_view_palette[pen]>>4)&15)*17,file);fputc((pt_view_palette[pen]&15)*17,file);
    }
    if(fclose(file))return 20;
    for(i=0;i<4;++i)free(canvas.planes[i]);
    free(font);free(e);pt_document_release(&d);puts("Layout fixture and actual renderer preview written");return 0;
}
