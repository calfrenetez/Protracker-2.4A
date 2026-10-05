/* Opt-in CIA adapter. Portable objects and software models do not qualify
 * native execution, source quiet or timing.
 * Task-side positive query protocol, distinct from diagnostic bool arm API.
 * The caller owns the retained native entry; runtime qualification is separate.
 */
#include "native_cia_ram_adapter.h"

static int supported_frequency(uint32_t frequency)
{return frequency==709379U||frequency==715909U;}
static int current(const struct pt_private_cia_ram_adapter *a)
{
    volatile struct CIA *h;
    if(!a||a->self!=a||!a->vector_owned||!a->task||a->task!=FindTask(NULL)||!a->resource||
       !a->timer||!a->irq||a->chip>=2||a->bit>=2||a->server.is_Data!=a->irq||
       a->server.is_Code!=pt_private_native_ram_irq||a->irq->timer!=a->timer||a->irq->task!=a->task||
       !a->irq->port||SysBase->LibNode.lib_Version<36||a->resource->lib_Version<36||
       a->timer->dd_Library.lib_Version<36||!supported_frequency(a->frequency))return 0;
    h=PT_PRIVATE_CIA_HARDWARE(a->chip);
    return a->control==(a->bit?&h->ciacrb:&h->ciacra)&&
        a->low==(a->bit?&h->ciatblo:&h->ciatalo)&&a->high==(a->bit?&h->ciatbhi:&h->ciatahi);
}
/* Caller is an actual normal original task under Disable, not a fabricated
 * runtime flag. Own bit only; SetICR0/AbleICR0 sample without changing bits.
 * No raw ICR read/write, no clear of shared level2/6/Paula interrupts.
 */
static int stopped_masked_pending_clear(struct pt_private_cia_ram_adapter *a)
{
    struct Library *cia_resource=a->resource;WORD before_mask;unsigned own=1U<<a->bit;
    before_mask=AbleICR(cia_resource,0);
    AbleICR(cia_resource,(WORD)own);
    PT_PRIVATE_CIA_CONTROL_WRITE(a->control,a->saved_control&~1U);
    if(*a->control&1U)return 0;
    SetICR(cia_resource,(WORD)own);
    a->last_mask=AbleICR(cia_resource,0);a->last_pending=SetICR(cia_resource,0);
    if(((unsigned)(UWORD)a->last_mask&own)||((unsigned)(UWORD)a->last_pending&own)||
       (((unsigned)(UWORD)before_mask^(unsigned)(UWORD)a->last_mask)&~own&0x1fU))return 0;
    /* Previous callback returned before normal task resumed; mask/stop/pending
     * observations now exclude future owned entry. Only THEN disable dispatch. */
    a->irq->armed=0;return 1;
}
static int sampled_quiet(struct pt_private_cia_ram_adapter *a)
{
    struct Library *cia_resource=a->resource;unsigned own=1U<<a->bit;
    a->last_mask=AbleICR(cia_resource,0);a->last_pending=SetICR(cia_resource,0);
    return !(*a->control&1U)&&!((unsigned)(UWORD)a->last_mask&own)&&!((unsigned)(UWORD)a->last_pending&own);
}
int pt_private_cia_ram_clock(void *context,uint64_t *ticks,uint32_t *frequency)
{
    struct pt_private_cia_ram_adapter *a=context;struct EClockVal value={0};ULONG actual;
    struct Device *TimerBase;
    if(!a||!ticks||!frequency||!a->timer||!supported_frequency(a->frequency))return 0;
    TimerBase=a->timer;actual=ReadEClock(&value);
    if(actual!=a->frequency||!supported_frequency(actual))return 0;
    *ticks=((uint64_t)value.ev_hi<<32)|value.ev_lo;*frequency=actual;return 1;
}
int pt_private_cia_ram_acquire(struct pt_private_cia_ram_adapter *a,struct Device *timer,
    struct pt_private_ram_irq *irq,uint32_t frequency)
{
    struct Task *task;unsigned chip,bit;struct Library *cia_resource;volatile struct CIA *h;
    struct Device *TimerBase=timer;struct EClockVal value={0};ULONG actual;
    if(!a||!timer||!irq||!irq->port||a->self||a->vector_owned||a->resource||a->close_attempted||
       a->phase!=PT_PRIVATE_CIA_EMPTY||irq->armed||!supported_frequency(frequency)||
       SysBase->LibNode.lib_Version<36||timer->dd_Library.lib_Version<36)return 0;
    task=FindTask(NULL);if(!task)return 0;
    /* Eligibility is observed before any source acquisition. This is not the
     * musical epoch and must never shift the subsequently copied deadline. */
    actual=ReadEClock(&value);if(actual!=frequency||!supported_frequency(actual))return 0;
    a->task=task;a->timer=timer;a->irq=irq;a->frequency=frequency;
    a->exec_version=SysBase->LibNode.lib_Version;a->timer_version=timer->dd_Library.lib_Version;
    irq->timer=timer;irq->task=task;
    a->server.is_Node.ln_Type=NT_INTERRUPT;a->server.is_Node.ln_Pri=0;
    a->server.is_Node.ln_Name="PT RAM ledger CIA draft";
    a->server.is_Code=pt_private_native_ram_irq;a->server.is_Data=irq;
    Disable();
    for(chip=0;chip<2;++chip){
        cia_resource=(struct Library *)OpenResource(chip?"ciaa.resource":"ciab.resource");
        /* Refuse unsupported pending-status semantics BEFORE acquisition. */
        if(!cia_resource||cia_resource->lib_Version<36)continue;
        h=PT_PRIVATE_CIA_HARDWARE(chip);
        for(bit=0;bit<2;++bit){volatile UBYTE *control=bit?&h->ciacrb:&h->ciacra;UBYTE saved=*control;
            if(saved&1U)continue; /* before AddICRVector or mask/ICR writes */
            if(AddICRVector(cia_resource,bit,&a->server))continue; /* foreign vector untouched */
            a->self=a;a->vector_owned=1;a->resource=cia_resource;a->resource_version=cia_resource->lib_Version;
            a->chip=chip;a->bit=bit;a->control=control;a->saved_control=saved;
            a->low=bit?&h->ciatblo:&h->ciatalo;a->high=bit?&h->ciatbhi:&h->ciatahi;
            a->phase=PT_PRIVATE_CIA_STOPPED;
            if(!stopped_masked_pending_clear(a)){a->phase=PT_PRIVATE_CIA_UNCERTAIN;Enable();return -1;}
            Enable();return 1;
        }
    }
    Enable();a->task=NULL;a->timer=NULL;a->irq=NULL;return 0;
}
int pt_private_cia_ram_arm_at(void *context,uint64_t deadline,uint32_t expected_frequency)
{
    struct pt_private_cia_ram_adapter *a=context;struct pt_private_ram_port *p;
    struct pt_private_ram_command *command;struct Library *cia_resource;uint64_t now;uint32_t frequency;
    unsigned count,own;int result=-1;
    if(!current(a)||a->phase==PT_PRIVATE_CIA_UNCERTAIN||a->close_attempted)return -1;
    Disable();
    if(!current(a))goto done;
    p=a->irq->port;
    /* Outer publication is serialized; one exact prepared immutable packet.
     * These fixed port values are copied metadata, not a validated-owner flag. */
    if(p->armed_index<0||p->armed_index>=2||p->adapter.context!=a)goto uncertain;
    command=p->command+p->armed_index;
    if(!command->live||!command->armed||command->finished||command->uncertain||!command->ledger||
       !command->packet.ticket||command->arm_tick!=deadline||command->packet.session!=p->session||
       command->packet.generation!=p->generation||p->frequency!=a->frequency)goto uncertain;
    if(!stopped_masked_pending_clear(a))goto uncertain;
    a->phase=PT_PRIVATE_CIA_STOPPED;a->active_ticket=0;
    if(expected_frequency!=a->frequency||pt_private_cia_ram_clock(a,&now,&frequency)!=1||
       frequency!=expected_frequency||now>=deadline||deadline-now>65535U){result=0;goto done;}
    count=(unsigned)(deadline-now);own=1U<<a->bit;cia_resource=a->resource;
    *a->low=(UBYTE)count;*a->high=(UBYTE)(count>>8);
    a->active_ticket=command->packet.ticket;a->irq->armed=1;
    AbleICR(cia_resource,(WORD)(0x80U|own));
    a->last_mask=AbleICR(cia_resource,0);
    if(!((unsigned)(UWORD)a->last_mask&own))goto uncertain;
    /* Actual relative countdown is not an absolute-start guarantee. */
    PT_PRIVATE_CIA_CONTROL_WRITE(a->control,(a->saved_control&(a->bit?0x80U:0xc0U))|0x19U);
    if(!(*a->control&1U))goto uncertain; /* may already have elapsed; never safe0 */
    a->arm_observed=now;a->arm_frequency=frequency;a->arm_count=count;++a->arms;
    a->phase=PT_PRIVATE_CIA_ARMED;result=1;goto done;
uncertain:a->phase=PT_PRIVATE_CIA_UNCERTAIN;
done:Enable();return result;
}
int pt_private_cia_ram_disarm(struct pt_private_cia_ram_adapter *a,uint64_t ticket)
{
    int result=-1;
    if(!current(a)||!ticket||a->active_ticket!=ticket||a->phase==PT_PRIVATE_CIA_UNCERTAIN||a->close_attempted)return -1;
    Disable();
    if(current(a)&&a->active_ticket==ticket&&stopped_masked_pending_clear(a)){
        ++a->disarms;a->phase=PT_PRIVATE_CIA_STOPPED;a->active_ticket=0;result=1;
    }else a->phase=PT_PRIVATE_CIA_UNCERTAIN;
    Enable();return result;
}
int pt_private_cia_ram_ticket_quiet(void *context,uint64_t ticket,unsigned cancel)
{
    struct pt_private_cia_ram_adapter *a=context;struct pt_private_ram_port *p;int result=-1;
    if(!a||!ticket||a->self!=a||a->task!=FindTask(NULL))return -1;
    if(a->phase==PT_PRIVATE_CIA_CLOSED)return 1; /* prior positive source closure, no new callback */
    if(!current(a)||a->phase==PT_PRIVATE_CIA_UNCERTAIN||a->close_attempted)return -1;
    Disable();
    if(!current(a))goto done;
    ++a->quiet_probes;p=a->irq->port;
    if(a->active_ticket&&a->active_ticket!=ticket){
        /* Do not cancel a replacement source. Under normal-task exclusion,
         * every future dispatch names ONLY the new exact armed packet index;
         * no old callback remains on the single CPU after task resumes. */
        if(p->armed_index<0||p->armed_index>=2||!p->command[p->armed_index].live||
           p->command[p->armed_index].packet.ticket!=a->active_ticket)goto done;
        result=1;goto done;
    }
    if(!cancel&&a->irq->armed){result=0;goto done;}
    if(!stopped_masked_pending_clear(a)){a->phase=PT_PRIVATE_CIA_UNCERTAIN;goto done;}
    a->phase=PT_PRIVATE_CIA_STOPPED;a->active_ticket=0;result=1;
done:Enable();return result;
}
int pt_private_cia_ram_source_close(void *context)
{
    struct pt_private_cia_ram_adapter *a=context;struct Library *cia_resource;int result=-1;
    if(!a||a->self!=a||a->task!=FindTask(NULL))return -1;
    if(a->phase==PT_PRIVATE_CIA_CLOSED)return 1;
    if(!current(a)||a->close_attempted)return -1;
    Disable();
    if(!current(a))goto done;
    a->close_attempted=1; /* one guarded cleanup, including uncertain arm */
    if(!stopped_masked_pending_clear(a))goto uncertain;
    cia_resource=a->resource;a->removal_attempted=1;
    RemICRVector(cia_resource,a->bit,&a->server);
    /* Deallocation is the documented exact-vector API. These post-call
     * hardware/resource samples also require the normal-task exclusion above. */
    if(!sampled_quiet(a))goto uncertain;
    a->vector_owned=0;a->active_ticket=0;a->phase=PT_PRIVATE_CIA_CLOSED;result=1;goto done;
uncertain:a->phase=PT_PRIVATE_CIA_UNCERTAIN;
done:Enable();return result;
}
struct pt_private_ram_adapter pt_private_cia_ram_api(struct pt_private_cia_ram_adapter *a)
{return (struct pt_private_ram_adapter){a,pt_private_cia_ram_clock,pt_private_cia_ram_arm_at,
    pt_private_cia_ram_ticket_quiet,pt_private_cia_ram_source_close};}
