#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define PT_CIA_OWNER_STUB
typedef uint8_t UBYTE;typedef int16_t WORD;
struct Task {unsigned identity;};struct Library {unsigned chip;};
struct Interrupt {struct {int ln_Type,ln_Pri;char *ln_Name;} is_Node;void *is_Data;void (*is_Code)(void);};
struct CIA {UBYTE ciacra,ciacrb,ciatalo,ciatahi,ciatblo,ciatbhi;};
#define NT_INTERRUPT 2
static struct Task task={1},other={2},*current=&task;
static struct CIA host_hardware[2];static struct Library resource[2]={{0},{1}};
static struct Interrupt foreign, *vectors[2][2];
static unsigned enabled[2],pending[2],missing,adds,removes,changes,writes,depth,hold_start;
static struct Task *FindTask(const char *name){assert(!name);return current;}
static void Disable(void){++depth;}static void Enable(void){assert(depth);--depth;}
static void *OpenResource(const char *name){unsigned chip=!strcmp(name,"ciaa.resource");assert(depth);return missing&(1U<<chip)?NULL:&resource[chip];}
static struct Interrupt *AddICRVector(struct Library *r,unsigned bit,struct Interrupt *s)
{assert(depth && bit<2);++adds;if(vectors[r->chip][bit])return vectors[r->chip][bit];vectors[r->chip][bit]=s;enabled[r->chip]|=1U<<bit;return NULL;}
static void RemICRVector(struct Library *r,unsigned bit,struct Interrupt *s)
{assert(depth && vectors[r->chip][bit]==s);vectors[r->chip][bit]=NULL;enabled[r->chip]&=~(1U<<bit);++removes;}
static WORD AbleICR(struct Library *r,WORD mask)
{unsigned old=enabled[r->chip];assert(depth && (mask&0x7f)<=3);++changes;if(mask&0x80)enabled[r->chip]|=mask&0x7f;else enabled[r->chip]&=~mask;return old;}
static WORD SetICR(struct Library *r,WORD mask)
{unsigned old=pending[r->chip];assert(depth && (mask&0x7f)<=3);++changes;if(mask&0x80)pending[r->chip]|=mask&0x7f;else pending[r->chip]&=~mask;return old;}
static void control_write(volatile UBYTE *p,UBYTE value)
{assert(depth);++writes;*p=value;if(hold_start)*p|=1;}
#define PT_CIA_HARDWARE(chip) (&host_hardware[chip])
#define PT_CIA_CONTROL_WRITE(pointer,value) control_write(pointer,value)
#include "native_cia_owner.h"
static void handler(void){}
struct clock_input {uint64_t ticks;uint32_t frequency;unsigned fail,calls;};
static int read_clock(void *context,uint64_t *ticks,uint32_t *frequency)
{
    struct clock_input *c=context;assert(depth);++c->calls;
    assert(!(host_hardware[0].ciacra&1U));
    if(c->fail)return 0;
    *ticks=c->ticks;*frequency=c->frequency;return 1;
}
static void reset(void)
{assert(!depth);memset(host_hardware,0,sizeof(host_hardware));memset(vectors,0,sizeof(vectors));enabled[0]=enabled[1]=0x1c;pending[0]=pending[1]=0x1c;missing=adds=removes=changes=writes=hold_start=0;current=&task;}
int main(void)
{
    struct pt_diagnostic_cia t,copy;struct CIA before[2];unsigned cases=0;int data=1;
    reset();t=(struct pt_diagnostic_cia){0};vectors[0][0]=vectors[0][1]=vectors[1][0]=vectors[1][1]=&foreign;
    memcpy(before,host_hardware,sizeof(before));assert(!pt_diagnostic_cia_acquire(&t,handler,&data));
    assert(adds==4 && !removes && !changes && !writes && !memcmp(before,host_hardware,sizeof(before)) && !depth);
    assert(pt_diagnostic_cia_close(&t));++cases;
    reset();t=(struct pt_diagnostic_cia){0};vectors[0][0]=&foreign;host_hardware[0].ciacrb=0x80;
    assert(pt_diagnostic_cia_acquire(&t,handler,&data) && t.chip==0 && t.bit==1);
    assert(vectors[0][0]==&foreign && !pt_diagnostic_cia_acquire(&t,handler,&data));
    assert(!pt_diagnostic_cia_arm(&t,0) && !pt_diagnostic_cia_arm(&t,65536) && !writes);
    assert(pt_diagnostic_cia_arm(&t,0x1234));assert(host_hardware[0].ciatblo==0x34 && host_hardware[0].ciatbhi==0x12 && host_hardware[0].ciacrb==0x99);
    assert(enabled[0]==0x1e && pending[0]==0x1c);
    copy=t;assert(!pt_diagnostic_cia_arm(&copy,1) && !pt_diagnostic_cia_close(&copy));
    current=&other;assert(!pt_diagnostic_cia_arm(&t,1) && !pt_diagnostic_cia_close(&t) && t.held);current=&task;
    hold_start=1;assert(!pt_diagnostic_cia_close(&t) && t.held && t.resource && !removes);hold_start=0;
    assert(pt_diagnostic_cia_close(&t) && !t.held && !t.resource && !t.self && !t.task);
    assert(removes==1 && vectors[0][0]==&foreign && !vectors[0][1] && host_hardware[0].ciacrb==0x80);
    assert(enabled[0]==0x1c && pending[0]==0x1c && pt_diagnostic_cia_close(&t) && !depth);++cases;
    reset();t=(struct pt_diagnostic_cia){0};host_hardware[0].ciacra=1;
    assert(pt_diagnostic_cia_acquire(&t,handler,&data) && t.bit==1);
    assert(host_hardware[0].ciacra==1 && !writes && removes==0 && adds==1 && !vectors[0][0]);
    assert(pt_diagnostic_cia_close(&t) && removes==1 && host_hardware[0].ciacra==1);++cases;
    reset();t=(struct pt_diagnostic_cia){0};missing=1;host_hardware[1].ciacra=0xc0;
    assert(pt_diagnostic_cia_acquire(&t,handler,&data) && t.chip==1 && t.bit==0);
    assert(pt_diagnostic_cia_arm(&t,65535) && host_hardware[1].ciacra==0xd9);
    assert(pt_diagnostic_cia_close(&t) && host_hardware[1].ciacra==0xc0 && !depth);++cases;
    reset();t=(struct pt_diagnostic_cia){0};missing=3;
    assert(!pt_diagnostic_cia_acquire(&t,handler,&data) && !adds && !writes && pt_diagnostic_cia_close(&t));++cases;
    reset();t=(struct pt_diagnostic_cia){0};assert(pt_diagnostic_cia_acquire(&t,handler,&data));
    t.server.is_Data=NULL;assert(!pt_diagnostic_cia_close(&t) && !removes && t.held);
    t.server.is_Data=&data;assert(pt_diagnostic_cia_close(&t));++cases;
    /* Failed arm must not publish new count latches or drop the live vector. */
    reset();t=(struct pt_diagnostic_cia){0};assert(pt_diagnostic_cia_acquire(&t,handler,&data));
    host_hardware[0].ciatalo=0x56;host_hardware[0].ciatahi=0x78;hold_start=1;
    assert(!pt_diagnostic_cia_arm(&t,0x1234) && t.held && t.resource);
    assert(host_hardware[0].ciatalo==0x56 && host_hardware[0].ciatahi==0x78 && !removes);
    assert(vectors[0][0]==&t.server && enabled[0]==0x1c && !depth);
    hold_start=0;assert(pt_diagnostic_cia_close(&t));++cases;
    /* Every free vector may still belong to a running hardware timer. Refuse
     * all four without stopping them or touching their count/control bytes. */
    reset();t=(struct pt_diagnostic_cia){0};
    host_hardware[0]=(struct CIA){0xc1,0x81,1,2,3,4};
    host_hardware[1]=(struct CIA){0xc1,0x81,5,6,7,8};
    /* Enabled free-vector running timers are preserved refused candidates; even
     * the resource Add/Rem pair would erase their originally enabled mask. */
    enabled[0]=0x1f;enabled[1]=0x1d;pending[0]=0x1f;pending[1]=0x1d;
    memcpy(before,host_hardware,sizeof(before));
    assert(!pt_diagnostic_cia_acquire(&t,handler,&data));
    assert(!adds && !removes && !changes && !writes && !memcmp(before,host_hardware,sizeof(before)));
    assert(pending[0]==0x1f && pending[1]==0x1d && enabled[0]==0x1f && enabled[1]==0x1d);
    assert(!vectors[0][0] && !vectors[0][1] && !vectors[1][0] && !vectors[1][1]);
    assert(pt_diagnostic_cia_close(&t) && !depth);++cases;
    /* A stopped timer with a foreign vector still refuses before mask/control
     * changes; running candidates are skipped without querying their vectors. */
    reset();t=(struct pt_diagnostic_cia){0};
    host_hardware[0]=(struct CIA){0xc1,0x80,1,2,3,4};
    host_hardware[1]=(struct CIA){0xc1,0x81,5,6,7,8};
    enabled[0]=0x1f;enabled[1]=0x1d;pending[0]=0x1f;pending[1]=0x1d;vectors[0][1]=&foreign;
    memcpy(before,host_hardware,sizeof(before));
    assert(!pt_diagnostic_cia_acquire(&t,handler,&data));
    assert(adds==1 && !removes && !changes && !writes && !memcmp(before,host_hardware,sizeof(before)));
    assert(pending[0]==0x1f && pending[1]==0x1d && enabled[0]==0x1f && enabled[1]==0x1d);
    assert(!vectors[0][0] && vectors[0][1]==&foreign && !vectors[1][0] && !vectors[1][1]);
    assert(pt_diagnostic_cia_close(&t) && !depth);++cases;
    /* Actual sample after setup; bad clocks/expired targets never write latches
     * or start hardware, and retain the vector with its interrupt masked. */
    {
        struct clock_input c={100,709379,0,0};uint64_t observed=99;unsigned count=99;
        reset();t=(struct pt_diagnostic_cia){0};assert(pt_diagnostic_cia_acquire(&t,handler,&data));
        host_hardware[0].ciatalo=0x56;host_hardware[0].ciatahi=0x78;
        assert(!pt_diagnostic_cia_arm_at(&t,read_clock,&c,100,709379,&observed,&count));
        assert(observed==99 && count==99 && host_hardware[0].ciatalo==0x56 && host_hardware[0].ciatahi==0x78);
        assert(enabled[0]==0x1c && t.held && !removes);
        c.fail=1;assert(!pt_diagnostic_cia_arm_at(&t,read_clock,&c,200,709379,&observed,&count));c.fail=0;
        c.frequency=715909;assert(!pt_diagnostic_cia_arm_at(&t,read_clock,&c,200,709379,&observed,&count));c.frequency=709379;
        assert(!pt_diagnostic_cia_arm_at(&t,read_clock,&c,65636,709379,&observed,&count));
        assert(observed==99 && count==99 && enabled[0]==0x1c && !depth);
        c.ticks=UINT64_MAX-65535;
        assert(pt_diagnostic_cia_arm_at(&t,read_clock,&c,UINT64_MAX,709379,&observed,&count));
        assert(observed==c.ticks && count==65535 && host_hardware[0].ciatalo==255 && host_hardware[0].ciatahi==255);
        assert(host_hardware[0].ciacra==0x19 && pending[0]==0x1c && enabled[0]==0x1d);
        assert(pt_diagnostic_cia_close(&t) && !depth);++cases;
    }
    assert(!depth);printf("CIA OWNER HOST PASS: %u busy/vector/register/task/copy/retained-stop cases\n",cases);return 0;
}
