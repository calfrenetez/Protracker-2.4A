#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "../src/native/recovery_preferences.h"

/* Exercise the actual controller and binder. Only DOS/stat are deterministic
 * host stubs: no ENV, source file, recovery directory or native UI is touched. */
static int preferences_stat(const char *,struct stat *);
#define stat(path,info) preferences_stat(path,info)
#include "../src/native/recovery.c"
#undef stat

static unsigned env_calls,directory_locks,source_locks,unlocks,stat_calls,groups;
static int lock_failure,name_failure,name_unterminated,name_empty,name_control,examine_failure;
static int stat_failure,negative_timestamp;
static LONG directory_type=1;
static char locked_source[PT_RECOVERY_SOURCE_MAX+1];
LONG GetVar(STRPTR name,STRPTR out,LONG capacity,ULONG flags)
{
    (void)name;(void)out;
    assert(capacity>1 && flags==(GVF_GLOBAL_ONLY|GVF_BINARY_VAR));
    ++env_calls;return -1;
}
LONG IoErr(void) {return ERROR_OBJECT_NOT_FOUND;}
BPTR Lock(STRPTR path,LONG mode)
{
    int directory=!strcmp(path,"Work:Recovery") || !strcmp(path,"Alias:Recovery");
    assert(mode==ACCESS_READ);
    if(directory)++directory_locks;
    else {
        assert(strlen(path)<sizeof(locked_source));
        strcpy(locked_source,path);++source_locks;
    }
    if(lock_failure==(directory?1:2))return 0;
    return directory?1:2;
}
void UnLock(BPTR lock) {assert(lock==1 || lock==2);++unlocks;}
LONG Examine(BPTR lock,struct FileInfoBlock *info)
{
    assert(lock==1);info->fib_DirEntryType=directory_type;return !examine_failure;
}
LONG NameFromLock(BPTR lock,STRPTR out,LONG capacity)
{
    const char *name=lock==1?"Work:Recovery":locked_source;
    assert((lock==1 || lock==2) && capacity>0);
    if(name_failure==(int)lock)return 0;
    if(name_unterminated==(int)lock)memset(out,'x',(size_t)capacity);
    else if(name_empty==(int)lock)out[0]=0;
    else if(name_control==(int)lock)strcpy(out,"Work:Bad\nName.ptg");
    else {assert(strlen(name)<(size_t)capacity);strcpy(out,name);}
    return 1;
}
LONG ExNext(BPTR lock,struct FileInfoBlock *info)
{(void)lock;(void)info;abort();}
LONG AddPart(STRPTR path,STRPTR name,LONG capacity)
{(void)path;(void)name;(void)capacity;abort();}
static int preferences_stat(const char *path,struct stat *info)
{
    assert(!strcmp(path,locked_source));++stat_calls;
    if(stat_failure)return -1;
    memset(info,0,sizeof(*info));info->st_mtime=negative_timestamp?-1:125;return 0;
}
static void *allocate(void *context,size_t bytes)
{(void)context;(void)bytes;abort();}
static void release(void *context,void *memory)
{(void)context;(void)memory;abort();}
struct idle_state {unsigned calls,refuse_at;int result;};
static int idle(void *context)
{
    struct idle_state *s=context;++s->calls;
    return s->refuse_at && s->calls>=s->refuse_at?0:s->result;
}
static struct idle_state quiet(void)
{struct idle_state s={0,0,1};return s;}
static void reset_faults(void)
{
    lock_failure=name_failure=name_unterminated=name_empty=name_control=examine_failure=0;
    stat_failure=negative_timestamp=0;directory_type=1;
}
static void initialize(struct pt_native_recovery *r,int configured,int bound)
{
    struct pt_allocator a={NULL,allocate,release};
    struct pt_native_recovery_configuration c;
    reset_faults();memset(r,0,sizeof(*r));
    assert(!pt_native_recovery_configure(r,&a));
    assert(r->allocator.allocate==allocate && r->allocator.release==release);
    if(configured) {
        assert(pt_native_recovery_get_configuration(r,&c));
        assert(pt_native_recovery_apply_configuration(r,&c));
    }
    if(bound)assert(pt_native_recovery_bind(r,NULL));
}
static struct pt_native_recovery_configuration enabled_configuration(void)
{
    struct pt_native_recovery_configuration c;
    memset(&c,0,sizeof(c));strcpy(c.directory,"Work:Recovery");
    c.interval_seconds=60;c.enabled=1;c.media=PT_NATIVE_RECOVERY_MEDIA_FIXED;
    return c;
}
static void open_draft(struct pt_native_recovery_preferences *p,
    struct pt_native_recovery *r)
{
    struct idle_state s=quiet();memset(p,0,sizeof(*p));
    assert(pt_native_recovery_preferences_open(p,r,idle,&s));
    assert(s.calls==2 && p->open==1);
}
static void assert_refused(struct pt_native_recovery_preferences *p,
    struct pt_native_recovery *r,const struct pt_native_recovery_source *source,
    struct idle_state *s)
{
    struct pt_native_recovery before=*r;
    struct pt_native_recovery_preferences draft=*p;
    struct pt_native_recovery_source original;
    if(source)original=*source;
    assert(pt_native_recovery_preferences_apply(p,r,source,idle,s)==
        PT_NATIVE_RECOVERY_PREFERENCES_REFUSED);
    assert(!memcmp(r,&before,sizeof(*r)) && !memcmp(p,&draft,sizeof(*p)));
    if(source)assert(!memcmp(source,&original,sizeof(original)));
}
static void entry_cases(void)
{
    struct pt_native_recovery r,before;
    struct pt_native_recovery_preferences p,held;
    struct pt_native_recovery_configuration out,old_out;
    struct idle_state s;unsigned i,calls;
    initialize(&r,1,1);before=r;memset(&p,0,sizeof(p));held=p;
    for(i=0;i<4;++i) {
        s=quiet();
        if(i==0)s.result=0;
        if(i==1)s.result=2;
        if(i==2)s.refuse_at=2;
        assert(!pt_native_recovery_preferences_open(&p,&r,i==3?NULL:idle,&s));
        assert(!memcmp(&p,&held,sizeof(p)) && !memcmp(&r,&before,sizeof(r)));
    }
    for(i=0;i<4;++i) {
        r=before;
        if(i==0)r.schedule.busy=1;
        if(i==1)r.store.busy=1;
        if(i==2)r.store.opened=1;
        if(i==3)r.store.owned=1;
        s=quiet();calls=directory_locks+source_locks;
        assert(!pt_native_recovery_preferences_open(&p,&r,idle,&s));
        assert(!s.calls && !memcmp(&p,&held,sizeof(p)) &&
            directory_locks+source_locks==calls);
    }
    r=before;s=quiet();
    assert(!pt_native_recovery_preferences_open((void *)&r,&r,idle,&s));
    assert(!pt_native_recovery_preferences_open((void *)(UINTPTR_MAX-1),&r,idle,&s));
    assert(!memcmp(&r,&before,sizeof(r)));
    memset(&out,0x5a,sizeof(out));old_out=out;
    assert(!pt_native_recovery_preferences_get(&p,&out));
    assert(!memcmp(&out,&old_out,sizeof(out)));
    open_draft(&p,&r);held=p;s=quiet();
    assert(!pt_native_recovery_preferences_open(&p,&r,idle,&s));
    assert(!memcmp(&p,&held,sizeof(p)) && !memcmp(&r,&before,sizeof(r)));
    assert(!pt_native_recovery_preferences_get(&p,(void *)&p));
    assert(!pt_native_recovery_preferences_get(&p,(void *)(UINTPTR_MAX-1)));
    assert(!pt_native_recovery_preferences_edit(&p,&p.draft));
    assert(!pt_native_recovery_preferences_edit(&p,(void *)(UINTPTR_MAX-1)));
    assert(!memcmp(&p,&held,sizeof(p)));
    ++groups;
}
static void cancel_cases(void)
{
    struct pt_native_recovery r,before;
    struct pt_native_recovery_preferences p;
    struct pt_native_recovery_configuration edited,out;
    unsigned calls,variables;
    initialize(&r,1,1);before=r;calls=directory_locks+source_locks;variables=env_calls;
    open_draft(&p,&r);assert(pt_native_recovery_preferences_get(&p,&edited));
    edited=enabled_configuration();edited.interval_seconds=29;
    assert(pt_native_recovery_preferences_edit(&p,&edited));
    assert(pt_native_recovery_preferences_get(&p,&out) && !memcmp(&out,&edited,sizeof(out)));
    assert(!memcmp(&r,&before,sizeof(r)));
    assert(pt_native_recovery_preferences_cancel(&p) && !p.open);
    assert(!pt_native_recovery_preferences_cancel(&p));
    assert(!pt_native_recovery_preferences_edit(&p,&edited));
    assert(!pt_native_recovery_preferences_get(&p,&out));
    assert(!memcmp(&r,&before,sizeof(r)) && directory_locks+source_locks==calls && env_calls==variables);
    ++groups;
}
static void noop_and_owned_cases(void)
{
    struct pt_native_recovery r,before,preserved;
    struct pt_native_recovery_preferences p;
    struct pt_native_recovery_configuration c;
    struct pt_native_recovery_source unknown={0};struct idle_state s;
    unsigned i,dirs,sources,stats;struct pt_project project={0};
    /* Default disabled draft is no-op even before first configuration/bind. */
    initialize(&r,0,0);before=r;open_draft(&p,&r);s=quiet();
    assert(pt_native_recovery_preferences_apply(&p,&r,&unknown,idle,&s)==PT_NATIVE_RECOVERY_PREFERENCES_NOOP);
    assert(!memcmp(&r,&before,sizeof(r)) && !r.bound && !r.configured && !p.open);
    initialize(&r,1,1);r.schedule.armed=1;r.schedule.since=20;r.schedule.observed=41;
    r.schedule.have_snapshot=1;r.schedule.snapshot_revision=13;r.schedule.snapshot_saved_revision=6;
    r.info.document_id=99;r.info.revision=17;r.info.saved_revision=6;
    strcpy(r.info.source,"Work:Original.ptg");r.source_timestamp=55;r.project=&project;
    before=r;open_draft(&p,&r);p.draft.directory[100]='x';s=quiet();
    dirs=directory_locks;sources=source_locks;stats=stat_calls;
    assert(pt_native_recovery_preferences_apply(&p,&r,NULL,idle,&s)==PT_NATIVE_RECOVERY_PREFERENCES_NOOP);
    assert(!memcmp(&r,&before,sizeof(r)) && directory_locks==dirs && source_locks==sources && stat_calls==stats);
    c=enabled_configuration();assert(pt_native_recovery_apply_configuration(&r,&c));
    r.bound=0;r.schedule.armed=1;r.schedule.since=20;r.schedule.observed=41;
    r.schedule.have_snapshot=1;r.schedule.snapshot_revision=13;r.schedule.snapshot_saved_revision=6;
    before=r;open_draft(&p,&r);strcpy(p.draft.directory,"Alias:Recovery");s=quiet();
    dirs=directory_locks;sources=source_locks;stats=stat_calls;
    assert(pt_native_recovery_preferences_apply(&p,&r,&unknown,idle,&s)==PT_NATIVE_RECOVERY_PREFERENCES_NOOP);
    assert(!memcmp(&r,&before,sizeof(r)) && directory_locks==dirs+1 && source_locks==sources && stat_calls==stats);
    r.bound=1;before=r;
    for(i=0;i<4;++i) {
        r=before;open_draft(&p,&r);
        if(i==0)r.store.opened=1;
        if(i==1)r.store.owned=1;
        if(i==2)r.store.busy=1;
        if(i==3)r.schedule.busy=1;
        s=quiet();dirs=directory_locks;sources=source_locks;stats=stat_calls;
        assert_refused(&p,&r,&unknown,&s);p.draft.enabled=0;s=quiet();
        assert_refused(&p,&r,&unknown,&s);
        assert(!s.calls && directory_locks==dirs && source_locks==sources && stat_calls==stats);
    }
    r=before;open_draft(&p,&r);p.draft.interval_seconds=45;s=quiet();
    sources=source_locks;stats=stat_calls;
    assert(pt_native_recovery_preferences_apply(&p,&r,&unknown,idle,&s)==PT_NATIVE_RECOVERY_PREFERENCES_APPLIED);
    preserved=r;preserved.schedule=before.schedule;memcpy(preserved.root,before.root,sizeof(preserved.root));
    preserved.configured=before.configured;preserved.removable=before.removable;
    assert(!memcmp(&preserved,&before,sizeof(before)) && source_locks==sources && stat_calls==stats);
    assert(r.bound && r.info.document_id==99 && r.project==&project && r.source_timestamp==55);
    ++groups;
}
static void validation_cases(void)
{
    struct pt_native_recovery r;struct pt_native_recovery_preferences p;
    struct pt_native_recovery_configuration c,bad;struct idle_state s;
    unsigned i,dirs,sources,stats,unlocked;
    initialize(&r,1,1);c=enabled_configuration();
    for(i=0;i<12;++i) {
        open_draft(&p,&r);bad=c;
        if(i==0)bad.enabled=2;
        if(i==1)bad.media=3;
        if(i==2)bad.allow_removable=2;
        if(i==3)bad.interval_seconds=29;
        if(i==4)bad.interval_seconds=86401;
        if(i==5)memset(bad.directory,'x',sizeof(bad.directory));
        if(i==6)bad.directory[4]='\n';
        if(i==7)bad.directory[0]=0;
        if(i==8)bad.media=PT_NATIVE_RECOVERY_MEDIA_UNKNOWN;
        if(i==9) {bad.media=PT_NATIVE_RECOVERY_MEDIA_REMOVABLE;bad.allow_removable=0;}
        if(i==10) {bad.enabled=0;bad.interval_seconds=0;}
        if(i==11) {bad.enabled=0;bad.media=PT_NATIVE_RECOVERY_MEDIA_UNKNOWN;}
        assert(pt_native_recovery_preferences_edit(&p,&bad));s=quiet();
        dirs=directory_locks;sources=source_locks;stats=stat_calls;
        assert_refused(&p,&r,NULL,&s);
        assert(directory_locks==dirs && source_locks==sources && stat_calls==stats);
    }
    for(i=0;i<5;++i) {
        open_draft(&p,&r);assert(pt_native_recovery_preferences_edit(&p,&c));
        lock_failure=i==0?1:0;examine_failure=i==1;directory_type=i==2?-1:1;
        name_failure=i==3?1:0;name_unterminated=i==4?1:0;
        dirs=directory_locks;unlocked=unlocks;s=quiet();assert_refused(&p,&r,NULL,&s);
        assert(directory_locks==dirs+1 && unlocks==unlocked+(i?1:0));reset_faults();
    }
    for(i=0;i<3;++i) {
        open_draft(&p,&r);c=enabled_configuration();c.interval_seconds=i?86400:30;
        if(i==2) {c.media=PT_NATIVE_RECOVERY_MEDIA_REMOVABLE;c.allow_removable=1;c.interval_seconds=45;}
        assert(pt_native_recovery_preferences_edit(&p,&c));s=quiet();
        assert(pt_native_recovery_preferences_apply(&p,&r,NULL,idle,&s)==PT_NATIVE_RECOVERY_PREFERENCES_APPLIED);
        assert(r.schedule.policy.interval_seconds==c.interval_seconds && r.removable==(i==2));
    }
    ++groups;
}
static void first_bind_cases(void)
{
    struct pt_native_recovery r;struct pt_native_recovery_preferences p;
    struct pt_native_recovery_configuration c=enabled_configuration();
    struct pt_native_recovery_source source={0},unknown={0};struct idle_state s;
    unsigned i,dirs,sources,stats,unlocked;
    assert(pt_native_recovery_source_commit(&source,PT_NATIVE_RECOVERY_SOURCE_LOAD,1,
        "Work:Original.ptg",sizeof("Work:Original.ptg")));
    initialize(&r,0,0);open_draft(&p,&r);
    assert(pt_native_recovery_preferences_edit(&p,&c));s=quiet();assert_refused(&p,&r,&unknown,&s);
    s=quiet();assert_refused(&p,&r,NULL,&s);
    for(i=0;i<7;++i) {
        initialize(&r,0,0);open_draft(&p,&r);assert(pt_native_recovery_preferences_edit(&p,&c));
        lock_failure=i==0?2:0;name_failure=i==1?2:0;name_unterminated=i==2?2:0;
        name_empty=i==3?2:0;name_control=i==4?2:0;
        stat_failure=i==5;negative_timestamp=i==6;
        dirs=directory_locks;sources=source_locks;stats=stat_calls;unlocked=unlocks;
        s=quiet();assert_refused(&p,&r,&source,&s);
        assert(directory_locks==dirs+1 && source_locks==sources+1);
        assert(unlocks==unlocked+(i?2:1) && stat_calls==stats+(i>=5));reset_faults();
    }
    initialize(&r,0,0);open_draft(&p,&r);assert(pt_native_recovery_preferences_edit(&p,&c));
    dirs=directory_locks;sources=source_locks;stats=stat_calls;s=quiet();
    assert(pt_native_recovery_preferences_apply(&p,&r,&source,idle,&s)==PT_NATIVE_RECOVERY_PREFERENCES_APPLIED);
    assert(r.bound && r.configured && r.info.document_id && r.source_timestamp==125);
    assert(!strcmp(r.info.source,"Work:Original.ptg") && !r.store.opened && !r.store.owned);
    assert(directory_locks==dirs+1 && source_locks==sources+1 && stat_calls==stats+1);
    assert(pt_native_recovery_source_commit(&source,PT_NATIVE_RECOVERY_SOURCE_NEW,1,NULL,0));
    initialize(&r,0,0);open_draft(&p,&r);p.draft.interval_seconds=45;s=quiet();
    dirs=directory_locks;sources=source_locks;stats=stat_calls;
    assert(pt_native_recovery_preferences_apply(&p,&r,&source,idle,&s)==PT_NATIVE_RECOVERY_PREFERENCES_APPLIED);
    assert(r.bound && r.configured && !r.info.source[0] && r.info.document_id && !r.source_timestamp);
    assert(directory_locks==dirs && source_locks==sources && stat_calls==stats);
    initialize(&r,1,0);open_draft(&p,&r);s=quiet();
    assert(pt_native_recovery_preferences_apply(&p,&r,&unknown,idle,&s)==PT_NATIVE_RECOVERY_PREFERENCES_NOOP);
    assert(!r.bound);
    ++groups;
}
static void source_cases(void)
{
    struct pt_native_recovery_source source={0},before,malformed;
    char out[PT_RECOVERY_SOURCE_MAX+1],old_out[sizeof(out)],maximum[sizeof(out)],overflow[sizeof(out)+1];
    unsigned i;
    memset(out,0x5a,sizeof(out));memcpy(old_out,out,sizeof(out));before=source;
    assert(!pt_native_recovery_source_get(&source,out,sizeof(out)) && !memcmp(out,old_out,sizeof(out)));
    assert(!pt_native_recovery_source_commit(&source,PT_NATIVE_RECOVERY_SOURCE_RESTORE,1,NULL,0));
    assert(!memcmp(&source,&before,sizeof(source)));
    assert(pt_native_recovery_source_commit(&source,PT_NATIVE_RECOVERY_SOURCE_LOAD,1,
        "Work:Original.ptg",sizeof("Work:Original.ptg")));
    assert(pt_native_recovery_source_get(&source,out,sizeof(out)) && !strcmp(out,"Work:Original.ptg"));
    before=source;
    for(i=0;i<5;++i) {
        assert(!pt_native_recovery_source_commit(&source,(enum pt_native_recovery_source_transition)i,0,
            "Work:Failed.ptg",sizeof("Work:Failed.ptg")));
        assert(!memcmp(&source,&before,sizeof(source)));
    }
    assert(pt_native_recovery_source_commit(&source,PT_NATIVE_RECOVERY_SOURCE_RESTORE,1,
        "Work:Recovery/session.ptg",sizeof("Work:Recovery/session.ptg")));
    assert(!memcmp(&source,&before,sizeof(source)));
    assert(pt_native_recovery_source_commit(&source,PT_NATIVE_RECOVERY_SOURCE_SAVE,1,
        "Work:Saved.ptg",sizeof("Work:Saved.ptg")));
    assert(!strcmp(source.name,"Work:Saved.ptg"));
    assert(pt_native_recovery_source_commit(&source,PT_NATIVE_RECOVERY_SOURCE_SAVE_AS,1,
        "Work:New.ptg",sizeof("Work:New.ptg")));
    assert(!strcmp(source.name,"Work:New.ptg"));before=source;
    memset(maximum,'m',sizeof(maximum));maximum[sizeof(maximum)-1]=0;
    memset(overflow,'x',sizeof(overflow));overflow[sizeof(overflow)-1]=0;
    assert(!pt_native_recovery_source_commit(&source,PT_NATIVE_RECOVERY_SOURCE_SAVE,1,overflow,sizeof(overflow)));
    assert(!pt_native_recovery_source_commit(&source,PT_NATIVE_RECOVERY_SOURCE_SAVE,1,maximum,sizeof(maximum)-1));
    assert(!pt_native_recovery_source_commit(&source,PT_NATIVE_RECOVERY_SOURCE_SAVE,1,"bad\nname",sizeof("bad\nname")));
    assert(!pt_native_recovery_source_commit(&source,PT_NATIVE_RECOVERY_SOURCE_SAVE,1,"",1));
    assert(!pt_native_recovery_source_commit(&source,PT_NATIVE_RECOVERY_SOURCE_SAVE,1,source.name,sizeof(source.name)));
    assert(!pt_native_recovery_source_commit(&source,PT_NATIVE_RECOVERY_SOURCE_SAVE,1,(void *)(UINTPTR_MAX-1),4));
    assert(!pt_native_recovery_source_commit(&source,(enum pt_native_recovery_source_transition)255,1,NULL,0));
    assert(!pt_native_recovery_source_commit(&source,PT_NATIVE_RECOVERY_SOURCE_NEW,1,"ignored",sizeof("ignored")));
    assert(!memcmp(&source,&before,sizeof(source)));
    memset(out,0x5a,sizeof(out));memcpy(old_out,out,sizeof(out));
    assert(!pt_native_recovery_source_get(&source,out,strlen(source.name)));
    assert(!memcmp(out,old_out,sizeof(out)));
    assert(!pt_native_recovery_source_get(&source,source.name,sizeof(source.name)));
    assert(!pt_native_recovery_source_get(&source,(void *)(UINTPTR_MAX-1),4));
    assert(!memcmp(&source,&before,sizeof(source)));
    malformed=source;memset(malformed.name,'x',sizeof(malformed.name));
    assert(!pt_native_recovery_source_get(&malformed,out,sizeof(out)) && !memcmp(out,old_out,sizeof(out)));
    assert(pt_native_recovery_source_commit(&source,PT_NATIVE_RECOVERY_SOURCE_SAVE,1,maximum,sizeof(maximum)));
    assert(pt_native_recovery_source_get(&source,out,sizeof(out)) && !memcmp(out,maximum,sizeof(out)));
    assert(pt_native_recovery_source_commit(&source,PT_NATIVE_RECOVERY_SOURCE_NEW,1,NULL,0));
    assert(source.kind==PT_NATIVE_RECOVERY_SOURCE_UNTITLED && !source.name[0]);
    assert(pt_native_recovery_source_get(&source,out,1) && !out[0]);
    ++groups;
}
static void final_idle_cases(void)
{
    struct pt_native_recovery r,before;struct pt_native_recovery_preferences p,held;
    struct pt_native_recovery_source source={0};struct idle_state s;
    struct pt_native_recovery_configuration c=enabled_configuration();
    unsigned dirs,sources,stats,variables;
    initialize(&r,0,0);open_draft(&p,&r);assert(pt_native_recovery_preferences_edit(&p,&c));
    assert(pt_native_recovery_source_commit(&source,PT_NATIVE_RECOVERY_SOURCE_LOAD,1,
        "Work:Original.ptg",sizeof("Work:Original.ptg")));
    dirs=directory_locks;sources=source_locks;stats=stat_calls;variables=env_calls;
    s=quiet();s.refuse_at=2;assert_refused(&p,&r,&source,&s);
    assert(s.calls==2 && directory_locks==dirs+1 && source_locks==sources+1 && stat_calls==stats+1 && env_calls==variables);
    /* No-op is also refused if activity starts before publication/close. */
    initialize(&r,1,0);open_draft(&p,&r);s=quiet();s.refuse_at=2;
    assert_refused(&p,&r,&source,&s);assert(s.calls==2);
    before=r;held=p;s=quiet();
    assert(pt_native_recovery_preferences_apply((void *)&r,&r,&source,idle,&s)==PT_NATIVE_RECOVERY_PREFERENCES_REFUSED);
    assert(pt_native_recovery_preferences_apply(&p,&r,(void *)&r,idle,&s)==PT_NATIVE_RECOVERY_PREFERENCES_REFUSED);
    assert(pt_native_recovery_preferences_apply(&p,&r,(void *)&p,idle,&s)==PT_NATIVE_RECOVERY_PREFERENCES_REFUSED);
    assert(pt_native_recovery_preferences_apply(&p,&r,(void *)(UINTPTR_MAX-1),idle,&s)==PT_NATIVE_RECOVERY_PREFERENCES_REFUSED);
    assert(!memcmp(&r,&before,sizeof(r)) && !memcmp(&p,&held,sizeof(p)));
    ++groups;
}
int main(void)
{
    entry_cases();cancel_cases();noop_and_owned_cases();validation_cases();
    first_bind_cases();source_cases();final_idle_cases();assert(groups==7);
    puts("NATIVE RECOVERY PREFERENCES HOST PASS: 7 seam groups; draft/cancel, atomic apply/initial bind, source tracking, idle and alias refusal");
    puts("NOT TESTED: requester UI/key dispatch/modal loop, native ABI, emulator or physical A1200");
    return 0;
}
