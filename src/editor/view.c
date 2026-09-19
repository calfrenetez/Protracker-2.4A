#include <stdio.h>
#include <string.h>
#include "view.h"
enum {BLACK,GREY,WHITE,DARK,BLUE,YELLOW,NAVY,MID};
const uint16_t pt_view_palette[16]={0x000,0x888,0xddd,0x444,0x49d,0xff5,0x013,0xaaa,0x222,0xbbb,0x666,0x999,0x555,0x777,0xccc,0xfff};
static void rect(struct pt_canvas *c,int x,int y,int w,int h,unsigned pen)
{
    int row,first,last;unsigned p;uint8_t left,right;
    if(x<0) {w+=x;x=0;}if(y<0) {h+=y;y=0;}
    if(x+w>640)w=640-x;
    if(y+h>512)h=512-y;
    if(w<=0 || h<=0)return;
    first=x/8;last=(x+w-1)/8;left=(uint8_t)(255>>(x&7));right=(uint8_t)(255<<(7-((x+w-1)&7)));
    if(first==last)left&=right;
    for(p=0;p<4;++p)for(row=y;row<y+h;++row) {
        uint8_t *v=c->planes[p]+row*80+first;uint8_t value=(pen&(1U<<p))?255:0;
        *v=(uint8_t)((*v&~left)|(value&left));
        if(last>first) {
            if(last>first+1)memset(v+1,value,(size_t)(last-first-1));
            v+=last-first;*v=(uint8_t)((*v&~right)|(value&right));
        }
    }
}
/* Transparent glyph stamps touch at most three bytes per plane per scanline.
   Avoid calling a rectangle renderer once per set pixel on a 68030. */
static void glyph_row(struct pt_canvas *c,int x,int y,unsigned bits,unsigned pen,int scale)
{
    static const uint8_t stretch[16]={0,3,4,7,24,27,28,31,32,35,36,39,56,59,60,63};
    unsigned p,k,width=scale==3?12:(unsigned)scale*8,count;uint32_t mask,v=bits;
    uint8_t m0,m1,m2;size_t offset;
    /* Four source bits become six destination bits. This replaces eight
       variable shifts per row without changing the 1,2,1,2 pixel spacing. */
    if(scale==3)v=((unsigned)stretch[bits>>4]<<6)|stretch[bits&15];
    if(scale==2) {v=(v|(v<<4))&0x0f0f;v=(v|(v<<2))&0x3333;v=(v|(v<<1))&0x5555;v|=v<<1;}
    if(x<0 || x+(int)width>640 || y<0 || y+1>=512) {
        for(k=0;k<8;++k)if(bits&(128U>>k)) {
            int start=scale==3?(int)k*3/2:(int)k*scale;
            int end=scale==3?(int)(k+1)*3/2:(int)(k+1)*scale;
            rect(c,x+start,y,end-start,2,pen);
        }
        return;
    }
    mask=v<<(24-width-(x&7));count=(width+(x&7)+7)/8;
    m0=(uint8_t)(mask>>16);m1=(uint8_t)(mask>>8);m2=(uint8_t)mask;
    offset=(size_t)y*80+x/8;
    /* Both scanlines share these masks. Avoid recomputing/shifting a mask
       for every byte of every plane on the 68000-compatible build. */
    for(p=0;p<4;++p) {
        uint8_t *d=c->planes[p]+offset;
        if(pen&(1U<<p)) {
            d[0]|=m0;d[80]|=m0;
            if(count>1) {d[1]|=m1;d[81]|=m1;}
            if(count>2) {d[2]|=m2;d[82]|=m2;}
        } else {
            d[0]&=(uint8_t)~m0;d[80]&=(uint8_t)~m0;
            if(count>1) {d[1]&=(uint8_t)~m1;d[81]&=(uint8_t)~m1;}
            if(count>2) {d[2]&=(uint8_t)~m2;d[82]&=(uint8_t)~m2;}
        }
    }
}

static void letters(struct pt_canvas *c,const uint8_t *font,int x,int y,const char *s,unsigned pen,int scale)
{
    unsigned ch,row;
    for(;*s && x<640;++s,x+=scale==3?12:8*scale) {
        ch=(unsigned char)*s;if(ch>='a' && ch<='z')ch-=32;if(ch<32 || ch>95)ch='?';
        for(row=0;row<5;++row)glyph_row(c,x,y+(int)row*2,font[(ch-32)*8+row],pen,scale);
    }
}
static void small(struct pt_canvas *c,const uint8_t *font,int x,int y,const char *s,unsigned pen)
{letters(c,font,x,y,s,pen,1);}
static void panel(struct pt_canvas *c,int x,int y,int w,int h,unsigned pen)
{
    rect(c,x,y,w,h,pen);rect(c,x,y,w,1,WHITE);rect(c,x,y,1,h,WHITE);
    rect(c,x,y+h-1,w,1,DARK);rect(c,x+w-1,y,1,h,DARK);
}
static void medium(struct pt_canvas *c,const uint8_t *font,int x,int y,const char *s,unsigned pen)
{letters(c,font,x,y,s,pen,3);}
static void label(struct pt_canvas *c,const uint8_t *font,int x,int y,int w,int h,const char *s,unsigned active)
{
    int tx=x+(w-(int)strlen(s)*12)/2;
    panel(c,x,y,w,h,active?YELLOW:GREY);
    if(!active)medium(c,font,tx+1,y+(h-10)/2+1,s,DARK);
    medium(c,font,tx,y+(h-10)/2,s,active?BLACK:WHITE);
}
static void arrow(struct pt_canvas *c,int x,int y,int up)
{
    int r;panel(c,x,y,20,19,GREY);
    for(r=0;r<5;++r)rect(c,x+9-r,y+(up?4+r:12-r),r*2+1,1,WHITE);
    rect(c,x+7,y+(up?8:4),5,7,WHITE);
}
void pt_editor_draw(const struct pt_editor *e,struct pt_canvas *c,const uint8_t font[580])
{
    const struct pt_project *p=e->project;unsigned i,r,ch,page=pt_channels_page(&p->channels),first=page*4;
    const struct pt_sample *sample=e->sample && e->sample<=p->sample_count?&p->samples[e->sample-1]:NULL;
    char s[80],note[4];int x,y;
    static const int field_x[6]={6,64,76,100,112,124};
    static const char *fields[9]={"POS","PATTERN","LENGTH","FINETUNE","SAMPLE","VOLUME","LENGTH","REPEAT","REPLEN"};
    static const char *buttons[5][3]={{"PLAY","STOP","MOD2WAV"},{"PATTERN","CLEAR","PAT2SMP"},
        {"EDIT","EDIT OP.","POS ED."},{"RECORD","DISK OP.","SAMPLER"},{"SAMPLE","SAMPLER",""}};
    unsigned long values[9];size_t bytes=0;
    values[0]=e->position;values[1]=e->pattern;values[2]=p->order_count;
    values[3]=sample?(unsigned)(sample->finetune&15):0;values[4]=e->sample;
    values[5]=sample?sample->volume:0;values[6]=sample?sample->pcm.frames:0;
    values[7]=sample?sample->loop_start:0;values[8]=sample?sample->loop_end-sample->loop_start:0;
    for(i=0;i<p->sample_count;++i)bytes+=(size_t)p->samples[i].pcm.frames*p->samples[i].pcm.channels*(p->samples[i].pcm.bits/8);
    panel(c,0,0,640,512,GREY);
    for(i=0;i<9;++i) {
        y=2+(int)i*19;label(c,font,2,y,116,19,fields[i],0);panel(c,118,y,72,19,GREY);
        if(i==3)snprintf(s,sizeof(s),"%ld",sample?(long)sample->finetune:0L);
        else snprintf(s,sizeof(s),i>=6?"%05lX":i==5?"%04lX":"%04lu",values[i]);
        /* Extended sample lengths are shown exactly if they fit; wider values
           belong in the sampler detail view, never silently wrap. */
        if(strlen(s)>5)strcpy(s,"LARGE");
        medium(c,font,185-(int)strlen(s)*12,y+5,s,BLACK);arrow(c,190,y,0);arrow(c,210,y,1);
    }
    label(c,font,82,2,18,19,"I",0);label(c,font,100,2,18,19,"D",0);
    for(r=0;r<5;++r)for(i=0;i<3;++i)label(c,font,230+(int)i*123,2+(int)r*19,123,19,buttons[r][i],r==2 && i==0 && e->editing);
    for(i=0;i<5;++i) {if(i<4)snprintf(s,sizeof(s),"%u",i+1);else s[0]=0;label(c,font,599,2+(int)i*19,39,19,s,0);}
    label(c,font,230,97,408,19,"QUADRASCOPE",0);
    for(i=0;i<4;++i) {
        x=230+(int)i*102;panel(c,x,116,102,39,GREY);rect(c,x+17,117,84,37,BLACK);
        snprintf(s,sizeof(s),"%u",first+i+1);small(c,font,x+1,120,s,WHITE);
        /* Stopped scopes: no invented waveform or implied audio activity. */
        if(first+i<p->channels.count)rect(c,x+18,135,81,1,YELLOW);
    }
    panel(c,230,155,408,18,GREY);medium(c,font,240,159,"PROTRACKER 2.4G",WHITE);
    small(c,font,492,159,"2.3F: 8BITBUBSY",WHITE);
    panel(c,2,174,636,19,GREY);medium(c,font,12,179,"SONGNAME:",WHITE);
    snprintf(s,sizeof(s),"%.32s",p->title);medium(c,font,146,179,s,NAVY);label(c,font,590,174,48,19,"LOAD",0);
    panel(c,2,193,636,19,GREY);medium(c,font,12,198,"SAMPLENAME:",WHITE);
    snprintf(s,sizeof(s),"%.32s",sample?sample->name:"");medium(c,font,146,198,s,NAVY);
    panel(c,590,193,48,19,GREY);snprintf(s,sizeof(s),"%luK",(unsigned long)((bytes+1023)/1024));small(c,font,596,198,s,NAVY);
    panel(c,2,212,636,26,GREY);panel(c,14,216,42,18,MID);snprintf(s,sizeof(s),"%02u",p->speed);medium(c,font,22,220,s,NAVY);
    snprintf(s,sizeof(s),"%03u",p->bpm);small(c,font,70,216,s,NAVY);small(c,font,66,227,"TEMPO",WHITE);
    small(c,font,106,216,"STATUS:",WHITE);snprintf(s,sizeof(s),"%.45s",e->status);small(c,font,162,216,s,NAVY);
    if(strlen(e->status)>45)small(c,font,106,227,e->status+45,NAVY);
    small(c,font,528,216,"TUNE",WHITE);snprintf(s,sizeof(s),"%06lu",(unsigned long)bytes);small(c,font,586,216,s,NAVY);
    small(c,font,528,227,"TIMING",WHITE);small(c,font,610,227,"---",NAVY);
    if(pt_editor_dirty(e))small(c,font,512,216,"*",YELLOW);
    for(i=0;i<4;++i) {
        ch=first+i;x=38+(int)i*150;panel(c,x,238,150,16,GREY);
        if(ch<p->channels.count) {
            const struct pt_channel *channel=&p->channels.track[ch];char route=channel->route==PT_PAULA?'P':channel->route==PT_AMIGUS?'A':'M';
            snprintf(s,sizeof(s),"%u",ch+1);medium(c,font,x+64,241,s,BLACK);snprintf(s,sizeof(s),"%c",route);small(c,font,x+130,241,s,NAVY);
        }
        rect(c,x,254,148,240,BLACK);
        if(ch>=p->channels.count)continue;
        for(r=0;r<PT_EDITOR_ROWS && r+e->first_row<64;++r) {
            const struct pt_event *event=&p->events[(e->pattern*64+r+e->first_row)*p->channels.count+ch];
            unsigned highlight=ch==p->channels.selected && r+e->first_row==e->row;
            y=254+(int)r*12;pt_editor_note(event,note);
            medium(c,font,x+6,y+1,note,highlight && !e->field?YELLOW:BLUE);
            snprintf(s,sizeof(s),"%02X",event->instrument);medium(c,font,x+64,y+1,s,BLUE);
            snprintf(s,sizeof(s),"%X%02X",event->effect,event->parameter);medium(c,font,x+100,y+1,s,highlight?YELLOW:BLUE);
            if(highlight) {
                int fx=x+field_x[e->field]-1,fw=e->field?13:37;
                rect(c,fx,y,fw,1,YELLOW);rect(c,fx,y+11,fw,1,YELLOW);rect(c,fx,y,1,12,YELLOW);rect(c,fx+fw-1,y,1,12,YELLOW);
            }
        }
    }
    panel(c,2,238,35,16,GREY);rect(c,3,254,33,240,BLACK);
    for(r=0;r<PT_EDITOR_ROWS && r+e->first_row<64;++r) {
        snprintf(s,sizeof(s),"%02u",r+e->first_row);medium(c,font,8,255+(int)r*12,s,WHITE);
    }
    for(i=0;i<4;++i) {snprintf(s,sizeof(s),"%u-%u",i*4+1,i*4+4);label(c,font,4+(int)i*61,495,61,17,s,page==i);}
    panel(c,248,495,168,17,GREY);label(c,font,416,495,60,17,"PLAY",0);
    panel(c,476,495,18,17,GREY);rect(c,482,500,6,7,WHITE);label(c,font,494,495,58,17,"STOP",0);
    panel(c,552,495,86,17,GREY);small(c,font,557,499,"PATTERN",WHITE);snprintf(s,sizeof(s),"%02X",e->pattern);small(c,font,618,499,s,NAVY);
    pt_editor_draw_playback(e,c,font);
    if(e->panel) {
        panel(c,230,2,369,95,GREY);
        label(c,font,230,2,369,19,e->panel==1?"EDIT OP.":"DISK OP.",0);
        label(c,font,230,21,184,38,e->panel==1?"UNDO":"SAVE NEW",0);
        label(c,font,414,21,185,38,e->panel==1?"REDO":"QUIT",0);
        label(c,font,230,59,369,38,"BACK",0);
    }
}

/* Small refresh region: CIA timing never waits for the display. Waveforms are
   current voice sample data scaled by current volume, not an audio capture. */
void pt_editor_draw_playback(const struct pt_editor *e,struct pt_canvas *c,const uint8_t font[580])
{
    unsigned i,j,first=pt_channels_page(&e->project->channels)*4;char s[32];
    for(i=0;i<4;++i) {
        int x=248+(int)i*102,previous=135;
        rect(c,x,117,81,37,BLACK);
        if(first+i>=e->project->channels.count)continue;
        for(j=0;j<81;++j) {
            int y=135;
            if(e->playback.active && !first)y-=(int)e->playback.wave[i][j]*e->playback.volume[i]/512;
            rect(c,x+(int)j,y<previous?y:previous,1,(y<previous?previous-y:y-previous)+1,YELLOW);previous=y;
        }
    }
    panel(c,248,495,168,17,GREY);
    if(e->playback.active) {
        snprintf(s,sizeof(s),"POS %03u ROW %02u",e->playback.order,e->playback.row);small(c,font,253,499,s,NAVY);
    }
    panel(c,14,216,42,18,MID);
    snprintf(s,sizeof(s),"%02u",e->playback.active?e->playback.speed:e->project->speed);medium(c,font,22,220,s,NAVY);
    rect(c,70,216,24,10,GREY);snprintf(s,sizeof(s),"%03u",e->playback.active?e->playback.bpm:e->project->bpm);small(c,font,70,216,s,NAVY);
    rect(c,610,227,24,10,GREY);small(c,font,610,227,e->playback.active?"CIA":"---",NAVY);
}
