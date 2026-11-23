#ifndef PT_TEST_NATIVE_MIDI_LOOPBACK_H
#define PT_TEST_NATIVE_MIDI_LOOPBACK_H

#include "../src/core/routed_midi_wire.h"
#include <hardware/intbits.h>
#include <proto/misc.h>
#include <resources/misc.h>

enum {
    SERIAL_STATUS = 0x018, SERIAL_DATA = 0x030, SERIAL_PERIOD = 0x032,
    INT_REQUEST = 0x01e, INT_ENABLE = 0x01c, INT_WRITE = 0x09c, ADK_READ = 0x010,
    SERIAL_REQUESTS = INTF_TBE | INTF_RBF, BUFFER_EMPTY = 0x2000, TRANSMIT_IDLE = 0x3000,
    RX_FULL = 0x4000, RX_OVERRUN = 0x8000, RECEIVE_VALID = 0x0100
};

struct Library *MiscBase;

struct pt_test_midi_uart {
    unsigned owned, period, restored, status;
    UWORD original_requests, final_requests;
};

struct pt_test_midi_receipt {
    unsigned received, idle;
    uint8_t bytes[3];
};

static volatile UWORD *serial_register(unsigned offset)
{
    return (volatile UWORD *)(uintptr_t)(0xdff000UL + offset);
}

static int serial_quiet(void)
{
    return (*serial_register(SERIAL_STATUS) & 0xf000) == TRANSMIT_IDLE;
}

static int serial_reserve(struct pt_test_midi_uart *uart)
{
    const char name[] = "PT private serial loopback diagnostic";
    MiscBase = (struct Library *)OpenResource((CONST_STRPTR)MISCNAME);
    if (!MiscBase || AllocMiscResource(MR_SERIALPORT, (CONST_STRPTR)name))
        return 0;
    uart->owned = 1;
    uart->original_requests = *serial_register(INT_REQUEST) & SERIAL_REQUESTS;
    return !(*serial_register(INT_ENABLE) & SERIAL_REQUESTS) && !(*serial_register(ADK_READ) & 0x0800) &&
        !(uart->original_requests & INTF_RBF) && serial_quiet();
}

/* Only used in a private guest. SERPER is write-only, so the caller retires
 * the guest and restores its host profile after the diagnostic. */
static void serial_configure(struct pt_test_midi_uart *uart, uint32_t hz)
{
    uart->period = (hz * 5 + 15625) / 31250 - 1;
    *serial_register(SERIAL_PERIOD) = uart->period;
}

static const char *serial_release(struct pt_test_midi_uart *uart)
{
    if (!uart->owned)
        return NULL;
    if (uart->period) {
        for (unsigned i = 0; !serial_quiet(); ++i) {
            if (i == 49 || (*serial_register(INT_ENABLE) & SERIAL_REQUESTS))
                return "drain";
            const UWORD status = *serial_register(SERIAL_STATUS);
            if (status & (RX_FULL | RX_OVERRUN))
                *serial_register(INT_WRITE) = INTF_RBF;
            Delay(1);
        }
        if (*serial_register(INT_ENABLE) & SERIAL_REQUESTS)
            return "requests";
        const UWORD current = *serial_register(INT_REQUEST) & SERIAL_REQUESTS;
        if (current & ~uart->original_requests)
            *serial_register(INT_WRITE) = current & ~uart->original_requests;
        if (uart->original_requests & ~current)
            *serial_register(INT_WRITE) = INTF_SETCLR | (uart->original_requests & ~current);
        for (unsigned i = 0; ; ++i) {
            if (*serial_register(INT_ENABLE) & SERIAL_REQUESTS || !serial_quiet() || i == 5)
                return "requests";
            uart->final_requests = *serial_register(INT_REQUEST) & SERIAL_REQUESTS;
            if (uart->final_requests == uart->original_requests)
                break;
            Delay(1);
        }
        uart->status = *serial_register(SERIAL_STATUS);
        uart->restored = 1;
    }
    FreeMiscResource(MR_SERIALPORT);
    uart->owned = 0;
    return NULL;
}

static unsigned serial_count(const struct pt_diagnostic_cia *clock)
{
    const unsigned high = *clock->high;
    const unsigned low = *clock->low;
    return high == *clock->high ? high * 256 + low : 0;
}

static int receive_byte(const struct pt_diagnostic_cia *clock, unsigned limit, uint8_t *out)
{
    for (unsigned i = 0; i < 32768; ++i) {
        unsigned count = serial_count(clock);
        if ((count && count < limit) || !(*clock->control & 1U))
            return 0;
        UWORD status = *serial_register(SERIAL_STATUS);
        if (status & RX_OVERRUN)
            return 0;
        if (status & RX_FULL) {
            *serial_register(INT_WRITE) = INTF_RBF;
            *out = (uint8_t)status;
            return (status & RECEIVE_VALID) != 0;
        }
    }
    return 0;
}

/* Only the alarm IRQ transmits the first byte. The task must receive
 * each byte before submitting its successor. */
static int serial_receive(const struct pt_diagnostic_cia *clock, unsigned limit,
    const struct pt_routed_midi_wire *message, struct pt_test_midi_receipt *receipt)
{
    for (unsigned i = 0; i < 3; ++i) {
        if (i) {
            if (!(*serial_register(SERIAL_STATUS) & BUFFER_EMPTY))
                return 0;
            *serial_register(SERIAL_DATA) = message->word[i];
        }
        if (!receive_byte(clock, limit, receipt->bytes + i))
            return 0;
        ++receipt->received;
        if (receipt->bytes[i] != (uint8_t)message->word[i])
            return 0;
    }
    for (unsigned i = 0; i < 32768; ++i) {
        unsigned remaining = serial_count(clock);
        if ((remaining && remaining < limit) || !(*clock->control & 1U))
            break;
        if (serial_quiet()) {
            receipt->idle = 1;
            break;
        }
    }
    return receipt->idle;
}
#endif
