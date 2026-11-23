#include "camd_adapter.h"
#include "camd_vectors.h"
#include <exec/libraries.h>
#include <exec/lists.h>
#include <proto/exec.h>
#include <utility/tagitem.h>
#include <stddef.h>
#include <string.h>

/* Only the public prefix needed for discovery. Inspect the receiver list
 * itself so receive drivers count even if NextClusterLink skips their nodes. */
struct cluster_prefix {
    struct Node node;
    WORD participants;
    struct List receivers;
};

_Static_assert(sizeof(void *) == 4 && offsetof(struct Node, ln_Name) == 10 &&
    offsetof(struct cluster_prefix, receivers) == 16, "Classic CAMD cluster layout");

static int valid(const struct pt_native_camd *native, const void *library)
{
    return native && native->self == native && library && library == native->library;
}

static void *open_library(void *context)
{
    struct pt_native_camd *native = context;
    if (!native || native->self != native || native->library || native->lock)
        return NULL;
    native->library = OpenLibrary((CONST_STRPTR)"camd.library", 37);
    return native->library;
}

static void *create_node(void *context, void *library, const char *name)
{
    struct pt_native_camd *native = context;
    const struct TagItem tags[] = {{ PT_CAMD_NODE_NAME_TAG, (ULONG)(uintptr_t)name }, { TAG_DONE, 0 }};
    if (!valid(native, library) || native->lock || !name || !name[0])
        return NULL;
    return (void *)(uintptr_t)pt_native_camd_vector(library, PT_CAMD_CREATE_NODE, 0, tags, NULL);
}

static void *lock_library(void *context, void *library)
{
    struct pt_native_camd *native = context;
    if (!valid(native, library) || native->lock)
        return NULL;
    native->lock = (void *)(uintptr_t)pt_native_camd_vector(library, PT_CAMD_LOCK, 0, NULL, NULL);
    return native->lock;
}

static enum pt_camd_done unlock_library(void *context, void *library, void *lock)
{
    struct pt_native_camd *native = context;
    if (!valid(native, library) || !lock || lock != native->lock)
        return PT_CAMD_NO_EFFECT;
    (void)pt_native_camd_vector(library, PT_CAMD_UNLOCK, 0, lock, NULL);
    native->lock = NULL;
    return PT_CAMD_DONE;
}

static int has_receiver(const void *cluster)
{
    const size_t list_start = offsetof(struct cluster_prefix, receivers);
    const unsigned char *list = (const unsigned char *)cluster + list_start;
    const struct Node *first;
    memcpy(&first, list + offsetof(struct List, lh_Head), sizeof(first));
    return first && (const void *)first != list + offsetof(struct List, lh_Tail);
}

static enum pt_camd_next next_cluster(void *context, void *library, const void *last, struct pt_camd_view *out)
{
    struct pt_native_camd *native = context;
    struct pt_camd_view view = {0};
    const char *name;
    unsigned i;
    if (!valid(native, library) || !native->lock || !out)
        return PT_CAMD_READ_FAILED;
    view.cursor = (const void *)(uintptr_t)pt_native_camd_vector(library, PT_CAMD_NEXT_CLUSTER, 0, last, NULL);
    if (!view.cursor)
        return PT_CAMD_END;
    memcpy(&name, (const unsigned char *)view.cursor + offsetof(struct Node, ln_Name), sizeof(name));
    if (!name || !name[0])
        return PT_CAMD_READ_FAILED;
    for (i = 0; i < sizeof(view.name) && name[i]; ++i)
        view.name[i] = name[i];
    if (i == sizeof(view.name))
        return PT_CAMD_READ_FAILED;
    view.receivers = has_receiver(view.cursor);
    *out = view;
    return PT_CAMD_ITEM;
}

static int available(struct pt_native_camd *native, struct Library *library, const char *name)
{
    void *lock = lock_library(native, library);
    const void *cluster;
    int found;
    if (!lock)
        return 0;
    cluster = (const void *)(uintptr_t)pt_native_camd_vector(library, PT_CAMD_FIND_CLUSTER, 0, name, NULL);
    found = cluster && has_receiver(cluster);
    return unlock_library(native, library, lock) == PT_CAMD_DONE && found;
}

static void *add_sender(void *context, void *library, void *node, const char *name)
{
    struct pt_native_camd *native = context;
    void *link;
    const struct TagItem tags[] = {{ PT_CAMD_LINK_LOCATION_TAG, (ULONG)(uintptr_t)name }, { TAG_DONE, 0 }};
    if (!node || !name || !name[0] || !available(native, library, name))
        return NULL;
    link = (void *)(uintptr_t)pt_native_camd_vector(library, PT_CAMD_ADD_LINK, PT_CAMD_SENDER, node, tags);
    if (link && !available(native, library, name)) {
        (void)pt_native_camd_vector(library, PT_CAMD_REMOVE_LINK, 0, link, NULL);
        link = NULL;
    }
    return link;
}

static enum pt_camd_done remove_link(void *context, void *library, void *link)
{
    struct pt_native_camd *native = context;
    if (!valid(native, library) || native->lock || !link)
        return PT_CAMD_NO_EFFECT;
    (void)pt_native_camd_vector(library, PT_CAMD_REMOVE_LINK, 0, link, NULL);
    return PT_CAMD_DONE;
}

static enum pt_camd_done delete_node(void *context, void *library, void *node)
{
    struct pt_native_camd *native = context;
    if (!valid(native, library) || native->lock || !node)
        return PT_CAMD_NO_EFFECT;
    (void)pt_native_camd_vector(library, PT_CAMD_DELETE_NODE, 0, node, NULL);
    return PT_CAMD_DONE;
}

static enum pt_camd_done close_library(void *context, void *library)
{
    struct pt_native_camd *native = context;
    if (!valid(native, library) || native->lock)
        return PT_CAMD_NO_EFFECT;
    CloseLibrary(library);
    native->library = NULL;
    return PT_CAMD_DONE;
}

int pt_native_camd_control_init(struct pt_native_camd *native, struct pt_camd_control *control)
{
    const struct pt_camd_ops ops = {
        .open = open_library, .create_node = create_node, .add_sender = add_sender,
        .lock = lock_library, .next_copy = next_cluster, .unlock = unlock_library,
        .remove_link = remove_link, .delete_node = delete_node, .close = close_library
    };
    if (!native || native->self || native->library || native->lock ||
        !pt_camd_control_init(control, &ops, native, sizeof(*native)))
        return 0;
    native->self = native;
    return 1;
}
