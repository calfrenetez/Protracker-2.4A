/* Forced host-only SDK and IRQ-layout model. No native ABI or placement proof.
 * Production entry, allocator, adapter, queue and ledger C compile separately.
 */
#ifndef PT_NATIVE_ENTRY_HOST_STUBS_H
#define PT_NATIVE_ENTRY_HOST_STUBS_H
#include <stdint.h>
#include <stddef.h>
#include <setjmp.h>
#define PT_PRIVATE_CIA_ADAPTER_STUB 1
#define PT_PRIVATE_NATIVE_RAM_IRQ_LAYOUT_H 1
#include "../src/native/readers_ram/native_ram_port.h"
typedef uint8_t UBYTE; typedef int8_t BYTE;
typedef uint16_t UWORD; typedef int16_t WORD;
typedef uint32_t ULONG; typedef int32_t LONG;
typedef void *APTR; typedef intptr_t BPTR;
struct Node {UBYTE ln_Type; BYTE ln_Pri; char *ln_Name;};
struct Library {uint16_t lib_Version;};
struct ExecBase {struct Library LibNode;};
struct Device {struct Library dd_Library;};
struct Task {struct Node tc_Node; ULONG tc_SigAlloc; void *tc_SPLower,*tc_SPUpper;};
struct Process {struct Task pr_Task;};
struct MsgPort {UBYTE mp_SigBit; struct Task *mp_SigTask;};
struct IORequest {struct Device *io_Device; WORD io_Command; UBYTE io_Flags; BYTE io_Error;};
struct entry_model_timeval {ULONG tv_secs,tv_micro;};
struct timerequest {struct IORequest tr_node; struct entry_model_timeval tr_time;};
struct EClockVal {ULONG ev_hi,ev_lo;};
struct Interrupt {struct Node is_Node; void (*is_Code)(void); void *is_Data;};
struct CIA {volatile UBYTE ciacra,ciacrb,ciatalo,ciatahi,ciatblo,ciatbhi;};
struct pt_private_ram_eclock {uint32_t hi,lo;};
struct pt_private_ram_irq {
    void *timer,*task; uint32_t signal; volatile uint32_t armed,calls;
    struct pt_private_ram_port *port; void *reserved;
    volatile struct pt_private_ram_eclock before; volatile uint32_t before_frequency;
    volatile struct pt_private_ram_eclock after; volatile uint32_t after_frequency;
    volatile int32_t dispatch_result;
};
#define NT_PROCESS 13U
#define NT_INTERRUPT 2U
#define MEMF_PUBLIC 1U
#define MEMF_CHIP 2U
#define MEMF_FAST 4U
#define MEMF_TOTAL 0x80000U
#define SIGBREAKF_CTRL_C (UINT32_C(1)<<12)
#define TIMERNAME "timer.device"
#define UNIT_ECLOCK 4U
#define UNIT_WAITECLOCK 5U
#define TR_ADDREQUEST 9U
#define IOERR_ABORTED (-2)
extern struct ExecBase *SysBase;
struct Task *FindTask(void *);
ULONG AvailMem(ULONG); APTR AllocMem(ULONG,ULONG); void FreeMem(APTR,ULONG); ULONG TypeOfMem(APTR);
struct MsgPort *CreateMsgPort(void); void DeleteMsgPort(struct MsgPort *);
APTR CreateIORequest(struct MsgPort *,ULONG); void DeleteIORequest(struct IORequest *);
LONG OpenDevice(const char *,ULONG,struct IORequest *,ULONG); void CloseDevice(struct IORequest *);
struct IORequest *CheckIO(struct IORequest *); LONG WaitIO(struct IORequest *);
void AbortIO(struct IORequest *); void SendIO(struct IORequest *);
LONG AllocSignal(LONG); void FreeSignal(LONG); ULONG SetSignal(ULONG,ULONG);
ULONG Wait(ULONG); void Signal(struct Task *,ULONG); void Delay(ULONG);
BPTR Output(void); LONG Write(BPTR,const void *,LONG);
void Disable(void); void Enable(void);
void *OpenResource(const char *);
struct Interrupt *AddICRVector(struct Library *,WORD,struct Interrupt *);
void RemICRVector(struct Library *,WORD,struct Interrupt *);
WORD AbleICR(struct Library *,WORD); WORD SetICR(struct Library *,WORD);
ULONG entry_model_read_eclock(struct Device *,struct EClockVal *);
volatile struct CIA *entry_model_hardware(unsigned);
void entry_model_control_write(volatile UBYTE *,unsigned);
#define ReadEClock(value) entry_model_read_eclock(TimerBase,(value))
#define PT_PRIVATE_CIA_HARDWARE(chip) entry_model_hardware(chip)
#define PT_PRIVATE_CIA_CONTROL_WRITE(pointer,value) entry_model_control_write((pointer),(value))
int pt_private_native_ram_dispatch(struct pt_private_ram_irq *);
void pt_private_native_ram_irq(void);

/* The model is separate from all production/source allocations. Static bounded
 * records describe OS acquisitions; no private core owner/receipt is forged.
 */
enum entry_model_mode {
    EM_PAL,EM_NTSC,EM_ALLOC_NULL,EM_PORT_NULL,EM_REQUEST_NULL,EM_DEVICE_ERROR,
    EM_SIGNAL_NULL,EM_NO_FAST,EM_OLD_DEVICE,EM_PORT_WRONG_TASK,EM_PORT_RANGE,
    EM_PORT_ORIGINAL,EM_PORT_DUPLICATE,EM_SIGNAL_DUPLICATE,EM_SIGNAL_UNALLOCATED,
    EM_DEVICE_NO_IDENTITY,EM_TASK_PORT,EM_TASK_WAITIO,EM_ALIAS_LIVE,EM_BOOTSTRAP_TYPE,
    EM_FOREIGN_CHECKIO,EM_ABORT_PENDING,EM_WAIT_ERROR,EM_ALREADY_COMPLETE,
    EM_DELETE_SIGNAL_STICKY,EM_CIA_BUSY,EM_CIA_ACQUIRE_UNCERTAIN,EM_CIA_ARM_UNCERTAIN,
    EM_CIA_CLOSE_UNCERTAIN,EM_LATE_PREPARE,EM_FREQUENCY,EM_WINDOW_SKIP,EM_CTRL_C,EM_TERMINATION_ONLY
};
enum entry_model_event {
    E_ALLOC,E_FREE,E_PORT,E_DELETE_PORT,E_REQUEST,E_DELETE_REQUEST,E_OPEN,E_CLOSE,
    E_CHECK,E_WAITIO,E_ABORT,E_SEND,E_ALLOC_SIGNAL,E_FREE_SIGNAL,E_SET_SIGNAL,
    E_WAIT,E_SIGNAL,E_DELAY,E_DISABLE,E_ENABLE,E_RESOURCE,E_ADD,E_REMOVE,
    E_MASK,E_PENDING,E_CONTROL,E_CLOCK,E_HOLD
};
struct entry_model_trace {unsigned event,index; uintptr_t pointer; uint64_t value;};
struct entry_model_block {void *pointer; size_t bytes; unsigned live;};
struct entry_model_os_request {struct timerequest value; unsigned live,opened,pending,complete,waited,unit;};
struct entry_model_resource {struct Library library; unsigned mask,pending; struct Interrupt *vector[2];};
struct entry_model {
    enum entry_model_mode mode; unsigned ordinal;
    struct ExecBase exec; struct Device timer; struct Process process,other;
    struct Task *task; struct MsgPort port[2]; unsigned port_live[2];
    struct entry_model_os_request request[2];
    struct CIA hardware[2]; struct entry_model_resource resource[2]; struct Interrupt foreign[2][2];
    struct entry_model_block block[32];
    struct entry_model_trace trace[4096]; unsigned trace_count;
    ULONG original_signals,received; uint64_t ticks,first,last; uint32_t frequency;
    unsigned alloc_calls,free_calls,port_calls,request_calls,open_calls,close_calls;
    unsigned abort_calls,waitio_calls,check_calls,delay_one,disable_depth,maximum_depth;
    unsigned adds,removes,signals,waits,irq_mode,source_exposed,hold,hold_trace;
    char output[8192]; size_t output_bytes;
    jmp_buf sink;
};
extern struct entry_model entry_model;
void entry_model_start(enum entry_model_mode,unsigned,const void *);
void entry_model_assert_result(int);
#endif
