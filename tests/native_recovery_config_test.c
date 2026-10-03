#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "../src/native/recovery.c"

/* DOS V36 returns the full length; V37+ returns copied bytes. No actual ENV
 * access. Model raw bytes so an embedded NUL cannot hide an invalid suffix. */
static const char *settings[4]={"Work:Recovery","fixed","30","0"};
static const char *names[4]={"PT24G_RECOVERY_DIR","PT24G_RECOVERY_MEDIA",
    "PT24G_RECOVERY_SECONDS","PT24G_RECOVERY_REMOVABLE"};
static LONG error,read_error;
static int error_index=-1,old_dos;
static size_t raw_length;
static unsigned locks;
static unsigned unlocks;
static int lock_failure,examine_failure,name_failure,name_unterminated;
static LONG directory_type=1;
LONG GetVar(STRPTR name,STRPTR out,LONG capacity,ULONG flags)
{
    unsigned i;size_t n,copied;
    assert(flags==(GVF_GLOBAL_ONLY|GVF_BINARY_VAR) && capacity>1);
    for(i=0;i<4 && strcmp(name,names[i]);++i) {}
    assert(i<4);
    if((int)i==error_index) {error=read_error;return -1;}
    if(!settings[i]) {error=ERROR_OBJECT_NOT_FOUND;return -1;}
    n=i==2 && raw_length?raw_length:strlen(settings[i]);
    copied=n<(size_t)capacity?n:(size_t)capacity-1;
    memcpy(out,settings[i],copied);out[copied]=0;error=(LONG)n;
    return (LONG)(old_dos?n:copied);
}
LONG IoErr(void) {return error;}
BPTR Lock(STRPTR path,LONG mode)
{assert((!strcmp(path,"Work:Recovery") || !strcmp(path,"Alias:Recovery")) && mode==ACCESS_READ);++locks;return lock_failure?0:1;}
void UnLock(BPTR lock) {assert(lock==1);++unlocks;}
LONG Examine(BPTR lock,struct FileInfoBlock *info)
{assert(lock==1);info->fib_DirEntryType=directory_type;return !examine_failure;}
LONG NameFromLock(BPTR lock,STRPTR out,LONG capacity)
{assert(lock==1 && capacity>14);if(name_failure)return 0;if(name_unterminated)memset(out,'x',(size_t)capacity);else strcpy(out,"Work:Recovery");return 1;}
LONG ExNext(BPTR lock,struct FileInfoBlock *info)
{(void)lock;(void)info;abort();}
LONG AddPart(STRPTR path,STRPTR name,LONG capacity)
{(void)path;(void)name;(void)capacity;abort();}
static void *allocate(void *context,size_t bytes)
{(void)context;(void)bytes;abort();}
static void release(void *context,void *memory)
{(void)context;(void)memory;abort();}
static void check(int expected,uint32_t seconds)
{
    struct pt_native_recovery r={0};struct pt_allocator a={NULL,allocate,release};
    unsigned before=locks;
    assert(pt_native_recovery_configure(&r,&a)==expected);
    assert(r.configured==expected);
    if(expected)assert(r.schedule.policy.interval_seconds==seconds && locks==before+1);
    else assert(locks==before); /* Invalid configuration never reaches storage. */
}
static void typed_configuration_cases(void)
{
    struct pt_native_recovery r={0},before;struct pt_allocator a={NULL,allocate,release};
    struct pt_native_recovery_configuration c,bad,out,expected;size_t i;
    unsigned calls,unlocked;struct pt_project project={0};
    memset(&out,0x5a,sizeof(out));assert(pt_native_recovery_get_configuration(&r,&out));
    assert(!out.enabled && !out.directory[0] && out.media==PT_NATIVE_RECOVERY_MEDIA_UNKNOWN && out.interval_seconds==300);
    before=r;assert(!pt_native_recovery_apply_configuration(&r,&out) && !memcmp(&r,&before,sizeof(r)));
    /* Startup accepts its own allocator as input; this used to erase callbacks. */
    r.allocator=a;assert(pt_native_recovery_configure(&r,&r.allocator));
    assert(r.allocator.allocate==allocate && r.allocator.release==release && r.allocator.context==NULL);
    assert(pt_native_recovery_get_configuration(&r,&c));assert(c.enabled && c.media==PT_NATIVE_RECOVERY_MEDIA_FIXED);
    assert(!strcmp(c.directory,"Work:Recovery") && c.interval_seconds==86400);
    r.bound=1;r.info.document_id=UINT64_C(0x123456789);r.info.revision=17;r.info.saved_revision=6;
    strcpy(r.info.source,"Work:Original.ptg");r.source_timestamp=55;r.project=&project;
    r.schedule.armed=1;r.schedule.since=20;r.schedule.observed=41;r.schedule.have_snapshot=1;
    r.schedule.snapshot_revision=13;r.schedule.snapshot_saved_revision=6;
    before=r;calls=locks;
    /* Cancel is discard of the local draft; get never advances the owner. */
    assert(pt_native_recovery_get_configuration(&r,&bad));bad.enabled=0;bad.interval_seconds=45;
    assert(!memcmp(&r,&before,sizeof(r)) && locks==calls);
    /* Semantic no-op ignores unused text/padding and retains failed-write backoff. */
    c.directory[100]='x';assert(pt_native_recovery_apply_configuration(&r,&c));
    assert(!memcmp(&r,&before,sizeof(r)) && locks==calls);
    strcpy(c.directory,"Alias:Recovery");unlocked=unlocks;
    assert(pt_native_recovery_apply_configuration(&r,&c));
    assert(!memcmp(&r,&before,sizeof(r)) && locks==calls+1 && unlocks==unlocked+1);
    assert(pt_native_recovery_get_configuration(&r,&c));
    for(i=0;i<4;++i) {
        r=before;
        if(i==0)r.schedule.busy=1;
        if(i==1)r.store.busy=1;
        if(i==2)r.store.opened=1;
        if(i==3)r.store.owned=2;
        {struct pt_native_recovery held=r;calls=locks;
            assert(!pt_native_recovery_apply_configuration(&r,&c));
            assert(!memcmp(&r,&held,sizeof(r)) && locks==calls);
            bad=c;bad.enabled=0;assert(!pt_native_recovery_apply_configuration(&r,&bad));
            assert(!memcmp(&r,&held,sizeof(r)) && locks==calls);
            assert(!pt_native_recovery_configure(&r,&r.allocator));
            assert(!memcmp(&r,&held,sizeof(r)) && locks==calls);
        }
    }
    r=before;calls=locks;
    /* Whole-controller disjoint output/input checks precede any access/write. */
    {
        void *aliases[]={&r,r.root,&r.schedule,&r.store,(unsigned char *)&r+sizeof(r)-1};
        for(i=0;i<sizeof(aliases)/sizeof(*aliases);++i) {
            assert(!pt_native_recovery_get_configuration(&r,aliases[i]));
            assert(!pt_native_recovery_apply_configuration(&r,aliases[i]));
            assert(!memcmp(&r,&before,sizeof(r)) && locks==calls);
        }
        assert(!pt_native_recovery_get_configuration(&r,(void *)(UINTPTR_MAX-1)));
        assert(!pt_native_recovery_apply_configuration(&r,(void *)(UINTPTR_MAX-1)));
    }
    memset(&out,0x5a,sizeof(out));expected=out;r.schedule.busy=1;
    assert(!pt_native_recovery_get_configuration(&r,&out) && !memcmp(&out,&expected,sizeof(out)));
    r=before;
    for(i=0;i<10;++i) {
        bad=c;
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
        calls=locks;assert(!pt_native_recovery_apply_configuration(&r,&bad));
        assert(!memcmp(&r,&before,sizeof(r)) && locks==calls);
    }
    bad=c;bad.interval_seconds=30;
    for(i=0;i<5;++i) {
        calls=locks;unlocked=unlocks;
        lock_failure=i==0;examine_failure=i==1;directory_type=i==2?-1:1;
        name_failure=i==3;name_unterminated=i==4;
        assert(!pt_native_recovery_apply_configuration(&r,&bad));
        assert(!memcmp(&r,&before,sizeof(r)) && locks==calls+1 && unlocks==unlocked+(i?1:0));
    }
    lock_failure=examine_failure=name_failure=name_unterminated=0;directory_type=1;
    assert(pt_native_recovery_apply_configuration(&r,&bad));
    {struct pt_native_recovery preserved=r;
        preserved.schedule=before.schedule;memcpy(preserved.root,before.root,sizeof(preserved.root));
        preserved.configured=before.configured;preserved.removable=before.removable;
        assert(!memcmp(&preserved,&before,sizeof(before)));
    }
    assert(r.bound && r.info.document_id==before.info.document_id && r.project==before.project && r.source_timestamp==55);
    assert(r.schedule.policy.interval_seconds==30 && r.schedule.policy.enabled && !r.schedule.armed && !r.schedule.have_snapshot && !r.schedule.since);
    assert(pt_native_recovery_get_configuration(&r,&c));before=r;calls=locks;
    c.enabled=0;c.interval_seconds=45;c.media=PT_NATIVE_RECOVERY_MEDIA_REMOVABLE;c.allow_removable=0;
    assert(pt_native_recovery_apply_configuration(&r,&c));
    assert(locks==calls && r.configured && !r.schedule.policy.enabled && r.removable && r.bound && r.source_timestamp==55 && r.info.document_id==before.info.document_id);
    assert(pt_native_recovery_get_configuration(&r,&out) && !out.enabled && out.media==PT_NATIVE_RECOVERY_MEDIA_REMOVABLE && out.interval_seconds==45);
    before=r;c.enabled=1;
    assert(!pt_native_recovery_apply_configuration(&r,&c) && !memcmp(&r,&before,sizeof(r)) && locks==calls);
    c.enabled=0;c.media=PT_NATIVE_RECOVERY_MEDIA_UNKNOWN;c.directory[0]=0;
    assert(pt_native_recovery_apply_configuration(&r,&c) && locks==calls);
    assert(r.configured && !r.root[0] && !r.schedule.policy.enabled);
    {struct pt_recovery_candidate found;assert(pt_native_recovery_find(&r,&found)==PT_RECOVERY_NONE && locks==calls);}
    /* Disabled but valid policy still tracks unnamed document transitions. */
    assert(pt_native_recovery_bind(&r,NULL) && r.bound && !r.info.source[0] && r.info.document_id && !r.schedule.policy.enabled);
    assert(pt_native_recovery_get_configuration(&r,&c));
    memset(c.directory,'x',sizeof(c.directory));c.directory[sizeof(c.directory)-1]=0;c.media=PT_NATIVE_RECOVERY_MEDIA_FIXED;
    assert(pt_native_recovery_apply_configuration(&r,&c) && locks==calls);
    assert(pt_native_recovery_get_configuration(&r,&out) && !memcmp(c.directory,out.directory,sizeof(c.directory)));
    puts("NATIVE RECOVERY CONTROLLER PASS: bounded get/apply, cancel/no-op/backoff, invalid/path/busy/owned refusal, allocator alias, retained identity and configured disabled policy");
}
int main(void)
{
    const char *bad[]={"","0","29","86401","+30"," 30","30x","30\ninvalid",
        "30\r","30\177","000000000000000000000000000000000000030invalid"};
    unsigned i;char overflow[400];
    for(old_dos=0;old_dos<2;++old_dos) {
        settings[2]="30";check(1,30);
        settings[2]=NULL;check(1,300);
        for(i=0;i<sizeof(bad)/sizeof(*bad);++i) {settings[2]=bad[i];check(0,0);}
        settings[2]="30\0hidden";raw_length=9;check(0,0);raw_length=0;
        settings[2]="86400";check(1,86400);
        error_index=2;read_error=224;check(0,0);error_index=-1;
        settings[3]=NULL;check(1,86400);
        settings[1]="removable";check(0,0);
        settings[3]="1";check(1,86400);
        settings[3]="yes";check(0,0);
        settings[3]="1\n0";check(0,0);
        settings[3]="0";settings[1]="fixed";
        memset(overflow,'x',sizeof(overflow)-1);overflow[sizeof(overflow)-1]=0;
        settings[0]=overflow;check(0,0);settings[0]="Work:Recovery";
        settings[1]="fixed\nremovable";check(0,0);settings[1]="fixed";
    }
    typed_configuration_cases();
    puts("NATIVE RECOVERY CONFIG PASS: V36/V37 truncation, errors, missing defaults, raw controls and explicit removable policy");
    return 0;
}
