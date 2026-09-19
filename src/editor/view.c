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
    unsigned p,k,r,width=(unsigned)scale*8;uint32_t mask,v=bits;
    if(scale==2) {v=(v|(v<<4))&0x0f0f;v=(v|(v<<2))&0x3333;v=(v|(v<<1))&0x5555;v|=v<<1;}
    if(x<0 || x+(int)width>640 || y<0 || y+1>=512) {
        for(k=0;k<8;++k)if(bits&(128U>>k))rect(c,x+(int)k*scale,y,scale,2,pen);
        return;
    }
    mask=v<<(24-width-(x&7));
    for(p=0;p<4;++p)for(r=0;r<2;++r) {
        uint8_t *dest=c->planes[p]+(y+(int)r)*80+x/8;
        for(k=0;k<(width+(x&7)+7)/8;++k) {
            uint8_t m=(uint8_t)(mask>>(16-k*8));
            if(pen&(1U<<p))dest[k]|=m;else dest[k]&=(uint8_t)~m;
        }
    }
}
static void letters(struct pt_canvas *c,const uint8_t *font,int x,int y,const char *s,unsigned pen,int scale)
{
    unsigned ch,row;
    for(;*s && x<640;++s,x+=8*scale) {
        ch=(unsigned char)*s;if(ch>='a' && ch<='z')ch-=32;if(ch<32 || ch>95)ch='?';
        for(row=0;row<5;++row)glyph_row(c,x,y+(int)row*2,font[(ch-32)*8+row],pen,scale);
    }
}
static void text(struct pt_canvas *c,const uint8_t *font,int x,int y,const char *s,unsigned pen)
{letters(c,font,x,y,s,pen,2);}
static void small(struct pt_canvas *c,const uint8_t *font,int x,int y,const char *s,unsigned pen)
{letters(c,font,x,y,s,pen,1);}
static void panel(struct pt_canvas *c,int x,int y,int w,int h,unsigned pen)
{
    rect(c,x,y,w,h,pen);rect(c,x,y,w,1,WHITE);rect(c,x,y,1,h,WHITE);
    rect(c,x,y+h-1,w,1,DARK);rect(c,x+w-1,y,1,h,DARK);
}
static void button(struct pt_canvas *c,const uint8_t *font,int x,int y,int w,const char *label,unsigned active,unsigned disabled)
{
    int tx=x+(w-(int)strlen(label)*16)/2;
    panel(c,x,y,w,24,active?YELLOW:GREY);
    if(!active && !disabled)text(c,font,tx+1,y+8,label,DARK);
    text(c,font,tx,y+7,label,active?BLACK:disabled?DARK:WHITE);
}
static void value(struct pt_canvas *c,const uint8_t *font,int y,const char *label,unsigned n,unsigned digits)
{
    char s[16];panel(c,4,y,124,24,GREY);panel(c,128,y,108,24,MID);
    text(c,font,12,y+7,label,WHITE);snprintf(s,sizeof(s),"%0*u",(int)digits,n);text(c,font,224-(int)strlen(s)*16,y+7,s,NAVY);
}
void pt_editor_draw(const struct pt_editor *e,struct pt_canvas *c,const uint8_t font[580])
{
    const struct pt_project *p=e->project;unsigned i,r,ch,page=pt_channels_page(&p->channels),first=page*4;
    char s[80],note[4];int x,y;
    static const int field_x[6]={2,52,68,92,108,124};
    panel(c,0,0,640,512,GREY);panel(c,4,4,632,20,GREY);
    text(c,font,12,9,"PROTRACKER 2.4G",WHITE);text(c,font,388,9,"ENHANCED EDITOR",WHITE);
    value(c,font,28,"PATTERN",e->pattern,3);value(c,font,52,"LENGTH",p->order_count,3);
    value(c,font,76,"CHANS",p->channels.count,2);value(c,font,100,"SAMPLE",e->sample,2);
    button(c,font,240,28,198,"PLAY",0,1);button(c,font,438,28,198,"STOP",0,1);
    button(c,font,240,52,198,e->editing?"EDIT ON":"EDIT OFF",e->editing,0);button(c,font,438,52,198,"SAVE NEW",0,0);
    button(c,font,240,76,198,"UNDO",0,!e->history.cursor);button(c,font,438,76,198,"REDO",0,e->history.cursor==e->history.count);
    button(c,font,240,100,198,"PATTERN -",0,0);button(c,font,438,100,198,"PATTERN +",0,0);
    panel(c,4,128,632,38,DARK);rect(c,6,130,628,34,BLACK);
    text(c,font,14,137,"SAMPLE",MID);
    if(e->sample && e->sample<=p->sample_count) {
        const struct pt_sample *sample=&p->samples[e->sample-1];const struct pt_pcm *pcm=&sample->pcm;
        snprintf(s,sizeof(s),"%02u %2u BIT",e->sample,pcm->bits);small(c,font,14,150,s,BLUE);
        if(pcm->frames)for(i=0;i<488;++i) {
            size_t frame=(size_t)((uint64_t)i*pcm->frames/488);int32_t v=pcm->data[frame*pcm->channels];
            int offset=(int)((int64_t)v*14/((int32_t)1<<(pcm->bits-1)));
            rect(c,140+(int)i,147-(offset>0?offset:0),1,(offset<0?-offset:offset)+1,YELLOW);
        }
    }
    panel(c,4,170,632,24,GREY);text(c,font,12,177,"SONG:",WHITE);snprintf(s,sizeof(s),"%.28s",p->title);text(c,font,100,177,s,NAVY);
    snprintf(s,sizeof(s),"%03u/%02u",p->bpm,p->speed);small(c,font,560,177,s,NAVY);
    panel(c,4,198,632,20,GREY);small(c,font,12,203,e->status,NAVY);
    for(i=0;i<4;++i) {
        ch=first+i;x=36+(int)i*150;panel(c,x,222,150,22,ch==p->channels.selected?MID:GREY);
        if(ch<p->channels.count) {
            const struct pt_channel *channel=&p->channels.track[ch];char route=channel->route==PT_PAULA?'P':channel->route==PT_AMIGUS?'A':'M';
            snprintf(s,sizeof(s),"%02u [%c]%s",ch+1,route,channel->muted?" M":channel->solo?" S":"");text(c,font,x+8,228,s,NAVY);
        }
        rect(c,x,246,148,240,BLACK);
        if(ch>=p->channels.count)continue;
        for(r=0;r<PT_EDITOR_ROWS && r+e->first_row<64;++r) {
            const struct pt_event *event=&p->events[(e->pattern*64+r+e->first_row)*p->channels.count+ch];
            y=246+(int)r*12;pt_editor_note(event,note);
            if(r+e->first_row==e->row)rect(c,x,y,148,12,8);
            text(c,font,x+2,y+1,note,BLUE);snprintf(s,sizeof(s),"%02X",event->instrument);text(c,font,x+52,y+1,s,BLUE);
            snprintf(s,sizeof(s),"%X%02X",event->effect,event->parameter);text(c,font,x+92,y+1,s,BLUE);
            if(ch==p->channels.selected && r+e->first_row==e->row) {
                int fx=x+field_x[e->field]-1,fw=e->field?17:49;
                rect(c,fx,y,fw,1,YELLOW);rect(c,fx,y+11,fw,1,YELLOW);rect(c,fx,y,1,12,YELLOW);rect(c,fx+fw-1,y,1,12,YELLOW);
            }
        }
    }
    rect(c,4,246,30,240,BLACK);
    for(r=0;r<PT_EDITOR_ROWS && r+e->first_row<64;++r) {
        snprintf(s,sizeof(s),"%02u",r+e->first_row);text(c,font,3,247+(int)r*12,s,r+e->first_row==e->row?YELLOW:WHITE);
    }
    for(i=0;i<4;++i) {snprintf(s,sizeof(s),"%u-%u",i*4+1,i*4+4);button(c,font,4+(int)i*100,488,100,s,page==i,i*4>=p->channels.count);}
    snprintf(s,sizeof(s),"%s",pt_editor_dirty(e)?"EDITED":"SAVED");small(c,font,420,495,s,pt_editor_dirty(e)?YELLOW:NAVY);
    button(c,font,536,488,100,"QUIT",0,0);
}
