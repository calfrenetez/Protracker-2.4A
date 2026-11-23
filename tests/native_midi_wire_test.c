#include "../src/core/countdown_window.h"
#include "../src/core/elapsed_clock.h"
#include "../src/core/routed_midi_wire.h"
#include "native_cia_alarm_lifecycle.h"
#include <hardware/intbits.h>
#include <proto/misc.h>
#include <resources/misc.h>
#include <stdio.h>
#include <string.h>

enum {
    SERIAL_STATUS = 0x018, SERIAL_DATA = 0x030, SERIAL_PERIOD = 0x032,
    INT_REQUEST = 0x01e, INT_ENABLE = 0x01c, INT_WRITE = 0x09c, ADK_READ = 0x010,
    SERIAL_REQUESTS = INTF_TBE | INTF_RBF, BUFFER_EMPTY = 0x2000, TRANSMIT_IDLE = 0x3000,
    RX_FULL = 0x4000, RX_OVERRUN = 0x8000, RECEIVE_VALID = 0x0100,
    LEAD = 128, EPOCH = 8192, RATE = 48000
};

struct Library *MiscBase;
extern void pt_cia_local_serial_midi_irq(void);

struct serial_entry {
    uint16_t first, excluded, attempts, marker;
    uint8_t before[3], after[3];
};
_Static_assert(offsetof(struct serial_entry, marker) == 6 && offsetof(struct serial_entry, before) == 8 &&
    offsetof(struct serial_entry, after) == 11, "Serial IRQ observation layout");

struct serial_case {
    struct pt_countdown_window window;
    struct pt_routed_midi_wire message;
    struct pt_cia_clock_pair arm;
    struct serial_entry raw;
    unsigned frame, delay, status, calls, received, idle;
    uint8_t bytes[3];
};

static volatile UWORD *reg(unsigned offset)
{
    return (volatile UWORD *)(uintptr_t)(0xdff000UL + offset);
}

static int quiet(void)
{
    return (*reg(SERIAL_STATUS) & 0xf000) == TRANSMIT_IDLE;
}

static unsigned sample_count(const struct pt_diagnostic_cia *clock)
{
    const unsigned high = *clock->high;
    const unsigned low = *clock->low;
    return high == *clock->high ? high * 256 + low : 0;
}

/* Wait for this fixture's own receive byte while the original countdown runs.
 * Only the IRQ issues the first transmit word. The task drains the remaining
 * two bytes in the same message before the port can admit another frame. */
static int receive_byte(const struct pt_diagnostic_cia *clock, unsigned limit, uint8_t *out)
{
    for (unsigned i = 0; i < 32768; ++i) {
        unsigned count = sample_count(clock);
        if ((count && count < limit) || !(*clock->control & 1U))
            return 0;
        UWORD status = *reg(SERIAL_STATUS);
        if (status & RX_OVERRUN)
            return 0;
        if (status & RX_FULL) {
            *reg(INT_WRITE) = INTF_RBF;
            *out = (uint8_t)status;
            return (status & RECEIVE_VALID) != 0;
        }
    }
    return 0;
}

static int prepare(struct serial_case *row, const struct pt_elapsed_clock *grid,
    uint16_t anchor, unsigned frame, unsigned index, unsigned status, int stop)
{
    struct pt_routed_frame_program program = {.key = {1, 1, frame, index + 1},
        .stop = stop, .midi_count = 1, .midi = {{0, {status, 60, stop ? 0 : 90}}}};
    if (pt_elapsed_clock_deadline(grid, frame, &program.first) != PT_ELAPSED_OK ||
        pt_elapsed_clock_deadline(grid, frame + 1, &program.last) != PT_ELAPSED_OK ||
        !pt_countdown_window_prepare(0, anchor, program.first, program.last, &row->window) ||
        !pt_routed_midi_wire_prepare(&program, 0, &row->message))
        return 0;
    row->frame = frame;
    row->raw.first = row->window.first_count;
    row->raw.excluded = row->window.end_count;
    return 1;
}

static int issue(struct local_alarm *alarm, const struct pt_diagnostic_cia *clock,
    struct serial_case *row, int expired)
{
    struct local_alarm_data *irq = &alarm->data;
    Disable();
    if (!quiet() || !pt_diagnostic_cia_current(clock) || irq->armed || irq->entry || irq->stray ||
        irq->calls != alarm->delivered) {
        Enable();
        return 0;
    }
    pt_cia_direct_clock_pair(clock->high, clock->low, &row->arm);
    const unsigned count = pt_cia_stable_count(row->arm.after);
    const unsigned threshold = expired ? row->raw.excluded - LEAD : row->raw.first + LEAD;
    if (count <= threshold || row->arm.marker != 0x2452) {
        Enable();
        return 0;
    }
    row->delay = count - threshold;
    SetSignal(0, irq->signal);
    irq->entry = &row->raw;
    irq->command = row->message.word[0];
    irq->result = UINT32_MAX;
    irq->armed = 1;
    const int armed = pt_diagnostic_cia_arm(&alarm->timer, row->delay);
    if (!armed)
        irq->armed = 0;
    Enable();
    if (!armed)
        return 0;
    const ULONG wake = Wait(irq->signal | alarm->deadline_signal | SIGBREAKF_CTRL_C);
    Disable();
    row->status = irq->result;
    row->calls = irq->calls;
    if (wake != irq->signal || irq->armed || irq->calls != alarm->delivered + 1 || irq->stray) {
        irq->armed = 0;
        Enable();
        return 0;
    }
    ++alarm->delivered;
    irq->entry = NULL;
    Enable();

    if (expired) {
        row->idle = quiet();
        const unsigned before = pt_cia_stable_count(row->raw.before);
        return row->status == 2 && !row->raw.marker && before && before <= row->raw.excluded &&
            row->idle && !(*reg(INT_REQUEST) & INTF_RBF);
    }
    if (row->status != 1 || row->raw.marker != 0x4752)
        return 0;
    for (unsigned i = 0; i < 3; ++i) {
        if (i) {
            if (!(*reg(SERIAL_STATUS) & BUFFER_EMPTY))
                return 0;
            *reg(SERIAL_DATA) = row->message.word[i];
        }
        if (!receive_byte(clock, row->raw.excluded - 5000, row->bytes + i))
            return 0;
        ++row->received;
        if (row->bytes[i] != (uint8_t)row->message.word[i])
            return 0;
    }
    for (unsigned i = 0; i < 32768; ++i) {
        unsigned remaining = sample_count(clock);
        if ((remaining && remaining < row->raw.excluded - 5000U) || !(*clock->control & 1U))
            break;
        if (quiet()) {
            row->idle = 1;
            break;
        }
    }
    struct pt_countdown_bracket bracket;
    return row->idle && pt_countdown_window_bracket(&row->window, pt_cia_stable_count(row->raw.before),
        pt_cia_stable_count(row->raw.after), &bracket);
}

static void report(const struct serial_case *row, unsigned index)
{
    const struct serial_entry *r = &row->raw;
    const uint8_t *arm = row->arm.after;
    printf("MIDI wire %u frame=%u window=%lu/%lu bound=%u/%u arm=%02x%02x%02x delay=%u "
        "calls=%u status=%u attempts=%u pre=%02x%02x%02x post=%02x%02x%02x mark=%04x "
        "tx=%03x/%03x/%03x rx=%02x%02x%02x received=%u idle=%u\n",
        index, row->frame, (unsigned long)row->window.first, (unsigned long)row->window.end,
        (unsigned)r->first, (unsigned)r->excluded, (unsigned)arm[0], (unsigned)arm[1], (unsigned)arm[2], row->delay,
        row->calls, row->status, (unsigned)r->attempts, (unsigned)r->before[0], (unsigned)r->before[1],
        (unsigned)r->before[2], (unsigned)r->after[0], (unsigned)r->after[1], (unsigned)r->after[2],
        (unsigned)r->marker, (unsigned)row->message.word[0], (unsigned)row->message.word[1],
        (unsigned)row->message.word[2], (unsigned)row->bytes[0], (unsigned)row->bytes[1], (unsigned)row->bytes[2],
        row->received, row->idle);
}

static int hold(void) __attribute__((noreturn));
static int hold(void)
{
    puts("MIDI wire HOLD: original timer or UART retained");
    fflush(stdout);
    for (;;)
        Delay(50);
}

int main(int argc, char **argv)
{
    const char name[] = "PT private serial loopback diagnostic";
    if (argc != 2 || strcmp(argv[1], "internal-loopback"))
        return 20;
    struct pt_native_eclock timebase = {0};
    struct pt_diagnostic_cia clock = {0};
    struct local_alarm alarm = {.signal = -1};
    struct pt_elapsed_clock grid;
    struct pt_cia_clock_pair origin = {0};
    struct serial_case rows[3] = {0};
    volatile ULONG strays = 0;
    uint64_t now;
    uint32_t hz = 0;
    UWORD original_requests = 0;
    unsigned owned = 0, configured = 0, period = 0;
    int rc = 20;

    MiscBase = (struct Library *)OpenResource((CONST_STRPTR)MISCNAME);
    if (!MiscBase || AllocMiscResource(MR_SERIALPORT, (CONST_STRPTR)name))
        goto finish;
    owned = 1;
    original_requests = *reg(INT_REQUEST) & SERIAL_REQUESTS;
    if ((*reg(INT_ENABLE) & SERIAL_REQUESTS) || (*reg(ADK_READ) & 0x0800) || (original_requests & INTF_RBF) ||
        !quiet() || !pt_native_eclock_open(&timebase) || !pt_native_eclock_read(&timebase, &now, &hz) ||
        (hz != 709379 && hz != 715909) || now > UINT64_MAX - hz ||
        !pt_diagnostic_cia_acquire(&clock, pt_cia_direct_clock_stray, (void *)&strays) ||
        !open_alarm(&alarm, &clock, now + hz, pt_cia_local_serial_midi_irq) ||
        pt_elapsed_clock_init(&grid, hz, RATE, EPOCH, 0) != PT_ELAPSED_OK)
        goto finish;

    period = (hz * 5 + 15625) / 31250 - 1;
    *reg(SERIAL_PERIOD) = period;
    configured = 1;
    if (!pt_cia_start_masked_countdown(&clock))
        goto finish;
    pt_cia_direct_clock_pair(clock.high, clock.low, &origin);
    const unsigned count = pt_cia_stable_count(origin.after);
    if (origin.marker != 0x2452 || !count ||
        !prepare(rows, &grid, count, 0, 0, 0x95, 0) ||
        !prepare(rows + 1, &grid, count, 1920, 1, 0x85, 1) ||
        !prepare(rows + 2, &grid, count, 3000, 2, 0x95, 0) ||
        !issue(&alarm, &clock, rows, 0) || !issue(&alarm, &clock, rows + 1, 0) ||
        !issue(&alarm, &clock, rows + 2, 1))
        goto finish;
    rc = 0;

finish:
    if (!close_alarm(&alarm) || !pt_diagnostic_cia_close(&clock))
        return hold();
    pt_native_eclock_close(&timebase);
    if (configured) {
        for (unsigned i = 0; !quiet(); ++i) {
            if (i == 49 || (*reg(INT_ENABLE) & SERIAL_REQUESTS))
                return hold();
            if (*reg(SERIAL_STATUS) & RX_FULL)
                *reg(INT_WRITE) = INTF_RBF;
            Delay(1);
        }
        *reg(INT_WRITE) = SERIAL_REQUESTS;
        if (original_requests)
            *reg(INT_WRITE) = INTF_SETCLR | original_requests;
        if ((*reg(INT_REQUEST) & SERIAL_REQUESTS) != original_requests)
            return hold();
    }
    if (owned)
        FreeMiscResource(MR_SERIALPORT);
    if (strays || alarm.data.stray || alarm.data.calls != 3)
        rc = 20;
    printf("MIDI wire clock hz=%lu timer=%u/%u alarm=%u/%u anchor=%02x%02x%02x epoch=%u period=%u\n",
        (unsigned long)hz, clock.chip, clock.bit, alarm.timer.chip, alarm.timer.bit, (unsigned)origin.after[0],
        (unsigned)origin.after[1], (unsigned)origin.after[2], EPOCH, period);
    for (unsigned i = 0; i < 3; ++i)
        report(rows + i, i);
    printf("MIDI wire finished callbacks=%lu stray=%lu/%lu closed=1 rc=%d\n",
        (unsigned long)alarm.data.calls, (unsigned long)alarm.data.stray, (unsigned long)strays, rc);
    return rc;
}
