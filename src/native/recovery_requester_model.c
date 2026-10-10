#include "recovery_requester.h"
#include <string.h>

const struct pt_recovery_requester_rect
pt_recovery_requester_controls[PT_RECOVERY_FOCUS_COUNT]={
    {14,12,538,32},{14,52,538,74},{14,94,110,116},
    {230,94,538,116},{14,130,538,150},{14,206,126,232},{146,206,258,232}
};
static int disjoint(const void *a,size_t na,const void *b,size_t nb)
{
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    if(!a || !b || na>UINTPTR_MAX-x || nb>UINTPTR_MAX-y)return 0;
    return x+na<=y || y+nb<=x;
}
static size_t length(const char *s,size_t capacity)
{
    size_t n;for(n=0;n<capacity && s[n];++n) {}
    return n;
}
static uint32_t seconds(const char *s)
{
    uint32_t value=0;size_t i;
    for(i=0;i<5 && s[i];++i)value=value*10+(unsigned)(s[i]-'0');
    return value;
}
static int valid(const struct pt_recovery_requester_model *m)
{
    size_t n,i;
    if(!m || sizeof(*m)>UINTPTR_MAX-(uintptr_t)m || m->preferences.open!=1 || m->focus>=PT_RECOVERY_FOCUS_COUNT ||
        m->preferences.draft.enabled>1 || m->preferences.draft.allow_removable>1 ||
        m->preferences.draft.media>PT_NATIVE_RECOVERY_MEDIA_REMOVABLE)return 0;
    n=length(m->preferences.draft.directory,PT_NATIVE_RECOVERY_ROOT_SIZE);
    if(n==PT_NATIVE_RECOVERY_ROOT_SIZE || m->directory_cursor>n)return 0;
    n=length(m->interval,sizeof(m->interval));
    if(n==sizeof(m->interval) || m->interval_cursor>n)return 0;
    for(i=0;i<n;++i)if(m->interval[i]<'0' || m->interval[i]>'9')return 0;
    return seconds(m->interval)==m->preferences.draft.interval_seconds;
}
int pt_recovery_requester_model_open(struct pt_recovery_requester_model *m,
    const struct pt_native_recovery *r,pt_native_recovery_preferences_idle idle,void *context)
{
    struct pt_recovery_requester_model staged;uint32_t value;char reversed[5];size_t n=0,i;
    if(!m || !r || !disjoint(m,sizeof(*m),r,sizeof(*r)) || m->preferences.open)return 0;
    memset(&staged,0,sizeof(staged));
    if(!pt_native_recovery_preferences_open(&staged.preferences,r,idle,context))return 0;
    value=staged.preferences.draft.interval_seconds;
    do {reversed[n++]=(char)('0'+value%10);value/=10;}while(value && n<sizeof(reversed));
    if(value)return 0;
    for(i=0;i<n;++i)staged.interval[i]=reversed[n-i-1];
    staged.interval_cursor=(uint16_t)n;
    staged.directory_cursor=(uint16_t)length(staged.preferences.draft.directory,PT_NATIVE_RECOVERY_ROOT_SIZE);
    if(!valid(&staged))return 0;
    *m=staged;return 1;
}
unsigned pt_recovery_requester_hit(int x,int y)
{
    unsigned i;
    for(i=0;i<PT_RECOVERY_FOCUS_COUNT;++i) {
        const struct pt_recovery_requester_rect *r=&pt_recovery_requester_controls[i];
        if(x>=r->left && x<r->right && y>=r->top && y<r->bottom)return i;
    }
    return PT_RECOVERY_FOCUS_COUNT;
}
int pt_recovery_requester_model_focus(struct pt_recovery_requester_model *m,unsigned focus)
{
    if(!valid(m) || focus>=PT_RECOVERY_FOCUS_COUNT)return 0;
    m->focus=(uint8_t)focus;return 1;
}
static enum pt_recovery_requester_event activate(struct pt_recovery_requester_model *m)
{
    struct pt_native_recovery_configuration c=m->preferences.draft;
    if(m->focus==PT_RECOVERY_FOCUS_APPLY)return PT_RECOVERY_EVENT_APPLY;
    if(m->focus==PT_RECOVERY_FOCUS_CANCEL)return PT_RECOVERY_EVENT_CANCEL;
    if(m->focus==PT_RECOVERY_FOCUS_ENABLED)c.enabled^=1;
    else if(m->focus==PT_RECOVERY_FOCUS_REMOVABLE)c.allow_removable^=1;
    else if(m->focus==PT_RECOVERY_FOCUS_MEDIA)c.media=(uint8_t)((c.media+1)%3);
    else {m->focus=(uint8_t)((m->focus+1)%PT_RECOVERY_FOCUS_COUNT);return PT_RECOVERY_EVENT_CHANGED;}
    return pt_native_recovery_preferences_edit(&m->preferences,&c)?PT_RECOVERY_EVENT_CHANGED:PT_RECOVERY_EVENT_NONE;
}
enum pt_recovery_requester_event pt_recovery_requester_model_key(
    struct pt_recovery_requester_model *m,enum pt_recovery_requester_key key,unsigned character)
{
    struct pt_native_recovery_configuration c;char text[PT_NATIVE_RECOVERY_ROOT_SIZE];
    char *target;size_t n,capacity;uint16_t *cursor;int interval;
    if(!valid(m))return PT_RECOVERY_EVENT_NONE;
    if(key==PT_RECOVERY_KEY_ESCAPE)return PT_RECOVERY_EVENT_CANCEL;
    if(key==PT_RECOVERY_KEY_TAB || key==PT_RECOVERY_KEY_BACKTAB) {
        m->focus=(uint8_t)((m->focus+(key==PT_RECOVERY_KEY_TAB?1:PT_RECOVERY_FOCUS_COUNT-1))%PT_RECOVERY_FOCUS_COUNT);
        return PT_RECOVERY_EVENT_CHANGED;
    }
    if(key==PT_RECOVERY_KEY_ACTIVATE)return activate(m);
    interval=m->focus==PT_RECOVERY_FOCUS_INTERVAL;
    if(m->focus!=PT_RECOVERY_FOCUS_DIRECTORY && !interval) {
        return key==PT_RECOVERY_KEY_CHARACTER && character==' '?activate(m):PT_RECOVERY_EVENT_NONE;
    }
    c=m->preferences.draft;
    capacity=interval?sizeof(m->interval):sizeof(c.directory);
    target=interval?m->interval:c.directory;
    n=length(target,capacity);memcpy(text,target,n+1);
    cursor=interval?&m->interval_cursor:&m->directory_cursor;
    if(key==PT_RECOVERY_KEY_LEFT) {if(*cursor)--*cursor;return PT_RECOVERY_EVENT_CHANGED;}
    if(key==PT_RECOVERY_KEY_RIGHT) {if(*cursor<n)++*cursor;return PT_RECOVERY_EVENT_CHANGED;}
    if(key==PT_RECOVERY_KEY_HOME) {*cursor=0;return PT_RECOVERY_EVENT_CHANGED;}
    if(key==PT_RECOVERY_KEY_END) {*cursor=(uint16_t)n;return PT_RECOVERY_EVENT_CHANGED;}
    if(key==PT_RECOVERY_KEY_CLEAR) {text[0]=0;*cursor=0;}
    else if(key==PT_RECOVERY_KEY_BACKSPACE) {
        if(!*cursor)return PT_RECOVERY_EVENT_NONE;
        memmove(text+*cursor-1,text+*cursor,n-*cursor+1);--*cursor;
    } else if(key==PT_RECOVERY_KEY_DELETE) {
        if(*cursor==n)return PT_RECOVERY_EVENT_NONE;
        memmove(text+*cursor,text+*cursor+1,n-*cursor);
    } else if(key==PT_RECOVERY_KEY_CHARACTER) {
        if(character<32 || character==127 || character>255 || n+1>=capacity ||
            (interval && (character<'0' || character>'9')))return PT_RECOVERY_EVENT_NONE;
        memmove(text+*cursor+1,text+*cursor,n-*cursor+1);
        text[*cursor]=(char)character;++*cursor;
    } else return PT_RECOVERY_EVENT_NONE;
    if(interval)c.interval_seconds=seconds(text);
    else memcpy(c.directory,text,length(text,sizeof(text))+1);
    if(!pt_native_recovery_preferences_edit(&m->preferences,&c))return PT_RECOVERY_EVENT_NONE;
    if(interval)memcpy(m->interval,text,length(text,sizeof(m->interval))+1);
    return PT_RECOVERY_EVENT_CHANGED;
}
int pt_recovery_requester_model_cancel(struct pt_recovery_requester_model *m)
{return m && sizeof(*m)<=UINTPTR_MAX-(uintptr_t)m?pt_native_recovery_preferences_cancel(&m->preferences):0;}
enum pt_native_recovery_preferences_result pt_recovery_requester_model_apply(
    struct pt_recovery_requester_model *m,struct pt_native_recovery *r,
    const struct pt_native_recovery_source *source,pt_native_recovery_preferences_idle idle,void *context)
{
    if(!valid(m) || !r || !disjoint(m,sizeof(*m),r,sizeof(*r)) ||
        (source && (!disjoint(m,sizeof(*m),source,sizeof(*source)) ||
        !disjoint(r,sizeof(*r),source,sizeof(*source)))))return PT_NATIVE_RECOVERY_PREFERENCES_REFUSED;
    return pt_native_recovery_preferences_apply(&m->preferences,r,source,idle,context);
}
