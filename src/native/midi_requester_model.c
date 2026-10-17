#include "midi_requester.h"
#include <string.h>

const struct pt_midi_requester_rect pt_midi_requester_controls[PT_MIDI_FOCUS_COUNT]={
    {14,52,538,76},{14,86,538,110},{14,120,538,144},
    {14,86,538,110},{14,72,538,96},{14,110,538,134},
    {14,206,126,232},{146,206,258,232}
};
static int apart(const void *a,size_t an,const void *b,size_t bn)
{
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return a&&b&&an<=UINTPTR_MAX-x&&bn<=UINTPTR_MAX-y&&
        (x<=y?an<=y-x:bn<=x-y);
}
static size_t length(const char *s)
{size_t n;for(n=0;n<PT_MIDI_ENDPOINT&&s[n];++n){}return n;}
static int valid(const struct pt_midi_requester_model *m)
{
    size_t n;
    if(!m||sizeof(*m)>UINTPTR_MAX-(uintptr_t)m||m->self!=m||!m->setup||
        (m->stage!=PT_MIDI_REQUESTER_CHOOSE&&m->stage!=PT_MIDI_REQUESTER_FIELD))return 0;
    if(m->stage==PT_MIDI_REQUESTER_CHOOSE)return m->target==0;
    if(m->target==PT_MIDI_SETUP_CLOCK)return m->flags<=3;
    n=length(m->name);
    return (m->target==PT_MIDI_SETUP_INPUT||m->target==PT_MIDI_SETUP_OUTPUT)&&
        n<PT_MIDI_ENDPOINT&&m->cursor<=n;
}
static const unsigned *order(const struct pt_midi_requester_model *m,unsigned *count)
{
    static const unsigned chooser[]={PT_MIDI_FOCUS_INPUT,PT_MIDI_FOCUS_OUTPUT,PT_MIDI_FOCUS_CLOCK,PT_MIDI_FOCUS_CANCEL};
    static const unsigned text[]={PT_MIDI_FOCUS_NAME,PT_MIDI_FOCUS_APPLY,PT_MIDI_FOCUS_CANCEL};
    static const unsigned flags[]={PT_MIDI_FOCUS_SEND,PT_MIDI_FOCUS_RECEIVE,PT_MIDI_FOCUS_APPLY,PT_MIDI_FOCUS_CANCEL};
    *count=4;
    if(m->stage==PT_MIDI_REQUESTER_CHOOSE)return chooser;
    if(m->target==PT_MIDI_SETUP_CLOCK)return flags;
    *count=3;return text;
}
enum pt_midi_setup_result pt_midi_requester_model_open(struct pt_midi_requester_model *m,
    struct pt_midi_setup *setup,unsigned track)
{
    struct pt_midi_setup_status status;struct pt_midi_setup_view view;
    const struct pt_midi_setup_discovery unknown={0};enum pt_midi_setup_result result;
    if(!m||!setup||!apart(m,sizeof(*m),setup,pt_midi_setup_size())||m->self)return PT_MIDI_SETUP_INVALID;
    result=pt_midi_setup_get(setup,&status);
    if(result!=PT_MIDI_SETUP_OK)return result;
    if(status.draft)return PT_MIDI_SETUP_IN_USE;
    result=pt_midi_setup_view(setup,track,&unknown,&view);
    if(result!=PT_MIDI_SETUP_OK)return result;
    memset(m,0,sizeof(*m));m->self=m;m->setup=setup;m->track=track;
    m->stage=PT_MIDI_REQUESTER_CHOOSE;m->focus=PT_MIDI_FOCUS_INPUT;
    return PT_MIDI_SETUP_OK;
}
enum pt_midi_setup_result pt_midi_requester_model_select(struct pt_midi_requester_model *m,
    enum pt_midi_setup_target target)
{
    const struct pt_midi_setup_discovery unknown={0};struct pt_midi_setup_view view;
    enum pt_midi_setup_result result;
    if(!valid(m))return PT_MIDI_SETUP_INVALID;
    if(m->busy)return PT_MIDI_SETUP_BUSY;
    if(m->held)return PT_MIDI_SETUP_HELD;
    if(m->stage!=PT_MIDI_REQUESTER_CHOOSE)return PT_MIDI_SETUP_IN_USE;
    if(target!=PT_MIDI_SETUP_INPUT&&target!=PT_MIDI_SETUP_OUTPUT&&target!=PT_MIDI_SETUP_CLOCK)return PT_MIDI_SETUP_INVALID;
    m->busy=1;result=pt_midi_setup_view(m->setup,m->track,&unknown,&view);
    if(result==PT_MIDI_SETUP_OK)result=pt_midi_setup_begin(m->setup,target,m->track);
    m->busy=0;m->last_result=result;
    if(result!=PT_MIDI_SETUP_OK){if(result==PT_MIDI_SETUP_HELD)m->held=1;return result;}
    m->target=target;m->stage=PT_MIDI_REQUESTER_FIELD;
    if(target==PT_MIDI_SETUP_CLOCK){m->flags=view.flags;m->focus=PT_MIDI_FOCUS_SEND;}
    else {
        memcpy(m->name,target==PT_MIDI_SETUP_INPUT?view.input:view.output,PT_MIDI_ENDPOINT);
        m->cursor=(unsigned)length(m->name);m->focus=PT_MIDI_FOCUS_NAME;
    }
    return PT_MIDI_SETUP_OK;
}
unsigned pt_midi_requester_hit(const struct pt_midi_requester_model *m,int x,int y)
{
    const unsigned *items;unsigned count,i;
    if(!valid(m)||m->busy||m->held)return PT_MIDI_FOCUS_COUNT;
    items=order(m,&count);
    for(i=0;i<count;++i){
        const struct pt_midi_requester_rect *r=&pt_midi_requester_controls[items[i]];
        if(x>=r->left&&x<r->right&&y>=r->top&&y<r->bottom)return items[i];
    }
    return PT_MIDI_FOCUS_COUNT;
}
int pt_midi_requester_model_focus(struct pt_midi_requester_model *m,unsigned focus)
{
    const unsigned *items;unsigned count,i;
    if(!valid(m)||m->busy||m->held)return 0;
    items=order(m,&count);for(i=0;i<count;++i)if(items[i]==focus){m->focus=focus;return 1;}
    return 0;
}
enum pt_midi_requester_event pt_midi_requester_model_key(struct pt_midi_requester_model *m,
    enum pt_midi_requester_key key,unsigned character)
{
    const unsigned *items;unsigned count,i;size_t n;
    if(!valid(m)||m->busy)return PT_MIDI_EVENT_NONE;
    if(key==PT_MIDI_KEY_ESCAPE)return PT_MIDI_EVENT_CANCEL;
    if(m->held)return PT_MIDI_EVENT_NONE;
    items=order(m,&count);
    for(i=0;i<count&&items[i]!=m->focus;++i){}
    if(i==count)return PT_MIDI_EVENT_NONE;
    if(key==PT_MIDI_KEY_TAB||key==PT_MIDI_KEY_BACKTAB){
        m->focus=items[(i+(key==PT_MIDI_KEY_TAB?1:count-1))%count];return PT_MIDI_EVENT_CHANGED;
    }
    if(key==PT_MIDI_KEY_ACTIVATE||(key==PT_MIDI_KEY_CHARACTER&&character==' '&&m->focus!=PT_MIDI_FOCUS_NAME)){
        if(m->focus==PT_MIDI_FOCUS_CANCEL)return PT_MIDI_EVENT_CANCEL;
        if(m->focus==PT_MIDI_FOCUS_APPLY)return PT_MIDI_EVENT_APPLY;
        if(m->stage==PT_MIDI_REQUESTER_CHOOSE){
            enum pt_midi_setup_target target=m->focus==PT_MIDI_FOCUS_INPUT?PT_MIDI_SETUP_INPUT:
                m->focus==PT_MIDI_FOCUS_OUTPUT?PT_MIDI_SETUP_OUTPUT:PT_MIDI_SETUP_CLOCK;
            (void)pt_midi_requester_model_select(m,target);return PT_MIDI_EVENT_CHANGED;
        }
        if(m->focus==PT_MIDI_FOCUS_SEND)m->flags^=1;
        else if(m->focus==PT_MIDI_FOCUS_RECEIVE)m->flags^=2;
        else m->focus=PT_MIDI_FOCUS_APPLY;
        return PT_MIDI_EVENT_CHANGED;
    }
    if(m->focus!=PT_MIDI_FOCUS_NAME)return PT_MIDI_EVENT_NONE;
    n=length(m->name);
    if(key==PT_MIDI_KEY_LEFT){if(m->cursor)--m->cursor;return PT_MIDI_EVENT_CHANGED;}
    if(key==PT_MIDI_KEY_RIGHT){if(m->cursor<n)++m->cursor;return PT_MIDI_EVENT_CHANGED;}
    if(key==PT_MIDI_KEY_HOME){m->cursor=0;return PT_MIDI_EVENT_CHANGED;}
    if(key==PT_MIDI_KEY_END){m->cursor=(unsigned)n;return PT_MIDI_EVENT_CHANGED;}
    if(key==PT_MIDI_KEY_CLEAR){m->name[0]=0;m->cursor=0;}
    else if(key==PT_MIDI_KEY_BACKSPACE){
        if(!m->cursor)return PT_MIDI_EVENT_NONE;
        memmove(m->name+m->cursor-1,m->name+m->cursor,n-m->cursor+1);--m->cursor;
    }else if(key==PT_MIDI_KEY_DELETE){
        if(m->cursor==n)return PT_MIDI_EVENT_NONE;
        memmove(m->name+m->cursor,m->name+m->cursor+1,n-m->cursor);
    }else if(key==PT_MIDI_KEY_CHARACTER){
        if(character<32||character==127||character>255||n==PT_MIDI_ENDPOINT-1)return PT_MIDI_EVENT_NONE;
        memmove(m->name+m->cursor+1,m->name+m->cursor,n-m->cursor+1);
        m->name[m->cursor]=(char)character;++m->cursor;
    }else return PT_MIDI_EVENT_NONE;
    return PT_MIDI_EVENT_CHANGED;
}
enum pt_midi_setup_result pt_midi_requester_model_apply(struct pt_midi_requester_model *m)
{
    enum pt_midi_setup_result result;
    if(!valid(m))return PT_MIDI_SETUP_INVALID;
    if(m->busy)return PT_MIDI_SETUP_BUSY;
    if(m->held)return PT_MIDI_SETUP_HELD;
    if(m->stage!=PT_MIDI_REQUESTER_FIELD)return PT_MIDI_SETUP_INVALID;
    m->busy=1;
    result=m->target==PT_MIDI_SETUP_CLOCK?pt_midi_setup_flags(m->setup,m->flags):
        pt_midi_setup_name(m->setup,m->name,length(m->name)+1);
    if(result==PT_MIDI_SETUP_OK)result=pt_midi_setup_commit(m->setup);
    m->busy=0;m->last_result=result;
    if(result==PT_MIDI_SETUP_OK)m->self=NULL;
    else if(result==PT_MIDI_SETUP_HELD)m->held=1;
    return result;
}
enum pt_midi_setup_result pt_midi_requester_model_cancel(struct pt_midi_requester_model *m)
{
    enum pt_midi_setup_result result=PT_MIDI_SETUP_OK;
    if(!valid(m))return PT_MIDI_SETUP_INVALID;
    if(m->busy)return PT_MIDI_SETUP_BUSY;
    if(m->held){m->self=NULL;return PT_MIDI_SETUP_HELD;}
    m->busy=1;
    if(m->stage==PT_MIDI_REQUESTER_FIELD)result=pt_midi_setup_cancel(m->setup);
    m->busy=0;m->last_result=result;
    if(result==PT_MIDI_SETUP_OK)m->self=NULL;
    else if(result==PT_MIDI_SETUP_HELD)m->held=1;
    return result;
}
