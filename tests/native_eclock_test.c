#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "../src/native/eclock.h"
static struct ExecBase base={{36}};struct ExecBase *SysBase=&base;
static struct Device device={{36}};static struct MsgPort port;static struct IORequest request;
static unsigned fail_at,calls,ports,requests,opens,reads;static ULONG rate=700001;
struct MsgPort *CreateMsgPort(void){++calls;if(fail_at==1)return 0;++ports;return &port;}
void DeleteMsgPort(struct MsgPort *p){assert(p==&port && !requests && ports==1);--ports;}
struct IORequest *CreateIORequest(struct MsgPort *p,unsigned long n)
{assert(p==&port && n==sizeof(struct timerequest));++calls;if(fail_at==2)return 0;++requests;return &request;}
void DeleteIORequest(struct IORequest *p){assert(p==&request && requests==1 && !opens);--requests;}
int OpenDevice(const char *name,unsigned unit,struct IORequest *p,unsigned flags)
{assert(!strcmp(name,TIMERNAME) && unit==UNIT_ECLOCK && p==&request && !flags);++calls;if(fail_at==3)return 1;p->io_Device=&device;++opens;return 0;}
void CloseDevice(struct IORequest *p){assert(p==&request && opens==1);--opens;}
ULONG fake_read(struct Device *p,struct EClockVal *v)
{assert(p==&device && opens==1);++reads;v->ev_hi=0x12345678;v->ev_lo=0xabcdef01;return rate;}
int main(void)
{
    struct pt_native_eclock c={0},before;uint64_t ticks=99;uint32_t frequency=77;unsigned i,n;
    assert(!pt_native_eclock_read(&c,&ticks,&frequency) && !reads && ticks==99 && frequency==77);
    base.LibNode.lib_Version=35;assert(!pt_native_eclock_open(&c) && !calls);base.LibNode.lib_Version=36;
    for(i=1;i<=4;++i){fail_at=i;device.dd_Library.lib_Version=i==4?35:36;
        assert(!pt_native_eclock_open(&c) && !c.port && !c.request && !c.opened && !ports && !requests && !opens);
        pt_native_eclock_close(&c);}
    fail_at=0;device.dd_Library.lib_Version=36;
    for(i=0;i<3;++i) {
        assert(pt_native_eclock_open(&c));before=c;n=calls;
        assert(!pt_native_eclock_open(&c) && calls==n && !memcmp(&before,&c,sizeof(c)));
        assert(pt_native_eclock_read(&c,&ticks,&frequency) && ticks==0x12345678abcdef01ULL && frequency==700001);
        n=reads;assert(!pt_native_eclock_read(&c,NULL,&frequency) && reads==n);
        assert(!pt_native_eclock_read(&c,&ticks,(uint32_t *)&ticks) && reads==n);
        assert(!pt_native_eclock_read(&c,(uint64_t *)&c,&frequency) && reads==n && !memcmp(&before,&c,sizeof(c)));
        ticks=99;frequency=77;rate=0;
        assert(!pt_native_eclock_read(&c,&ticks,&frequency) && ticks==99 && frequency==77);rate=700001;
        pt_native_eclock_close(&c);pt_native_eclock_close(&c);n=reads;
        assert(!ports && !requests && !opens && !pt_native_eclock_read(&c,&ticks,&frequency) && reads==n);
    }
    puts("NATIVE ECLOCK OWNER PASS: version/allocation/open failures, exact cleanup, local device base,64-bit assembly, atomic outputs and closed-read refusal; fake Exec only");
    return 0;
}
