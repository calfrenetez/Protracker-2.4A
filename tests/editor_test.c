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
static unsigned pixel_pen(const struct pt_canvas *c,unsigned x,unsigned y)
{
    unsigned p,pen=0;
    for(p=0;p<4;++p)if(c->planes[p][y*80+x/8]&(128>>(x&7)))pen|=1U<<p;
    return pen;
}
static void aligned_borders(const struct pt_canvas *c)
{
    static const unsigned columns[]={16,140,200,245,370,490,618};
    unsigned row,i,y;
    /* Independently fixed screen coordinates: shared parameter/command seams,
       uninterrupted header-to-pattern rails, and two distinct strip edges. */
    for(row=0;row<9;++row)if(row!=7)for(i=0;i<sizeof(columns)/sizeof(columns[0]);++i)
        assert(pixel_pen(c,columns[i],2+row*19)==2 ||
               (row==2 && i==3 && pixel_pen(c,columns[i],2+row*19)==3));
    for(i=0;i<5;++i)for(y=234;y<491;++y) {
        assert(pixel_pen(c,36+i*150,y)==2);
        assert(pixel_pen(c,37+i*150,y)==3);
    }
    assert(pixel_pen(c,580,210)==3 && pixel_pen(c,580,211)==2);
    for(row=6;row<9;++row)for(y=2+row*19+2;y<2+row*19+18;++y)
        assert(pixel_pen(c,119,y)==7); /* Five digits must not paint the bevel. */
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
static void channel_controls(struct pt_editor *e)
{
    struct pt_project *p=e->project;struct pt_channel initial;struct pt_event event;
    struct pt_allocator a={NULL,allocate,release};struct pt_document reopened;uint8_t *bytes;size_t n,written;unsigned i;
    pt_channels_init(&p->channels);assert(pt_channels_resize(&p->channels,16)==PT_CHANNEL_OK);
    assert(pt_editor_init(e,p));initial=p->channels.track[0];event=p->events[0];
    pt_editor_key(e,0x13,8);assert(e->panel==4 && !pt_editor_dirty(e));
    assert(pt_editor_key(e,0x44,1)==PT_UI_PATTERN);
    e->editing=1;pt_editor_key(e,0x20,0);assert(p->channels.track[0].route==PT_AMIGUS);
    pt_editor_click(e,500,30);assert(p->channels.track[0].route==PT_MIDI);
    pt_editor_click(e,250,50);pt_editor_key(e,0x21,0);
    assert(p->channels.track[0].muted && p->channels.track[0].solo && e->history.count==4);
    assert(!memcmp(&event,&p->events[0],sizeof(event)) && e->row==0);
    assert(pt_editor_key(e,0x21,8)==PT_UI_SAVE);pt_editor_saved(e);
    assert(pt_project_size(p,&n)==PT_PROJECT_OK);bytes=malloc(n);assert(bytes);
    assert(pt_project_encode(p,bytes,n,&written)==PT_PROJECT_OK && written==n);
    pt_document_init(&reopened,&a);assert(pt_document_load(&reopened,bytes,n,SIZE_MAX)==PT_PROJECT_OK);
    assert(!memcmp(&reopened.project.channels,&p->channels,sizeof(p->channels)));free(bytes);pt_document_release(&reopened);
    pt_editor_click(e,500,65);assert(p->channels.selected==1);
    pt_editor_key(e,0x31,8);assert(!p->channels.track[0].solo && p->channels.selected==1 && pt_editor_dirty(e));
    pt_editor_key(e,0x31,9);assert(!pt_editor_dirty(e) && p->channels.track[0].solo);
    for(i=0;i<4;++i)pt_editor_key(e,0x31,8);
    assert(!memcmp(&initial,&p->channels.track[0],sizeof(initial)));
    pt_editor_key(e,0x51,0);assert(p->channels.selected==4);
    pt_editor_key(e,0x19,0);assert(strstr(e->status,"PAULA LIMIT") && !e->history.cursor && e->history.count==4);
    pt_editor_key(e,0x31,9);assert(p->channels.track[0].route==PT_AMIGUS);
    pt_editor_key(e,0x19,0);assert(p->channels.track[4].route==PT_PAULA && e->history.count==2);
    pt_editor_key(e,0x45,0);assert(!e->panel && !e->quit_pending);
    pt_editor_click(e,38+130,PT_EDITOR_HEADER_Y+3);assert(e->panel==4 && p->channels.selected==4);
    pt_editor_key(e,0x13,8);assert(!e->panel);
    /* Pattern entry and channel changes undo in their actual order. */
    e->field=0;pt_editor_key(e,0x32,0);assert(e->history.count==3);
    pt_editor_key(e,0x13,8);pt_editor_key(e,0x16,0);assert(e->history.count==4);
    pt_editor_key(e,0x31,8);assert(!p->channels.track[4].muted);
    pt_editor_key(e,0x31,8);assert(e->history.cursor==2);
}
static void channel_details(struct pt_editor *e)
{
    struct pt_project *p=e->project;struct pt_channel initial;unsigned i,selected;
    struct pt_allocator a={NULL,allocate,release};struct pt_document reopened;uint8_t *bytes;size_t n,written;
    assert(pt_editor_init(e,p));selected=p->channels.selected;initial=p->channels.track[selected];
    pt_editor_key(e,0x13,8);pt_editor_click(e,500,50);assert(e->channel_details && !pt_editor_dirty(e));
    pt_editor_key(e,0x19,0);assert(e->number_field==5);
    pt_editor_key(e,0x23,0);pt_editor_key(e,0x23,0);pt_editor_key(e,0x23,0);
    pt_editor_key(e,0x44,0);assert(e->number_field==5 && !pt_editor_dirty(e));
    pt_editor_click(e,520,65);pt_editor_key(e,0x42,0);assert(p->channels.selected==selected);
    pt_editor_key(e,0x41,0);pt_editor_key(e,0x44,0);assert(!e->number_field && p->channels.track[selected].pan==255);
    pt_editor_click(e,400,30);assert(e->number_field==6);pt_editor_key(e,0x23,0);pt_editor_key(e,0x44,0);assert(p->channels.track[selected].group==15);
    pt_editor_key(e,0x37,0);pt_editor_key(e,0x0a,0);pt_editor_key(e,0x44,0);assert(e->number_field==7);
    pt_editor_key(e,0x41,0);pt_editor_key(e,1,0);pt_editor_key(e,7,0);pt_editor_key(e,0x44,0);assert(e->number_field==7);
    pt_editor_key(e,0x41,0);pt_editor_key(e,6,0);pt_editor_key(e,0x44,0);assert(p->channels.track[selected].midi_channel==16);
    pt_editor_click(e,250,50);assert(e->name_entry);pt_editor_key(e,0x21,8);assert(e->name_fresh);
    pt_editor_key(e,0x35,0);pt_editor_key(e,0x20,0);pt_editor_key(e,0x21,0);pt_editor_key(e,0x21,0);
    pt_editor_key(e,0x42,0);pt_editor_click(e,520,65);assert(p->channels.selected==selected);
    pt_editor_key(e,0x44,0);assert(!strcmp(p->channels.track[selected].name,"BASS") && !e->name_entry);
    assert(e->history.count==4);pt_editor_saved(e);
    assert(pt_project_size(p,&n)==PT_PROJECT_OK);bytes=malloc(n);assert(bytes);
    assert(pt_project_encode(p,bytes,n,&written)==PT_PROJECT_OK && written==n);
    pt_document_init(&reopened,&a);assert(pt_document_load(&reopened,bytes,n,SIZE_MAX)==PT_PROJECT_OK);
    assert(!memcmp(&reopened.project.channels,&p->channels,sizeof(p->channels)));free(bytes);pt_document_release(&reopened);
    pt_editor_key(e,0x36,0);for(i=0;i<20;++i)pt_editor_key(e,0x32,0);assert(strlen(e->name_text)==15);
    pt_editor_key(e,0x45,0);assert(!strcmp(p->channels.track[selected].name,"BASS") && !pt_editor_dirty(e));
    pt_editor_key(e,0x36,0);pt_editor_key(e,0x46,0);pt_editor_key(e,0x44,0);assert(!p->channels.track[selected].name[0]);
    pt_editor_key(e,0x31,8);assert(!pt_editor_dirty(e) && !strcmp(p->channels.track[selected].name,"BASS"));
    for(i=0;i<4;++i)pt_editor_key(e,0x31,8);
    assert(!memcmp(&p->channels.track[selected],&initial,sizeof(initial)));
    for(i=0;i<4;++i)pt_editor_key(e,0x31,9);
    assert(!pt_editor_dirty(e));
    pt_editor_key(e,0x19,0);pt_editor_key(e,0x45,0);assert(!pt_editor_dirty(e));
    pt_editor_key(e,0x45,0);assert(e->panel==4 && !e->channel_details);
    pt_editor_key(e,0x22,0);assert(e->channel_details);pt_editor_click(e,520,87);assert(!e->channel_details);
    pt_editor_key(e,0x45,0);assert(!e->panel);
}
static void sampler_controls(struct pt_editor *e)
{
    struct pt_project *p=e->project;const struct pt_pcm *pcm;int32_t *before;size_t count;unsigned i,c;
    assert(pt_editor_init(e,p));pcm=&p->samples[0].pcm;count=(size_t)pcm->frames*pcm->channels;
    assert(count);before=malloc(count*sizeof(*before));assert(before);memcpy(before,pcm->data,count*sizeof(*before));
    {
        char name[32];unsigned i,volume=p->samples[0].volume;int fine=p->samples[0].finetune;
        memcpy(name,p->samples[0].name,sizeof(name));pt_editor_click(e,200,200);assert(e->name_entry==2);
        for(i=0;i<40;++i)pt_editor_key(e,0x32,0);assert(strlen(e->name_text)==31);
        pt_editor_click(e,520,65);pt_editor_key(e,0x42,0);assert(e->sample==1 && e->name_entry==2);
        pt_editor_key(e,0x45,0);assert(!strcmp(p->samples[0].name,name) && !pt_editor_dirty(e));
        pt_editor_key(e,0x36,9);assert(e->name_entry==2);pt_editor_key(e,0x35,0);pt_editor_key(e,0x20,0);pt_editor_key(e,0x21,0);pt_editor_key(e,0x21,0);pt_editor_key(e,0x44,0);
        assert(!strcmp(p->samples[0].name,"BASS") && !memcmp(p->samples[0].pcm.data,before,count*sizeof(*before)));
        pt_editor_key(e,0x31,8);assert(!strcmp(p->samples[0].name,name));
        pt_editor_click(e,volume?200:220,104);assert(p->samples[0].volume==(volume?volume-1:1));
        pt_editor_key(e,0x31,8);assert(p->samples[0].volume==volume);
        pt_editor_click(e,fine==7?200:220,65);assert(p->samples[0].finetune==(fine==7?6:fine+1));
        pt_editor_key(e,0x31,8);assert(p->samples[0].finetune==fine && !pt_editor_dirty(e));
    }
    pt_editor_click(e,500,70);assert(e->panel==5);
    assert(pt_editor_click(e,250,30)==PT_UI_SAMPLE_LOAD);
    e->load_pending=1;e->quit_pending=1;
    assert(pt_editor_key(e,0x18,9)==PT_UI_SAMPLE_LOAD && !e->load_pending && !e->quit_pending);
    assert(pt_editor_click(e,400,30)==PT_UI_SAMPLE_SAVE);
    assert(pt_editor_key(e,0x11,1)==PT_UI_SAMPLE_SVX);
    assert(pt_editor_key(e,0x11,0)==PT_UI_SAMPLE_SAVE);
    pt_editor_key(e,0x42,0);assert(e->panel==6);
    assert(pt_editor_click(e,520,65)==PT_UI_SAMPLE_SVX && e->panel==6);
    assert(pt_editor_key(e,0x11,0)==PT_UI_SAMPLE_SVX);
    pt_editor_click(e,250,10);assert(e->panel==5);
    assert(pt_editor_click(e,500,30)==PT_UI_AUDITION);
    {
        unsigned revision=e->history.revision,sample=e->sample;
        pt_editor_click(e,520,87);assert(e->panel==10 && e->raw_format.bits==8 && e->raw_format.rate==8287);
        assert(pt_editor_click(e,250,30)==PT_UI_RAW_LOAD && pt_editor_key(e,0x11,0)==PT_UI_RAW_SAVE);
        pt_editor_click(e,520,65);assert(e->raw_format.unsigned8);
        pt_editor_key(e,3,0);assert(e->raw_format.bits==24 && !e->raw_format.unsigned8);
        pt_editor_key(e,0x16,0);assert(!e->raw_format.unsigned8);
        pt_editor_key(e,0x21,0);pt_editor_click(e,400,87);assert(e->raw_format.channels==2 && e->raw_format.little_endian);
        pt_editor_key(e,0x12,0);assert(!e->raw_format.little_endian);
        e->sample=0;pt_editor_key(e,0x13,0);assert(e->number_field==4); /* Empty/no selection may set import rate. */
        pt_editor_key(e,0x0a,0);pt_editor_key(e,0x44,0);assert(e->number_field==4 && e->raw_format.rate==8287);
        assert(pt_editor_key(e,0x28,0)==PT_UI_NONE && e->number_field==4);
        pt_editor_key(e,0x45,0);e->sample=sample;pt_editor_key(e,0x13,0);
        pt_editor_key(e,4,0);pt_editor_key(e,8,0);pt_editor_key(e,0x0a,0);pt_editor_key(e,0x0a,0);pt_editor_key(e,0x0a,0);pt_editor_key(e,0x44,0);
        assert(!e->number_field && e->raw_format.rate==48000 && e->history.revision==revision);
        pt_editor_click(e,520,30);assert(e->panel==5);pt_editor_key(e,0x32,0);assert(e->panel==10);
        pt_editor_key(e,0x42,0);assert(e->panel==5);
    }
    e->editing=1;pt_editor_key(e,0x31,0);assert(!pt_editor_dirty(e));
    pt_editor_click(e,10,260);assert(e->sample_marking);
    pt_editor_key(e,0x13,0);assert(!pt_editor_dirty(e) && strstr(e->status,"FINISH"));
    pt_editor_click(e,629,260);assert(!e->sample_marking && !e->sample_start && e->sample_end==pcm->frames);
    pt_editor_click(e,250,50);assert(pt_editor_dirty(e));pcm=&p->samples[0].pcm;
    for(i=0;i<pcm->frames;++i)for(c=0;c<pcm->channels;++c)
        assert(pcm->data[(size_t)i*pcm->channels+c]==before[(size_t)(pcm->frames-1-i)*pcm->channels+c]);
    pt_editor_key(e,0x31,8);assert(!memcmp(pcm->data,before,count*sizeof(*before)) && !pt_editor_dirty(e));
    pt_editor_key(e,0x31,9);pt_editor_saved(e);assert(!pt_editor_dirty(e));
    pt_editor_click(e,250,88);assert(pt_editor_dirty(e));
    pt_editor_key(e,0x31,8);assert(!pt_editor_dirty(e));
    pt_editor_click(e,10,260);assert(e->sample_marking);
    pt_editor_click(e,218,88);assert(e->sample==2 && e->sample_range_slot==2 && !e->sample_marking);
    pt_editor_key(e,0x0b,0);assert(e->sample==1 && e->sample_end==pcm->frames);
    for(i=0;i<(unsigned)p->pattern_count*64*p->channels.count;++i)p->events[i].slice=0;
    pt_editor_click(e,400,10);assert(e->panel==6);
    pt_editor_click(e,250,30);assert(p->samples[0].loop==PT_LOOP_FORWARD && p->samples[0].loop_end==pcm->frames);
    pt_editor_key(e,0x19,0);assert(p->samples[0].loop==PT_LOOP_PINGPONG);
    e->loop_fade=1;pt_editor_key(e,0x35,0);assert(p->samples[0].loop==PT_LOOP_FORWARD && p->samples[0].loop_start==1);
    pt_editor_key(e,0x31,8);assert(p->samples[0].loop==PT_LOOP_PINGPONG && !p->samples[0].loop_start);
    pt_editor_click(e,500,10);assert(e->panel==7);
    {unsigned revision=e->history.revision;
        pt_editor_key(e,0x33,0);assert(e->slice_pending && !e->slice_count && e->history.revision==revision);
        pt_editor_key(e,0x37,0);assert(e->slice_count==1 && e->slice_markers[0]==0);
        pt_editor_click(e,400,50);assert(!e->slice_pending && p->samples[0].slice_count==1);
        pt_editor_key(e,0x31,8);revision=e->history.revision;
        pt_editor_key(e,0x14,0);assert(e->slice_pending && e->slice_count && !e->slice_markers[0]);
        pt_editor_click(e,500,50);assert(!e->slice_pending && e->history.revision==revision);
        pt_editor_key(e,0x31,9);assert(p->samples[0].slice_count==1);
        pt_editor_key(e,0x14,0);assert(e->slice_pending);
        pt_editor_click(e,10,260);pt_editor_key(e,0x19,0);assert(e->slice_pending && strstr(e->status,"FINISH"));
        pt_editor_key(e,0x20,0);pt_editor_key(e,0x22,0);assert(!e->slice_count);
        pt_editor_key(e,0x37,0);assert(e->slice_count==1);
        pt_editor_key(e,0x0c,0);assert(!e->slice_pending && !e->sample_marking);
    }
    pt_editor_key(e,0x0b,0);assert(e->sample==1);
    {uint32_t revision=e->history.revision,start,end,frames=p->samples[0].pcm.frames;unsigned j;
        pt_editor_click(e,321,10);assert(e->panel==5);
        pt_editor_click(e,322,10);assert(e->panel==6);
        pt_editor_click(e,414,10);assert(e->panel==7);
        pt_editor_click(e,506,10);assert(e->panel==8);
        pt_editor_key(e,0x21,0);assert(e->number_field==1);
        assert(pt_editor_click(e,590,180)==PT_UI_NONE); /* Modal entry cannot open Load. */
        pt_editor_key(e,1,0);pt_editor_key(e,0x44,0);assert(!e->number_field && e->sample_start==1);
        pt_editor_key(e,0x12,0);pt_editor_key(e,0x0a,0);pt_editor_key(e,0x44,0);
        assert(e->number_field==2 && e->sample_end==frames && strstr(e->status,"INVALID"));
        pt_editor_key(e,0x45,0);assert(!e->number_field && e->panel==8);
        pt_editor_key(e,0x12,0);for(j=0;j<10;++j)pt_editor_key(e,9,0);pt_editor_key(e,0x44,0);
        assert(e->number_field==2 && e->sample_end==frames);pt_editor_key(e,0x45,0);
        pt_editor_click(e,400,87);pt_editor_key(e,2,0);pt_editor_key(e,0x44,0);assert(e->sample_end==2);
        pt_editor_key(e,0x34,0);pt_editor_wave_bounds(e,&start,&end);assert(start==1 && end==2);
        pt_editor_click(e,10,260);pt_editor_click(e,629,260);assert(e->sample_start==1 && e->sample_end==2);
        pt_editor_key(e,0x1b,0);pt_editor_key(e,0x1a,1);assert(e->sample_start==1 && e->sample_end==2);
        for(j=0;j<40;++j)pt_editor_key(e,0x4e,0);pt_editor_wave_bounds(e,&start,&end);assert(start==frames-1 && end==frames);
        for(j=0;j<40;++j)pt_editor_key(e,0x4f,0);pt_editor_wave_bounds(e,&start,&end);assert(!start && end==1);
        pt_editor_key(e,0x23,0);pt_editor_wave_bounds(e,&start,&end);assert(!start && end==frames);
        pt_editor_key(e,0x17,0);pt_editor_wave_bounds(e,&start,&end);assert(end-start==frames/2+frames%2);
        pt_editor_key(e,0x18,0);assert(e->sample_start==1 && e->sample_end==2 && e->history.revision==revision);
        pt_editor_key(e,0x0c,0);pt_editor_wave_bounds(e,&start,&end);assert(!start && end==p->samples[1].pcm.frames);
    }
    pt_editor_key(e,0x0b,0);
    {unsigned bits=p->samples[0].pcm.bits;uint32_t rate=p->samples[0].pcm.rate,revision=e->history.revision;
        pt_editor_key(e,0x33,0);assert(e->panel==9 && e->format_bits==bits && e->format_rate==rate);
        pt_editor_key(e,bits==16?1:2,0);assert(e->history.revision==revision && p->samples[0].pcm.bits==bits);
        pt_editor_key(e,0x13,0);pt_editor_key(e,0x0a,0);pt_editor_key(e,0x44,0);
        assert(e->number_field==3 && e->format_rate==rate);pt_editor_key(e,0x45,0);
        pt_editor_click(e,250,87);assert(p->samples[0].pcm.bits==(bits==16?8:16) && p->samples[0].pcm.rate==rate);
        pt_editor_key(e,0x31,8);assert(p->samples[0].pcm.bits==bits);
        pt_editor_key(e,0x31,9);assert(p->samples[0].pcm.bits==(bits==16?8:16));
        assert(e->format_filtered);pt_editor_click(e,500,70);assert(e->panel==9 && !e->format_filtered);
        pt_editor_key(e,0x23,0);assert(e->format_filtered);pt_editor_key(e,0x42,0);assert(e->panel==5);
    }
    pt_editor_key(e,0x45,0);assert(!e->panel && !e->quit_pending);
    free(before);
}
int main(int argc,char **argv)
{
    struct pt_allocator a={NULL,allocate,release};struct pt_document doc;struct pt_editor *e;
    struct pt_canvas canvas;struct pt_event old;uint8_t *input,*font;size_t n,fn;unsigned i;
    assert(argc==5);input=readfile(argv[1],&n);font=readfile(argv[2],&fn);assert(fn==580);
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
    assert(pt_editor_click(e,250,20)==PT_UI_PLAY);
    assert(pt_editor_click(e,250,21)==PT_UI_PATTERN);
    assert(pt_editor_click(e,250,39)==PT_UI_PATTERN);
    assert(pt_editor_click(e,250,96)==PT_UI_AUDITION);
    assert(pt_editor_click(e,250,97)==PT_UI_NONE);
    assert(pt_editor_key(e,0x57,0)==PT_UI_PLAY && pt_editor_key(e,0x58,0)==PT_UI_PATTERN);
    e->playback.active=1;assert(pt_editor_key(e,0x40,0)==PT_UI_STOP);e->playback.active=0;
    /* The reference-aligned view and mouse cells share the same row origin.
       Exercise the relocated bottom edge and every widened digit cell. */
    e->first_row=0;e->project->channels.selected=0;
    {
        static const int cell_x[6]={49,103,117,139,153,167};
        for(i=0;i<6;++i) {pt_editor_click(e,cell_x[i],PT_EDITOR_PATTERN_Y+1);assert(e->row==0 && e->field==i);}
        pt_editor_click(e,49,PT_EDITOR_PATTERN_Y+239);assert(e->row==19);
        pt_editor_click(e,200,PT_EDITOR_HEADER_Y+1);assert(e->row==19 && e->project->channels.selected==1);
        assert(pt_editor_click(e,430,PT_EDITOR_BOTTOM_Y)==PT_UI_PLAY);
        assert(pt_editor_click(e,510,PT_EDITOR_BOTTOM_Y)==PT_UI_STOP);
        pt_editor_click(e,49,PT_EDITOR_PATTERN_Y+1);
    }
    for(i=0;i<4;++i) {canvas.planes[i]=malloc(PT_VIEW_PLANE_BYTES);assert(canvas.planes[i]);}
    pt_editor_status(e,"EDITOR DEVELOPMENT - AUDIO NOT CONNECTED");e->quit_pending=0;e->panel=0;
    for(i=0;i<4;++i) {pt_editor_key(e,0x50+i,0);pt_editor_draw(e,&canvas,font);}
    e->playback.active=1;e->playback.speed=6;e->playback.bpm=150;
    for(i=0;i<4;++i) {unsigned j;e->playback.volume[i]=64;for(j=0;j<81;++j)e->playback.wave[i][j]=j%2?-128:127;}
    pt_editor_key(e,0x50,0);pt_editor_draw(e,&canvas,font);pt_editor_draw_playback(e,&canvas,font);
    aligned_borders(&canvas);
    ppm(argv[3],&canvas);
    /* Incremental updates must produce the full renderer's exact pixels, and
       the reported rectangles must cover every changed byte/pixel. */
    {
        struct pt_view_cache cache={0};struct pt_canvas incremental,shown;
        struct pt_view_rect areas[PT_VIEW_DIRTY_MAX];unsigned step,p,j,y,x,n;
        static const unsigned actions[]={0x4d,0x4e,0x4f,0x42,0x4c,0x50,0x51,0x52,0x53,0x40,0x31,0x32,0x46,0x0c,0x0b,0x5a,0x5b};
        for(p=0;p<4;++p) {incremental.planes[p]=calloc(1,PT_VIEW_PLANE_BYTES);shown.planes[p]=calloc(1,PT_VIEW_PLANE_BYTES);assert(incremental.planes[p] && shown.planes[p]);}
        for(step=0;step<225;++step) {
            if(step && step<100)pt_editor_key(e,actions[(step-1)%(sizeof(actions)/sizeof(actions[0]))],0);
            if(step==156) {size_t size;uint8_t *bytes=readfile(argv[4],&size);assert(pt_editor_source_load(e,bytes,size)==PT_EDIT_OK);free(bytes);}
            if(step==157)pt_editor_click(e,520,30);
            if(step==158)pt_editor_key(e,0x4e,0);
            if(step==159)pt_editor_key(e,0x44,0);
            if(step==160)pt_editor_key(e,0x4f,0);
            if(step==161)pt_editor_click(e,520,65);
            if(step==162)pt_editor_key(e,0x0b,0);
            if(step==163)pt_editor_key(e,0x44,0);
            if(step==164)pt_editor_key(e,0x31,8);
            if(step==165)pt_editor_key(e,0x31,9);
            if(step==166)pt_editor_key(e,0x45,0);
            if(step==167)pt_editor_key(e,0x31,8);
            if(step==168)pt_editor_key(e,0x31,9);
            if(step==169)pt_editor_key(e,0x13,8);
            if(step==170)pt_editor_click(e,520,50);
            if(step==171)pt_editor_click(e,250,30);
            if(step==172)pt_editor_key(e,8,0);
            if(step==173)pt_editor_key(e,0x0a,0);
            if(step==174)pt_editor_key(e,0x44,0);
            if(step==175)pt_editor_key(e,0x36,0);
            if(step==176)pt_editor_key(e,0x35,0);
            if(step==177)pt_editor_key(e,0x20,0);
            if(step==178)pt_editor_key(e,0x21,0);
            if(step==179)pt_editor_key(e,0x21,0);
            if(step==180)pt_editor_key(e,0x44,0);
            if(step==181)pt_editor_key(e,0x31,8);
            if(step==182)pt_editor_key(e,0x31,9);
            if(step==183)pt_editor_key(e,0x24,0);
            if(step==184)pt_editor_key(e,0x23,0);
            if(step==185)pt_editor_key(e,0x44,0);
            if(step==186)pt_editor_key(e,0x37,0);
            if(step==187)pt_editor_key(e,0x45,0);
            if(step==188)pt_editor_key(e,0x45,0);
            if(step==189)pt_editor_key(e,0x45,0);
            if(step==190)pt_editor_key(e,0x17,8);
            if(step==191)pt_editor_key(e,0x21,0);
            if(step==192)pt_editor_key(e,1,0);
            if(step==193)pt_editor_key(e,0x44,0);
            if(step==194)pt_editor_key(e,0x45,0);
            if(step==195)pt_editor_key(e,0x33,0);
            if(step==196)pt_editor_key(e,0x31,8);
            if(step==197)pt_editor_key(e,0x4d,0);
            if(step==198)pt_editor_key(e,0x42,0);
            if(step==199)pt_editor_key(e,0x0c,0);
            if(step==200)pt_editor_key(e,0x1b,0);
            if(step==201)pt_editor_key(e,0x16,0);
            if(step==202)pt_editor_click(e,520,87);
            if(step==203)pt_editor_click(e,520,87);
            if(step==204)pt_editor_key(e,0x28,0);
            if(step==205)pt_editor_key(e,0x17,8);
            if(step==206)pt_editor_key(e,0x21,0);
            if(step==207)pt_editor_key(e,0x0a,0);
            if(step==208)pt_editor_key(e,0x44,0);
            if(step==209)pt_editor_key(e,0x45,0);
            if(step==210)pt_editor_click(e,400,87);
            if(step==211)pt_editor_click(e,200,200);
            if(step==212)pt_editor_key(e,0x35,0);
            if(step==213)pt_editor_key(e,0x20,0);
            if(step==214)pt_editor_key(e,0x21,0);
            if(step==215)pt_editor_key(e,0x21,0);
            if(step==216)pt_editor_key(e,0x44,0);
            if(step==217)pt_editor_key(e,0x31,8);
            if(step==218)pt_editor_click(e,220,65);
            if(step==219)pt_editor_click(e,200,104);
            if(step==220)pt_editor_key(e,0x36,9);
            if(step==221)pt_editor_key(e,0x46,0);
            if(step==222)pt_editor_key(e,0x45,0);
            if(step==223)pt_editor_key(e,0x31,8);
            if(step==224)pt_editor_key(e,0x31,9);
            if(step==143) {e->panel=5;pt_editor_key(e,0x32,0);}
            if(step==144)pt_editor_key(e,3,0);
            if(step==145)pt_editor_key(e,0x21,0);
            if(step==146)pt_editor_key(e,0x12,0);
            if(step==147)pt_editor_key(e,0x13,0);
            if(step==148)pt_editor_key(e,4,0);
            if(step==149)pt_editor_key(e,0x45,0);
            if(step==150)pt_editor_key(e,1,0);
            if(step==151)pt_editor_key(e,0x16,0);
            if(step==152)pt_editor_click(e,250,65);
            if(step==153)pt_editor_click(e,520,87);
            if(step==154)pt_editor_key(e,0x45,0);
            if(step==155)pt_editor_key(e,0x42,0);
            if(step%13==0) {e->playback.row=step%64;e->playback.bpm=125+step;e->playback.wave[0][step%81]=(int8_t)step;}
            if(step==30) {pt_editor_click(e,400,70);cache.valid=0;}
            if(step==31)pt_editor_click(e,400,80);
            if(step<100 && step%17==0)pt_editor_key(e,0x31,8);
            if(step<100 && step%19==0)pt_editor_key(e,0x35,8);
            if(step<100 && step%23==0)pt_editor_key(e,0x33,8);
            if(step==68)pt_editor_key(e,0x20,8);
            if(step==70)pt_editor_click(e,400,50);
            if(step==74)pt_editor_click(e,250,87);
            if(step==76)pt_editor_click(e,400,87);
            if(step==80)pt_editor_key(e,0x36,8);
            if(step==81)pt_editor_key(e,0x0b,0);
            if(step==82)pt_editor_key(e,0x44,0);
            if(step==84)pt_editor_key(e,0x45,0);
            if(step==85)pt_editor_key(e,0x13,8);
            if(step==86)pt_editor_key(e,0x16,0);
            if(step==87)pt_editor_key(e,0x21,0);
            if(step==88)pt_editor_key(e,0x42,0);
            if(step==90)pt_editor_key(e,0x37,0);
            if(step==93)pt_editor_key(e,0x31,8);
            if(step==95)pt_editor_key(e,0x45,0);
            if(step==96)pt_editor_key(e,0x28,8);
            if(step==97)pt_editor_click(e,50,260);
            if(step==98)pt_editor_click(e,400,260);
            if(step==99)pt_editor_key(e,0x0c,0);
            if(step==100)pt_editor_click(e,400,10);
            if(step==101)pt_editor_click(e,250,50);
            if(step==102)pt_editor_click(e,500,50);
            if(step==103)pt_editor_click(e,500,10);
            if(step==104)pt_editor_click(e,250,70);
            if(step==105)pt_editor_click(e,400,87);
            if(step==106)pt_editor_click(e,500,87);
            if(step==107)pt_editor_key(e,0x14,0);
            if(step==108)pt_editor_key(e,0x33,0);
            if(step==109)pt_editor_key(e,0x37,0);
            if(step==110)pt_editor_key(e,0x22,0);
            if(step==111)pt_editor_key(e,0x32,0);
            if(step==112)pt_editor_key(e,0x14,0);
            if(step==113)pt_editor_key(e,0x0c,0);
            if(step==114)pt_editor_click(e,550,10);
            if(step==115)pt_editor_key(e,0x0b,0);
            if(step==116)pt_editor_key(e,0x17,0);
            if(step==117)pt_editor_key(e,0x4e,0);
            if(step==118)pt_editor_key(e,0x21,0);
            if(step==119)pt_editor_key(e,1,0);
            if(step==120)pt_editor_key(e,0x44,0);
            if(step==121)pt_editor_key(e,0x12,0);
            if(step==122)pt_editor_key(e,2,0);
            if(step==123)pt_editor_key(e,0x44,0);
            if(step==124)pt_editor_key(e,0x34,0);
            if(step==125)pt_editor_key(e,0x21,0);
            if(step==126)pt_editor_key(e,0x45,0);
            if(step==127)pt_editor_click(e,10,260);
            if(step==128)pt_editor_click(e,629,260);
            if(step==129)pt_editor_key(e,0x23,0);
            if(step==130)pt_editor_key(e,0x42,0);
            if(step==131)pt_editor_key(e,0x33,0);
            if(step==132)pt_editor_key(e,2,0);
            if(step==133)pt_editor_key(e,0x13,0);
            if(step==134)pt_editor_key(e,0x0a,0);
            if(step==135)pt_editor_key(e,0x44,0);
            if(step==136)pt_editor_key(e,0x45,0);
            if(step==137)pt_editor_click(e,250,70);
            if(step==138)pt_editor_key(e,0x13,0);
            if(step==139)pt_editor_key(e,4,0);
            if(step==140)pt_editor_key(e,0x44,0);
            if(step==141)pt_editor_key(e,0x42,0);
            if(step==142)pt_editor_key(e,0x45,0);
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
        /* Native ticks present only these shared rectangles, without a full
           editor redraw. Check glyph-edge erasure as tempo digits change. */
        e->playback.active=1;e->playback.bpm=125;pt_editor_draw(e,&incremental,font);
        for(p=0;p<4;++p)memcpy(shown.planes[p],incremental.planes[p],PT_VIEW_PLANE_BYTES);
        for(step=0;step<4;++step) {
            static const unsigned tempos[]={150,111,200,125};e->playback.bpm=tempos[step];
            pt_editor_draw_playback(e,&incremental,font);pt_editor_draw(e,&canvas,font);
            for(j=0;j<PT_VIEW_PLAYBACK_AREAS;++j) {
                const struct pt_view_rect *a=&pt_view_playback_areas[j];
                for(y=a->y;y<a->y+a->height;++y)for(x=a->x;x<a->x+a->width;++x)for(p=0;p<4;++p) {
                    uint8_t mask=(uint8_t)(128>>(x&7));size_t offset=y*80+x/8;
                    shown.planes[p][offset]=(shown.planes[p][offset]&~mask)|(incremental.planes[p][offset]&mask);
                }
            }
            for(p=0;p<4;++p)assert(!memcmp(canvas.planes[p],shown.planes[p],PT_VIEW_PLANE_BYTES));
        }
        for(p=0;p<4;++p) {free(incremental.planes[p]);free(shown.planes[p]);}
    }

    /* Rendering exercised owned sample versions. Release them before starting
       independent workflows on a freshly loaded fixture. */
    pt_editor_dispose(e);input=readfile(argv[1],&n);
    assert(pt_document_load(&doc,input,n,SIZE_MAX)==PT_PROJECT_OK);free(input);
    blocks(e);channel_controls(e);channel_details(e);sampler_controls(e);
    for(i=0;i<4;++i)free(canvas.planes[i]);
    free(font);pt_editor_dispose(e);free(e);pt_document_release(&doc);
    puts("EDITOR PASS: bank/wrap/scroll, guarded note and nibble edits, OFF, undo/redo, save state, discard confirmation, atomic block copy/paste/clear/transpose/clone, all-page planar render");return 0;
}
