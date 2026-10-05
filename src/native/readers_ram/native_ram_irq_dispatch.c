/* RAM-only diagnostic C IRQ path. No raw exception entry/return is used.
 * Portable object checks do not qualify actual IRQ execution.
 * Stack/call-chain/ReadEClock/library-vector residency and WCET UNKNOWN.
 * A small source body does not establish bounded native execution cost.
 */
#include "native_ram_irq_layout.h"
int pt_private_native_ram_dispatch(struct pt_private_ram_irq *irq)
{
    ++irq->calls;
    return pt_private_ram_dispatch(irq->port);
}
