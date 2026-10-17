#include "midi_requester_parent.h"
#include <exec/execbase.h>
#include <exec/tasks.h>
#include <exec/libraries.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#define ALIGN_OF(t) offsetof(struct {char c;t value;},value)
#define PARENT_IDCMP (IDCMP_RAWKEY|IDCMP_MOUSEBUTTONS|IDCMP_REFRESHWINDOW|IDCMP_INACTIVEWINDOW)
static int span(const void *p,size_t n)
{return !n||(p&&n<=UINTPTR_MAX-(uintptr_t)p);}
static int apart(const void *p,size_t n,const void *q,size_t m)
{
    uintptr_t a=(uintptr_t)p,b=(uintptr_t)q;
    return span(p,n)&&span(q,m)&&(!n||!m||a>=b+m||b>=a+n);
}
static int zero(const void *p,size_t n)
{const unsigned char *b=p;size_t i;for(i=0;i<n;++i)if(b[i])return 0;return 1;}
static int fixed_parent(const struct Window *w)
{
    ULONG required=WFLG_BORDERLESS|WFLG_BACKDROP|WFLG_SIMPLE_REFRESH;
    ULONG forbidden=WFLG_SIZEGADGET|WFLG_DRAGBAR|WFLG_DEPTHGADGET|WFLG_CLOSEGADGET;
    return w->Width==640&&w->Height==512&&w->LeftEdge==0&&w->TopEdge==0&&
        !w->BorderLeft&&!w->BorderRight&&!w->BorderTop&&!w->BorderBottom&&
        (w->Flags&required)==required&&!(w->Flags&forbidden);
}
static int normal(const struct pt_midi_requester_parent_progress *p)
{
    return SysBase==p->exec&&p->exec->ThisTask==p->task&&
        (int)(signed char)p->exec->IDNestCnt==-1&&
        (int)(signed char)p->exec->TDNestCnt==-1&&
        (struct Library *)IntuitionBase==p->intuition&&
        p->intuition->lib_Version>=37;
}
static int same_parent(const struct pt_midi_requester_parent_progress *p,ULONG mask)
{
    return normal(p)&&p->parent->UserPort==p->port&&
        p->parent->WScreen==p->screen&&p->parent->RPort==p->rport&&
        p->parent->IDCMPFlags==mask&&fixed_parent(p->parent)&&
        p->port->mp_SigTask==p->task&&
        (unsigned)(unsigned char)p->port->mp_SigBit==p->signal_bit;
}
static enum pt_midi_parent_result held(struct pt_midi_requester_parent_progress *p)
{p->state=PT_MIDI_PARENT_HELD;p->busy=0;return p->state;}
/* HELD/busy spans every OS call. Record normal completion only after return. */
static int drain(struct pt_midi_requester_parent_progress *p,unsigned pass)
{
    unsigned n;
    for(n=0;n<PT_MIDI_PARENT_DRAIN_LIMIT;++n){
        struct Message *raw;struct IntuiMessage *message;ULONG kind;
        if(!same_parent(p,IDCMP_REFRESHWINDOW))return 0;
        ++p->get_calls[pass];p->pending_reply_attempted=0;
        raw=GetMsg(p->port);p->pending_message=raw;
        if(!same_parent(p,IDCMP_REFRESHWINDOW))return 0;
        if(!raw)return 1;
        if(raw->mn_Node.ln_Type!=NT_MESSAGE||raw->mn_Length<sizeof(*message)||
           !raw->mn_ReplyPort)return 0;
        message=(struct IntuiMessage *)raw;
        if(message->IDCMPWindow!=p->parent)return 0;
        kind=message->Class;
        p->pending_reply_attempted=1;ReplyMsg(raw);
        p->pending_message=NULL;++p->replied[pass];
        if(!same_parent(p,IDCMP_REFRESHWINDOW))return 0;
        if(kind==IDCMP_REFRESHWINDOW){
            p->refresh_active=1;BeginRefresh(p->parent);
            if(!same_parent(p,IDCMP_REFRESHWINDOW))return 0;
            p->repaint(p->parent,p->repaint_context);
            if(!same_parent(p,IDCMP_REFRESHWINDOW))return 0;
            EndRefresh(p->parent,TRUE);p->refresh_active=0;
            ++p->repainted[pass];
            if(!same_parent(p,IDCMP_REFRESHWINDOW))return 0;
        }
    }
    return 0;
}
static int restore(struct pt_midi_requester_parent_progress *p)
{
    if(p->restore_attempted||!same_parent(p,IDCMP_REFRESHWINDOW))return 0;
    p->phase=PT_MIDI_PARENT_RESTORE;p->restore_attempted=1;
    if(!ModifyIDCMP(p->parent,p->original_idcmp))return 0;
    if(!same_parent(p,p->original_idcmp))return 0;
    p->restore_confirmed=1;return 1;
}
static int known_apart(struct pt_midi_requester_parent_progress *p,
    struct Window *w,struct pt_midi_requester_model *m,struct ExecBase *exec,
    struct Task *task,struct Library *intuition,void *context,size_t bytes)
{
    return apart(p,sizeof(*p),w,sizeof(*w))&&
        apart(p,sizeof(*p),m,sizeof(*m))&&
        apart(p,sizeof(*p),w->UserPort,sizeof(*w->UserPort))&&
        apart(p,sizeof(*p),w->WScreen,sizeof(*w->WScreen))&&
        apart(p,sizeof(*p),w->RPort,sizeof(*w->RPort))&&
        apart(p,sizeof(*p),exec,sizeof(*exec))&&
        apart(p,sizeof(*p),task,sizeof(*task))&&
        apart(p,sizeof(*p),intuition,sizeof(*intuition))&&
        span(context,bytes)&&apart(p,sizeof(*p),context,bytes);
}
enum pt_midi_parent_result pt_native_midi_requester_parent_window(
    struct pt_midi_requester_parent_progress *p,struct Window *parent,
    struct pt_midi_requester_model *model,pt_midi_parent_repaint repaint,
    void *repaint_context,size_t repaint_bytes)
{
    struct ExecBase *exec=SysBase;struct Library *intuition=(struct Library *)IntuitionBase;
    struct Task *task;ULONG original;unsigned bit;
    if(!p||!span(p,sizeof(*p))||(uintptr_t)p%ALIGN_OF(struct pt_midi_requester_parent_progress))
        return PT_MIDI_PARENT_REFUSED;
    if(p->self==p){
        if(p->busy)return PT_MIDI_PARENT_BUSY;
        return p->state==PT_MIDI_PARENT_HELD?PT_MIDI_PARENT_HELD:PT_MIDI_PARENT_REFUSED;
    }
    if(!zero(p,sizeof(*p))||!exec||!intuition||intuition->lib_Version<37||
       (int)(signed char)exec->IDNestCnt!=-1||(int)(signed char)exec->TDNestCnt!=-1||
       !parent||!model||!repaint)return PT_MIDI_PARENT_REFUSED;
    task=FindTask(NULL);
    if(!task||exec!=SysBase||exec->ThisTask!=task||
       (int)(signed char)exec->IDNestCnt!=-1||(int)(signed char)exec->TDNestCnt!=-1||
       intuition!=(struct Library *)IntuitionBase||intuition->lib_Version<37||
       !parent->UserPort||!parent->WScreen||!parent->RPort||!fixed_parent(parent))
        return PT_MIDI_PARENT_REFUSED;
    original=parent->IDCMPFlags;bit=(unsigned)(unsigned char)parent->UserPort->mp_SigBit;
    if(original!=PARENT_IDCMP||!original||bit>=32||parent->UserPort->mp_SigTask!=task||
       !known_apart(p,parent,model,exec,task,intuition,repaint_context,repaint_bytes))
        return PT_MIDI_PARENT_REFUSED;
    p->self=p;p->parent=parent;p->port=parent->UserPort;p->screen=parent->WScreen;
    p->rport=parent->RPort;p->exec=exec;p->task=task;p->intuition=intuition;
    p->model=model;p->repaint=repaint;p->repaint_context=repaint_context;
    p->repaint_bytes=repaint_bytes;p->original_idcmp=original;p->signal_bit=bit;
    p->window_result=PT_MIDI_REQUESTER_UNAVAILABLE;p->busy=1;
    p->state=PT_MIDI_PARENT_HELD;p->phase=PT_MIDI_PARENT_ISOLATE;
    if(!same_parent(p,original))return held(p);
    p->isolation_attempted=1;
    if(!ModifyIDCMP(parent,IDCMP_REFRESHWINDOW))return held(p);
    if(!same_parent(p,IDCMP_REFRESHWINDOW))return held(p);
    p->isolation_confirmed=1;p->phase=PT_MIDI_PARENT_PRE_DRAIN;
    if(!drain(p,0))return held(p);
    p->phase=PT_MIDI_PARENT_CHILD;p->window_called=1;
    p->window_result=pt_native_midi_requester_model_window(parent,model);
    if(!same_parent(p,IDCMP_REFRESHWINDOW))return held(p);
    switch(p->window_result){
    case PT_MIDI_REQUESTER_HELD:case PT_MIDI_REQUESTER_REFUSED:
    case PT_MIDI_REQUESTER_UNAVAILABLE:case PT_MIDI_REQUESTER_CANCELLED:
    case PT_MIDI_REQUESTER_ACCEPTED:break;
    default:return held(p);
    }
    if(p->window_result==PT_MIDI_REQUESTER_HELD){
        /* The original model/source domain may be uncertain: no repaint. */
        if(!restore(p))return held(p);
        return held(p);
    }
    p->phase=PT_MIDI_PARENT_POST_DRAIN;
    if(!drain(p,1))return held(p);
    if(!restore(p))return held(p);
    p->phase=PT_MIDI_PARENT_COMPLETE;p->state=PT_MIDI_PARENT_RESTORED;p->busy=0;
    return p->state;
}
int pt_native_midi_requester_parent_restored(const struct pt_midi_requester_parent_progress *p)
{
    return p&&span(p,sizeof(*p))&&
        !((uintptr_t)p%ALIGN_OF(struct pt_midi_requester_parent_progress))&&
        p->self==p&&!p->busy&&p->state==PT_MIDI_PARENT_RESTORED&&
        p->restore_confirmed&&!p->pending_message&&!p->refresh_active&&
        p->window_result!=PT_MIDI_REQUESTER_HELD&&same_parent(p,p->original_idcmp);
}
