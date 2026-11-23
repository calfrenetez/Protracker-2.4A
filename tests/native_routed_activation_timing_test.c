/* Real E-clock brackets around the ordinary-RAM fixture. The fixture still
 * uses its controlled activation counter and never sends live device output. */
#include <assert.h>
#include <stdio.h>
#include "../src/core/routed_activation.h"
#include "../src/native/eclock.h"

enum benchmark_phase { EMPTY, PUBLISH_START, FIRE_START, PUBLISH_STOP, FIRE_STOP, PHASE_COUNT };
enum { ATTEMPTS = 24, SAMPLE_RATE = 48000 };

struct brackets {
    uint32_t minimum, maximum;
    unsigned count, within_frame;
};

static struct pt_native_eclock counter;
static struct pt_elapsed_clock grid;
static struct brackets bracket[PHASE_COUNT];

static uint64_t read_ticks(void)
{
    uint64_t ticks;
    uint32_t frequency;
    assert(pt_native_eclock_read(&counter, &ticks, &frequency));
    assert(frequency == grid.frequency);
    return ticks;
}

static void record(enum benchmark_phase phase, uint64_t before, uint64_t after)
{
    struct brackets *r = bracket + phase;
    uint64_t start_frame, end_frame;
    assert(after >= before && after - before <= UINT32_MAX);
    assert(pt_elapsed_clock_advance(&grid, grid.frequency, before, &start_frame) == PT_ELAPSED_OK);
    assert(pt_elapsed_clock_advance(&grid, grid.frequency, after, &end_frame) == PT_ELAPSED_OK);
    const uint32_t duration = (uint32_t)(after - before);
    if (!r->count || duration < r->minimum)
        r->minimum = duration;
    if (duration > r->maximum)
        r->maximum = duration;
    ++r->count;
    r->within_frame += start_frame == end_frame;
}

static enum pt_mixed_readers_result timed_publish(struct pt_mixed_readers_activation *owner, uint64_t ticket, int stop)
{
    const uint64_t before = read_ticks();
    const enum pt_mixed_readers_result result = pt_mixed_activation_publish(owner, ticket);
    const uint64_t after = read_ticks();
    record(stop ? PUBLISH_STOP : PUBLISH_START, before, after);
    return result;
}

static enum pt_mixed_activation_result timed_fire(struct pt_mixed_readers_activation *owner, uint64_t ticket, int stop)
{
    const uint64_t before = read_ticks();
    const enum pt_mixed_activation_result result = pt_mixed_activation_fire(owner, ticket);
    const uint64_t after = read_ticks();
    record(stop ? FIRE_STOP : FIRE_START, before, after);
    return result;
}

#define PT_ROUTED_ACTIVATION_TEST_MAIN run_routed_activation
#define PT_ROUTED_ACTIVATION_PUBLISH timed_publish
#define PT_ROUTED_ACTIVATION_FIRE timed_fire
#define PT_ROUTED_ACTIVATION_REPORT(message) ((void)0)
#include "routed_activation_test.c"

int main(int argc, char **argv)
{
    static const char *const label[PHASE_COUNT] = {
        "empty", "publish_start", "fire_start", "publish_stop", "fire_stop"
    };
    uint64_t epoch;
    uint32_t frequency;
    unsigned i;
    assert(argc == 1 && pt_native_eclock_open(&counter));
    assert(pt_native_eclock_read(&counter, &epoch, &frequency));
    assert(pt_elapsed_clock_init(&grid, frequency, SAMPLE_RATE, epoch, 0) == PT_ELAPSED_OK);
    for (i = 0; i < ATTEMPTS; ++i) {
        const uint64_t before = read_ticks();
        const uint64_t after = read_ticks();
        record(EMPTY, before, after);
        assert(run_routed_activation(argc, argv) == 0);
    }
    pt_native_eclock_close(&counter);
    assert(!counter.port && !counter.request && !counter.opened);
    printf("Routed activation brackets: hz=%lu rate=%u attempts=%u\n",
        (unsigned long)frequency, SAMPLE_RATE, ATTEMPTS);
    for (i = 0; i < PHASE_COUNT; ++i) {
        const struct brackets *r = bracket + i;
        assert(r->count == ATTEMPTS);
        printf("%s min=%lu max=%lu same_frame=%u\n", label[i],
            (unsigned long)r->minimum, (unsigned long)r->maximum, r->within_frame);
    }
    puts("Routed activation brackets: modeled output and exact controlled schedule verified");
    return 0;
}
