/* RAM diagnostic target layout, derived from the pinned CIA diagnostic.
 * Portable layout checks do not qualify native IRQ execution.
 * Offset typedefs enforce the 32-bit 68k layout during native compilation.
 * The timer/device/task/context lifetimes end only after positive source close.
 */
#ifndef PT_PRIVATE_NATIVE_RAM_IRQ_LAYOUT_H
#define PT_PRIVATE_NATIVE_RAM_IRQ_LAYOUT_H
#include "native_ram_port.h"
#include <stddef.h>
struct pt_private_ram_eclock {uint32_t hi,lo;};
/* Fixed samples of live SP, never an all-instruction high-water mark. The
 * versioned tail does not change a public core ABI or permit IRQ activation.
 * Original bounds and this owned record stay live through positive source
 * close. No paint/read of OS stack storage is performed.
 */
#define PT_PRIVATE_RAM_STACK_VERSION 1U
#define PT_PRIVATE_RAM_STACK_SAMPLE_LIMIT 7U
#define PT_PRIVATE_RAM_STACK_BAD_BOUNDS 1U
#define PT_PRIVATE_RAM_STACK_BAD_SAMPLE 2U
#define PT_PRIVATE_RAM_STACK_BAD_COUNT 4U
#define PT_PRIVATE_RAM_STACK_BAD_PHASE 8U
#define PT_PRIVATE_RAM_STACK_BAD_VERSION 16U
#define PT_PRIVATE_RAM_STACK_ERRORS 31U
#define PT_PRIVATE_RAM_STACK_ENTRY 0x0100U
#define PT_PRIVATE_RAM_STACK_SAVED 0x0200U
#define PT_PRIVATE_RAM_STACK_DISPATCH_BEFORE 0x0400U
#define PT_PRIVATE_RAM_STACK_DISPATCH_AFTER 0x0800U
#define PT_PRIVATE_RAM_STACK_AFTER_CLOCK 0x1000U
#define PT_PRIVATE_RAM_STACK_AFTER_SIGNAL 0x2000U
#define PT_PRIVATE_RAM_STACK_EXIT 0x4000U
#define PT_PRIVATE_RAM_STACK_PHASES 0x7f00U
struct pt_private_ram_irq {
    void *timer,*task;
    uint32_t signal;
    volatile uint32_t armed,calls;
    struct pt_private_ram_port *port;
    void *reserved; /* retain pinned original diagnostic offset layout */
    volatile struct pt_private_ram_eclock before;
    volatile uint32_t before_frequency;
    volatile struct pt_private_ram_eclock after;
    volatile uint32_t after_frequency;
    volatile int32_t dispatch_result;
    volatile uint32_t stack_version;
    uintptr_t system_lower,system_upper;
    volatile uintptr_t entry_sp,sampled_low_sp,exit_sp;
    volatile uint32_t stack_status,stack_samples;
};
#define PT_PRIVATE_OFFSET(field,value) typedef char pt_private_offset_##field[(offsetof(struct pt_private_ram_irq,field)==(value))?1:-1]
PT_PRIVATE_OFFSET(timer,0);PT_PRIVATE_OFFSET(task,4);PT_PRIVATE_OFFSET(signal,8);
PT_PRIVATE_OFFSET(armed,12);PT_PRIVATE_OFFSET(calls,16);PT_PRIVATE_OFFSET(port,20);
PT_PRIVATE_OFFSET(before,28);PT_PRIVATE_OFFSET(before_frequency,36);
PT_PRIVATE_OFFSET(after,40);PT_PRIVATE_OFFSET(after_frequency,48);PT_PRIVATE_OFFSET(dispatch_result,52);
PT_PRIVATE_OFFSET(stack_version,56);PT_PRIVATE_OFFSET(system_lower,60);PT_PRIVATE_OFFSET(system_upper,64);
PT_PRIVATE_OFFSET(entry_sp,68);PT_PRIVATE_OFFSET(sampled_low_sp,72);PT_PRIVATE_OFFSET(exit_sp,76);
PT_PRIVATE_OFFSET(stack_status,80);PT_PRIVATE_OFFSET(stack_samples,84);
typedef char pt_private_irq_stack_size[(sizeof(struct pt_private_ram_irq)==88)?1:-1];
#undef PT_PRIVATE_OFFSET
int pt_private_native_ram_dispatch(struct pt_private_ram_irq *);
void pt_private_native_ram_irq(void);
#endif
