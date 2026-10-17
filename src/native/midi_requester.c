/* Candidate own-window normal-Task UI. Saved metadata only: no CAMD operation,
 * parent editor dispatch, clock polling or source/owner teardown occurs here. */
#include <intuition/intuition.h>
#include <graphics/text.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <stdio.h>
#include <string.h>
#include "midi_requester.h"

static void label(struct Window *w,int x,int y,const char *text)
{
    Move(w->RPort,w->BorderLeft+x,w->BorderTop+y);
    Text(w->RPort,(STRPTR)text,(ULONG)strlen(text));
}
static void outline(struct Window *w,unsigned item,unsigned focus)
{
    const struct pt_midi_requester_rect *r=&pt_midi_requester_controls[item];
    int left=w->BorderLeft+r->left,top=w->BorderTop+r->top;
    int right=w->BorderLeft+r->right-1,bottom=w->BorderTop+r->bottom-1;
    SetAPen(w->RPort,item==focus?3:1);Move(w->RPort,left,top);Draw(w->RPort,right,top);
    Draw(w->RPort,right,bottom);Draw(w->RPort,left,bottom);Draw(w->RPort,left,top);SetAPen(w->RPort,1);
}
static const char *result_text(enum pt_midi_setup_result result)
{
    switch(result){
    case PT_MIDI_SETUP_OK:return "Draft only; Apply accepts this one field.";
    case PT_MIDI_SETUP_ACTIVE:return "Apply refused: stop playback/capture/workflow first.";
    case PT_MIDI_SETUP_VETO:return "Apply refused by editor ownership barrier; no Stop sent.";
    case PT_MIDI_SETUP_CAPACITY:return "Metadata history capacity unavailable; Cancel or retry.";
    case PT_MIDI_SETUP_STALE:return "Saved sources changed; Cancel and reopen this requester.";
    case PT_MIDI_SETUP_HELD:return "HELD: original controller retained; close UI only.";
    case PT_MIDI_SETUP_BUSY:return "Operation busy; no new operation started.";
    default:return "Apply refused; current saved settings remain unchanged.";
    }
}
static void draw(struct Window *w,const struct pt_midi_requester_model *m)
{
    char track[64];
    SetAPen(w->RPort,0);RectFill(w->RPort,w->BorderLeft,w->BorderTop,
        w->BorderLeft+PT_MIDI_REQUESTER_WIDTH-1,w->BorderTop+PT_MIDI_REQUESTER_HEIGHT-1);
    SetAPen(w->RPort,1);SetBPen(w->RPort,0);SetDrMd(w->RPort,JAM2);
    label(w,14,18,PT_MIDI_REQUESTER_NOTICE);
    if(m->stage==PT_MIDI_REQUESTER_CHOOSE){
        label(w,14,40,"Choose one field. No driver discovery or connection.");
        outline(w,PT_MIDI_FOCUS_INPUT,m->focus);label(w,22,68,"Global input endpoint identity");
        outline(w,PT_MIDI_FOCUS_OUTPUT,m->focus);
        snprintf(track,sizeof(track),"Current-track output endpoint (track %u)",m->track+1);label(w,22,102,track);
        outline(w,PT_MIDI_FOCUS_CLOCK,m->focus);label(w,22,136,"Global clock flags");
    }else if(m->target==PT_MIDI_SETUP_CLOCK){
        label(w,14,46,"Global clock flags: saved policy, no clock traffic.");
        outline(w,PT_MIDI_FOCUS_SEND,m->focus);outline(w,PT_MIDI_FOCUS_RECEIVE,m->focus);
        label(w,22,88,m->flags&1?"[X] Send clock when a backend is implemented":"[ ] Send clock when a backend is implemented");
        label(w,22,126,m->flags&2?"[X] Receive clock when a backend is implemented":"[ ] Receive clock when a backend is implemented");
    }else{
        label(w,14,46,m->target==PT_MIDI_SETUP_INPUT?"Global input endpoint identity":"Current-track output endpoint identity");
        label(w,14,70,"Exact Latin-1 name; 63 characters. Empty clears it.");
        outline(w,PT_MIDI_FOCUS_NAME,m->focus);label(w,20,102,m->name);
        if(m->focus==PT_MIDI_FOCUS_NAME){
            int x=w->BorderLeft+20+(int)m->cursor*8;
            Move(w->RPort,x,w->BorderTop+90);Draw(w->RPort,x,w->BorderTop+106);
        }
    }
    label(w,14,164,"Tab/Shift-Tab: focus. Space/Return: activate.");
    label(w,14,180,"Escape/right mouse/close: Cancel. Ctrl-U: clear name.");
    if(m->stage==PT_MIDI_REQUESTER_FIELD){outline(w,PT_MIDI_FOCUS_APPLY,m->focus);label(w,38,224,"Apply");}
    outline(w,PT_MIDI_FOCUS_CANCEL,m->focus);label(w,168,224,"Cancel");
    label(w,14,250,result_text(m->last_result));
}
static enum pt_midi_requester_event key(struct pt_midi_requester_model *m,
    ULONG kind,UWORD code,UWORD qualifier)
{
    if(kind==IDCMP_VANILLAKEY){
        if(code==27)return pt_midi_requester_model_key(m,PT_MIDI_KEY_ESCAPE,0);
        if(code==9)return pt_midi_requester_model_key(m,qualifier&3?PT_MIDI_KEY_BACKTAB:PT_MIDI_KEY_TAB,0);
        if(code==13||code==10)return pt_midi_requester_model_key(m,PT_MIDI_KEY_ACTIVATE,0);
        if(code==8)return pt_midi_requester_model_key(m,PT_MIDI_KEY_BACKSPACE,0);
        if(code==127)return pt_midi_requester_model_key(m,PT_MIDI_KEY_DELETE,0);
        if(code==21)return pt_midi_requester_model_key(m,PT_MIDI_KEY_CLEAR,0);
        return pt_midi_requester_model_key(m,PT_MIDI_KEY_CHARACTER,code);
    }
    if(kind!=IDCMP_RAWKEY||(code&0x80))return PT_MIDI_EVENT_NONE;
    if(code==0x45)return pt_midi_requester_model_key(m,PT_MIDI_KEY_ESCAPE,0);
    if(code==0x42)return pt_midi_requester_model_key(m,qualifier&3?PT_MIDI_KEY_BACKTAB:PT_MIDI_KEY_TAB,0);
    if(code==0x44||code==0x43)return pt_midi_requester_model_key(m,PT_MIDI_KEY_ACTIVATE,0);
    if(code==0x41)return pt_midi_requester_model_key(m,PT_MIDI_KEY_BACKSPACE,0);
    if(code==0x46)return pt_midi_requester_model_key(m,PT_MIDI_KEY_DELETE,0);
    if(code==0x4f)return pt_midi_requester_model_key(m,qualifier&8?PT_MIDI_KEY_HOME:PT_MIDI_KEY_LEFT,0);
    if(code==0x4e)return pt_midi_requester_model_key(m,qualifier&8?PT_MIDI_KEY_END:PT_MIDI_KEY_RIGHT,0);
    if(code==0x4c||code==0x4d)return pt_midi_requester_model_key(m,code==0x4c?PT_MIDI_KEY_BACKTAB:PT_MIDI_KEY_TAB,0);
    return PT_MIDI_EVENT_NONE;
}
/* Observe the already-open chooser without reopening/copying/resetting it.
 * Complete storage/source lifetime and nonalias placement remain caller duties. */
static enum pt_midi_setup_result chooser_admit(const struct pt_midi_requester_model *m)
{
    struct pt_midi_setup_status status;struct pt_midi_setup_view view;
    const struct pt_midi_setup_discovery unknown={0};enum pt_midi_setup_result result;
    if(!m||sizeof(*m)>UINTPTR_MAX-(uintptr_t)m||
       (uintptr_t)m%offsetof(struct {char prefix;struct pt_midi_requester_model value;},value)||
       m->self!=m||!m->setup||m->stage!=PT_MIDI_REQUESTER_CHOOSE||m->target||m->busy||m->held||
       (m->focus!=PT_MIDI_FOCUS_INPUT&&m->focus!=PT_MIDI_FOCUS_OUTPUT&&
        m->focus!=PT_MIDI_FOCUS_CLOCK&&m->focus!=PT_MIDI_FOCUS_CANCEL))return PT_MIDI_SETUP_INVALID;
    result=pt_midi_setup_get(m->setup,&status);
    if(result!=PT_MIDI_SETUP_OK)return result;
    if(status.draft)return PT_MIDI_SETUP_IN_USE;
    return pt_midi_setup_view(m->setup,m->track,&unknown,&view);
}
static enum pt_midi_requester_result requester_window(struct Window *parent,
    struct pt_midi_requester_model *m)
{
    struct Window *w=NULL;struct IntuiMessage *message;
    struct TextFont *font=NULL;struct TextAttr font_attr={(STRPTR)"topaz.font",8,0,0};
    enum pt_midi_requester_result result=PT_MIDI_REQUESTER_UNAVAILABLE;
    unsigned pressed=PT_MIDI_FOCUS_COUNT;int done=0;
    font=OpenFont(&font_attr);if(!font||font->tf_XSize!=8||font->tf_YSize!=8)goto close;
    w=OpenWindowTags(NULL,WA_CustomScreen,(ULONG)parent->WScreen,WA_Left,40,WA_Top,100,
        WA_InnerWidth,PT_MIDI_REQUESTER_WIDTH,WA_InnerHeight,PT_MIDI_REQUESTER_HEIGHT,
        WA_Title,(ULONG)"MIDI setup - saved settings only",WA_DragBar,TRUE,WA_CloseGadget,TRUE,
        WA_Activate,TRUE,WA_RMBTrap,TRUE,WA_SmartRefresh,TRUE,
        WA_IDCMP,IDCMP_VANILLAKEY|IDCMP_RAWKEY|IDCMP_MOUSEBUTTONS|IDCMP_CLOSEWINDOW|IDCMP_REFRESHWINDOW,TAG_DONE);
    if(!w)goto close;
    SetFont(w->RPort,font);draw(w,m);
    while(!done){
        Wait(1UL<<w->UserPort->mp_SigBit);
        while((message=(struct IntuiMessage *)GetMsg(w->UserPort))){
            ULONG kind=message->Class;UWORD code=message->Code,qualifier=message->Qualifier;
            int x=message->MouseX-w->BorderLeft,y=message->MouseY-w->BorderTop;
            enum pt_midi_requester_event event=PT_MIDI_EVENT_NONE;
            ReplyMsg((struct Message *)message);if(done)continue;
            if(kind==IDCMP_CLOSEWINDOW||(kind==IDCMP_MOUSEBUTTONS&&code==MENUDOWN))event=PT_MIDI_EVENT_CANCEL;
            else if(kind==IDCMP_RAWKEY||kind==IDCMP_VANILLAKEY)event=key(m,kind,code,qualifier);
            else if(kind==IDCMP_MOUSEBUTTONS&&code==SELECTDOWN){
                pressed=pt_midi_requester_hit(m,x,y);
                if(pt_midi_requester_model_focus(m,pressed))event=PT_MIDI_EVENT_CHANGED;
            }else if(kind==IDCMP_MOUSEBUTTONS&&code==SELECTUP){
                unsigned hit=pt_midi_requester_hit(m,x,y);
                if(hit<PT_MIDI_FOCUS_COUNT&&hit==pressed&&m->focus==hit&&hit!=PT_MIDI_FOCUS_NAME)
                    event=pt_midi_requester_model_key(m,PT_MIDI_KEY_ACTIVATE,0);
                pressed=PT_MIDI_FOCUS_COUNT;
            }
            if(event==PT_MIDI_EVENT_CANCEL){
                enum pt_midi_setup_result cancelled=pt_midi_requester_model_cancel(m);
                if(cancelled==PT_MIDI_SETUP_OK||cancelled==PT_MIDI_SETUP_HELD){
                    result=cancelled==PT_MIDI_SETUP_HELD?PT_MIDI_REQUESTER_HELD:PT_MIDI_REQUESTER_CANCELLED;done=1;
                }
            }else if(event==PT_MIDI_EVENT_APPLY){
                enum pt_midi_setup_result accepted=pt_midi_requester_model_apply(m);
                if(accepted==PT_MIDI_SETUP_OK||accepted==PT_MIDI_SETUP_HELD){
                    result=accepted==PT_MIDI_SETUP_HELD?PT_MIDI_REQUESTER_HELD:PT_MIDI_REQUESTER_ACCEPTED;done=1;
                }
            }
            if(!done&&kind==IDCMP_REFRESHWINDOW){BeginRefresh(w);draw(w,m);EndRefresh(w,TRUE);}
            else if(!done&&event!=PT_MIDI_EVENT_NONE)draw(w,m);
        }
    }
close:
    if(w){
        while((message=(struct IntuiMessage *)GetMsg(w->UserPort)))ReplyMsg((struct Message *)message);
        WaitBlit();CloseWindow(w);
    }
    if(font)CloseFont(font);
    return result;
}

/* Caller owns the actual model and any enclosing genuine modal ticket. */
enum pt_midi_requester_result pt_native_midi_requester_model_window(struct Window *parent,
    struct pt_midi_requester_model *m)
{
    enum pt_midi_setup_result ready;
    if(!parent||!parent->WScreen)return PT_MIDI_REQUESTER_REFUSED;
    ready=chooser_admit(m);
    if(ready!=PT_MIDI_SETUP_OK)return ready==PT_MIDI_SETUP_HELD?PT_MIDI_REQUESTER_HELD:PT_MIDI_REQUESTER_REFUSED;
    return requester_window(parent,m);
}

/* Preserve the original setup/track wrapper's Task-local model cleanup. */
enum pt_midi_requester_result pt_native_midi_requester(struct Window *parent,
    struct pt_midi_setup *setup,unsigned track)
{
    struct pt_midi_requester_model m={0};enum pt_midi_setup_result opened;
    enum pt_midi_requester_result result;
    if(!parent||!parent->WScreen)return PT_MIDI_REQUESTER_REFUSED;
    opened=pt_midi_requester_model_open(&m,setup,track);
    if(opened!=PT_MIDI_SETUP_OK)return opened==PT_MIDI_SETUP_HELD?PT_MIDI_REQUESTER_HELD:PT_MIDI_REQUESTER_REFUSED;
    result=requester_window(parent,&m);
    if(m.self){
        enum pt_midi_setup_result cancelled=pt_midi_requester_model_cancel(&m);
        if(cancelled!=PT_MIDI_SETUP_OK)result=PT_MIDI_REQUESTER_HELD;
    }
    return result;
}
