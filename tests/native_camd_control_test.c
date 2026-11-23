#include "../src/native/camd_adapter.h"
#include "../src/native/camd_vectors.h"
#include "../src/native/eclock.h"
#include "../src/core/elapsed_clock.h"
#include "../src/platform/camd_routes.h"
#include <assert.h>
#include <exec/libraries.h>
#include <proto/dos.h>
#include <stdio.h>
#include <string.h>
#include <utility/tagitem.h>

/* The loopback fixtures send only to their own temporary local receivers in
 * a driver-free image. Discovery can inspect an existing CAMD configuration
 * without creating any links. No playback provider is registered. */
static struct pt_native_camd native;
static struct pt_camd_control control;
static const char receiver_name[] = "PT24G local loopback";
static const char endpoint_name[] = "PT24G.test.loopback";
static const char sender_only_name[] = "PT24G.test.sender-only";

struct range {uint32_t min, max;};

static void include_ticks(struct range *range, uint64_t ticks)
{
    assert(ticks <= UINT32_MAX);
    if (ticks < range->min)
        range->min = (uint32_t)ticks;
    if (ticks > range->max)
        range->max = (uint32_t)ticks;
}

static void finish(void)
{
    unsigned i;
    enum pt_camd_result result;
    for (i = 0; i < PT_MIDI_PORTS + 3; ++i) {
        result = pt_camd_control_close_step(&control);
        if (result == PT_CAMD_FINISHED)
            break;
        assert(result == PT_CAMD_MORE);
    }
    assert(i < PT_MIDI_PORTS + 3 && pt_camd_control_state(&control) == PT_CAMD_CLOSED);
    assert(!native.library && !native.lock && !control.node && !control.library);
}

static void missing_library(void)
{
    assert(pt_camd_control_activate(&control) == PT_CAMD_REFUSED);
    assert(!native.library && !control.node && pt_camd_control_state(&control) == PT_CAMD_EMPTY);
    finish();
    puts("CAMD native control: missing library refused and closed");
}

static int discover_outputs(void)
{
    const enum pt_camd_result activation = pt_camd_control_activate(&control);
    if (activation != PT_CAMD_OK) {
        finish();
        printf("CAMD output discovery: activation failed result=%d closed=1\n", activation);
        return 5;
    }

    const enum pt_camd_result refresh = pt_camd_control_refresh(&control);
    const struct pt_midi_endpoints *registry = pt_camd_control_registry(&control);
    if (refresh != PT_CAMD_OK || !registry) {
        finish();
        printf("CAMD output discovery: scan failed result=%d closed=1\n", refresh);
        return 5;
    }

    unsigned count = 0;
    for (unsigned port = 0; port < PT_MIDI_PORTS; ++port) {
        if (pt_midi_endpoints_available(registry, port) != 1)
            continue;
        const struct pt_midi_endpoint_name *name = pt_midi_endpoints_name(registry, port);
        assert(name && !registry->link_active && !control.links[port]);
        printf("CAMD output %u: %s\n", ++count, name->identity);
    }
    const unsigned version = native.library->lib_Version;
    const unsigned revision = native.library->lib_Revision;
    finish();
    printf("CAMD output discovery: library=%u.%u outputs=%u closed=1\n", version, revision, count);
    return 0;
}

static void *link_to(struct Library *library, void *node, unsigned type, const char *name)
{
    const struct TagItem tags[] = {{ PT_CAMD_LINK_LOCATION_TAG, (ULONG)(uintptr_t)name }, { TAG_DONE, 0 }};
    return (void *)(uintptr_t)pt_native_camd_vector(library, PT_CAMD_ADD_LINK, type, node, tags);
}

static void local_note(struct Library *library, void *sender, void *receiver, uint32_t packed)
{
    uint32_t message[2] = {0};
    unsigned attempts;
    (void)pt_native_camd_vector(library, PT_CAMD_PUT, packed, sender, NULL);
    for (attempts = 0; attempts < 10; ++attempts) {
        if ((BOOL)pt_native_camd_vector(library, PT_CAMD_GET, 0, receiver, message))
            break;
        Delay(1);
    }
    assert(attempts < 10 && message[0] >> 8 == packed >> 8);
    assert(!(UBYTE)pt_native_camd_vector(library, PT_CAMD_GET_ERROR, 0, receiver, NULL));
}

static uint64_t read_ticks(struct pt_native_eclock *clock, uint32_t frequency)
{
    uint64_t ticks;
    uint32_t rate;
    assert(pt_native_eclock_read(clock, &ticks, &rate) && rate == frequency);
    return ticks;
}

static void measure_loopback(struct Library *library, void *sender, void *receiver, ULONG *stamp)
{
    enum {COUNT = 32, RATE = 48000};
    struct pt_native_eclock clock = {0};
    struct pt_elapsed_clock grid;
    struct range send = {UINT32_MAX, 0}, receive = {UINT32_MAX, 0}, span = {UINT32_MAX, 0};
    uint64_t epoch, previous;
    uint32_t frequency;
    unsigned i, send_inside = 0, receive_inside = 0;

    assert(pt_native_eclock_open(&clock));
    assert(pt_native_eclock_read(&clock, &epoch, &frequency) && frequency >= RATE);
    assert(pt_elapsed_clock_init(&grid, frequency, RATE, epoch, 0) == PT_ELAPSED_OK);
    previous = epoch;
    for (i = 0; i < COUNT; ++i) {
        const uint32_t packed = i & 1 ? UINT32_C(0x863c0000) : UINT32_C(0x963c6400);
        const ULONG marker = UINT32_C(0xcad00000) | i;
        uint32_t message[2] = {0};
        uint64_t before, after_send, after_receive, frame, excluded;
        *stamp = marker;
        before = read_ticks(&clock, frequency);
        (void)pt_native_camd_vector(library, PT_CAMD_PUT, packed, sender, NULL);
        after_send = read_ticks(&clock, frequency);
        *stamp = ~marker;
        assert((BOOL)pt_native_camd_vector(library, PT_CAMD_GET, 0, receiver, message));
        after_receive = read_ticks(&clock, frequency);
        assert(message[0] >> 8 == packed >> 8 && message[1] == marker);
        assert(!(UBYTE)pt_native_camd_vector(library, PT_CAMD_GET_ERROR, 0, receiver, NULL));
        assert(previous <= before && before <= after_send && after_send <= after_receive);
        previous = after_receive;

        assert(pt_elapsed_clock_advance(&grid, frequency, before, &frame) == PT_ELAPSED_OK);
        assert(pt_elapsed_clock_deadline(&grid, frame + 1, &excluded) == PT_ELAPSED_OK && excluded > before);
        send_inside += after_send < excluded;
        receive_inside += after_receive < excluded;
        include_ticks(&send, after_send - before);
        include_ticks(&receive, after_receive - after_send);
        include_ticks(&span, after_receive - before);
    }
    pt_native_eclock_close(&clock);
    assert(!clock.opened && !clock.port && !clock.request && receive_inside <= send_inside);
    printf("CAMD native timing: samples=%u hz=%lu rate=%u send=%lu/%lu receive=%lu/%lu total=%lu/%lu send_in_frame=%u receipt_in_frame=%u\n",
           (unsigned)COUNT, (unsigned long)frequency, (unsigned)RATE, (unsigned long)send.min,
           (unsigned long)send.max, (unsigned long)receive.min, (unsigned long)receive.max,
           (unsigned long)span.min, (unsigned long)span.max, send_inside, receive_inside);
}

#ifdef PT_CAMD_DIRECT_COUNT_TEST
#include "native_camd_direct_clock.h"
#endif

static int local_loopback(unsigned timing)
{
    int rc = 0;
    ULONG stamp = 0;
    const struct TagItem node_tags[] = {
        { PT_CAMD_NODE_NAME_TAG, (ULONG)(uintptr_t)receiver_name },
        { PT_CAMD_RECEIVE_QUEUE_TAG, 4 }, { PT_CAMD_RECEIVE_STAMP_TAG, (ULONG)(uintptr_t)&stamp }, { TAG_DONE, 0 }
    };
    struct Library *library;
    void *node, *receiver, *decoy;
    const struct pt_midi_endpoints *registry;
    int port;
    uint32_t tail[2];
    assert(pt_camd_control_activate(&control) == PT_CAMD_OK);
    library = native.library;
    assert(library && library->lib_Version == 40 && library->lib_Revision == 5);
    assert(pt_camd_control_refresh(&control) == PT_CAMD_OK);
    registry = pt_camd_control_registry(&control);
    assert(registry && !registry->available && !native.lock);

    node = (void *)(uintptr_t)pt_native_camd_vector(library, PT_CAMD_CREATE_NODE, 0, node_tags, NULL);
    assert(node);
    receiver = link_to(library, node, PT_CAMD_RECEIVER, endpoint_name);
    decoy = link_to(library, node, PT_CAMD_SENDER, sender_only_name);
    assert(receiver && decoy);
    assert(pt_camd_control_refresh(&control) == PT_CAMD_OK && !native.lock);
    port = pt_midi_endpoints_find(registry, endpoint_name, sizeof(endpoint_name));
    assert(port >= 0 && pt_midi_endpoints_available(registry, (unsigned)port));
    assert(pt_midi_endpoints_find(registry, sender_only_name, sizeof(sender_only_name)) == -1);
    assert(!registry->link_active && !registry->midi.ports[port].connected);
    assert(pt_camd_control_link(&control, (unsigned)port) == PT_CAMD_OK && control.links[port]);
    assert(pt_camd_control_link(&control, (unsigned)port) == PT_CAMD_IN_USE);

    if (timing == 1) {
        measure_loopback(library, control.links[port], node, &stamp);
#ifdef PT_CAMD_DIRECT_COUNT_TEST
    } else if (timing == 2) {
        rc = measure_direct(library, control.links[port], node, &stamp);
#endif
    } else {
        local_note(library, control.links[port], node, UINT32_C(0x963c6400));
        local_note(library, control.links[port], node, UINT32_C(0x863c0000));
    }
    assert(!(BOOL)pt_native_camd_vector(library, PT_CAMD_GET, 0, node, tail));
    assert(!registry->link_active && !registry->midi.ports[port].connected);
    assert(pt_camd_control_retire(&control, (unsigned)port) == PT_CAMD_OK && !control.links[port]);

    (void)pt_native_camd_vector(library, PT_CAMD_REMOVE_LINK, 0, receiver, NULL);
    (void)pt_native_camd_vector(library, PT_CAMD_REMOVE_LINK, 0, decoy, NULL);
    (void)pt_native_camd_vector(library, PT_CAMD_DELETE_NODE, 0, node, NULL);
    assert(pt_camd_control_refresh(&control) == PT_CAMD_OK && !registry->available);
    finish();
    if (timing == 1)
        puts("CAMD native timing: local messages and receive timestamps verified; original handles closed");
#ifdef PT_CAMD_DIRECT_COUNT_TEST
    else if (timing == 2)
        printf("CAMD direct closed: original clock, library, node and links released rc=%d\n", rc);
#endif
    else
        puts("CAMD native control: 40.5 receiver discovery, local note on/off and original handles closed");
    return rc;
}

static void routes_loopback(void)
{
    static const char cluster[2][PT_MIDI_ENDPOINT] = {"PT24G.test.a", "PT24G.test.b"};
    static const char project[2][PT_MIDI_ENDPOINT] = {"PT24G.test.b", "PT24G.test.a"};
    static const char duplicate[2][PT_MIDI_ENDPOINT] = {"PT24G.test.b", "PT24G.test.b"};
    static const char wrong_case[2][PT_MIDI_ENDPOINT] = {"pt24g.test.b", "PT24G.test.a"};
    static const char receiver_names[2][PT_MIDI_ENDPOINT] = {"PT24G receive A", "PT24G receive B"};
    struct pt_camd_routes map;
    struct pt_camd_route_lease borrowed = {0}, alternate = {0};
    struct Library *library;
    const struct pt_midi_endpoints *registry;
    void *nodes[2], *receivers[2];
    int ports[2];
    uint32_t message[2];
    const uint32_t notes[2] = {UINT32_C(0x903c3200), UINT32_C(0x91406400)};
    const uint32_t releases[2] = {UINT32_C(0x803c0000), UINT32_C(0x81400000)};
    unsigned i;

    assert(pt_camd_control_activate(&control) == PT_CAMD_OK);
    library = native.library;
    assert(library && library->lib_Version == 40 && library->lib_Revision == 5);
    assert(pt_camd_control_refresh(&control) == PT_CAMD_OK);
    registry = pt_camd_control_registry(&control);
    assert(registry && !registry->available);

    for (i = 0; i < 2; ++i) {
        const struct TagItem tags[] = {
            {PT_CAMD_NODE_NAME_TAG, (ULONG)(uintptr_t)receiver_names[i]},
            {PT_CAMD_RECEIVE_QUEUE_TAG, 4}, {TAG_DONE, 0}
        };
        nodes[i] = (void *)(uintptr_t)pt_native_camd_vector(library, PT_CAMD_CREATE_NODE, 0, tags, NULL);
        assert(nodes[i]);
        receivers[i] = link_to(library, nodes[i], PT_CAMD_RECEIVER, cluster[i]);
        assert(receivers[i]);
        assert(pt_camd_control_refresh(&control) == PT_CAMD_OK);
        ports[i] = pt_midi_endpoints_find(registry, cluster[i], strlen(cluster[i]) + 1);
        assert(ports[i] >= 0);
    }
    assert(ports[0] < ports[1]);
    map.count = 99;
    assert(!pt_camd_routes_resolve(&control, project, 2, &map) && map.count == 99);
    for (i = 0; i < 2; ++i)
        assert(pt_camd_control_link(&control, (unsigned)ports[i]) == PT_CAMD_OK);

    assert(!pt_camd_routes_resolve(&control, duplicate, 2, &map) && map.count == 99);
    assert(!pt_camd_routes_resolve(&control, wrong_case, 2, &map) && map.count == 99);
    assert(pt_camd_routes_resolve(&control, project, 2, &map) && map.count == 2);
    assert(map.slot[0] == ports[1] && map.slot[1] == ports[0]);
    assert(pt_camd_routes_borrow(&control, project, 2, &borrowed));
    struct pt_camd_route_lease copy = borrowed;
    struct pt_camd_routes witness = {.count = 99};
    void *original_sender = control.links[ports[0]];
    assert(!pt_camd_routes_borrow(&control, project, 2, &alternate));
    assert(!pt_camd_routes_release(&copy));
    assert(!pt_camd_routes_snapshot(&copy, &witness) && witness.count == 99);
    assert(!pt_camd_routes_snapshot_for(&borrowed, cluster, 2, &witness) && witness.count == 99);
    assert(pt_camd_routes_snapshot_for(&borrowed, project, 2, &map) && map.count == 2);
    assert(pt_camd_control_retire(&control, (unsigned)ports[0]) == PT_CAMD_IN_USE);
    assert(pt_camd_control_close_step(&control) == PT_CAMD_IN_USE);

    for (i = 0; i < map.count; ++i) {
        local_note(library, control.links[map.slot[i]], nodes[1 - i], notes[i]);
        local_note(library, control.links[map.slot[i]], nodes[1 - i], releases[i]);
        assert(!(BOOL)pt_native_camd_vector(library, PT_CAMD_GET, 0, nodes[0], message));
        assert(!(BOOL)pt_native_camd_vector(library, PT_CAMD_GET, 0, nodes[1], message));
    }
    assert(!registry->link_active);
    (void)pt_native_camd_vector(library, PT_CAMD_REMOVE_LINK, 0, receivers[0], NULL);
    assert(pt_camd_control_refresh(&control) == PT_CAMD_OK && control.links[ports[0]]);
    assert(!pt_camd_routes_resolve(&control, project, 2, &map));
    assert(!pt_camd_routes_snapshot_for(&borrowed, project, 2, &witness) && witness.count == 99);
    assert(pt_camd_control_retire(&control, (unsigned)ports[0]) == PT_CAMD_IN_USE);
    assert(control.links[ports[0]] == original_sender);
    assert(map.count == 2 && map.slot[0] == ports[1] && map.slot[1] == ports[0]);
    receivers[0] = link_to(library, nodes[0], PT_CAMD_RECEIVER, cluster[0]);
    assert(receivers[0] && pt_camd_control_refresh(&control) == PT_CAMD_OK &&
           pt_camd_routes_snapshot(&borrowed, &witness) && witness.count == 2);
    assert(control.links[ports[0]] == original_sender);
    assert(pt_camd_routes_release(&borrowed) && !pt_camd_routes_release(&borrowed));
    assert(!pt_camd_routes_snapshot(&copy, &witness));
    (void)pt_native_camd_vector(library, PT_CAMD_REMOVE_LINK, 0, receivers[0], NULL);
    (void)pt_native_camd_vector(library, PT_CAMD_REMOVE_LINK, 0, receivers[1], NULL);
    for (i = 0; i < 2; ++i) {
        assert(pt_camd_control_retire(&control, (unsigned)ports[i]) == PT_CAMD_OK);
        (void)pt_native_camd_vector(library, PT_CAMD_DELETE_NODE, 0, nodes[i], NULL);
    }
    assert(pt_camd_control_refresh(&control) == PT_CAMD_OK && !registry->available);
    finish();
    puts("CAMD native routes: project order preserved, both local receivers verified, missing receiver refused");
    puts("CAMD route lease: original links retained during Stop, temporary receiver loss refused, release confirmed");
}

int main(int argc, char **argv)
{
    assert(argc == 2 && pt_native_camd_control_init(&native, &control));
    if (!strcmp(argv[1], "absent"))
        missing_library();
    else if (!strcmp(argv[1], "outputs"))
        return discover_outputs();
    else if (!strcmp(argv[1], "loopback"))
        return local_loopback(0);
#ifdef PT_CAMD_DIRECT_COUNT_TEST
    else if (!strcmp(argv[1], "direct"))
        return local_loopback(2);
#endif
    else if (!strcmp(argv[1], "routes"))
        routes_loopback();
    else {
        assert(!strcmp(argv[1], "timing"));
        return local_loopback(1);
    }
    return 0;
}
