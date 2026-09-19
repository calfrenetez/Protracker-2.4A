#include <stdio.h>
#include <string.h>
#include "editor.h"
static const unsigned periods[36]={856,808,762,720,678,640,604,570,538,508,480,453,
    428,404,381,360,339,320,302,285,269,254,240,226,214,202,190,180,170,160,151,143,135,127,120,113};
static const char *notes[12]={"C-","C#","D-","D#","E-","F-","F#","G-","G#","A-","A#","B-"};
void pt_editor_note(const struct pt_event *e,char out[4])
{
    unsigned i,n;
    memcpy(out,"---",4);
    if(e->kind==PT_NOTE_OFF) {memcpy(out,"OFF",4);return;}
    if(e->kind==PT_NOTE_MIDI) {
        /* Display MIDI note 0 as C-0 (not scientific pitch notation). */
        n=e->pitch;if(n/12>9) {snprintf(out,4,"%03u",n);return;}
    } else if(e->kind==PT_NOTE_PERIOD) {
        for(i=0;i<36 && periods[i]!=e->pitch;++i) {}
        if(i==36) {memcpy(out,"???",4);return;}
        n=i+12;
    } else return;
    out[0]=notes[n%12][0];out[1]=notes[n%12][1];out[2]=(char)('0'+n/12);out[3]=0;
}
void pt_editor_status(struct pt_editor *e,const char *s)
{snprintf(e->status,sizeof(e->status),"%s",s);}
int pt_editor_init(struct pt_editor *e,struct pt_project *p)
{
    if(!e || pt_project_validate(p,NULL)!=PT_PROJECT_OK)return 0;
    memset(e,0,sizeof(*e));e->project=p;e->sample=p->sample_count?1:0;e->octave=1;e->new_channels=p->channels.count;
    if(pt_pattern_history_init(&e->history,p,e->commands,128,e->changes,2048)!=PT_EDIT_OK)return 0;
    e->clipboard.events=e->clipboard_events;e->clipboard.capacity=1024;
    pt_editor_status(e,"READY - F8 PLAY / F9 PATTERN / F10 STOP");return 1;
}
int pt_editor_dirty(const struct pt_editor *e) {return pt_pattern_dirty(&e->history);}
void pt_editor_saved(struct pt_editor *e)
{pt_pattern_mark_saved(&e->history);e->quit_pending=0;pt_editor_status(e,"PROJECT SAVED AND VERIFIED");}
static void visible(struct pt_editor *e)
{
    if(e->row<e->first_row)e->first_row=e->row;
    if(e->row>=e->first_row+PT_EDITOR_ROWS)e->first_row=e->row-PT_EDITOR_ROWS+1;
}
static void undo(struct pt_editor *e,int direction)
{
    enum pt_edit_result r=pt_pattern_undo(e->project,&e->history,direction);
    pt_editor_status(e,r==PT_EDIT_OK?(direction<0?"UNDO":"REDO"):r==PT_EDIT_END?"NO MORE HISTORY":"UNDO CONFLICT - EDIT PRESERVED");
}
static void channel_panel(struct pt_editor *e)
{
    e->panel=4;pt_editor_status(e,"CHANNEL: P/A/M ROUTE; U MUTE; S SOLO; TAB NEXT; ESC BACK");
}
/* Route is exclusive; mute and solo are independent saved channel properties. */
static void channel_edit(struct pt_editor *e,unsigned setting)
{
    unsigned selected=e->project->channels.selected;
    struct pt_channel value=e->project->channels.track[selected];enum pt_edit_result result;
    if(setting==8)value.muted^=1;
    else if(setting==16)value.solo^=1;
    else value.route=(uint8_t)setting;
    result=pt_pattern_channel_apply(e->project,&e->history,selected,&value);
    pt_editor_status(e,result==PT_EDIT_OK?"CHANNEL UPDATED - CONTROL-Z TO UNDO":
        result==PT_EDIT_PAULA_LIMIT?"PAULA LIMIT: CHANGE ANOTHER PAULA ROUTE FIRST":
        result==PT_EDIT_CAPACITY?"UNDO BUDGET EXCEEDED - NO CHANGE":"CHANNEL CHANGE REFUSED");
}
static int apply(struct pt_editor *e,struct pt_event event)
{
    struct pt_event_update u;enum pt_edit_result r;
    u.index=(e->pattern*64+e->row)*e->project->channels.count+e->project->channels.selected;u.event=event;
    r=pt_pattern_apply(e->project,&e->history,&u,1);
    pt_editor_status(e,r==PT_EDIT_OK?"PATTERN EDITED":r==PT_EDIT_CAPACITY?"UNDO BUDGET EXCEEDED - NO CHANGE":"INVALID EVENT - NO CHANGE");
    return r==PT_EDIT_OK;
}
int pt_editor_selection(const struct pt_editor *e,struct pt_editor_selection *s)
{
    *s=e->selection;
    if(!s->active || s->pattern!=e->pattern) {memset(s,0,sizeof(*s));return 0;}
    if(s->marking) {
        unsigned c=e->project->channels.selected;
        s->r0=e->row<s->anchor_row?e->row:s->anchor_row;
        s->r1=(e->row>s->anchor_row?e->row:s->anchor_row)+1;
        s->c0=c<s->anchor_channel?c:s->anchor_channel;
        s->c1=(c>s->anchor_channel?c:s->anchor_channel)+1;
    }
    return 1;
}
static void unmark(struct pt_editor *e)
{memset(&e->selection,0,sizeof(e->selection));pt_editor_status(e,"BLOCK UNMARKED");}
static void mark(struct pt_editor *e)
{
    if(e->selection.active) {unmark(e);return;}
    e->selection.active=1;e->selection.marking=1;e->selection.pattern=e->pattern;
    e->selection.anchor_row=e->row;e->selection.anchor_channel=e->project->channels.selected;
    pt_editor_status(e,"MARKING BLOCK - MOVE CURSOR; COPY FREEZES SELECTION");
}
static void select_all(struct pt_editor *e)
{
    memset(&e->selection,0,sizeof(e->selection));e->selection.active=1;e->selection.pattern=e->pattern;
    e->selection.r1=64;e->selection.c1=e->project->channels.count;
    pt_editor_status(e,"WHOLE PATTERN SELECTED - COPY TO CLONE AT ROW 00 CH 1");
}
/* op: 0 copy, 1 paste, 2 clear, -1/+3 transpose down/up. */
static void block_edit(struct pt_editor *e,int op)
{
    struct pt_editor_selection s;enum pt_edit_result result;unsigned r,c,n=0;
    uint32_t revision=e->history.revision;char status[76];
    if(op!=1 && !pt_editor_selection(e,&s)) {pt_editor_status(e,"MARK A BLOCK FIRST - CONTROL-B OR EDIT OP. > MARK");return;}
    if(op==0) {
        result=pt_pattern_copy(e->project,e->pattern,s.r0,s.r1,s.c0,s.c1,&e->clipboard);
        if(result==PT_EDIT_OK) {
            s.marking=0;e->selection=s;
            snprintf(status,sizeof(status),"COPIED %u ROWS X %u CHANNELS - PASTE AT CURSOR",e->clipboard.rows,e->clipboard.channels);
            pt_editor_status(e,status);return;
        }
    } else if(op==1) {
        if(!e->clipboard.rows) {pt_editor_status(e,"CLIPBOARD EMPTY - COPY A BLOCK FIRST");return;}
        if(e->row+e->clipboard.rows>64 || e->project->channels.selected+e->clipboard.channels>e->project->channels.count) {
            pt_editor_status(e,"PASTE WOULD CROSS PATTERN EDGE - NO CHANGE");return;
        }
        result=pt_pattern_paste(e->project,&e->history,e->pattern,e->row,e->project->channels.selected,&e->clipboard,e->scratch,1024);
    } else if(op==2) {
        for(r=s.r0;r<s.r1;++r)for(c=s.c0;c<s.c1;++c) {
            e->scratch[n].index=(e->pattern*64+r)*e->project->channels.count+c;
            memset(&e->scratch[n].event,0,sizeof(e->scratch[n].event));++n;
        }
        result=pt_pattern_apply(e->project,&e->history,e->scratch,n);
    } else result=pt_pattern_transpose(e->project,&e->history,e->pattern,s.r0,s.r1,s.c0,s.c1,op<0?-1:1,e->scratch,1024);
    if(result==PT_EDIT_OK) {
        if(op!=1) {s.marking=0;e->selection=s;}
        pt_editor_status(e,e->history.revision==revision?"BLOCK ALREADY MATCHES - NO CHANGE":
            op==1?"BLOCK PASTED - CONTROL-Z TO UNDO":op==2?"BLOCK CLEARED - CONTROL-Z TO UNDO":"BLOCK TRANSPOSED - CONTROL-Z TO UNDO");
    } else pt_editor_status(e,result==PT_EDIT_UNSUPPORTED?"TRANSPOSE OUT OF RANGE OR RAW PERIOD - NO CHANGE":
        result==PT_EDIT_CAPACITY?"UNDO BUDGET EXCEEDED - NO CHANGE":"BLOCK EDIT REFUSED - NO CHANGE");
}
static enum pt_editor_action quit(struct pt_editor *e)
{
    if(!pt_editor_dirty(e) || e->quit_pending)return PT_UI_QUIT;
    e->quit_pending=1;pt_editor_status(e,"UNSAVED EDITS: ESC AGAIN TO DISCARD; OTHER KEY CANCELS");return PT_UI_NONE;
}
static void new_panel(struct pt_editor *e)
{
    e->panel=3;e->new_channels=e->project->channels.count;e->new_pending=0;
    pt_editor_status(e,"NEW SONG: CHOOSE CHANNELS; CREATE OR ENTER TO CONTINUE");
}
static enum pt_editor_action request_new(struct pt_editor *e)
{
    if(pt_editor_dirty(e) && !e->new_pending) {
        e->new_pending=1;pt_editor_status(e,"UNSAVED EDITS: CREATE AGAIN TO DISCARD; OTHER INPUT CANCELS");return PT_UI_NONE;
    }
    e->new_pending=0;return PT_UI_NEW;
}
static enum pt_editor_action request_load(struct pt_editor *e)
{
    e->quit_pending=0;
    if(pt_editor_dirty(e) && !e->load_pending) {
        e->load_pending=1;pt_editor_status(e,"UNSAVED EDITS: LOAD AGAIN TO DISCARD; OTHER KEY CANCELS");return PT_UI_NONE;
    }
    e->load_pending=0;return PT_UI_LOAD;
}
static void pattern_step(struct pt_editor *e,int d)
{e->pattern=(e->pattern+e->project->pattern_count+d)%e->project->pattern_count;memset(&e->selection,0,sizeof(e->selection));}
static int hexkey(unsigned raw)
{
    if(raw>=1 && raw<=9)return (int)raw;
    if(raw==10)return 0;
    switch(raw) {case 0x20:return 10;case 0x35:return 11;case 0x33:return 12;
        case 0x22:return 13;case 0x12:return 14;case 0x23:return 15;default:return -1;}
}
enum pt_editor_action pt_editor_key(struct pt_editor *e,unsigned raw,unsigned qualifier)
{
    struct pt_project *p=e->project;struct pt_event event;int n=-1,h;unsigned i;
    static const unsigned keys[24]={0x31,0x21,0x32,0x22,0x33,0x34,0x24,0x35,0x25,0x36,0x26,0x37,
        0x10,0x02,0x11,0x03,0x12,0x13,0x05,0x14,0x06,0x15,0x07,0x16};
    if(raw&0x80)return PT_UI_NONE;
    if(e->panel==3 && raw==0x44)return request_new(e);
    if(e->new_pending)pt_editor_status(e,"NEW SONG CANCELLED - EDITS PRESERVED");
    e->new_pending=0;
    if((qualifier&8) && raw==0x18)return request_load(e);
    if(e->load_pending)pt_editor_status(e,"LOAD CANCELLED - EDITS PRESERVED");
    e->load_pending=0;
    if(raw==0x45 && e->panel==3) {e->panel=0;pt_editor_status(e,"NEW SONG CANCELLED - EDITS PRESERVED");return PT_UI_NONE;}
    if(raw==0x45 && e->panel==4) {e->panel=0;pt_editor_status(e,"CHANNEL SETTINGS CLOSED");return PT_UI_NONE;}
    if(raw==0x45)return quit(e);
    e->quit_pending=0;
    if(e->panel==3) {
        if(raw==0x0b && e->new_channels>1)--e->new_channels;
        if(raw==0x0c && e->new_channels<16)++e->new_channels;
        return PT_UI_NONE;
    }
    /* Raw Amiga qualifiers: either Shift=bits0/1, Control=bit3. */
    if(qualifier&8) {
        if(raw==0x31)undo(e,(qualifier&3)?1:-1);
        else if(raw==0x21)return (qualifier&3)?PT_UI_SAVE_AS:PT_UI_SAVE;
        else if(raw==0x36)new_panel(e); /* Control-N */
        else if(raw==0x37 && (qualifier&3))return PT_UI_EXPORT_MOD;
        else if(raw==0x13) {if(e->panel==4)e->panel=0;else channel_panel(e);} /* Control-R */
        else if(e->panel==4)return PT_UI_NONE;
        else if(raw==0x12)e->panel=e->panel==1?0:1; /* Control-E */
        else if(raw==0x35)mark(e); /* Control-B */
        else if(raw==0x20)select_all(e);
        else if(raw==0x33)block_edit(e,0);
        else if(raw==0x34)block_edit(e,1);
        else if(raw==0x46)block_edit(e,2);
        else if(raw==0x0b)block_edit(e,-1);
        else if(raw==0x0c)block_edit(e,3);
        return PT_UI_NONE;
    }
    if(e->panel==4) {
        if(raw==0x19)channel_edit(e,PT_PAULA);
        else if(raw==0x20)channel_edit(e,PT_AMIGUS);
        else if(raw==0x37)channel_edit(e,PT_MIDI);
        else if(raw==0x16)channel_edit(e,8);
        else if(raw==0x21)channel_edit(e,16);
        else if(raw==0x42 || raw==0x4e || raw==0x4f)pt_channels_step(&p->channels,raw==0x4f || (raw==0x42 && (qualifier&3))?-1:1);
        else if(raw>=0x50 && raw<=0x53) {i=(raw-0x50)*4;if(i<p->channels.count)p->channels.selected=(uint8_t)i;}
        else if(raw==0x57)return PT_UI_PLAY;
        else if(raw==0x44)return (qualifier&3)?PT_UI_PATTERN:PT_UI_PLAY;
        else if(raw==0x58)return PT_UI_PATTERN;
        else if(raw==0x59 || (raw==0x40 && e->playback.active))return PT_UI_STOP;
        return PT_UI_NONE;
    }
    switch(raw) {
        case 0x57:return PT_UI_PLAY;
        case 0x58:return PT_UI_PATTERN;
        case 0x59:return PT_UI_STOP;
        case 0x44:return (qualifier&3)?PT_UI_PATTERN:PT_UI_PLAY;
        case 0x40:if(e->playback.active)return PT_UI_STOP;e->editing=!e->editing;pt_editor_status(e,e->editing?"EDIT ON - NOTES Z-M/Q-U; DELETE CLEARS; BACKSPACE OFF":"EDIT OFF");break;
        case 0x4c:e->row=(e->row+63)%64;visible(e);break;
        case 0x4d:e->row=(e->row+1)%64;visible(e);break;
        case 0x4e:if(++e->field==6) {e->field=0;pt_channels_step(&p->channels,1);}break;
        case 0x4f:if(!e->field) {e->field=5;pt_channels_step(&p->channels,-1);}else --e->field;break;
        case 0x42:pt_channels_step(&p->channels,(qualifier&3)?-1:1);break;
        case 0x50:case 0x51:case 0x52:case 0x53:
            i=(raw-0x50)*4;if(i<p->channels.count)p->channels.selected=(uint8_t)i;break;
        case 0x54:e->octave=0;break;
        case 0x55:e->octave=1;break;
        case 0x56:e->octave=2;break;
        case 0x0b:if(e->sample)--e->sample;break;
        case 0x0c:if(e->sample<p->sample_count)++e->sample;break;
        case 0x5a:pattern_step(e,-1);break; /* numeric keypad ( */
        case 0x5b:pattern_step(e,1);break;
        default:
            if(!e->editing)break;
            event=p->events[(e->pattern*64+e->row)*p->channels.count+p->channels.selected];
            if(raw==0x46) {memset(&event,0,sizeof(event));if(apply(e,event))e->row=(e->row+1)%64;}
            else if(e->field==0) {
                if(raw==0x41) {event.kind=PT_NOTE_OFF;event.pitch=0;event.slice=0;event.flags=0;event.velocity=0;}
                else {
                    for(i=0;i<24;++i)if(raw==keys[i]) {n=(int)(i+e->octave*12);break;}
                    if(n<0)break;
                    if(p->channels.track[p->channels.selected].route==PT_MIDI) {event.kind=PT_NOTE_MIDI;event.pitch=(uint16_t)(n+12);}
                    else {if(n>=36) {pt_editor_status(e,"NOTE OUTSIDE CLASSIC THREE-OCTAVE RANGE");break;}event.kind=PT_NOTE_PERIOD;event.pitch=(uint16_t)periods[n];}
                    event.instrument=(uint8_t)e->sample;event.slice=0;event.flags=0;event.velocity=0;
                }
                if(apply(e,event))e->row=(e->row+1)%64;
            } else if((h=hexkey(raw))>=0) {
                if(e->field==1)event.instrument=(uint8_t)((event.instrument&15)|(h<<4));
                if(e->field==2)event.instrument=(uint8_t)((event.instrument&240)|h);
                if(e->field==3)event.effect=(uint8_t)h;
                if(e->field==4)event.parameter=(uint8_t)((event.parameter&15)|(h<<4));
                if(e->field==5)event.parameter=(uint8_t)((event.parameter&240)|h);
                if(apply(e,event)) {if(e->field==5) {e->field=3;e->row=(e->row+1)%64;}else ++e->field;}
            }
            visible(e);break;
    }
    return PT_UI_NONE;
}
enum pt_editor_action pt_editor_click(struct pt_editor *e,int x,int y)
{
    unsigned c,r,f;
    if(x<0 || x>=640 || y<0 || y>=512)return PT_UI_NONE;
    if(e->panel==3 && x>=230 && x<414 && y>=59 && y<97)return request_new(e);
    if(e->new_pending)pt_editor_status(e,"NEW SONG CANCELLED - EDITS PRESERVED");
    e->new_pending=0;
    if(x>=590 && y>=174 && y<193)return request_load(e);
    if(e->load_pending)pt_editor_status(e,"LOAD CANCELLED - EDITS PRESERVED");
    e->load_pending=0;
    if(e->panel==2 && x>=414 && x<599 && y>=21 && y<59)return quit(e);
    e->quit_pending=0;
    if(e->panel==3) {
        if(x>=230 && x<599 && y>=21 && y<40) {
            if(x<353 && e->new_channels>1)--e->new_channels;
            if(x>=476 && e->new_channels<16)++e->new_channels;
        } else if(x>=414 && x<599 && y>=59 && y<97) {e->panel=0;pt_editor_status(e,"NEW SONG CANCELLED - EDITS PRESERVED");}
        return PT_UI_NONE;
    }
    if(e->panel==4 && x>=230 && x<599 && y>=2 && y<97) {
        r=(unsigned)(y-PT_EDITOR_CONTROL_Y)/PT_EDITOR_CONTROL_HEIGHT;c=(unsigned)(x-230)/123;
        if(r==1)channel_edit(e,c==0?PT_PAULA:c==1?PT_AMIGUS:PT_MIDI);
        else if(r==2 && c<2)channel_edit(e,c==0?8:16);
        else if(r==3 && c!=1)pt_channels_step(&e->project->channels,c==0?-1:1);
        else if(r==4) {if(c<2)undo(e,c==0?-1:1);else e->panel=0;}
        return PT_UI_NONE;
    }
    if(e->panel==1 && x>=230 && x<599 && y>=2 && y<97) {
        r=(unsigned)(y-PT_EDITOR_CONTROL_Y)/PT_EDITOR_CONTROL_HEIGHT;c=(unsigned)(x-230)/123;
        if(r==1) {if(c<2)undo(e,c==0?-1:1);else mark(e);}
        else if(r==2)block_edit(e,(int)c);
        else if(r==3) {if(c==2)select_all(e);else block_edit(e,c==0?-1:3);}
        else if(r==4) {if(c==0)unmark(e);else e->panel=0;}
        return PT_UI_NONE;
    }
    if(e->panel==2 && x>=230 && x<599 && y>=2 && y<97) {
        if(y>=78)e->panel=0;
        else if(y>=59)return PT_UI_EXPORT_MOD;
        else if(y>=21) {if(e->panel==2)return PT_UI_SAVE_AS;undo(e,x<414?-1:1);}
        return PT_UI_NONE;
    }
    if(y>=PT_EDITOR_BOTTOM_Y && x>=4 && x<248) {c=(unsigned)(x-4)/61*4;if(c<e->project->channels.count)e->project->channels.selected=(uint8_t)c;}
    else if(y>=PT_EDITOR_PATTERN_Y && y<PT_EDITOR_PATTERN_Y+240 && x>=38 && x<638) {
        c=(unsigned)(x-38)/150+pt_channels_page(&e->project->channels)*4;
        r=(unsigned)(y-PT_EDITOR_PATTERN_Y)/12+e->first_row;
        if(c<e->project->channels.count && r<64) {
            e->project->channels.selected=(uint8_t)c;e->row=r;
            f=(unsigned)(x-38)%150;
            e->field=f<60?0:f<78?1:f<98?2:f<114?3:f<128?4:5;
        }
    } else if(y>=PT_EDITOR_HEADER_Y && y<PT_EDITOR_PATTERN_Y && x>=38 && x<638) {
        c=(unsigned)(x-38)/150+pt_channels_page(&e->project->channels)*4;
        if(c<e->project->channels.count) {e->project->channels.selected=(uint8_t)c;
            if((unsigned)(x-38)%150>=120)channel_panel(e);}
    } else if(x>=230 && x<599 && y>=PT_EDITOR_CONTROL_Y && y<PT_EDITOR_COMMAND_BOTTOM) {
        r=(unsigned)(y-PT_EDITOR_CONTROL_Y)/PT_EDITOR_CONTROL_HEIGHT;c=(unsigned)(x-230)/123;
        if(r==0 && c==0)return PT_UI_PLAY;
        else if(r==0 && c==1)return PT_UI_STOP;
        else if(r==1 && c==0)return PT_UI_PATTERN;
        else if(r==1 && c==1)new_panel(e);
        else if(r==4 && c==0)return PT_UI_AUDITION;
        else if(r==2 && c==0) {e->editing=!e->editing;pt_editor_status(e,e->editing?"EDIT ON":"EDIT OFF");}
        else if(r==2 && c==1)e->panel=1;
        else if(r==3 && c==1)e->panel=2;
        else pt_editor_status(e,"THIS CONTROL IS NOT YET CONNECTED");
    } else if(x>=190 && x<230 && y>=2 && y<173) {
        int direction=x<210?-1:1;r=(unsigned)(y-PT_EDITOR_CONTROL_Y)/PT_EDITOR_CONTROL_HEIGHT;
        if(r==0) {
            if(direction>0 && e->position+1<e->project->order_count)++e->position;
            if(direction<0 && e->position)--e->position;
            e->pattern=e->project->orders[e->position];memset(&e->selection,0,sizeof(e->selection));
        } else if(r==1)pattern_step(e,direction);
        else if(r==4) {if(direction<0 && e->sample)--e->sample;if(direction>0 && e->sample<e->project->sample_count)++e->sample;}
        else pt_editor_status(e,"SAMPLE/SONG PARAMETER EDITING NOT YET CONNECTED");
    } else if(y>=PT_EDITOR_BOTTOM_Y && x>=416 && x<476)return PT_UI_PLAY;
    else if(y>=PT_EDITOR_BOTTOM_Y && x>=476 && x<552)return PT_UI_STOP;
    else if((y>=PT_EDITOR_BOTTOM_Y && x>=416 && x<552) || (x>=599 && y<97) || (x>=590 && y>=174 && y<193))
        pt_editor_status(e,"THIS CONTROL IS NOT YET CONNECTED");
    return PT_UI_NONE;
}
