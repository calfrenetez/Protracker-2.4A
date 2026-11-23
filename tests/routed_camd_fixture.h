#ifndef PT_TEST_ROUTED_CAMD_FIXTURE_H
#define PT_TEST_ROUTED_CAMD_FIXTURE_H

#include "../src/platform/camd_routes.h"

struct fixture_camd_links {
    unsigned receivers;
    unsigned node, lock, link[2];
};

struct fixture_camd {
    struct pt_camd_control owner;
    struct pt_camd_route_lease lease;
    struct fixture_camd_links links;
};

static const char fixture_camd_clusters[2][PT_MIDI_ENDPOINT] = {"synth.b", "synth.a"};

static void *fixture_camd_open(void *context)
{
    return context;
}

static void *fixture_camd_node(void *context, void *library, const char *name)
{
    struct fixture_camd_links *fixture = context;
    assert(context == library && name && name[0]);
    return &fixture->node;
}

static void *fixture_camd_sender(void *context, void *library, void *node, const char *name)
{
    struct fixture_camd_links *fixture = context;
    assert(context == library && node == &fixture->node);
    for (unsigned i = 0; i < 2; ++i)
        if (!strcmp(name, fixture_camd_clusters[i]) && !fixture->link[i]) {
            fixture->link[i] = 1;
            return fixture->link + i;
        }
    return NULL;
}

static void *fixture_camd_lock(void *context, void *library)
{
    struct fixture_camd_links *fixture = context;
    assert(context == library);
    return &fixture->lock;
}

static enum pt_camd_next fixture_camd_next(void *context, void *library, const void *last, struct pt_camd_view *out)
{
    struct fixture_camd_links *fixture = context;
    assert(context == library);
    if (last == fixture_camd_clusters[1])
        return PT_CAMD_END;
    assert(!last || last == fixture_camd_clusters[0]);
    const unsigned i = last != NULL;
    *out = (struct pt_camd_view){.cursor = fixture_camd_clusters[i], .receivers = (fixture->receivers >> i) & 1U};
    memcpy(out->name, fixture_camd_clusters[i], sizeof(out->name));
    return PT_CAMD_ITEM;
}

static enum pt_camd_done fixture_camd_unlock(void *context, void *library, void *lock)
{
    struct fixture_camd_links *fixture = context;
    assert(context == library && lock == &fixture->lock);
    return PT_CAMD_DONE;
}

static enum pt_camd_done fixture_camd_remove(void *context, void *library, void *link)
{
    struct fixture_camd_links *fixture = context;
    assert(context == library && (link == fixture->link || link == fixture->link + 1));
    *(unsigned *)link = 0;
    return PT_CAMD_DONE;
}

static enum pt_camd_done fixture_camd_delete(void *context, void *library, void *node)
{
    struct fixture_camd_links *fixture = context;
    assert(context == library && node == &fixture->node && !fixture->link[0] && !fixture->link[1]);
    return PT_CAMD_DONE;
}

static enum pt_camd_done fixture_camd_close(void *context, void *library)
{
    assert(context == library);
    return PT_CAMD_DONE;
}

static void fixture_camd_start(struct fixture_camd *fixture, const char (*names)[PT_MIDI_ENDPOINT])
{
    const struct pt_camd_ops ops = {fixture_camd_open, fixture_camd_node, fixture_camd_sender, fixture_camd_lock,
        fixture_camd_next, fixture_camd_unlock, fixture_camd_remove, fixture_camd_delete, fixture_camd_close};
    fixture->links.receivers = 3;
    assert(pt_camd_control_init(&fixture->owner, &ops, &fixture->links, sizeof(fixture->links)));
    assert(pt_camd_control_activate(&fixture->owner) == PT_CAMD_OK);
    assert(pt_camd_control_refresh(&fixture->owner) == PT_CAMD_OK);
    assert(pt_camd_control_link(&fixture->owner, 0) == PT_CAMD_OK);
    assert(pt_camd_control_link(&fixture->owner, 1) == PT_CAMD_OK);
    assert(pt_camd_routes_borrow(&fixture->owner, names, 2, &fixture->lease));
}

static void fixture_camd_finish(struct fixture_camd *fixture)
{
    assert(pt_camd_routes_release(&fixture->lease));
    for (unsigned i = 0; i < 5; ++i) {
        enum pt_camd_result state = pt_camd_control_close_step(&fixture->owner);
        if (state == PT_CAMD_FINISHED) {
            assert(!fixture->links.link[0] && !fixture->links.link[1]);
            return;
        }
        assert(state == PT_CAMD_MORE);
    }
    assert(!"CAMD fixture did not finish");
}
#endif
