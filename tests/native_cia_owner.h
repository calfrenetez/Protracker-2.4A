#ifndef PT_DIAGNOSTIC_CIA_OWNER_H
#define PT_DIAGNOSTIC_CIA_OWNER_H
#include <stdint.h>
/* Diagnostic only: no frontend binding, audio commands or Paula MMIO.
 * Single CPU, original task, zero-init/noncopyable. Borrowed handler/data must
 * remain alive until successful close. Never replace an occupied vector.
 * Only the acquired timer's control/count registers and ICR bit are changed.
 * Free timer count latches are not restored; saved stopped control is restored.
 * All foreign timer/control/ICR bits remain untouched. */
#ifndef PT_CIA_OWNER_STUB
#include <exec/interrupts.h>
#include <exec/tasks.h>
#include <hardware/cia.h>
#include <proto/exec.h>
#define CIA_BASE_NAME cia_resource
#include <proto/cia.h>
#endif
#ifndef PT_CIA_HARDWARE
#define PT_CIA_HARDWARE(chip) ((volatile struct CIA *)(chip?0xbfe001UL:0xbfd000UL))
#endif
#ifndef PT_CIA_CONTROL_WRITE
#define PT_CIA_CONTROL_WRITE(pointer,value) (*(pointer)=(value))
#endif
struct pt_diagnostic_cia {
    struct pt_diagnostic_cia *self;
    struct Task *task;
    struct Library *resource;
    struct Interrupt server;
    void *data;void (*code)(void);
    volatile UBYTE *control,*low,*high;
    UBYTE saved_control;
    unsigned chip,bit,held;
};
static int pt_diagnostic_cia_current(const struct pt_diagnostic_cia *t)
{
    return t && t->held && t->self==t && t->task==FindTask(NULL) &&
        t->resource && t->control && t->low && t->high &&
        t->server.is_Data==t->data && t->server.is_Code==t->code;
}
static int pt_diagnostic_cia_acquire(struct pt_diagnostic_cia *t,void (*code)(void),void *data)
{
    unsigned chip,bit;struct Library *cia_resource;volatile struct CIA *hardware;
    if(!t || !code || !data || t->self || t->held || t->resource)return 0;
    t->task=FindTask(NULL);if(!t->task)return 0;
    t->server.is_Node.ln_Type=NT_INTERRUPT;t->server.is_Node.ln_Pri=0;
    t->server.is_Node.ln_Name="PT timer-only diagnostic";
    t->server.is_Code=code;t->server.is_Data=data;t->code=code;t->data=data;
    /* Handler is initialized but unarmed. Brief exclusion also prevents an
     * inherited pending bit invoking it before the acquired timer is quiet. */
    Disable();
    for(chip=0;chip<2;++chip) {
        cia_resource=(struct Library *)OpenResource(chip?"ciaa.resource":"ciab.resource");
        if(!cia_resource)continue;
        for(bit=0;bit<2;++bit) {
            if(AddICRVector(cia_resource,bit,&t->server))continue;
            AbleICR(cia_resource,(WORD)(1U<<bit));
            hardware=PT_CIA_HARDWARE(chip);
            t->control=bit?&hardware->ciacrb:&hardware->ciacra;
            t->saved_control=*t->control;
            /* A running free-vector timer is not assumed disposable. Release
             * our vector without writing ANY hardware register, then try next. */
            if(t->saved_control&1U) {
                RemICRVector(cia_resource,bit,&t->server);t->control=NULL;continue;
            }
            SetICR(cia_resource,(WORD)(1U<<bit));
            t->resource=cia_resource;t->chip=chip;t->bit=bit;
            t->low=bit?&hardware->ciatblo:&hardware->ciatalo;
            t->high=bit?&hardware->ciatbhi:&hardware->ciatahi;
            t->self=t;t->held=1;Enable();return 1;
        }
    }
    Enable();t->task=NULL;t->control=NULL;return 0;
}
/* Caller publishes handler state and clears ONLY its own signal under the same
 * brief exclusion before arm. Relative count is a hardware countdown, not an
 * absolute-start guarantee. No finished/pending IO, allocation or task wait. */
static inline int pt_diagnostic_cia_arm(struct pt_diagnostic_cia *t,unsigned count)
{
    struct Library *cia_resource;
    if(!pt_diagnostic_cia_current(t) || !count || count>65535)return 0;
    cia_resource=t->resource;Disable();
    AbleICR(cia_resource,(WORD)(1U<<t->bit));
    PT_CIA_CONTROL_WRITE(t->control,t->saved_control&~1U);
    if(*t->control&1U){Enable();return 0;}
    SetICR(cia_resource,(WORD)(1U<<t->bit));
    *t->low=(UBYTE)count;*t->high=(UBYTE)(count>>8);
    AbleICR(cia_resource,(WORD)(0x80U|(1U<<t->bit)));
    /* One-shot, force load, EClock input, no timer-driven peripheral output.
     * Retain unrelated serial/TOD bits from original stopped control. */
    PT_CIA_CONTROL_WRITE(t->control,(t->saved_control&(t->bit?0x80U:0xc0U))|0x19U);
    Enable();return 1;
}
/* Separate absolute-arm experiment. Stop/mask/pending setup precedes the
 * fresh actual clock sample; never subtract a calibrated programming offset.
 * read must be a bounded IRQ-safe clock reader, with no owner mutation. Caller
 * publishes handler state under exclusion just as for relative arm. A failed
 * read/deadline leaves the owned timer stopped and masked, retaining its vector.
 * This reduces setup bias; it does not promise exact hardware activation. */
static inline int pt_diagnostic_cia_arm_at(struct pt_diagnostic_cia *t,
    int (*read)(void *,uint64_t *,uint32_t *),void *context,
    uint64_t deadline,uint32_t expected_frequency,uint64_t *observed,unsigned *count_out)
{
    struct Library *cia_resource;uint64_t now;uint32_t frequency;unsigned count;
    if(!pt_diagnostic_cia_current(t) || !read || !context || !expected_frequency ||
       !observed || !count_out)return 0;
    cia_resource=t->resource;Disable();
    AbleICR(cia_resource,(WORD)(1U<<t->bit));
    PT_CIA_CONTROL_WRITE(t->control,t->saved_control&~1U);
    if(*t->control&1U){Enable();return 0;}
    SetICR(cia_resource,(WORD)(1U<<t->bit));
    /* Stopped timer/pending clear: enable setup cannot fire this timer. */
    AbleICR(cia_resource,(WORD)(0x80U|(1U<<t->bit)));
    if(!read(context,&now,&frequency) || frequency!=expected_frequency ||
       now>=deadline || deadline-now>65535) {
        AbleICR(cia_resource,(WORD)(1U<<t->bit));Enable();return 0;
    }
    count=(unsigned)(deadline-now);
    *t->low=(UBYTE)count;*t->high=(UBYTE)(count>>8);
    PT_CIA_CONTROL_WRITE(t->control,(t->saved_control&(t->bit?0x80U:0xc0U))|0x19U);
    *observed=now;*count_out=count;Enable();return 1;
}
static int pt_diagnostic_cia_close(struct pt_diagnostic_cia *t)
{
    struct Library *cia_resource;
    if(!t)return 0;
    if(!t->held)return !t->self && !t->resource;
    if(!pt_diagnostic_cia_current(t))return 0;
    cia_resource=t->resource;Disable();
    AbleICR(cia_resource,(WORD)(1U<<t->bit));
    PT_CIA_CONTROL_WRITE(t->control,t->saved_control&~1U);
    if(*t->control&1U){Enable();return 0;} /* Retain live owner on uncertainty. */
    SetICR(cia_resource,(WORD)(1U<<t->bit));
    RemICRVector(cia_resource,t->bit,&t->server);
    t->held=0;t->resource=NULL;t->self=NULL;t->task=NULL;
    t->control=t->low=t->high=NULL;Enable();return 1;
}
#endif
