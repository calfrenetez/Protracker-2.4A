/* SOURCE ONLY / NOT RUN. Includes the actual proposed C implementation, using
 * a deterministic API/register model. It cannot qualify an Amiga API or bus.
 */
#define PT_REPLAY_STOP_PRIVATE_SOURCE 1
#define PT_REPLAY_STOP_STUB 1
#include "native_replay_cia_stop_stubs.h"
#include <assert.h>
#include <string.h>
struct ExecBase *SysBase;
uintptr_t pt_replay_stop_stub_sp;
volatile struct CIA pt_replay_stop_stub_hardware[2];
volatile UBYTE pt_replay_enabled;
struct Library * volatile pt_replay_cia_base;
#include "../src/native/native_replay_cia_stop.c"

static struct ExecBase exec;
static struct Task task,foreign_task;
static struct Library resources[2];
static struct Interrupt server,foreign_server;
static struct {
    unsigned chip,bit,mask,pending,add,remove,control,able,set,disable,enable;
    unsigned order[32],used;
    int foreign,stuck_control,stuck_mask,stuck_pending,foreign_drift,post_dirty;
} model;
static void code(void) {}
static void other_code(void) {}
static void event(unsigned n) {assert(model.used<32);model.order[model.used++]=n;}
void Disable(void) {assert(SysBase->IDNestCnt==-1);++SysBase->IDNestCnt;++model.disable;}
void Enable(void) {assert(SysBase->IDNestCnt==0);--SysBase->IDNestCnt;++model.enable;}
void Wait(unsigned long bits) {(void)bits;assert(0&&"unexpected nonreturning owner hold");}
struct Task *FindTask(const char *name) {assert(!name);return SysBase->ThisTask;}
struct Library *OpenResource(CONST_STRPTR name)
{return !strcmp((const char *)name,"ciab.resource")?&resources[0]:!strcmp((const char *)name,"ciaa.resource")?&resources[1]:NULL;}
struct Interrupt *AddICRVector(struct Library *resource,unsigned bit,struct Interrupt *s)
{
    assert(resource==&resources[model.chip]&&bit==model.bit&&s==&server);
    assert(SysBase->IDNestCnt==0);++model.add;
    if(model.foreign)return &foreign_server;
    model.mask|=1u<<bit;return NULL;
}
void RemICRVector(struct Library *resource,unsigned bit,struct Interrupt *s)
{
    assert(resource==&resources[model.chip]&&bit==model.bit&&s==&server);
    assert(SysBase->IDNestCnt==0&&!pt_replay_enabled);
    assert(!((model.mask|model.pending)&(1u<<bit)));
    assert(!(*(bit?&pt_replay_stop_stub_hardware[model.chip].ciacrb:
                    &pt_replay_stop_stub_hardware[model.chip].ciacra)&1u));
    ++model.remove;event(5);
    if(model.post_dirty)model.mask|=1u<<bit;
}
WORD AbleICR(struct Library *resource,WORD mask)
{
    unsigned prior=model.mask;assert(resource==&resources[model.chip]);
    assert(SysBase->IDNestCnt==0);++model.able;
    if(mask) {
        assert((unsigned)mask==(1u<<model.bit));event(1);
        if(!model.stuck_mask)model.mask&=~(unsigned)mask;
        if(model.foreign_drift)model.mask^=0x10u;
    } else event(model.remove?6:0);
    return (WORD)prior;
}
WORD SetICR(struct Library *resource,WORD mask)
{
    unsigned prior=model.pending;assert(resource==&resources[model.chip]);
    assert(SysBase->IDNestCnt==0);++model.set;
    if(mask) {
        assert((unsigned)mask==(1u<<model.bit));event(3);
        if(!model.stuck_pending)model.pending&=~(unsigned)mask;
    } else event(model.remove?7:4);
    return (WORD)prior;
}
void pt_replay_stop_stub_control_write(volatile UBYTE *p,UBYTE value)
{
    assert(SysBase->IDNestCnt==0);++model.control;event(2);
    if(!model.stuck_control)*p=value;
}
static void reset(unsigned chip,unsigned bit)
{
    memset(&cia_owner,0,sizeof(cia_owner)); /* test TU only; no production reset */
    memset(&model,0,sizeof(model));memset(&exec,0,sizeof(exec));
    memset((void *)pt_replay_stop_stub_hardware,0,sizeof(pt_replay_stop_stub_hardware));
    resources[0].lib_Version=resources[1].lib_Version=39;
    task=(struct Task){{NT_PROCESS},(void *)(uintptr_t)0x1000,(void *)(uintptr_t)0x2000};
    foreign_task=task;exec.LibNode.lib_Version=39;exec.ThisTask=&task;
    exec.IDNestCnt=exec.TDNestCnt=-1;SysBase=&exec;pt_replay_stop_stub_sp=0x1800;
    server=(struct Interrupt){code,&task};model.chip=chip;model.bit=bit;
    pt_replay_enabled=0;pt_replay_cia_base=NULL;
    model.mask=0x10u;model.pending=0x10u;
}
static void acquire(void)
{
    assert(!pt_native_replay_add_cia_vector(&resources[model.chip],model.bit,&server,
                                           PT_REPLAY_STOP_HARDWARE(model.chip)));
    assert(cia_owner.phase==CIA_STOP_OWNED&&model.add==1&&!model.remove);
    pt_replay_cia_base=&resources[model.chip];pt_replay_enabled=255;
    *(model.bit?&pt_replay_stop_stub_hardware[model.chip].ciacrb:
                 &pt_replay_stop_stub_hardware[model.chip].ciacra)=0xc1;
    model.pending|=1u<<model.bit;model.used=0;
}
static void unknown_once(void)
{
    unsigned calls;
    assert(pt_native_replay_cia_retire()==-1&&cia_owner.phase==CIA_STOP_HELD);
    calls=model.able+model.set+model.remove+model.control;
    assert(pt_native_replay_cia_retire()==-1);
    assert(model.able+model.set+model.remove+model.control==calls);
    assert(pt_replay_cia_base==&resources[model.chip]);
    assert(exec.IDNestCnt==-1);
}
int main(void)
{
    unsigned chip,bit,n;
    static const unsigned success_order[]={0,1,2,3,0,4,5,6,7};
    for(chip=0;chip<2;++chip)for(bit=0;bit<2;++bit) {
        reset(chip,bit);acquire();assert(pt_native_replay_cia_retire()==1);
        assert(cia_owner.phase==CIA_STOP_RETIRED&&model.remove==1);
        assert(!pt_replay_cia_base&&!pt_replay_enabled);
        assert(model.mask==0x10u&&model.pending==0x10u);
        assert((*(bit?&pt_replay_stop_stub_hardware[chip].ciacrb:
                      &pt_replay_stop_stub_hardware[chip].ciacra))==0xc0);
        assert(model.used==sizeof(success_order)/sizeof(*success_order));
        for(n=0;n<model.used;++n)assert(model.order[n]==success_order[n]);
        assert(model.disable==model.enable&&exec.IDNestCnt==-1);
        assert(pt_native_replay_cia_retire()==1&&model.remove==1); /* observe, no replay */
        assert(pt_native_replay_add_cia_vector(&resources[chip],bit,&server,
                                             PT_REPLAY_STOP_HARDWARE(chip)));
        assert(model.add==1); /* intentionally one-shot; no generation reset API */
    }
    reset(0,0);pt_replay_stop_stub_hardware[0].ciacra=1;
    assert(pt_native_replay_add_cia_vector(&resources[0],0,&server,PT_REPLAY_STOP_HARDWARE(0)));
    assert(!model.add&&!model.remove&&!model.control&&!model.able&&!model.set);
    reset(0,0);model.foreign=1;
    assert(pt_native_replay_add_cia_vector(&resources[0],0,&server,PT_REPLAY_STOP_HARDWARE(0))==(uintptr_t)&foreign_server);
    assert(model.add==1&&!model.remove&&!model.control&&!model.able&&!model.set);
    reset(0,0);pt_replay_enabled=1;
    assert(pt_native_replay_add_cia_vector(&resources[0],0,&server,PT_REPLAY_STOP_HARDWARE(0))&&!model.add);
    for(n=0;n<10;++n) {
        reset(n&1u,(n>>1)&1u);acquire();
        switch(n) {
        case 0:exec.ThisTask=&foreign_task;break;
        case 1:pt_replay_stop_stub_sp=0x2000;break;
        case 2:server.is_Code=other_code;break;
        case 3:server.is_Data=&foreign_task;break;
        case 4:pt_replay_cia_base=&resources[!model.chip];break;
        case 5:model.stuck_control=1;break;
        case 6:model.stuck_mask=1;break;
        case 7:model.stuck_pending=1;break;
        case 8:model.foreign_drift=1;break;
        case 9:model.post_dirty=1;break;
        }
        /* Wrong alias is intentionally retained unchanged on uncertainty. */
        if(n==4) {
            struct Library *wrong=pt_replay_cia_base;
            assert(pt_native_replay_cia_retire()==-1&&cia_owner.phase==CIA_STOP_HELD);
            assert(pt_replay_cia_base==wrong&&!model.remove);
            assert(pt_native_replay_cia_retire()==-1&&!model.remove);
        } else unknown_once();
        assert(model.remove==(n==9?1u:0u));
    }
    return 0;
}
