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
{assert(!strcmp(path,"Work:Recovery") && mode==ACCESS_READ);++locks;return 1;}
void UnLock(BPTR lock) {assert(lock==1);}
LONG Examine(BPTR lock,struct FileInfoBlock *info)
{assert(lock==1);info->fib_DirEntryType=1;return 1;}
LONG NameFromLock(BPTR lock,STRPTR out,LONG capacity)
{assert(lock==1 && capacity>14);strcpy(out,"Work:Recovery");return 1;}
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
    puts("NATIVE RECOVERY CONFIG PASS: V36/V37 truncation, errors, missing defaults, raw controls and explicit removable policy");
    return 0;
}
