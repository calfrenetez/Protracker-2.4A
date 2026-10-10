/* Own-window normal-Task modal UI. All policy and source binding stay in the
 * recovery controller/preferences seam; this file never polls or stops audio. */
#include <intuition/intuition.h>
#include <graphics/text.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <string.h>
#include "recovery_requester.h"

static void label(struct Window *w,int x,int y,const char *s)
{
    Move(w->RPort,w->BorderLeft+x,w->BorderTop+y);
    Text(w->RPort,(STRPTR)s,(ULONG)strlen(s));
}
static void outline(struct Window *w,const struct pt_recovery_requester_rect *r,unsigned focus)
{
    int l=w->BorderLeft+r->left,t=w->BorderTop+r->top;
    int right=w->BorderLeft+r->right-1,bottom=w->BorderTop+r->bottom-1;
    SetAPen(w->RPort,focus?3:1);Move(w->RPort,l,t);Draw(w->RPort,right,t);
    Draw(w->RPort,right,bottom);Draw(w->RPort,l,bottom);Draw(w->RPort,l,t);
    SetAPen(w->RPort,1);
}
static void text_field(struct Window *w,const struct pt_recovery_requester_model *m,
    unsigned focus,const char *text,size_t cursor)
{
    const struct pt_recovery_requester_rect *r=&pt_recovery_requester_controls[focus];
    size_t width=(size_t)(r->right-r->left-12)/8,start=0,n=strlen(text);char shown[65];
    if(cursor>=width)start=cursor-width+1;
    if(n-start>width)n=width;else n-=start;
    memcpy(shown,text+start,n);shown[n]=0;
    label(w,r->left+6,r->top+14,shown);
    if(m->focus==focus) {
        int x=w->BorderLeft+r->left+6+(int)(cursor-start)*8;
        Move(w->RPort,x,w->BorderTop+r->top+4);Draw(w->RPort,x,w->BorderTop+r->top+16);
    }
}
static void draw(struct Window *w,const struct pt_recovery_requester_model *m,const char *status)
{
    static const char *media[]={"Unknown","Fixed","Removable"};unsigned i;
    const struct pt_native_recovery_configuration *c=&m->preferences.draft;
    SetAPen(w->RPort,0);RectFill(w->RPort,w->BorderLeft,w->BorderTop,
        w->BorderLeft+PT_RECOVERY_REQUESTER_WIDTH-1,w->BorderTop+PT_RECOVERY_REQUESTER_HEIGHT-1);
    SetAPen(w->RPort,1);SetBPen(w->RPort,0);SetDrMd(w->RPort,JAM2);
    for(i=0;i<PT_RECOVERY_FOCUS_COUNT;++i)outline(w,&pt_recovery_requester_controls[i],m->focus==i);
    label(w,22,26,c->enabled?"[X] Recovery enabled":"[ ] Recovery enabled");
    label(w,14,46,"Directory (existing directory; up to 359 characters)");
    text_field(w,m,PT_RECOVERY_FOCUS_DIRECTORY,c->directory,m->directory_cursor);
    label(w,14,88,"Seconds");label(w,230,88,"Media (explicit classification)");
    text_field(w,m,PT_RECOVERY_FOCUS_INTERVAL,m->interval,m->interval_cursor);
    label(w,238,109,media[c->media]);
    label(w,22,144,c->allow_removable?"[X] Allow removable storage":"[ ] Allow removable storage");
    label(w,14,170,"Session only. Interval: 30..86400 seconds.");
    label(w,14,186,"Tab/Shift-Tab: focus. Space/Return: activate.");
    label(w,14,198,"Escape/right mouse/close: Cancel. Ctrl-U: clear text.");
    label(w,38,224,"Apply");label(w,168,224,"Cancel");label(w,14,250,status);
}
static enum pt_recovery_requester_event key(struct pt_recovery_requester_model *m,
    ULONG kind,UWORD code,UWORD qualifier)
{
    if(kind==IDCMP_VANILLAKEY) {
        if(code==27)return pt_recovery_requester_model_key(m,PT_RECOVERY_KEY_ESCAPE,0);
        if(code==9)return pt_recovery_requester_model_key(m,(qualifier&3)?PT_RECOVERY_KEY_BACKTAB:PT_RECOVERY_KEY_TAB,0);
        if(code==13 || code==10)return pt_recovery_requester_model_key(m,PT_RECOVERY_KEY_ACTIVATE,0);
        if(code==8)return pt_recovery_requester_model_key(m,PT_RECOVERY_KEY_BACKSPACE,0);
        if(code==127)return pt_recovery_requester_model_key(m,PT_RECOVERY_KEY_DELETE,0);
        if(code==21)return pt_recovery_requester_model_key(m,PT_RECOVERY_KEY_CLEAR,0);
        return pt_recovery_requester_model_key(m,PT_RECOVERY_KEY_CHARACTER,code);
    }
    if(kind!=IDCMP_RAWKEY || (code&0x80))return PT_RECOVERY_EVENT_NONE;
    if(code==0x45)return pt_recovery_requester_model_key(m,PT_RECOVERY_KEY_ESCAPE,0);
    if(code==0x42)return pt_recovery_requester_model_key(m,(qualifier&3)?PT_RECOVERY_KEY_BACKTAB:PT_RECOVERY_KEY_TAB,0);
    if(code==0x44 || code==0x43)return pt_recovery_requester_model_key(m,PT_RECOVERY_KEY_ACTIVATE,0);
    if(code==0x41)return pt_recovery_requester_model_key(m,PT_RECOVERY_KEY_BACKSPACE,0);
    if(code==0x46)return pt_recovery_requester_model_key(m,PT_RECOVERY_KEY_DELETE,0);
    if(code==0x4f)return pt_recovery_requester_model_key(m,(qualifier&8)?PT_RECOVERY_KEY_HOME:PT_RECOVERY_KEY_LEFT,0);
    if(code==0x4e)return pt_recovery_requester_model_key(m,(qualifier&8)?PT_RECOVERY_KEY_END:PT_RECOVERY_KEY_RIGHT,0);
    if(code==0x4c || code==0x4d)return pt_recovery_requester_model_key(m,code==0x4c?PT_RECOVERY_KEY_BACKTAB:PT_RECOVERY_KEY_TAB,0);
    return PT_RECOVERY_EVENT_NONE;
}
enum pt_recovery_requester_result pt_native_recovery_requester(struct Window *parent,
    struct pt_native_recovery *r,const struct pt_native_recovery_source *source,
    pt_native_recovery_preferences_idle idle,void *context)
{
    struct pt_recovery_requester_model m={0};struct Window *w=NULL;struct IntuiMessage *message;
    struct TextFont *font=NULL;struct TextAttr font_attr={(STRPTR)"topaz.font",8,0,0};
    const char *status="Edit the draft, then choose Apply or Cancel.";
    enum pt_recovery_requester_result result=PT_RECOVERY_REQUESTER_UNAVAILABLE;
    unsigned pressed=PT_RECOVERY_FOCUS_COUNT;int done=0;
    if(!parent || !parent->WScreen || !pt_recovery_requester_model_open(&m,r,idle,context))
        return PT_RECOVERY_REQUESTER_REFUSED;
    font=OpenFont(&font_attr);
    if(!font || font->tf_XSize!=8 || font->tf_YSize!=8)goto close;
    w=OpenWindowTags(NULL,WA_CustomScreen,(ULONG)parent->WScreen,WA_Left,40,WA_Top,100,
        WA_InnerWidth,PT_RECOVERY_REQUESTER_WIDTH,WA_InnerHeight,PT_RECOVERY_REQUESTER_HEIGHT,
        WA_Title,(ULONG)"Recovery settings - session only",WA_DragBar,TRUE,WA_CloseGadget,TRUE,
        WA_Activate,TRUE,WA_RMBTrap,TRUE,WA_SmartRefresh,TRUE,
        WA_IDCMP,IDCMP_VANILLAKEY|IDCMP_RAWKEY|IDCMP_MOUSEBUTTONS|IDCMP_CLOSEWINDOW|IDCMP_REFRESHWINDOW,TAG_DONE);
    if(!w)goto close;
    SetFont(w->RPort,font);draw(w,&m,status);
    while(!done) {
        Wait(1UL<<w->UserPort->mp_SigBit);
        while((message=(struct IntuiMessage *)GetMsg(w->UserPort))) {
            ULONG kind=message->Class;UWORD code=message->Code,qualifier=message->Qualifier;
            int x=message->MouseX-w->BorderLeft,y=message->MouseY-w->BorderTop;
            enum pt_recovery_requester_event event=PT_RECOVERY_EVENT_NONE;
            ReplyMsg((struct Message *)message);
            if(done)continue;
            if(kind==IDCMP_CLOSEWINDOW || (kind==IDCMP_MOUSEBUTTONS && code==MENUDOWN))
                event=PT_RECOVERY_EVENT_CANCEL;
            else if(kind==IDCMP_RAWKEY || kind==IDCMP_VANILLAKEY)event=key(&m,kind,code,qualifier);
            else if(kind==IDCMP_MOUSEBUTTONS && code==SELECTDOWN) {
                pressed=pt_recovery_requester_hit(x,y);
                if(pt_recovery_requester_model_focus(&m,pressed))event=PT_RECOVERY_EVENT_CHANGED;
            } else if(kind==IDCMP_MOUSEBUTTONS && code==SELECTUP) {
                unsigned hit=pt_recovery_requester_hit(x,y);
                if(hit<PT_RECOVERY_FOCUS_COUNT && hit==pressed && m.focus==hit &&
                    hit!=PT_RECOVERY_FOCUS_DIRECTORY && hit!=PT_RECOVERY_FOCUS_INTERVAL)
                    event=pt_recovery_requester_model_key(&m,PT_RECOVERY_KEY_ACTIVATE,0);
                pressed=PT_RECOVERY_FOCUS_COUNT;
            }
            if(event==PT_RECOVERY_EVENT_CANCEL) {
                result=PT_RECOVERY_REQUESTER_CANCELLED;done=1;
            } else if(event==PT_RECOVERY_EVENT_APPLY) {
                enum pt_native_recovery_preferences_result applied=pt_recovery_requester_model_apply(&m,r,source,idle,context);
                if(applied==PT_NATIVE_RECOVERY_PREFERENCES_REFUSED) {
                    status="Apply refused. Check values/path; current settings are kept.";
                } else {
                    result=applied==PT_NATIVE_RECOVERY_PREFERENCES_APPLIED?
                        PT_RECOVERY_REQUESTER_APPLIED:PT_RECOVERY_REQUESTER_NOOP;
                    done=1;
                }
            } else if(event==PT_RECOVERY_EVENT_CHANGED)status="Draft only. Return in a text field moves to the next control.";
            if(!done && kind==IDCMP_REFRESHWINDOW) {BeginRefresh(w);draw(w,&m,status);EndRefresh(w,TRUE);}
            else if(!done && (event!=PT_RECOVERY_EVENT_NONE))draw(w,&m,status);
        }
    }
close:
    if(w) {
        while((message=(struct IntuiMessage *)GetMsg(w->UserPort)))ReplyMsg((struct Message *)message);
        WaitBlit();CloseWindow(w);
    }
    if(font)CloseFont(font);
    if(m.preferences.open)pt_recovery_requester_model_cancel(&m);
    return result;
}
