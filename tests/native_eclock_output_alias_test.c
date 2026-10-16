#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../src/native/eclock.h"

static struct ExecBase base={{36}};
struct ExecBase *SysBase=&base;
/* Complete allocations and surrounding caller storage, rather than an
 * IORequest-sized stand-in for the larger requested timerequest. */
static struct {uint64_t before[2];struct timerequest value;uint64_t after[2];} request_storage;
static struct {uint64_t before[2];struct MsgPort value;uint64_t after[2];} port_storage;
static struct {uint64_t before[2];struct Device value;uint64_t after[2];} device_storage;
static unsigned ports,requests,opens,reads,refusals,controls;
struct MsgPort *CreateMsgPort(void)
{assert(!ports);++ports;return &port_storage.value;}
void DeleteMsgPort(struct MsgPort *p)
{assert(p==&port_storage.value && ports==1 && !requests);--ports;}
struct IORequest *CreateIORequest(struct MsgPort *p,unsigned long bytes)
{assert(p==&port_storage.value && bytes==sizeof(request_storage.value) && !requests);++requests;return &request_storage.value.tr_node;}
void DeleteIORequest(struct IORequest *p)
{assert(p==&request_storage.value.tr_node && requests==1 && !opens);--requests;}
int OpenDevice(const char *name,unsigned unit,struct IORequest *p,unsigned flags)
{assert(!strcmp(name,TIMERNAME) && unit==UNIT_ECLOCK && p==&request_storage.value.tr_node && !flags && !opens);p->io_Device=&device_storage.value;++opens;return 0;}
void CloseDevice(struct IORequest *p)
{assert(p==&request_storage.value.tr_node && opens==1);--opens;}
ULONG fake_read(struct Device *device,struct EClockVal *value)
{assert(device==&device_storage.value && opens==1);++reads;value->ev_hi=0x12345678;value->ev_lo=0xabcdef01;return 700001;}

static void refused(struct pt_native_eclock *clock,uint64_t *ticks,uint32_t *frequency)
{
    struct pt_native_eclock prior=*clock;
    unsigned char request_before[sizeof(request_storage)],port_before[sizeof(port_storage)],device_before[sizeof(device_storage)];
    unsigned n=reads;int result;
    memcpy(request_before,&request_storage,sizeof(request_before));
    memcpy(port_before,&port_storage,sizeof(port_before));
    memcpy(device_before,&device_storage,sizeof(device_before));
    result=pt_native_eclock_read(clock,ticks,frequency);
    if(result || reads!=n)fprintf(stderr,"ECLOCK OUTPUT REFUSAL: expected0/no read, observed%d/%u reads\n",result,reads-n);
    assert(!result && reads==n && !memcmp(&prior,clock,sizeof(prior)));
    assert(!memcmp(request_before,&request_storage,sizeof(request_before)));
    assert(!memcmp(port_before,&port_storage,sizeof(port_before)));
    assert(!memcmp(device_before,&device_storage,sizeof(device_before)));
    assert(ports==1 && requests==1 && opens==1);++refusals;
}
static void control(struct pt_native_eclock *clock)
{
    uint64_t ticks=99;uint32_t frequency=77;unsigned n=reads;
    assert(pt_native_eclock_read(clock,&ticks,&frequency));
    assert(reads==n+1 && ticks==UINT64_C(0x12345678abcdef01) && frequency==700001);++controls;
}
int main(void)
{
    struct pt_native_eclock clock={0};
    struct region {void *data;size_t bytes;} region[4];unsigned i,j,which;
    uint64_t ticks=99;uint32_t frequency=77;
    memset(&request_storage,0,sizeof(request_storage));memset(&port_storage,0,sizeof(port_storage));memset(&device_storage,0,sizeof(device_storage));
    device_storage.value.dd_Library.lib_Version=36;
    assert(pt_native_eclock_open(&clock));
    /* The unchanged baseline writes this genuine private request field and
     * invokes the clock. This first oracle is aligned and wholly writable. */
    refused(&clock,&ticks,&request_storage.value.tr_time.tv_micro);control(&clock);
    region[0]=(struct region){&request_storage.value,sizeof(request_storage.value)};
    region[1]=(struct region){&port_storage.value,sizeof(port_storage.value)};
    region[2]=(struct region){&device_storage.value,sizeof(device_storage.value)};
    region[3]=(struct region){&clock,sizeof(clock)};
    for(i=0;i<4;++i)for(which=0;which<2;++which) {
        size_t output=which?sizeof(frequency):sizeof(ticks);
        uintptr_t start=(uintptr_t)region[i].data;
        uintptr_t address[4]={start-(output-1),start,start+region[i].bytes/2,start+region[i].bytes-1};
        for(j=0;j<4;++j) {
            ticks=99;frequency=77;
            refused(&clock,which?&ticks:(uint64_t *)address[j],which?(uint32_t *)address[j]:&frequency);
            assert(ticks==99 && frequency==77);control(&clock);
        }
    }
    assert(refusals==33 && controls==33);
    pt_native_eclock_close(&clock);assert(!ports && !requests && !opens);
    puts("NATIVE ECLOCK OUTPUT PASS:33 refusals/33 controls;full request,port,device header and owner spans;no read or mutation on refusal,64-bit value and exact cleanup;fake Exec only");
    return 0;
}
