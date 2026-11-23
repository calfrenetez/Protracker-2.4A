#include "../src/native/camd_adapter.h"
#include "../src/native/camd_vectors.h"
#include <assert.h>
#include <exec/libraries.h>
#include <proto/dos.h>
#include <stdio.h>
#include <string.h>
#include <utility/tagitem.h>

/* Native CLI fixture for a private image with empty DEVS:MIDI. Sends only to
 * its own temporary local receiver. No playback provider is registered. */
static struct pt_native_camd native;
static struct pt_camd_control control;
static const char receiver_name[] = "PT24G local loopback";
static const char endpoint_name[] = "PT24G.test.loopback";
static const char sender_only_name[] = "PT24G.test.sender-only";

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

static void local_loopback(void)
{
    const struct TagItem node_tags[] = {
        { PT_CAMD_NODE_NAME_TAG, (ULONG)(uintptr_t)receiver_name },
        { PT_CAMD_RECEIVE_QUEUE_TAG, 4 }, { TAG_DONE, 0 }
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

    local_note(library, control.links[port], node, UINT32_C(0x963c6400));
    local_note(library, control.links[port], node, UINT32_C(0x863c0000));
    assert(!(BOOL)pt_native_camd_vector(library, PT_CAMD_GET, 0, node, tail));
    assert(!registry->link_active && !registry->midi.ports[port].connected);
    assert(pt_camd_control_retire(&control, (unsigned)port) == PT_CAMD_OK && !control.links[port]);

    (void)pt_native_camd_vector(library, PT_CAMD_REMOVE_LINK, 0, receiver, NULL);
    (void)pt_native_camd_vector(library, PT_CAMD_REMOVE_LINK, 0, decoy, NULL);
    (void)pt_native_camd_vector(library, PT_CAMD_DELETE_NODE, 0, node, NULL);
    assert(pt_camd_control_refresh(&control) == PT_CAMD_OK && !registry->available);
    finish();
    puts("CAMD native control: 40.5 receiver discovery, local note on/off and original handles closed");
}

int main(int argc, char **argv)
{
    assert(argc == 2 && pt_native_camd_control_init(&native, &control));
    if (!strcmp(argv[1], "absent"))
        missing_library();
    else {
        assert(!strcmp(argv[1], "loopback"));
        local_loopback();
    }
    return 0;
}
