#include "../src/core/routed_activation.h"
#include "../src/native/paula_memory.h"
#include "../src/native/paula_reservation.h"
#include <proto/dos.h>
#include <stdio.h>

/* Private emulator diagnostic. The allocation and live lock cover every
 * audio register used here. All sample data and mixer volumes are zero. */
enum {
    ALL = 15, FIRST_STOP = 5, REMAINING = 10, CHIP_BYTES = 128,
    DMA_READ = 0x002, DMA_SET_CLEAR = 0x096, DMA_MASTER = 0x0200,
    INT_ENABLE_READ = 0x01c, INT_REQUEST_READ = 0x01e,
    INT_ENABLE_WRITE = 0x09a, INT_REQUEST_WRITE = 0x09c, AUDIO_INTERRUPTS = 0x0780,
    ADK_CONTROL_READ = 0x010, SET_BITS = 0x8000
};

static volatile UWORD *custom_word(unsigned reg)
{
    return (volatile UWORD *)(uintptr_t)(0xdff000UL + reg);
}

static int owns_channels(const struct pt_native_paula_reservation *owner)
{
    return owner->opened && owner->phase == 3 && owner->locked && owner->mask == ALL &&
        owner->key && !owner->failed && !owner->closing && !owner->pending &&
        owner->lock->ioa_AllocKey == owner->key && (uintptr_t)owner->lock->ioa_Request.io_Unit == ALL &&
        !CheckIO((struct IORequest *)owner->lock);
}

struct trace_row {
    unsigned stage;
    const char *point;
    uint16_t dma, request, enable, adk;
    unsigned owned, write_count, last_register, last_word;
};

static struct trace_row trace[64];
static unsigned observations, stage;

static struct trace_row snapshot(const struct pt_native_paula_reservation *owner, const char *point,
    unsigned count, const struct pt_paula_dma_write *last)
{
    const struct trace_row row = {
        stage, point, *custom_word(DMA_READ), *custom_word(INT_REQUEST_READ),
        *custom_word(INT_ENABLE_READ), *custom_word(ADK_CONTROL_READ),
        owns_channels(owner), count, last ? last->offset : 0, last ? last->value : 0
    };
    if (observations < sizeof(trace) / sizeof(trace[0]))
        trace[observations++] = row;
    return row;
}

static void report_observations(void)
{
    for (unsigned i = 0; i < observations; ++i) {
        const struct trace_row *row = trace + i;
        printf("Paula trace %02u stage=%u point=%s dma=%04x req=%04x ena=%04x adk=%04x "
            "owned=%u writes=%u reg=%04x value=%04x\n",
            i, row->stage, row->point, (unsigned)row->dma, (unsigned)row->request,
            (unsigned)row->enable, (unsigned)row->adk, row->owned, row->write_count,
            row->last_register, row->last_word);
    }
}

static int apply(const struct pt_native_paula_reservation *owner,
    const struct pt_paula_dma_write *words, unsigned count, unsigned expected_dma)
{
    int accepted = 0;

    Disable();
    snapshot(owner, "before-registers", 0, NULL);
    if (owns_channels(owner)) {
        for (unsigned i = 0; i < count; ++i)
            *custom_word(words[i].offset) = words[i].value;
        accepted = (*custom_word(DMA_READ) & (DMA_MASTER | ALL)) == (DMA_MASTER | expected_dma);
        snapshot(owner, "after-registers", count, count ? words + count - 1 : NULL);
    }
    Enable();
    return accepted;
}

static int observe_new_fetches(const struct pt_native_paula_reservation *owner, unsigned channels)
{
    /* The first wait lets a previously buffered fetch settle. A fresh pass
     * then checks the request edges with only this owner's IRQ bits masked. */
    Delay(1);
    Disable();
    if (!owns_channels(owner)) {
        Enable();
        return 0;
    }
    snapshot(owner, "before-request-clear", 0, NULL);
    *custom_word(INT_REQUEST_WRITE) = AUDIO_INTERRUPTS;
    const struct pt_paula_dma_write clear = {INT_REQUEST_WRITE, AUDIO_INTERRUPTS};
    snapshot(owner, "after-request-clear", 1, &clear);
    Enable();
    Delay(2);
    Disable();
    const struct trace_row observed = snapshot(owner, "after-request-wait", 0, NULL);
    int valid = observed.owned && (observed.dma & ALL) == channels &&
        (observed.request & AUDIO_INTERRUPTS) == (channels << 7);
    Enable();
    return valid;
}

static int prepare(struct pt_routed_activation_program *input, const void *chip, unsigned slots,
    unsigned active, enum pt_mixed_readers_kind kind, struct pt_paula_dma_program *program)
{
    input->audio_count = 0;
    input->paula_active = active;
    for (unsigned slot = 0; slot < 4; ++slot) {
        if (!(slots & (1U << slot)))
            continue;
        struct pt_routed_audio_program *action = input->audio + input->audio_count++;
        *action = (struct pt_routed_audio_program){.route = PT_MIXED_READERS_PAULA, .slot = slot, .kind = kind};
        if (kind == PT_MIXED_READERS_TRIGGER) {
            action->output.paula.location = (uintptr_t)chip + slot * 32;
            action->output.paula.words = 16;
            action->output.paula.period = 440;
        } else if (kind == PT_MIXED_READERS_CONTROL) {
            action->output.paula.period = 520;
        }
    }
    return pt_routed_paula_dma_prepare(input, ALL, program);
}

static int reset_owned_channels(struct pt_native_paula_reservation *owner)
{
    if (!owns_channels(owner))
        return 0;
    owner->command->ioa_AllocKey = owner->key;
    owner->command->ioa_Request.io_Unit = (struct Unit *)(uintptr_t)ALL;
    pt_native_paula_submit(owner, CMD_RESET);
    for (unsigned i = 0; i < 50; ++i) {
        if (pt_native_paula_command_done(owner))
            return owns_channels(owner) && !owner->command->ioa_Request.io_Error &&
                (uintptr_t)owner->command->ioa_Request.io_Unit == ALL && owner->command->ioa_AllocKey == owner->key;
        Delay(1);
    }
    return 0;
}

static int wait_for_release(struct pt_native_paula_reservation *owner)
{
    for (unsigned i = 0; i < 50; ++i) {
        if (pt_native_paula_reservation_close(owner))
            return !owner->port && !owner->lock && !owner->command && !owner->opened;
        Delay(1);
    }
    return 0;
}

static int hold(void) __attribute__((noreturn));
static int hold(void)
{
    report_observations();
    puts("Paula trace HOLD: original device or Chip allocation retained");
    fflush(stdout);
    for (;;)
        Delay(50);
}

int main(void)
{
    struct pt_native_paula_reservation owner = {0};
    struct pt_routed_activation_program input = {0};
    struct pt_paula_dma_program out;
    void *chip = NULL;
    int rc = 20, masked = 0;
    stage = 1;
    unsigned original_enable = 0;

    if (!pt_native_paula_reservation_open(&owner))
        goto finish;
    for (unsigned i = 0; i < 50; ++i) {
        enum pt_paula_reservation_result ready = pt_native_paula_reservation_advance(&owner);
        if (ready != PT_PAULA_RESERVATION_PENDING)
            break;
        Delay(1);
    }
    if (!owns_channels(&owner))
        goto finish;

    stage = 2;
    chip = AllocMem(CHIP_BYTES, MEMF_CHIP | MEMF_PUBLIC | MEMF_CLEAR);
    if (!chip || !(TypeOfMem(chip) & MEMF_CHIP) ||
        !(TypeOfMem((UBYTE *)chip + CHIP_BYTES - 1) & MEMF_CHIP) ||
        !prepare(&input, chip, ALL, 0, PT_MIXED_READERS_TRIGGER, &out))
        goto finish;

    Disable();
    snapshot(&owner, "initial-owned-state", 0, NULL);
    if (owns_channels(&owner) && (*custom_word(DMA_READ) & (DMA_MASTER | ALL)) == DMA_MASTER &&
        !(*custom_word(INT_REQUEST_READ) & AUDIO_INTERRUPTS) && !(*custom_word(ADK_CONTROL_READ) & 0xff)) {
        original_enable = *custom_word(INT_ENABLE_READ) & AUDIO_INTERRUPTS;
        *custom_word(INT_ENABLE_WRITE) = AUDIO_INTERRUPTS;
        const struct pt_paula_dma_write clear = {INT_ENABLE_WRITE, AUDIO_INTERRUPTS};
        snapshot(&owner, "after-interrupt-mask", 1, &clear);
        masked = 1;
    }
    Enable();
    if (!masked || !apply(&owner, out.preload, out.preload_count, 0) || !observe_new_fetches(&owner, 0))
        goto finish;

    stage = 3;
    if (out.frame_count != 1 || !apply(&owner, out.frame, out.frame_count, ALL) ||
        !observe_new_fetches(&owner, ALL))
        goto finish;

    stage = 4;
    if (!prepare(&input, chip, ALL, ALL, PT_MIXED_READERS_CONTROL, &out) || out.preload_count ||
        !apply(&owner, out.frame, out.frame_count, ALL) || !observe_new_fetches(&owner, ALL))
        goto finish;

    stage = 5;
    if (!prepare(&input, chip, FIRST_STOP, ALL, PT_MIXED_READERS_STOP, &out) || out.frame_count != 1 ||
        !apply(&owner, out.frame, out.frame_count, REMAINING) || !observe_new_fetches(&owner, REMAINING))
        goto finish;

    stage = 6;
    if (!prepare(&input, chip, REMAINING, REMAINING, PT_MIXED_READERS_STOP, &out) || out.frame_count != 1 ||
        !apply(&owner, out.frame, out.frame_count, 0) || !observe_new_fetches(&owner, 0))
        goto finish;
    rc = 0;

finish:;
    const unsigned outcome_stage = stage;
    stage = 7;
    if (masked) {
        const struct pt_paula_dma_write clear = {DMA_SET_CLEAR, ALL};
        /* After a failure only the still-original owner can clear its DMA.
         * A positive final Stop already issued and must not be replayed. */
        if ((rc && !apply(&owner, &clear, 1, 0)) || !observe_new_fetches(&owner, 0) ||
            !reset_owned_channels(&owner))
            return hold();
        Disable();
        if (!owns_channels(&owner) || (*custom_word(DMA_READ) & ALL) ||
            (*custom_word(INT_REQUEST_READ) & AUDIO_INTERRUPTS)) {
            Enable();
            return hold();
        }
        *custom_word(INT_ENABLE_WRITE) = AUDIO_INTERRUPTS;
        if (original_enable)
            *custom_word(INT_ENABLE_WRITE) = SET_BITS | original_enable;
        snapshot(&owner, "interrupt-mask-restored", 0, NULL);
        int restored = (*custom_word(INT_ENABLE_READ) & AUDIO_INTERRUPTS) == original_enable;
        Enable();
        if (!restored)
            return hold();
    }
    if (!wait_for_release(&owner))
        return hold();
    if (chip)
        FreeMem(chip, CHIP_BYTES);
    report_observations();
    printf("Paula trace finished stage=%u rc=%d owned-closed=1\n", outcome_stage, rc);
    if (rc)
        return rc;
    return 0;
}
