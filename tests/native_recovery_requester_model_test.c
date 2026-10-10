#include "../src/native/recovery_requester.h"
/* Reuse the current real controller/binder DOS stubs. This renamed existing
 * seven-group entry is compiled but never called by this model fixture. */
#define main pt_preferences_uncalled_baseline
#include "native_recovery_preferences_test.c"
#undef main

static unsigned requester_groups;
static struct pt_recovery_requester_model model_open(struct pt_native_recovery *r)
{
    struct pt_recovery_requester_model m={0};struct idle_state s=quiet();
    assert(pt_recovery_requester_model_open(&m,r,idle,&s));assert(s.calls==2);return m;
}
static void input(struct pt_recovery_requester_model *m,unsigned focus,const char *text)
{
    const unsigned char *p=(const unsigned char *)text;
    assert(pt_recovery_requester_model_focus(m,focus));
    assert(pt_recovery_requester_model_key(m,PT_RECOVERY_KEY_CLEAR,0)==PT_RECOVERY_EVENT_CHANGED);
    for(;*p;++p)assert(pt_recovery_requester_model_key(m,PT_RECOVERY_KEY_CHARACTER,*p)==PT_RECOVERY_EVENT_CHANGED);
}
static void enable(struct pt_recovery_requester_model *m)
{
    input(m,PT_RECOVERY_FOCUS_DIRECTORY,"Work:Recovery");
    input(m,PT_RECOVERY_FOCUS_INTERVAL,"60");
    assert(pt_recovery_requester_model_focus(m,PT_RECOVERY_FOCUS_ENABLED));
    assert(pt_recovery_requester_model_key(m,PT_RECOVERY_KEY_ACTIVATE,0)==PT_RECOVERY_EVENT_CHANGED);
    assert(pt_recovery_requester_model_focus(m,PT_RECOVERY_FOCUS_MEDIA));
    assert(pt_recovery_requester_model_key(m,PT_RECOVERY_KEY_CHARACTER,' ')==PT_RECOVERY_EVENT_CHANGED);
}
static void expect_refusal(struct pt_recovery_requester_model *m,struct pt_native_recovery *r,
    const struct pt_native_recovery_source *source,struct idle_state *s)
{
    struct pt_recovery_requester_model original=*m;struct pt_native_recovery before=*r;
    struct pt_native_recovery_source previous=*source;
    assert(pt_recovery_requester_model_apply(m,r,source,idle,s)==PT_NATIVE_RECOVERY_PREFERENCES_REFUSED);
    assert(!memcmp(m,&original,sizeof(*m)) && !memcmp(r,&before,sizeof(*r)) &&
        !memcmp(source,&previous,sizeof(*source)));
}
static void entry_and_hits(void)
{
    struct pt_native_recovery r,before;struct pt_recovery_requester_model m={0},previous;
    struct idle_state s;unsigned i,calls;
    initialize(&r,0,0);before=r;previous=m;s=quiet();s.result=0;
    assert(!pt_recovery_requester_model_open(&m,&r,idle,&s));
    assert(!memcmp(&r,&before,sizeof(r)) && !memcmp(&m,&previous,sizeof(m)));
    s=quiet();assert(!pt_recovery_requester_model_open((void *)&r,&r,idle,&s));
    assert(!pt_recovery_requester_model_open((void *)(UINTPTR_MAX-1),&r,idle,&s));
    assert(!s.calls && !memcmp(&r,&before,sizeof(r)));
    for(i=0;i<4;++i) {
        r=before;s=quiet();calls=directory_locks+source_locks;
        if(i==0)r.store.opened=1;
        if(i==1)r.store.owned=1;
        if(i==2)r.store.busy=1;
        if(i==3)r.schedule.busy=1;
        assert(!pt_recovery_requester_model_open(&m,&r,idle,&s));
        assert(!s.calls && directory_locks+source_locks==calls && !memcmp(&m,&previous,sizeof(m)));
    }
    r=before;m=model_open(&r);previous=m;s=quiet();
    assert(!pt_recovery_requester_model_open(&m,&r,idle,&s));
    assert(!s.calls && !memcmp(&m,&previous,sizeof(m)));
    for(i=0;i<PT_RECOVERY_FOCUS_COUNT;++i) {
        const struct pt_recovery_requester_rect *p=&pt_recovery_requester_controls[i];
        assert(pt_recovery_requester_hit(p->left,p->top)==i);
        assert(pt_recovery_requester_hit(p->right-1,p->bottom-1)==i);
    }
    assert(pt_recovery_requester_hit(-1,-1)==PT_RECOVERY_FOCUS_COUNT);
    assert(pt_recovery_requester_hit(552,264)==PT_RECOVERY_FOCUS_COUNT);
    assert(pt_recovery_requester_hit(14,32)==PT_RECOVERY_FOCUS_COUNT);
    assert(!pt_recovery_requester_model_focus(&m,PT_RECOVERY_FOCUS_COUNT));
    assert(!memcmp(&m,&previous,sizeof(m)));++requester_groups;
}
static void focus_and_cancel(void)
{
    struct pt_native_recovery r,before;struct pt_recovery_requester_model m;
    unsigned i,dirs,sources,variables;struct pt_native_recovery_source source={0},old;
    initialize(&r,0,0);before=r;m=model_open(&r);old=source;
    dirs=directory_locks;sources=source_locks;variables=env_calls;
    for(i=1;i<=PT_RECOVERY_FOCUS_COUNT;++i) {
        assert(pt_recovery_requester_model_key(&m,PT_RECOVERY_KEY_TAB,0)==PT_RECOVERY_EVENT_CHANGED);
        assert(m.focus==i%PT_RECOVERY_FOCUS_COUNT);
    }
    for(i=1;i<=PT_RECOVERY_FOCUS_COUNT;++i) {
        assert(pt_recovery_requester_model_key(&m,PT_RECOVERY_KEY_BACKTAB,0)==PT_RECOVERY_EVENT_CHANGED);
        assert(m.focus==(PT_RECOVERY_FOCUS_COUNT-i)%PT_RECOVERY_FOCUS_COUNT);
    }
    enable(&m);input(&m,PT_RECOVERY_FOCUS_INTERVAL,"29");
    assert(pt_recovery_requester_model_focus(&m,PT_RECOVERY_FOCUS_MEDIA));
    assert(pt_recovery_requester_model_key(&m,PT_RECOVERY_KEY_ACTIVATE,0)==PT_RECOVERY_EVENT_CHANGED);
    assert(m.preferences.draft.media==PT_NATIVE_RECOVERY_MEDIA_REMOVABLE && !m.preferences.draft.allow_removable);
    assert(pt_recovery_requester_model_focus(&m,PT_RECOVERY_FOCUS_REMOVABLE));
    assert(pt_recovery_requester_model_key(&m,PT_RECOVERY_KEY_ACTIVATE,0)==PT_RECOVERY_EVENT_CHANGED);
    for(i=0;i<PT_RECOVERY_FOCUS_COUNT;++i) {
        assert(pt_recovery_requester_model_focus(&m,i));
        assert(pt_recovery_requester_model_key(&m,PT_RECOVERY_KEY_ESCAPE,0)==PT_RECOVERY_EVENT_CANCEL);
        assert(m.preferences.open && !memcmp(&r,&before,sizeof(r)));
    }
    assert(pt_recovery_requester_model_focus(&m,PT_RECOVERY_FOCUS_CANCEL));
    assert(pt_recovery_requester_model_key(&m,PT_RECOVERY_KEY_ACTIVATE,0)==PT_RECOVERY_EVENT_CANCEL);
    assert(pt_recovery_requester_model_cancel(&m) && !m.preferences.open);
    assert(!pt_recovery_requester_model_cancel(&m));
    assert(pt_recovery_requester_model_key(&m,PT_RECOVERY_KEY_TAB,0)==PT_RECOVERY_EVENT_NONE);
    assert(!memcmp(&r,&before,sizeof(r)) && !memcmp(&source,&old,sizeof(source)) &&
        directory_locks==dirs && source_locks==sources && env_calls==variables);++requester_groups;
}
static void text_and_capacity(void)
{
    struct pt_native_recovery r,before;struct pt_recovery_requester_model m,old;unsigned i;
    initialize(&r,0,0);before=r;m=model_open(&r);input(&m,PT_RECOVERY_FOCUS_DIRECTORY,"Work:AB");
    assert(pt_recovery_requester_model_key(&m,PT_RECOVERY_KEY_LEFT,0)==PT_RECOVERY_EVENT_CHANGED);
    assert(pt_recovery_requester_model_key(&m,PT_RECOVERY_KEY_CHARACTER,'X')==PT_RECOVERY_EVENT_CHANGED);
    assert(!strcmp(m.preferences.draft.directory,"Work:AXB"));
    assert(pt_recovery_requester_model_key(&m,PT_RECOVERY_KEY_BACKSPACE,0)==PT_RECOVERY_EVENT_CHANGED);
    assert(pt_recovery_requester_model_key(&m,PT_RECOVERY_KEY_DELETE,0)==PT_RECOVERY_EVENT_CHANGED);
    assert(!strcmp(m.preferences.draft.directory,"Work:A"));
    assert(pt_recovery_requester_model_key(&m,PT_RECOVERY_KEY_HOME,0)==PT_RECOVERY_EVENT_CHANGED);
    assert(pt_recovery_requester_model_key(&m,PT_RECOVERY_KEY_CHARACTER,'!')==PT_RECOVERY_EVENT_CHANGED);
    assert(!strcmp(m.preferences.draft.directory,"!Work:A"));
    assert(pt_recovery_requester_model_key(&m,PT_RECOVERY_KEY_END,0)==PT_RECOVERY_EVENT_CHANGED);
    assert(m.directory_cursor==7);
    assert(pt_recovery_requester_model_key(&m,PT_RECOVERY_KEY_ACTIVATE,0)==PT_RECOVERY_EVENT_CHANGED);
    assert(m.focus==PT_RECOVERY_FOCUS_INTERVAL); /* Return does not Apply. */
    input(&m,PT_RECOVERY_FOCUS_DIRECTORY,"");
    for(i=0;i<359;++i)assert(pt_recovery_requester_model_key(&m,PT_RECOVERY_KEY_CHARACTER,'x')==PT_RECOVERY_EVENT_CHANGED);
    old=m;
    assert(pt_recovery_requester_model_key(&m,PT_RECOVERY_KEY_CHARACTER,'x')==PT_RECOVERY_EVENT_NONE);
    assert(!memcmp(&m,&old,sizeof(m)) && m.preferences.draft.directory[359]==0);
    for(i=0;i<3;++i) {
        assert(pt_recovery_requester_model_key(&m,PT_RECOVERY_KEY_CHARACTER,i==0?0:i==1?127:256)==PT_RECOVERY_EVENT_NONE);
        assert(!memcmp(&m,&old,sizeof(m)));
    }
    input(&m,PT_RECOVERY_FOCUS_INTERVAL,"86400");old=m;
    assert(pt_recovery_requester_model_key(&m,PT_RECOVERY_KEY_CHARACTER,'1')==PT_RECOVERY_EVENT_NONE);
    assert(pt_recovery_requester_model_key(&m,PT_RECOVERY_KEY_CHARACTER,'x')==PT_RECOVERY_EVENT_NONE);
    assert(!memcmp(&m,&old,sizeof(m)) && m.preferences.draft.interval_seconds==86400);
    input(&m,PT_RECOVERY_FOCUS_INTERVAL,"");assert(!m.preferences.draft.interval_seconds);
    assert(!memcmp(&r,&before,sizeof(r)));++requester_groups;
}
static void validation_and_correction(void)
{
    struct pt_native_recovery r;struct pt_recovery_requester_model m;struct idle_state s;
    struct pt_native_recovery_source source={0};unsigned i;
    const char *bad[]={"29","86401",""};
    assert(pt_native_recovery_source_commit(&source,PT_NATIVE_RECOVERY_SOURCE_NEW,1,NULL,0));
    for(i=0;i<3;++i) {
        initialize(&r,0,0);m=model_open(&r);enable(&m);input(&m,PT_RECOVERY_FOCUS_INTERVAL,bad[i]);
        s=quiet();expect_refusal(&m,&r,&source,&s);
        input(&m,PT_RECOVERY_FOCUS_INTERVAL,i?"86400":"30");
        assert(pt_recovery_requester_model_focus(&m,PT_RECOVERY_FOCUS_APPLY));
        assert(pt_recovery_requester_model_key(&m,PT_RECOVERY_KEY_ACTIVATE,0)==PT_RECOVERY_EVENT_APPLY);
        s=quiet();assert(pt_recovery_requester_model_apply(&m,&r,&source,idle,&s)==PT_NATIVE_RECOVERY_PREFERENCES_APPLIED);
        assert(r.bound && r.schedule.policy.interval_seconds==(i?86400U:30U) && !m.preferences.open);
    }
    initialize(&r,0,0);m=model_open(&r);enable(&m);
    assert(pt_recovery_requester_model_focus(&m,PT_RECOVERY_FOCUS_MEDIA));
    assert(pt_recovery_requester_model_key(&m,PT_RECOVERY_KEY_ACTIVATE,0)==PT_RECOVERY_EVENT_CHANGED);
    assert(m.preferences.draft.media==PT_NATIVE_RECOVERY_MEDIA_REMOVABLE);
    s=quiet();expect_refusal(&m,&r,&source,&s);
    assert(pt_recovery_requester_model_focus(&m,PT_RECOVERY_FOCUS_REMOVABLE));
    assert(pt_recovery_requester_model_key(&m,PT_RECOVERY_KEY_ACTIVATE,0)==PT_RECOVERY_EVENT_CHANGED);
    s=quiet();assert(pt_recovery_requester_model_apply(&m,&r,&source,idle,&s)==PT_NATIVE_RECOVERY_PREFERENCES_APPLIED);
    assert(r.removable && !m.preferences.open);++requester_groups;
}
static void first_bind_and_noop(void)
{
    struct pt_native_recovery r,before;struct pt_recovery_requester_model m;
    struct pt_native_recovery_source source={0};struct idle_state s;unsigned calls;
    initialize(&r,0,0);before=r;m=model_open(&r);calls=directory_locks+source_locks;s=quiet();
    assert(pt_recovery_requester_model_apply(&m,&r,&source,idle,&s)==PT_NATIVE_RECOVERY_PREFERENCES_NOOP);
    assert(!memcmp(&r,&before,sizeof(r)) && directory_locks+source_locks==calls && !r.bound);
    initialize(&r,0,0);m=model_open(&r);enable(&m);
    assert(pt_native_recovery_source_commit(&source,PT_NATIVE_RECOVERY_SOURCE_SAVE_AS,1,"Work:Current.ptg",17));
    s=quiet();assert(pt_recovery_requester_model_apply(&m,&r,&source,idle,&s)==PT_NATIVE_RECOVERY_PREFERENCES_APPLIED);
    assert(r.bound && !strcmp(r.info.source,"Work:Current.ptg"));
    r.schedule.armed=1;r.schedule.since=20;r.schedule.observed=41;
    before=r;m=model_open(&r);calls=directory_locks+source_locks;s=quiet();
    assert(pt_recovery_requester_model_apply(&m,&r,&source,idle,&s)==PT_NATIVE_RECOVERY_PREFERENCES_NOOP);
    assert(!memcmp(&r,&before,sizeof(r)) && directory_locks+source_locks==calls);
    m=model_open(&r);input(&m,PT_RECOVERY_FOCUS_INTERVAL,"45");calls=source_locks;s=quiet();
    assert(pt_recovery_requester_model_apply(&m,&r,&source,idle,&s)==PT_NATIVE_RECOVERY_PREFERENCES_APPLIED);
    assert(source_locks==calls && !strcmp(r.info.source,"Work:Current.ptg"));++requester_groups;
}
static void binding_failure_and_recheck(void)
{
    struct pt_native_recovery r,before;struct pt_recovery_requester_model m,old;
    struct pt_native_recovery_source source={0};struct idle_state s;unsigned i,dirs,sources;
    assert(pt_native_recovery_source_commit(&source,PT_NATIVE_RECOVERY_SOURCE_LOAD,1,"Work:Current.ptg",17));
    for(i=0;i<3;++i) {
        initialize(&r,0,0);m=model_open(&r);enable(&m);
        if(i==0)lock_failure=2;
        if(i==1)name_failure=2;
        if(i==2)stat_failure=1;
        s=quiet();expect_refusal(&m,&r,&source,&s);
    }
    initialize(&r,0,0);m=model_open(&r);enable(&m);s=quiet();s.refuse_at=2;
    expect_refusal(&m,&r,&source,&s);
    initialize(&r,0,0);m=model_open(&r);enable(&m);r.store.owned=1;
    dirs=directory_locks;sources=source_locks;s=quiet();expect_refusal(&m,&r,&source,&s);
    assert(!s.calls && directory_locks==dirs && source_locks==sources);
    r.store.owned=0;before=r;old=m;s=quiet();
    assert(pt_recovery_requester_model_apply(&m,&r,(void *)&m,idle,&s)==PT_NATIVE_RECOVERY_PREFERENCES_REFUSED);
    assert(pt_recovery_requester_model_apply(&m,&r,(void *)&r,idle,&s)==PT_NATIVE_RECOVERY_PREFERENCES_REFUSED);
    assert(pt_recovery_requester_model_apply(&m,(void *)&m,&source,idle,&s)==PT_NATIVE_RECOVERY_PREFERENCES_REFUSED);
    assert(!s.calls && !memcmp(&r,&before,sizeof(r)) && !memcmp(&m,&old,sizeof(m)));++requester_groups;
}
int main(void)
{
    entry_and_hits();focus_and_cancel();text_and_capacity();validation_and_correction();
    first_bind_and_noop();binding_failure_and_recheck();
    assert(requester_groups==6);
    puts("RECOVERY REQUESTER MODEL HOST PASS: 6 groups; input/focus, caps/cancel, real controller apply/bind/refusal");
    puts("NOT TESTED: native window/IDCMP/rendering, editor dispatch, main timer/modal lifecycle, emulator or A1200");
    return 0;
}
