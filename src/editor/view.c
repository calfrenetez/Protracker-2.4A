#include <stdio.h>
#include <string.h>
#include "view.h"
enum {BLACK,GREY,WHITE,DARK,BLUE,YELLOW,NAVY,MID};
const uint16_t pt_view_palette[16]={0x000,0x777,0xddd,0x444,0x5ae,0xff5,0x014,0xaaa,0x222,0xbbb,0x666,0x999,0x555,0x888,0xccc,0xfff};
const struct pt_view_rect pt_view_playback_areas[PT_VIEW_PLAYBACK_AREAS]={
    {230,PT_EDITOR_SCOPE_Y,408,PT_EDITOR_SCOPE_BOTTOM-PT_EDITOR_SCOPE_Y},
    {14,211,110,23}, {598,223,32,10}, {248,PT_EDITOR_BOTTOM_Y,168,21}
};
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
/* Keep the pinned glyph shapes, with spacing matched to each classic field. */
static void spaced(struct pt_canvas *c,const uint8_t *font,int x,int y,const char *s,unsigned pen,int advance)
{
    unsigned ch,row;
    for(;*s && x<640;++s,x+=advance) {
        ch=(unsigned char)*s;if(ch>='a' && ch<='z')ch-=32;if(ch<32 || ch>95)ch='?';
        for(row=0;row<5;++row)glyph_row(c,x,y+(int)row*2,font[(ch-32)*8+row],pen,3);
    }
}
static void small(struct pt_canvas *c,const uint8_t *font,int x,int y,const char *s,unsigned pen)
{letters(c,font,x,y,s,pen,1);}
static void panel(struct pt_canvas *c,int x,int y,int w,int h,unsigned pen)
{
    rect(c,x,y,w,h,pen);rect(c,x,y,w,1,WHITE);rect(c,x,y,1,h,WHITE);
    rect(c,x,y+h-1,w,1,DARK);rect(c,x+w-1,y,1,h,DARK);
    if(w>4 && h>4) {rect(c,x+1,y+1,w-2,1,MID);rect(c,x+1,y+1,1,h-2,MID);}
}
static void medium(struct pt_canvas *c,const uint8_t *font,int x,int y,const char *s,unsigned pen)
{letters(c,font,x,y,s,pen,3);}
/* A two-step bevel belongs on controls; the surrounding strips stay flatter.
   Selected controls reverse the light direction and move the lettering inward. */
static void button(struct pt_canvas *c,int x,int y,int w,int h,unsigned active)
{
    unsigned light=active?DARK:WHITE,shade=active?WHITE:DARK;
    rect(c,x,y,w,h,active?YELLOW:13);
    if(!active) {
        rect(c,x+2,y+2,w-4,2,11);
        rect(c,x+2,y+h-4,w-4,2,GREY);
    }
    rect(c,x,y,w,1,light);rect(c,x,y,1,h,light);
    rect(c,x+1,y+1,w-2,1,active?10:14);rect(c,x+1,y+1,1,h-2,active?10:14);
    rect(c,x+1,y+h-2,w-2,1,active?MID:10);rect(c,x+w-2,y+1,1,h-2,active?MID:10);
    rect(c,x,y+h-1,w,1,shade);rect(c,x+w-1,y,1,h,shade);
}
static void label(struct pt_canvas *c,const uint8_t *font,int x,int y,int w,int h,const char *s,unsigned active)
{
    int n=(int)strlen(s),advance=12,tx,ty=y+(h-10)/2-1;
    /* The old ten-pixel step crowded twelve-pixel glyphs and their shadows.
       Only narrow bank buttons need tighter spacing. Keep the original font. */
    if(n*advance+4>w)advance=11;
    if(n*advance+4>w)advance=10;
    tx=x+(w-(n?n*advance:0))/2;
    button(c,x,y,w,h,active);
    if(active) {++tx;++ty;}
    else {
        spaced(c,font,tx+1,ty+2,s,8,advance);
        spaced(c,font,tx+1,ty+1,s,DARK,advance);
    }
    spaced(c,font,tx,ty,s,active?BLACK:WHITE,advance);
}
static void arrow_shape(struct pt_canvas *c,int x,int y,int up,unsigned pen)
{
    int r;
    for(r=0;r<5;++r)rect(c,x+9-r,y+(up?4+r:12-r),r*2+1,1,pen);
    rect(c,x+7,y+(up?8:4),5,7,pen);
}
static void arrow(struct pt_canvas *c,int x,int y,int up)
{
    button(c,x,y,20,PT_EDITOR_CONTROL_HEIGHT,0);
    arrow_shape(c,x+1,y+1,up,DARK);arrow_shape(c,x,y,up,WHITE);
}
static void draw_status(const struct pt_editor *e,struct pt_canvas *c,const uint8_t *font,size_t bytes)
{
    const struct pt_project *p=e->project;char s[80];
    const char *status=strncmp(e->status,"READY -",7)?e->status:"ALL RIGHT";
    panel(c,2,211,636,23,GREY);panel(c,14,214,46,17,MID);
    snprintf(s,sizeof(s),"%02u",e->playback.active?e->playback.speed:p->speed);spaced(c,font,22,218,s,NAVY,14);
    panel(c,76,211,42,13,GREY);snprintf(s,sizeof(s),"%03u",e->playback.active?e->playback.bpm:p->bpm);spaced(c,font,79,213,s,NAVY,10);
    spaced(c,font,73,224,"TEMPO",WHITE,10);
    small(c,font,130,strlen(status)>30?213:218,"STATUS:",WHITE);
    snprintf(s,sizeof(s),"%.30s",status);spaced(c,font,190,strlen(status)>30?213:218,s,NAVY,10);
    if(strlen(status)>30) {snprintf(s,sizeof(s),"%.45s",status+30);small(c,font,126,223,s,NAVY);}
    spaced(c,font,498,213,"TUNE",WHITE,10);snprintf(s,sizeof(s),"%06lu",(unsigned long)bytes);
    if(strlen(s)>6)small(c,font,636-(int)strlen(s)*8,213,s,NAVY);else spaced(c,font,568,213,s,NAVY,10);
    spaced(c,font,498,223,"TIMING",WHITE,10);spaced(c,font,598,223,e->playback.active?"CIA":"---",NAVY,10);
    if(pt_editor_dirty(e))small(c,font,120,213,"*",YELLOW);
}
static void draw_pattern_row(const struct pt_editor *e,struct pt_canvas *c,const uint8_t *font,unsigned r)
{
    const struct pt_project *p=e->project;unsigned i,ch,first=pt_channels_page(&p->channels)*4,row=r+e->first_row;
    int x,y=PT_EDITOR_PATTERN_Y+(int)r*12;char s[16],note[4];struct pt_editor_selection selection;
    int selected=pt_editor_selection(e,&selection);
    static const int field_x[6]={10,64,78,100,114,128};
    if(r>=PT_EDITOR_ROWS)return;
    rect(c,3,y,33,12,BLACK);
    if(row<64) {snprintf(s,sizeof(s),"%02u",row);medium(c,font,8,y+1,s,WHITE);}
    for(i=0;i<4;++i) {
        ch=first+i;x=38+(int)i*150;rect(c,x,y,148,12,selected && row>=selection.r0 && row<selection.r1 && ch>=selection.c0 && ch<selection.c1?8:BLACK);
        if(ch>=p->channels.count || row>=64)continue;
            const struct pt_event *event=&p->events[(e->pattern*64+row)*p->channels.count+ch];
            unsigned highlight=ch==p->channels.selected && row==e->row;
            pt_editor_note(event,note);
            spaced(c,font,x+10,y+1,note,highlight && !e->field?YELLOW:BLUE,14);
            snprintf(s,sizeof(s),"%02X",event->instrument);spaced(c,font,x+64,y+1,s,BLUE,14);
            snprintf(s,sizeof(s),"%X%02X",event->effect,event->parameter);spaced(c,font,x+100,y+1,s,highlight?YELLOW:BLUE,14);
            if(highlight) {
                int fx=x+field_x[e->field]-1,fw=e->field?15:43;
                rect(c,fx,y,fw,1,YELLOW);rect(c,fx,y+11,fw,1,YELLOW);rect(c,fx,y,1,12,YELLOW);rect(c,fx+fw-1,y,1,12,YELLOW);
            }
    }
}
static void draw_sample(const struct pt_editor *e,struct pt_canvas *c,const uint8_t *font)
{
    const struct pt_sample *sample=e->sample && e->sample<=e->project->sample_count?&e->project->samples[e->sample-1]:NULL;
    const struct pt_pcm *pcm=sample?&sample->pcm:NULL;char text[80];unsigned channel,x;
    uint32_t view_start,view_end,span;
    pt_editor_wave_bounds(e,&view_start,&view_end);span=view_end-view_start;
    uint32_t start=e->sample_range_slot==e->sample?e->sample_start:0;
    uint32_t end=e->sample_range_slot==e->sample?e->sample_end:pcm?pcm->frames:0;
    panel(c,2,PT_EDITOR_HEADER_Y,636,PT_EDITOR_BOTTOM_Y-PT_EDITOR_HEADER_Y,GREY);
    if(!pcm) {label(c,font,2,PT_EDITOR_HEADER_Y,636,19,"SELECT A SAMPLE SLOT",0);return;}
    snprintf(text,sizeof(text),"SAMPLE %02u  %u BIT  %s  %lu HZ",e->sample,pcm->bits,pcm->channels==2?"STEREO":"MONO",(unsigned long)pcm->rate);
    if(e->panel==6)snprintf(text,sizeof(text),"%s LOOP %lu - %lu  /  FADE %lu FRAMES",sample->loop==PT_LOOP_NONE?"NO":sample->loop==PT_LOOP_FORWARD?"FORWARD":sample->loop==PT_LOOP_PINGPONG?"PINGPONG":"CROSSFADE",(unsigned long)sample->loop_start,(unsigned long)sample->loop_end,(unsigned long)e->loop_fade);
    if(e->panel==7)snprintf(text,sizeof(text),"SLICES %u / %s %lu  -  %s",sample->slice_count,e->slice_pending?"PROPOSED":"SAVED",(unsigned long)(e->slice_pending?e->slice_count:sample->slice_count),e->slice_pending?"APPLY OR CANCEL":"MARKERS ONLY");
    if(e->panel==9)snprintf(text,sizeof(text),"FORMAT %u BIT %lu HZ > %u BIT %lu HZ / %s",pcm->bits,(unsigned long)pcm->rate,e->format_bits,(unsigned long)e->format_rate,e->format_filtered?"FILTER":"LINEAR");
    label(c,font,2,PT_EDITOR_HEADER_Y,636,19,text,0);
    for(channel=0;channel<pcm->channels;++channel) {
        int top=254+(int)channel*(216/pcm->channels),height=216/pcm->channels-4,mid=top+height/2;
        rect(c,10,top,620,height,BLACK);
        if(pcm->frames)for(x=0;x<620;++x) {
            uint32_t first=view_start+(uint32_t)((uint64_t)x*span/620),last=view_start+(uint32_t)((uint64_t)(x+1)*span/620),f;
            int32_t low=0,high=0;int y0,y1;
            if(last==first)last=first+1;
            for(f=first;f<last && f<pcm->frames;++f) {int32_t value=pcm->data[(size_t)f*pcm->channels+channel];if(value<low)low=value;if(value>high)high=value;}
            if(first>=start && first<end)rect(c,10+(int)x,top,1,height,8);
            y0=mid-(int)((int64_t)high*(height/2-2)/((int32_t)1<<(pcm->bits-1)));
            y1=mid-(int)((int64_t)low*(height/2-2)/((int32_t)1<<(pcm->bits-1)));
            rect(c,10+(int)x,y0,1,y1-y0+1,BLUE);
        }
        else rect(c,10,mid,620,1,BLUE);
        if(pcm->frames) {
            unsigned marker;const uint32_t *markers=e->panel==7 && e->slice_pending?e->slice_markers:sample->slices;
            size_t count=e->panel==7 && e->slice_pending?e->slice_count:sample->slice_count;
            for(marker=0;marker<count;++marker) {
                int mx;
                if(markers[marker]<view_start || markers[marker]>=view_end)continue;
                mx=10+(int)((uint64_t)(markers[marker]-view_start)*619/span);
                rect(c,mx,top,1,height,e->panel==7 && e->slice_pending?YELLOW:WHITE);
            }
            if(sample->loop && sample->loop_start<view_end && sample->loop_end>view_start) {
                uint32_t a=sample->loop_start<view_start?view_start:sample->loop_start,b=sample->loop_end>view_end?view_end:sample->loop_end;
                int lx=10+(int)((uint64_t)(a-view_start)*619/span),rx=10+(int)((uint64_t)(b-view_start)*619/span);
                if(sample->loop_start>=view_start)rect(c,lx,top,1,height,YELLOW);
                if(sample->loop_end<=view_end)rect(c,rx,top,1,height,YELLOW);
                rect(c,lx,top,rx-lx+1,2,YELLOW);
            }
        }
        if(e->sample_range_slot==e->sample && e->sample_marking && pcm->frames && e->sample_anchor>=view_start && e->sample_anchor<=view_end)rect(c,10+(int)((uint64_t)(e->sample_anchor-view_start)*619/span),top,1,height,YELLOW);
    }
    snprintf(text,sizeof(text),"RANGE %lu-%lu  VIEW %lu-%lu / %lu",(unsigned long)start,(unsigned long)end,(unsigned long)view_start,(unsigned long)view_end,(unsigned long)pcm->frames);
    small(c,font,12,478,text,NAVY);
}
void pt_editor_draw(const struct pt_editor *e,struct pt_canvas *c,const uint8_t font[580])
{
    const struct pt_project *p=e->project;unsigned i,r,ch,page=pt_channels_page(&p->channels),first=page*4;
    const struct pt_sample *sample=e->sample && e->sample<=p->sample_count?&p->samples[e->sample-1]:NULL;
    char s[80];int x,y;
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
        y=PT_EDITOR_CONTROL_Y+(int)i*PT_EDITOR_CONTROL_HEIGHT;label(c,font,2,y,i?116:80,PT_EDITOR_CONTROL_HEIGHT,fields[i],0);panel(c,118,y,72,PT_EDITOR_CONTROL_HEIGHT,GREY);
        if(i==3)snprintf(s,sizeof(s),"%ld",sample?(long)sample->finetune:0L);
        else snprintf(s,sizeof(s),i>=6?"%05lX":i==5?"%04lX":"%04lu",values[i]);
        /* Extended sample lengths are shown exactly if they fit; wider values
           belong in the sampler detail view, never silently wrap. */
        if(strlen(s)>5)strcpy(s,"LARGE");
        spaced(c,font,190-(int)strlen(s)*14,y+5,s,BLACK,14);arrow(c,190,y,0);arrow(c,210,y,1);
    }
    label(c,font,82,2,18,19,"I",0);label(c,font,100,2,18,19,"D",0);
    for(r=0;r<5;++r)for(i=0;i<3;++i)label(c,font,230+(int)i*123,PT_EDITOR_CONTROL_Y+(int)r*PT_EDITOR_CONTROL_HEIGHT,123,PT_EDITOR_CONTROL_HEIGHT,buttons[r][i],r==2 && i==0 && e->editing);
    for(i=0;i<5;++i) {if(i<4)snprintf(s,sizeof(s),"%u",i+1);else s[0]=0;label(c,font,599,PT_EDITOR_CONTROL_Y+(int)i*PT_EDITOR_CONTROL_HEIGHT,39,PT_EDITOR_CONTROL_HEIGHT,s,0);}
    label(c,font,230,PT_EDITOR_COMMAND_BOTTOM,408,PT_EDITOR_CONTROL_HEIGHT,"QUADRASCOPE",0);
    for(i=0;i<4;++i) {
        x=230+(int)i*102;panel(c,x,PT_EDITOR_SCOPE_Y,102,PT_EDITOR_SCOPE_BOTTOM-PT_EDITOR_SCOPE_Y,GREY);rect(c,x+17,PT_EDITOR_SCOPE_Y+1,84,PT_EDITOR_SCOPE_BOTTOM-PT_EDITOR_SCOPE_Y-2,BLACK);
        snprintf(s,sizeof(s),"%u",first+i+1);
        if(first+i+1<10)medium(c,font,x+1,PT_EDITOR_SCOPE_Y+3,s,WHITE);else small(c,font,x+1,PT_EDITOR_SCOPE_Y+3,s,WHITE);
        /* Stopped scopes: no invented waveform or implied audio activity. */
        if(first+i<p->channels.count)rect(c,x+18,(PT_EDITOR_SCOPE_Y+PT_EDITOR_SCOPE_BOTTOM)/2,81,1,YELLOW);
    }
    panel(c,230,PT_EDITOR_SCOPE_BOTTOM,408,PT_EDITOR_CONTROL_HEIGHT,GREY);spaced(c,font,248,159,"PROTRACKER 2.4G",WHITE,10);
    spaced(c,font,456,159,"2.3F: 8BITBUBSY",WHITE,10);
    panel(c,2,174,636,19,GREY);spaced(c,font,26,179,"SONGNAME:",WHITE,10);
    snprintf(s,sizeof(s),"%.32s",p->title);spaced(c,font,128,179,s,NAVY,12);label(c,font,590,174,48,19,"LOAD",0);
    panel(c,2,193,636,18,GREY);spaced(c,font,16,198,"SAMPLENAME:",WHITE,10);
    snprintf(s,sizeof(s),"%.32s",sample?sample->name:"");spaced(c,font,128,198,s,NAVY,12);
    panel(c,590,193,48,18,GREY);snprintf(s,sizeof(s),"%luK",(unsigned long)((bytes+1023)/1024));
    if(strlen(s)>3)small(c,font,636-(int)strlen(s)*8,198,s,WHITE);else spaced(c,font,636-(int)strlen(s)*10,198,s,WHITE,10);
    draw_status(e,c,font,bytes);
    panel(c,2,PT_EDITOR_HEADER_Y,636,PT_EDITOR_PATTERN_Y-PT_EDITOR_HEADER_Y,GREY);
    for(i=0;i<4;++i) {
        ch=first+i;x=38+(int)i*150;
        if(ch<p->channels.count) {
            const struct pt_channel *channel=&p->channels.track[ch];char route=channel->route==PT_PAULA?'P':channel->route==PT_AMIGUS?'A':'M';
            snprintf(s,sizeof(s),"%u",ch+1);medium(c,font,x+64,PT_EDITOR_HEADER_Y+3,s,BLACK);snprintf(s,sizeof(s),"%c",route);small(c,font,x+130,PT_EDITOR_HEADER_Y+3,s,NAVY);
            if(channel->muted)small(c,font,x+8,PT_EDITOR_HEADER_Y+3,"M",NAVY);
            if(channel->solo)small(c,font,x+24,PT_EDITOR_HEADER_Y+3,"S",NAVY);
        }
    }
    for(i=0;i<5;++i) {
        x=36+(int)i*150;
        rect(c,x,PT_EDITOR_HEADER_Y,1,PT_EDITOR_BOTTOM_Y-PT_EDITOR_HEADER_Y,WHITE);
        rect(c,x+1,PT_EDITOR_HEADER_Y,1,PT_EDITOR_BOTTOM_Y-PT_EDITOR_HEADER_Y,DARK);
    }
    for(r=0;r<PT_EDITOR_ROWS;++r)draw_pattern_row(e,c,font,r);
    for(i=0;i<4;++i) {snprintf(s,sizeof(s),"%u-%u",i*4+1,i*4+4);label(c,font,4+(int)i*61,PT_EDITOR_BOTTOM_Y,61,21,s,page==i);}
    panel(c,248,PT_EDITOR_BOTTOM_Y,168,21,GREY);label(c,font,416,PT_EDITOR_BOTTOM_Y,60,21,"PLAY",0);
    panel(c,476,PT_EDITOR_BOTTOM_Y,18,21,GREY);rect(c,482,498,6,9,WHITE);label(c,font,494,PT_EDITOR_BOTTOM_Y,58,21,"STOP",0);
    panel(c,552,PT_EDITOR_BOTTOM_Y,86,21,GREY);small(c,font,557,497,"PATTERN",WHITE);snprintf(s,sizeof(s),"%02X",e->pattern);small(c,font,618,497,s,NAVY);
    pt_editor_draw_playback(e,c,font);
    if(e->panel>=5) {
        static const char *tabs[4]={"SAMPLER","LOOPS","SLICES","RANGE"};
        static const char *ops[5][4][3]={
            {{"LOAD SMP","SAVE WAV","AUDITION"},{"REVERSE","NORMALIZE","DC OFFS"},{"GAIN /2","GAIN X2","FORMAT"},{"FADE IN","FADE OUT","ALL"}},
            {{"FORWARD","PINGPONG","OFF"},{"FADE -","","FADE +"},{"BAKE FADE","USE LOOP","SAVE IFF"},{"ALL","UNDO","REDO"}},
            {{"ADD START","DELETE","CLEAR"},{"AUTO","APPLY","CANCEL"},{"THRESH -","","THRESH +"},{"GAP -","","GAP +"}},
            {{"ZOOM IN","ZOOM OUT","FIT ALL"},{"PAN <","ZOOM SEL","PAN >"},{"START -","","START +"},{"END -","","END +"}},
            {{"8 BIT","16 BIT","24 BIT"},{"8287 HZ","22050 HZ","44100 HZ"},{"48000 HZ","","BACK"},{"APPLY","UNDO","REDO"}}};
        for(i=0;i<4;++i)label(c,font,230+(int)(i*369/4),2,(int)((i+1)*369/4-i*369/4),19,tabs[i],e->panel==5+i);
        for(r=0;r<4;++r)for(i=0;i<3;++i)label(c,font,230+(int)i*123,21+(int)r*19,123,19,ops[e->panel-5][r][i],
            (e->panel==6 && r==0 && sample && sample->loop==(i==2?PT_LOOP_NONE:i+1)) || (e->panel==9 && r==0 && e->format_bits==(i+1)*8));
        if(e->panel==6) {snprintf(s,sizeof(s),"%lu FR",(unsigned long)e->loop_fade);label(c,font,353,40,123,19,s,0);}
        if(e->panel==7) {
            snprintf(s,sizeof(s),"%u / 1000",e->slice_threshold);label(c,font,353,59,123,19,s,0);
            snprintf(s,sizeof(s),"%uMS Z:%s",e->slice_gap_ms,e->slice_zero?"ON":"OFF");label(c,font,353,78,123,19,s,e->slice_zero);
        }
        if(e->panel==9) {
            if(e->number_field==3)snprintf(s,sizeof(s),"%s_",e->number_text);
            else snprintf(s,sizeof(s),"%lu HZ",(unsigned long)e->format_rate);
            label(c,font,353,59,123,19,s,e->number_field==3);
            label(c,font,476,59,123,19,e->format_filtered?"FILTERED":"LINEAR",e->format_filtered);
        }
        if(e->panel==8)for(i=1;i<=2;++i) {
            if(e->number_field==i)snprintf(s,sizeof(s),"%s_",e->number_text);
            else snprintf(s,sizeof(s),"%lu",(unsigned long)(i==1?e->sample_start:e->sample_end));
            label(c,font,353,40+(int)i*19,123,19,s,e->number_field==i);
        }
        draw_sample(e,c,font);
    } else if(e->panel==4) {
        const struct pt_channel *channel=&p->channels.track[p->channels.selected];
        snprintf(s,sizeof(s),"CHANNEL %02u",p->channels.selected+1);label(c,font,230,2,369,19,s,0);
        label(c,font,230,21,123,19,"PAULA",channel->route==PT_PAULA);
        label(c,font,353,21,123,19,"AMIGUS",channel->route==PT_AMIGUS);
        label(c,font,476,21,123,19,"MIDI",channel->route==PT_MIDI);
        label(c,font,230,40,123,19,"MUTE",channel->muted);
        label(c,font,353,40,123,19,"SOLO",channel->solo);
        label(c,font,476,40,123,19,"",0);
        label(c,font,230,59,123,19,"PREV",0);
        snprintf(s,sizeof(s),"%02u / %02u",p->channels.selected+1,p->channels.count);label(c,font,353,59,123,19,s,0);
        label(c,font,476,59,123,19,"NEXT",0);
        label(c,font,230,78,123,19,"UNDO",0);label(c,font,353,78,123,19,"REDO",0);label(c,font,476,78,123,19,"BACK",0);
    } else if(e->panel==3) {
        label(c,font,230,2,369,19,"NEW SONG",0);
        label(c,font,230,21,123,19,"-",0);snprintf(s,sizeof(s),"%02u CH",e->new_channels);
        label(c,font,353,21,123,19,s,0);label(c,font,476,21,123,19,"+",0);
        label(c,font,230,40,369,19,"FIRST 4 PAULA; REST AMIGUS",0);
        label(c,font,230,59,184,38,"CREATE",e->new_pending);
        label(c,font,414,59,185,38,"CANCEL",0);
    } else if(e->panel==1) {
        static const char *ops[3][3]={{"UNDO","REDO","MARK"},{"COPY","PASTE","CLEAR"},{"SEMI -","SEMI +","ALL"}};
        label(c,font,230,2,369,19,"EDIT OP.",0);
        for(r=0;r<3;++r)for(i=0;i<3;++i)label(c,font,230+(int)i*123,21+(int)r*19,123,19,ops[r][i],r==0 && i==2 && e->selection.active);
        label(c,font,230,78,123,19,"UNMARK",0);label(c,font,353,78,246,19,"BACK",0);
    } else if(e->panel==2) {
        panel(c,230,2,369,95,GREY);
        label(c,font,230,2,369,19,e->panel==1?"EDIT OP.":"DISK OP.",0);
        label(c,font,230,21,184,38,e->panel==1?"UNDO":"SAVE NEW",0);
        label(c,font,414,21,185,38,e->panel==1?"REDO":"QUIT",0);
        label(c,font,230,59,369,19,"SAVE MOD",0);
        label(c,font,230,78,369,19,"BACK",0);
    }
}

/* Small refresh region: CIA timing never waits for the display. Waveforms are
   current voice sample data scaled by current volume, not an audio capture. */
void pt_editor_draw_playback(const struct pt_editor *e,struct pt_canvas *c,const uint8_t font[580])
{
    unsigned i,j,first=pt_channels_page(&e->project->channels)*4;char s[32];
    for(i=0;i<4;++i) {
        int x=248+(int)i*102,previous=(PT_EDITOR_SCOPE_Y+PT_EDITOR_SCOPE_BOTTOM)/2;
        rect(c,x,PT_EDITOR_SCOPE_Y+1,81,PT_EDITOR_SCOPE_BOTTOM-PT_EDITOR_SCOPE_Y-2,BLACK);
        if(first+i>=e->project->channels.count)continue;
        for(j=0;j<81;++j) {
            int y=(PT_EDITOR_SCOPE_Y+PT_EDITOR_SCOPE_BOTTOM)/2;
            if(e->playback.active && !first)y-=(int)e->playback.wave[i][j]*e->playback.volume[i]/512;
            rect(c,x+(int)j,y<previous?y:previous,1,(y<previous?previous-y:y-previous)+1,YELLOW);previous=y;
        }
    }
    panel(c,248,PT_EDITOR_BOTTOM_Y,168,21,GREY);
    if(e->playback.active) {
        snprintf(s,sizeof(s),"POS %03u ROW %02u",e->playback.order,e->playback.row);small(c,font,253,497,s,NAVY);
    }
    panel(c,14,214,46,17,MID);
    snprintf(s,sizeof(s),"%02u",e->playback.active?e->playback.speed:e->project->speed);spaced(c,font,22,218,s,NAVY,14);
    rect(c,79,213,32,10,GREY);snprintf(s,sizeof(s),"%03u",e->playback.active?e->playback.bpm:e->project->bpm);spaced(c,font,79,213,s,NAVY,10);
    rect(c,598,223,32,10,GREY);spaced(c,font,598,223,e->playback.active?"CIA":"---",NAVY,10);
}

unsigned pt_editor_draw_update(const struct pt_editor *e,struct pt_canvas *c,const uint8_t font[580],
                              struct pt_view_cache *old,struct pt_view_rect areas[PT_VIEW_DIRTY_MAX])
{
    struct pt_project metadata;struct pt_sample sample;unsigned i,r,count=0,page=pt_channels_page(&e->project->channels);
    size_t bytes=0;int full,playback_changed,selection_changed;struct pt_editor_selection selection;
    pt_editor_selection(e,&selection);
    selection_changed=memcmp(&selection,&old->selection,sizeof(selection))!=0;
    memcpy(&metadata,e->project,sizeof(metadata));metadata.channels.selected=0;
    memset(&sample,0,sizeof(sample));
    if(e->sample && e->sample<=e->project->sample_count)memcpy(&sample,&e->project->samples[e->sample-1],sizeof(sample));
    for(i=0;i<e->project->sample_count;++i) {
        const struct pt_pcm *pcm=&e->project->samples[i].pcm;
        bytes+=(size_t)pcm->frames*pcm->channels*(pcm->bits/8);
    }
    full=(e->panel>=5 && (old->sample_ui!=e->sample_ui || old->sample_start!=e->sample_start || old->sample_end!=e->sample_end || old->sample_marking!=e->sample_marking || old->sample_anchor!=e->sample_anchor || old->sample_range_slot!=e->sample_range_slot)) || !old->valid || old->page!=page || old->pattern!=e->pattern || old->first_row!=e->first_row ||
         old->position!=e->position || old->sample!=e->sample || old->editing!=e->editing || old->panel!=e->panel || (e->panel==4 && old->selected!=e->project->channels.selected) || (e->panel==1 && old->selection.active!=selection.active) ||
         old->sample_bytes!=bytes || memcmp(&metadata,&old->project,sizeof(metadata)) || memcmp(&sample,&old->sample_meta,sizeof(sample));
    playback_changed=!old->valid || memcmp(&e->playback,&old->playback,sizeof(e->playback));
    if(full) {pt_editor_draw(e,c,font);areas[count++]=(struct pt_view_rect){0,0,640,512};}
    else {
        if(e->panel==3 && old->new_channels!=e->new_channels) {
            char count_text[16];snprintf(count_text,sizeof(count_text),"%02u CH",e->new_channels);
            label(c,font,353,21,123,19,count_text,0);areas[count++]=(struct pt_view_rect){353,21,123,19};
        }
        if(e->panel==3 && old->new_pending!=e->new_pending) {
            label(c,font,230,59,184,38,"CREATE",e->new_pending);areas[count++]=(struct pt_view_rect){230,59,184,38};
        }
        if(strcmp(old->status,e->status) || old->dirty!=(unsigned)pt_editor_dirty(e)) {
            draw_status(e,c,font,bytes);areas[count++]=(struct pt_view_rect){2,211,636,23};
        }
        if(playback_changed) {
            pt_editor_draw_playback(e,c,font);
            for(i=0;i<PT_VIEW_PLAYBACK_AREAS;++i)areas[count++]=pt_view_playback_areas[i];
        }
    }
    for(r=0;r<PT_EDITOR_ROWS;++r) {
        struct pt_event events[4];unsigned row=r+e->first_row;int changed;
        memset(events,0,sizeof(events));
        for(i=0;i<4 && row<64;++i)if(page*4+i<e->project->channels.count)
            memcpy(&events[i],&e->project->events[(e->pattern*64+row)*e->project->channels.count+page*4+i],sizeof(events[i]));
        changed=memcmp(events,old->events[r],sizeof(events))!=0;
        if((row==old->row || row==e->row) && (old->row!=e->row || old->field!=e->field || old->selected!=e->project->channels.selected))changed=1;
        if(selection_changed)changed=1;
        if(!full && changed && e->panel<5) {draw_pattern_row(e,c,font,r);areas[count++]=(struct pt_view_rect){3,PT_EDITOR_PATTERN_Y+r*12,635,12};}
        memcpy(old->events[r],events,sizeof(events));
    }
    memcpy(&old->project,&metadata,sizeof(metadata));memcpy(&old->sample_meta,&sample,sizeof(sample));
    memcpy(&old->playback,&e->playback,sizeof(e->playback));strcpy(old->status,e->status);
    old->new_channels=e->new_channels;old->new_pending=e->new_pending;
    old->selection=selection;
    old->valid=1;old->page=page;old->pattern=e->pattern;old->first_row=e->first_row;old->position=e->position;
    old->sample_ui=e->sample_ui;old->sample_start=e->sample_start;old->sample_end=e->sample_end;old->sample_marking=e->sample_marking;old->sample_anchor=e->sample_anchor;old->sample_range_slot=e->sample_range_slot;
    old->sample=e->sample;old->editing=e->editing;old->panel=e->panel;old->row=e->row;old->field=e->field;
    old->selected=e->project->channels.selected;old->dirty=pt_editor_dirty(e);old->sample_bytes=bytes;
    return count;
}
