/* PRIVATE HOST SOURCE FIXTURE ONLY.
 * Authored and reviewed as source; this file has not been compiled or run.
 * All pointers below are opaque, local mock tokens. No native headers, SDK,
 * IRQ, timer, provider, resource, Task, Device, or hardware path is entered.
 * Assertions inspect actual mock state/effects, independently of the
 * transaction's recorded enums. The final marker is emitted only by main
 * after every assertion group, if a separately authorized HOST run occurs.
 */
#define PT_PRIVATE_TWO_LEASE_HOST_MODEL 1
#include "host_two_lease.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#ifdef NDEBUG
#error "HOST fixture requires enabled assertions"
#endif

enum operation { INSPECT, ACQUIRE, REMOVE_SERVER, RETURN_RESOURCE, OP_COUNT };
enum { TRACE_CAPACITY = 256 };

struct behavior {
    int result;
    unsigned perform_effect, drift_sibling;
};

struct trace_entry {
    enum operation operation;
    unsigned slot;
    int result;
    struct pt_host_pair_binding binding; /* Value copy, no borrowed retention. */
    const void *owner;
    struct pt_host_pair_image supplied_before;
    struct pt_host_pair_snapshot before[2], after[2];
};

struct mock_provider {
    const void *owner;
    struct pt_host_pair_binding binding[2];
    struct pt_host_pair_snapshot initial[2], actual[2];
    struct behavior behavior[OP_COUNT][2];
    unsigned calls[OP_COUNT][2];
    unsigned actual_acquisitions[2], actual_removals[2], actual_returns[2];
    unsigned trace_used;
    struct trace_entry trace[TRACE_CAPACITY];
};

struct fixture {
    struct pt_host_pair_transaction transaction;
    struct pt_host_pair_input input;
    struct pt_host_pair_ops ops;
    struct mock_provider provider;
};

struct audit {
    unsigned calls[OP_COUNT][2];
    unsigned acquisitions[2], removals[2], returns[2], trace_used;
};

static const unsigned char tokens[24] = {0};
static unsigned passed_groups;
static int old_vector_0(void *unused) { (void)unused; return 0; }
static int old_vector_1(void *unused) { (void)unused; return 0; }
static int server_0(void *unused) { (void)unused; return 0; }
static int server_1(void *unused) { (void)unused; return 0; }
static int foreign_server(void *unused) { (void)unused; return 0; }

static int registration_same(const struct pt_host_pair_registration *a,
    const struct pt_host_pair_registration *b)
{
    return a->owner == b->owner && a->queue == b->queue &&
        a->session == b->session && a->generation == b->generation;
}

static int image_same(const struct pt_host_pair_image *a,
    const struct pt_host_pair_image *b)
{
    return a->programmed_latch == b->programmed_latch && a->mask == b->mask &&
        a->pending == b->pending && a->control == b->control &&
        a->vector_data == b->vector_data && a->vector_code == b->vector_code;
}

static int binding_same(const struct pt_host_pair_binding *a,
    const struct pt_host_pair_binding *b)
{
    return registration_same(&a->registration, &b->registration) &&
        a->ticket == b->ticket && a->first == b->first &&
        a->frequency == b->frequency && a->resource == b->resource &&
        a->server == b->server && a->server_data == b->server_data &&
        a->server_code == b->server_code && a->chip == b->chip && a->bit == b->bit;
}

static int snapshot_same(const struct pt_host_pair_snapshot *a,
    const struct pt_host_pair_snapshot *b)
{
    /* Compare every member, including facts that a simple effect enum misses. */
    return image_same(&a->image, &b->image) &&
        a->resource_owner == b->resource_owner && a->server_owner == b->server_owner &&
        a->installed_server == b->installed_server && a->available == b->available &&
        a->resource_held == b->resource_held && a->server_installed == b->server_installed &&
        a->image_complete == b->image_complete && a->whole_exclusion == b->whole_exclusion &&
        a->full_return == b->full_return && a->callbacks_inflight == b->callbacks_inflight &&
        a->delivery_queued == b->delivery_queued;
}

static int input_same(const struct pt_host_pair_input *a,
    const struct pt_host_pair_input *b)
{
    unsigned i;
    if (a->source != b->source || a->original_task != b->original_task ||
        a->timer != b->timer || a->port != b->port ||
        !registration_same(&a->registration, &b->registration)) return 0;
    for (i = 0; i < 2; ++i)
        if (!binding_same(&a->binding[i], &b->binding[i]) ||
            !image_same(&a->before[i], &b->before[i])) return 0;
    return 1;
}

static unsigned mock_slot(const struct mock_provider *p,
    const struct pt_host_pair_binding *binding)
{
    unsigned i;
    for (i = 0; i < 2; ++i)
        if (binding_same(binding, &p->binding[i])) return i;
    /* A foreign or drifted callback binding must never reach a mock effect. */
    assert(0);
    return 0;
}

static struct trace_entry *begin_trace(struct mock_provider *p,
    enum operation operation, unsigned slot,
    const struct pt_host_pair_binding *binding, const void *owner,
    const struct pt_host_pair_image *before)
{
    struct trace_entry *entry;
    assert(p->trace_used < TRACE_CAPACITY);
    entry = &p->trace[p->trace_used++];
    entry->operation = operation;
    entry->slot = slot;
    entry->binding = *binding;
    entry->owner = owner;
    if (before) entry->supplied_before = *before;
    entry->before[0] = p->actual[0];
    entry->before[1] = p->actual[1];
    ++p->calls[operation][slot];
    return entry;
}

static void finish_trace(struct mock_provider *p, struct trace_entry *entry, int result)
{
    entry->result = result;
    entry->after[0] = p->actual[0];
    entry->after[1] = p->actual[1];
}

static int mock_inspect(void *context, const struct pt_host_pair_binding *binding,
    struct pt_host_pair_snapshot *out)
{
    struct mock_provider *p = context;
    unsigned slot = mock_slot(p, binding);
    struct trace_entry *entry = begin_trace(p, INSPECT, slot, binding, NULL, NULL);
    *out = p->actual[slot];
    finish_trace(p, entry, 1);
    /* Audit bookkeeping changes; actual provider snapshots are read-only. */
    assert(snapshot_same(&entry->before[0], &entry->after[0]));
    assert(snapshot_same(&entry->before[1], &entry->after[1]));
    return 1;
}

static void perform_actual_effect(struct mock_provider *p,
    enum operation operation, unsigned slot)
{
    struct pt_host_pair_snapshot *actual = &p->actual[slot];
    const struct pt_host_pair_binding *binding = &p->binding[slot];
    switch (operation) {
    case ACQUIRE:
        assert(snapshot_same(actual, &p->initial[slot]));
        actual->available = 0;
        actual->resource_held = actual->server_installed = 1;
        actual->resource_owner = actual->server_owner = p->owner;
        actual->installed_server = binding->server;
        actual->image.vector_data = binding->server_data;
        actual->image.vector_code = binding->server_code;
        ++p->actual_acquisitions[slot];
        break;
    case REMOVE_SERVER:
        assert(actual->resource_held == 1 && actual->server_installed == 1);
        assert(actual->resource_owner == p->owner && actual->server_owner == p->owner);
        assert(actual->installed_server == binding->server);
        actual->server_installed = 0;
        actual->server_owner = actual->installed_server = NULL;
        actual->image = p->initial[slot].image;
        ++p->actual_removals[slot];
        break;
    case RETURN_RESOURCE:
        assert(actual->resource_held == 1 && actual->server_installed == 0);
        assert(actual->resource_owner == p->owner);
        assert(actual->server_owner == NULL && actual->installed_server == NULL);
        assert(image_same(&actual->image, &p->initial[slot].image));
        *actual = p->initial[slot];
        ++p->actual_returns[slot];
        break;
    default:
        /* Inspection has no provider effect implementation. */
        assert(0);
    }
}

static int mock_mutate(void *context, enum operation operation,
    const struct pt_host_pair_binding *binding, const void *owner,
    const struct pt_host_pair_image *before)
{
    struct mock_provider *p = context;
    unsigned slot = mock_slot(p, binding), sibling = 1U - slot;
    struct behavior behavior = p->behavior[operation][slot];
    struct trace_entry *entry;
    assert(owner == p->owner);
    if (before) assert(image_same(before, &p->initial[slot].image));
    entry = begin_trace(p, operation, slot, binding, owner, before);
    if (behavior.perform_effect) perform_actual_effect(p, operation, slot);
    if (behavior.drift_sibling) p->actual[sibling].image.programmed_latch ^= 0x10U;
    finish_trace(p, entry, behavior.result);
    if (!behavior.drift_sibling)
        assert(snapshot_same(&entry->before[sibling], &entry->after[sibling]));
    return behavior.result;
}

static int mock_acquire(void *context, const struct pt_host_pair_binding *binding,
    const void *owner, const struct pt_host_pair_image *before)
{ return mock_mutate(context, ACQUIRE, binding, owner, before); }

static int mock_remove(void *context, const struct pt_host_pair_binding *binding,
    const void *owner, const struct pt_host_pair_image *before)
{ return mock_mutate(context, REMOVE_SERVER, binding, owner, before); }

static int mock_return(void *context, const struct pt_host_pair_binding *binding,
    const void *owner)
{ return mock_mutate(context, RETURN_RESOURCE, binding, owner, NULL); }

static void prepare(struct fixture *f)
{
    unsigned i, operation;
    memset(f, 0, sizeof(*f));
    f->input.source = &tokens[0];
    f->input.original_task = &tokens[1];
    f->input.timer = &tokens[2];
    f->input.port = &tokens[3];
    f->input.registration.owner = &tokens[4];
    f->input.registration.queue = &tokens[5];
    f->input.registration.session = UINT64_C(0x123456789);
    f->input.registration.generation = UINT64_C(0x987654321);
    for (i = 0; i < 2; ++i) {
        struct pt_host_pair_binding *binding = &f->input.binding[i];
        binding->registration = f->input.registration;
        binding->ticket = UINT64_C(0x1100000000) + i;
        binding->first = UINT64_C(0x2200000000) + (i ? 1537U : 0U);
        binding->frequency = 709379U;
        binding->resource = &tokens[6 + i];
        binding->server = &tokens[8 + i];
        binding->server_data = &tokens[10 + i];
        binding->server_code = i ? server_1 : server_0;
        binding->chip = i;
        binding->bit = i;
        f->input.before[i].programmed_latch = (uint16_t)(0x4311U + i * 0x101U);
        f->input.before[i].mask = (uint16_t)(0x0bU + i);
        f->input.before[i].pending = (uint16_t)(0x02U + i);
        f->input.before[i].control = (uint8_t)(0x40U + i);
        f->input.before[i].vector_data = &tokens[12 + i];
        f->input.before[i].vector_code = i ? old_vector_1 : old_vector_0;
        f->provider.binding[i] = *binding;
        f->provider.initial[i].image = f->input.before[i];
        f->provider.initial[i].available = 1;
        f->provider.initial[i].image_complete = 1;
        f->provider.initial[i].whole_exclusion = 1;
        f->provider.initial[i].full_return = 1;
        f->provider.actual[i] = f->provider.initial[i];
        for (operation = ACQUIRE; operation < OP_COUNT; ++operation) {
            f->provider.behavior[operation][i].result = PT_HOST_PAIR_EFFECT_DONE;
            f->provider.behavior[operation][i].perform_effect = 1;
        }
    }
    f->provider.owner = f->input.source;
    f->ops.context = &f->provider;
    f->ops.context_bytes = sizeof(f->provider);
    f->ops.inspect = mock_inspect;
    f->ops.acquire = mock_acquire;
    f->ops.remove_server = mock_remove;
    f->ops.return_resource = mock_return;
}

static void assert_identity(const struct fixture *f)
{
    unsigned i;
    assert(input_same(&f->transaction.input, &f->input));
    assert(input_same(&f->transaction.original, &f->input));
    for (i = 0; i < 2; ++i) {
        assert(binding_same(&f->transaction.record[i].binding, &f->input.binding[i]));
        assert(pt_host_pair_lookup(&f->transaction, &f->input.registration,
            f->input.binding[i].ticket) == &f->transaction.record[i]);
    }
}

static struct audit capture_audit(const struct fixture *f)
{
    struct audit audit;
    unsigned operation, i;
    memset(&audit, 0, sizeof(audit));
    for (operation = 0; operation < OP_COUNT; ++operation)
        for (i = 0; i < 2; ++i) audit.calls[operation][i] = f->provider.calls[operation][i];
    for (i = 0; i < 2; ++i) {
        audit.acquisitions[i] = f->provider.actual_acquisitions[i];
        audit.removals[i] = f->provider.actual_removals[i];
        audit.returns[i] = f->provider.actual_returns[i];
    }
    audit.trace_used = f->provider.trace_used;
    return audit;
}

static void assert_audit_same(const struct fixture *f, const struct audit *before,
    unsigned include_inspect)
{
    unsigned operation, i;
    for (operation = include_inspect ? INSPECT : ACQUIRE; operation < OP_COUNT; ++operation)
        for (i = 0; i < 2; ++i)
            assert(f->provider.calls[operation][i] == before->calls[operation][i]);
    for (i = 0; i < 2; ++i) {
        assert(f->provider.actual_acquisitions[i] == before->acquisitions[i]);
        assert(f->provider.actual_removals[i] == before->removals[i]);
        assert(f->provider.actual_returns[i] == before->returns[i]);
    }
    if (include_inspect) assert(f->provider.trace_used == before->trace_used);
}

static void assert_no_mutating_calls(const struct fixture *f)
{
    unsigned operation, i;
    for (operation = ACQUIRE; operation < OP_COUNT; ++operation)
        for (i = 0; i < 2; ++i) assert(f->provider.calls[operation][i] == 0);
    for (i = 0; i < 2; ++i) {
        assert(f->provider.actual_acquisitions[i] == 0);
        assert(f->provider.actual_removals[i] == 0);
        assert(f->provider.actual_returns[i] == 0);
    }
}

static void initialize(struct fixture *f)
{
    assert(pt_host_pair_init(&f->transaction, &f->input, &f->ops) == 1);
    assert_no_mutating_calls(f);
    assert_identity(f);
}

static void acquire_both(struct fixture *f)
{
    unsigned i;
    assert(pt_host_pair_acquire(&f->transaction) == PT_HOST_PAIR_ACQUIRED);
    assert_identity(f);
    for (i = 0; i < 2; ++i) {
        const struct pt_host_pair_snapshot *actual = &f->provider.actual[i];
        assert(f->provider.calls[ACQUIRE][i] == 1);
        assert(f->provider.actual_acquisitions[i] == 1);
        assert(f->provider.calls[REMOVE_SERVER][i] == 0);
        assert(f->provider.calls[RETURN_RESOURCE][i] == 0);
        assert(f->transaction.record[i].acquire == PT_HOST_PAIR_RECORDED_DONE);
        assert(actual->resource_held == 1 && actual->server_installed == 1);
        assert(actual->resource_owner == f->input.source && actual->server_owner == f->input.source);
        assert(actual->installed_server == f->input.binding[i].server);
        assert(actual->image.vector_data == f->input.binding[i].server_data);
        assert(actual->image.vector_code == f->input.binding[i].server_code);
    }
}

static void assert_effect_sequence(const struct fixture *f,
    const enum operation *operations, const unsigned *slots, unsigned expected_count)
{
    unsigned i, actual_count = 0;
    for (i = 0; i < f->provider.trace_used; ++i) {
        const struct trace_entry *entry = &f->provider.trace[i];
        if (entry->operation == INSPECT) continue;
        assert(actual_count < expected_count);
        assert(entry->operation == operations[actual_count]);
        assert(entry->slot == slots[actual_count]);
        assert(binding_same(&entry->binding, &f->input.binding[entry->slot]));
        assert(entry->owner == f->input.source);
        if (entry->operation != RETURN_RESOURCE)
            assert(image_same(&entry->supplied_before, &f->input.before[entry->slot]));
        ++actual_count;
    }
    assert(actual_count == expected_count);
}

static void assert_unknown_stops_effects(struct fixture *f)
{
    struct audit before = capture_audit(f), after;
    assert(f->transaction.fault == 1);
    assert(pt_host_pair_restore_once(&f->transaction, &f->input.registration,
        f->input.binding[0].ticket) == PT_HOST_PAIR_RETAINED);
    assert_audit_same(f, &before, 1);
    assert(pt_host_pair_close_once(&f->transaction) == PT_HOST_PAIR_RETAINED);
    assert_audit_same(f, &before, 1);
    after = capture_audit(f);
    assert(pt_host_pair_close_once(&f->transaction) == PT_HOST_PAIR_RETAINED);
    assert(pt_host_pair_restore_once(&f->transaction, &f->input.registration,
        f->input.binding[1].ticket) == PT_HOST_PAIR_RETAINED);
    assert_audit_same(f, &after, 1);
    assert_identity(f);
}

static void group_success_and_repeated_cleanup(void)
{
    struct fixture f;
    struct audit audit;
    static const enum operation operations[] = { ACQUIRE, ACQUIRE, REMOVE_SERVER,
        RETURN_RESOURCE, REMOVE_SERVER, RETURN_RESOURCE };
    static const unsigned slots[] = { 0, 1, 0, 0, 1, 1 };
    prepare(&f); initialize(&f); acquire_both(&f);
    assert(pt_host_pair_restore_once(&f.transaction, &f.input.registration,
        f.input.binding[0].ticket) == PT_HOST_PAIR_RESTORED);
    assert(snapshot_same(&f.provider.actual[0], &f.provider.initial[0]));
    assert(f.provider.actual[1].resource_held == 1 && f.provider.actual[1].server_installed == 1);
    assert(f.provider.actual_removals[0] == 1 && f.provider.actual_returns[0] == 1);
    audit = capture_audit(&f);
    assert(pt_host_pair_restore_once(&f.transaction, &f.input.registration,
        f.input.binding[0].ticket) == PT_HOST_PAIR_RESTORED);
    assert_audit_same(&f, &audit, 1);
    assert(pt_host_pair_close_once(&f.transaction) == PT_HOST_PAIR_RESTORED);
    audit = capture_audit(&f);
    assert(pt_host_pair_close_once(&f.transaction) == PT_HOST_PAIR_RESTORED);
    assert(pt_host_pair_restore_once(&f.transaction, &f.input.registration,
        f.input.binding[1].ticket) == PT_HOST_PAIR_RESTORED);
    assert_audit_same(&f, &audit, 1);
    assert(f.provider.actual_removals[1] == 1 && f.provider.actual_returns[1] == 1);
    assert(snapshot_same(&f.provider.actual[1], &f.provider.initial[1]));
    assert_effect_sequence(&f, operations, slots, 6);
    assert_identity(&f);
    ++passed_groups;
}

static void group_close_reverse_order(void)
{
    struct fixture f;
    struct audit audit;
    static const enum operation operations[] = { ACQUIRE, ACQUIRE, REMOVE_SERVER,
        RETURN_RESOURCE, REMOVE_SERVER, RETURN_RESOURCE };
    static const unsigned slots[] = { 0, 1, 1, 1, 0, 0 };
    prepare(&f); initialize(&f); acquire_both(&f);
    assert(pt_host_pair_close_once(&f.transaction) == PT_HOST_PAIR_RESTORED);
    assert_effect_sequence(&f, operations, slots, 6);
    assert(snapshot_same(&f.provider.actual[0], &f.provider.initial[0]));
    assert(snapshot_same(&f.provider.actual[1], &f.provider.initial[1]));
    audit = capture_audit(&f);
    assert(pt_host_pair_close_once(&f.transaction) == PT_HOST_PAIR_RESTORED);
    assert_audit_same(&f, &audit, 1);
    assert_identity(&f);
    ++passed_groups;
}

static void exercise_partial_rollback_after_second_refusal(void)
{
    struct fixture f;
    struct audit audit;
    static const enum operation remove_operations[] = { ACQUIRE, ACQUIRE, REMOVE_SERVER };
    static const unsigned remove_slots[] = { 0, 1, 0 };
    static const enum operation return_operations[] = { ACQUIRE, ACQUIRE,
        REMOVE_SERVER, RETURN_RESOURCE };
    static const unsigned return_slots[] = { 0, 1, 0, 0 };
    unsigned operation, variant;
    for (operation = REMOVE_SERVER; operation <= RETURN_RESOURCE; ++operation)
        for (variant = 0; variant < 3; ++variant) {
            prepare(&f); initialize(&f);
            f.provider.behavior[ACQUIRE][1].result = PT_HOST_PAIR_NO_EFFECTS;
            f.provider.behavior[ACQUIRE][1].perform_effect = 0;
            f.provider.behavior[operation][0].result = variant ? -47 : PT_HOST_PAIR_NO_EFFECTS;
            f.provider.behavior[operation][0].perform_effect = variant == 2;
            assert(pt_host_pair_acquire(&f.transaction) == PT_HOST_PAIR_RETAINED);
            /* The second no-effect refusal reaches the transaction's actual
             * rollback branch. Its sibling remains the complete initial image. */
            assert(snapshot_same(&f.provider.actual[1], &f.provider.initial[1]));
            assert(f.provider.calls[ACQUIRE][0] == 1 && f.provider.calls[ACQUIRE][1] == 1);
            assert(f.provider.actual_acquisitions[0] == 1 && f.provider.actual_acquisitions[1] == 0);
            assert(f.provider.calls[REMOVE_SERVER][0] == 1 && f.provider.calls[REMOVE_SERVER][1] == 0);
            assert(f.provider.calls[RETURN_RESOURCE][1] == 0);
            assert(f.provider.actual_removals[1] == 0 && f.provider.actual_returns[1] == 0);
            assert(f.transaction.record[0].acquire == PT_HOST_PAIR_RECORDED_DONE);
            assert(f.transaction.record[1].acquire == PT_HOST_PAIR_RECORDED_NO_EFFECTS);
            assert(f.transaction.record[0].restore_intent == 1);
            assert(f.transaction.record[1].restore_intent == 0);
            if (operation == REMOVE_SERVER) {
                assert(f.provider.actual_removals[0] == (variant == 2));
                assert(f.provider.calls[RETURN_RESOURCE][0] == 0);
                assert(f.provider.actual_returns[0] == 0);
                assert(f.transaction.record[0].remove_server == (variant ?
                    PT_HOST_PAIR_RECORDED_UNKNOWN : PT_HOST_PAIR_RECORDED_NO_EFFECTS));
                assert(f.transaction.record[0].return_resource == PT_HOST_PAIR_NOT_CALLED);
                assert(f.provider.actual[0].resource_held == 1);
                assert(f.provider.actual[0].resource_owner == f.input.source);
                assert(f.provider.actual[0].server_installed == (variant != 2));
                if (variant == 2)
                    assert(image_same(&f.provider.actual[0].image, &f.input.before[0]));
                assert_effect_sequence(&f, remove_operations, remove_slots, 3);
            } else {
                assert(f.provider.actual_removals[0] == 1);
                assert(f.provider.calls[RETURN_RESOURCE][0] == 1);
                assert(f.provider.actual_returns[0] == (variant == 2));
                assert(f.transaction.record[0].remove_server == PT_HOST_PAIR_RECORDED_DONE);
                assert(f.transaction.record[0].return_resource == (variant ?
                    PT_HOST_PAIR_RECORDED_UNKNOWN : PT_HOST_PAIR_RECORDED_NO_EFFECTS));
                assert(f.provider.actual[0].server_installed == 0);
                assert(image_same(&f.provider.actual[0].image, &f.input.before[0]));
                assert(f.provider.actual[0].resource_held == (variant != 2));
                if (variant == 2)
                    assert(snapshot_same(&f.provider.actual[0], &f.provider.initial[0]));
                assert_effect_sequence(&f, return_operations, return_slots, 4);
            }
            assert_identity(&f); /* Both exact records remain in caller storage. */
            audit = capture_audit(&f);
            assert(pt_host_pair_restore_once(&f.transaction, &f.input.registration,
                f.input.binding[0].ticket) == PT_HOST_PAIR_RETAINED);
            assert_audit_same(&f, &audit, 1);
            assert(pt_host_pair_close_once(&f.transaction) == PT_HOST_PAIR_RETAINED);
            assert_audit_same(&f, &audit, 1);
            assert(pt_host_pair_close_once(&f.transaction) == PT_HOST_PAIR_RETAINED);
            assert(pt_host_pair_restore_once(&f.transaction, &f.input.registration,
                f.input.binding[0].ticket) == PT_HOST_PAIR_RETAINED);
            assert_audit_same(&f, &audit, 1);
            assert(snapshot_same(&f.provider.actual[1], &f.provider.initial[1]));
            assert_identity(&f);
        }
}

static void group_no_effect_acquisition_refusals(void)
{
    struct fixture f;
    struct audit audit;
    static const enum operation first_operations[] = { ACQUIRE };
    static const unsigned first_slots[] = { 0 };
    static const enum operation second_operations[] = { ACQUIRE, ACQUIRE,
        REMOVE_SERVER, RETURN_RESOURCE };
    static const unsigned second_slots[] = { 0, 1, 0, 0 };
    unsigned refused_slot;
    for (refused_slot = 0; refused_slot < 2; ++refused_slot) {
        prepare(&f); initialize(&f);
        f.provider.behavior[ACQUIRE][refused_slot].result = PT_HOST_PAIR_NO_EFFECTS;
        f.provider.behavior[ACQUIRE][refused_slot].perform_effect = 0;
        assert(pt_host_pair_acquire(&f.transaction) == PT_HOST_PAIR_REFUSED);
        assert(f.transaction.record[refused_slot].acquire == PT_HOST_PAIR_RECORDED_NO_EFFECTS);
        assert(f.provider.actual_acquisitions[refused_slot] == 0);
        assert(snapshot_same(&f.provider.actual[0], &f.provider.initial[0]));
        assert(snapshot_same(&f.provider.actual[1], &f.provider.initial[1]));
        if (refused_slot == 0) {
            assert(f.transaction.record[1].acquire == PT_HOST_PAIR_NOT_CALLED);
            assert(f.provider.actual_removals[0] == 0 && f.provider.actual_returns[0] == 0);
            assert_effect_sequence(&f, first_operations, first_slots, 1);
        } else {
            assert(f.transaction.record[0].remove_server == PT_HOST_PAIR_RECORDED_DONE);
            assert(f.transaction.record[0].return_resource == PT_HOST_PAIR_RECORDED_DONE);
            assert(f.provider.actual_acquisitions[0] == 1);
            assert(f.provider.actual_removals[0] == 1 && f.provider.actual_returns[0] == 1);
            assert(f.provider.actual_removals[1] == 0 && f.provider.actual_returns[1] == 0);
            assert_effect_sequence(&f, second_operations, second_slots, 4);
        }
        audit = capture_audit(&f);
        assert(pt_host_pair_acquire(&f.transaction) == PT_HOST_PAIR_INVALID);
        assert_audit_same(&f, &audit, 1);
        (void)pt_host_pair_close_once(&f.transaction);
        assert_audit_same(&f, &audit, 1);
        audit = capture_audit(&f);
        (void)pt_host_pair_close_once(&f.transaction);
        assert_audit_same(&f, &audit, 1);
        assert_identity(&f);
    }
    exercise_partial_rollback_after_second_refusal();
    ++passed_groups;
}

static void group_remove_no_effect_and_unknown(void)
{
    struct fixture f;
    struct audit audit;
    unsigned variant;
    for (variant = 0; variant < 3; ++variant) {
        prepare(&f); initialize(&f); acquire_both(&f);
        f.provider.behavior[REMOVE_SERVER][0].result = variant ? -73 : PT_HOST_PAIR_NO_EFFECTS;
        f.provider.behavior[REMOVE_SERVER][0].perform_effect = variant == 2;
        assert(pt_host_pair_restore_once(&f.transaction, &f.input.registration,
            f.input.binding[0].ticket) == PT_HOST_PAIR_RETAINED);
        assert(f.provider.calls[REMOVE_SERVER][0] == 1);
        assert(f.provider.actual_removals[0] == (variant == 2));
        assert(f.provider.calls[RETURN_RESOURCE][0] == 0 && f.provider.actual_returns[0] == 0);
        assert(f.provider.actual[0].resource_held == 1);
        assert(f.provider.actual[0].resource_owner == f.input.source);
        assert(f.provider.actual[0].server_installed == (variant != 2));
        if (variant == 2) assert(image_same(&f.provider.actual[0].image, &f.input.before[0]));
        assert(f.provider.actual[1].resource_held == 1 && f.provider.actual[1].server_installed == 1);
        audit = capture_audit(&f);
        assert(pt_host_pair_restore_once(&f.transaction, &f.input.registration,
            f.input.binding[0].ticket) == PT_HOST_PAIR_RETAINED);
        assert_audit_same(&f, &audit, 1);
        if (variant) {
            assert(f.transaction.record[0].remove_server == PT_HOST_PAIR_RECORDED_UNKNOWN);
            assert_unknown_stops_effects(&f);
        } else {
            assert(f.transaction.record[0].remove_server == PT_HOST_PAIR_RECORDED_NO_EFFECTS);
            assert(pt_host_pair_close_once(&f.transaction) == PT_HOST_PAIR_RETAINED);
            /* Explicit close may clean the separately known sibling once. */
            assert(f.provider.calls[REMOVE_SERVER][0] == 1);
            assert(f.provider.calls[RETURN_RESOURCE][0] == 0);
            audit = capture_audit(&f);
            assert(pt_host_pair_close_once(&f.transaction) == PT_HOST_PAIR_RETAINED);
            assert_audit_same(&f, &audit, 1);
            assert_identity(&f);
        }
    }
    ++passed_groups;
}

static void group_resource_return_no_effect_and_unknown(void)
{
    struct fixture f;
    struct audit audit;
    unsigned variant;
    for (variant = 0; variant < 3; ++variant) {
        prepare(&f); initialize(&f); acquire_both(&f);
        f.provider.behavior[RETURN_RESOURCE][0].result = variant ? 7 : PT_HOST_PAIR_NO_EFFECTS;
        f.provider.behavior[RETURN_RESOURCE][0].perform_effect = variant == 2;
        assert(pt_host_pair_restore_once(&f.transaction, &f.input.registration,
            f.input.binding[0].ticket) == PT_HOST_PAIR_RETAINED);
        assert(f.transaction.record[0].remove_server == PT_HOST_PAIR_RECORDED_DONE);
        assert(f.provider.calls[REMOVE_SERVER][0] == 1 && f.provider.actual_removals[0] == 1);
        assert(f.provider.calls[RETURN_RESOURCE][0] == 1);
        assert(f.provider.actual_returns[0] == (variant == 2));
        assert(f.provider.actual[0].server_installed == 0);
        assert(image_same(&f.provider.actual[0].image, &f.input.before[0]));
        assert(f.provider.actual[0].resource_held == (variant != 2));
        assert(f.provider.actual[1].resource_held == 1 && f.provider.actual[1].server_installed == 1);
        audit = capture_audit(&f);
        assert(pt_host_pair_restore_once(&f.transaction, &f.input.registration,
            f.input.binding[0].ticket) == PT_HOST_PAIR_RETAINED);
        assert_audit_same(&f, &audit, 1);
        if (variant) {
            assert(f.transaction.record[0].return_resource == PT_HOST_PAIR_RECORDED_UNKNOWN);
            assert_unknown_stops_effects(&f);
        } else {
            assert(f.transaction.record[0].return_resource == PT_HOST_PAIR_RECORDED_NO_EFFECTS);
            assert(pt_host_pair_close_once(&f.transaction) == PT_HOST_PAIR_RETAINED);
            assert(f.provider.calls[REMOVE_SERVER][0] == 1);
            assert(f.provider.calls[RETURN_RESOURCE][0] == 1);
            audit = capture_audit(&f);
            assert(pt_host_pair_close_once(&f.transaction) == PT_HOST_PAIR_RETAINED);
            assert_audit_same(&f, &audit, 1);
            assert_identity(&f);
        }
    }
    ++passed_groups;
}

static void group_unknown_acquisitions(void)
{
    struct fixture f;
    unsigned unknown_slot, actual_effect;
    for (unknown_slot = 0; unknown_slot < 2; ++unknown_slot)
        for (actual_effect = 0; actual_effect < 2; ++actual_effect) {
            prepare(&f); initialize(&f);
            f.provider.behavior[ACQUIRE][unknown_slot].result = -91;
            f.provider.behavior[ACQUIRE][unknown_slot].perform_effect = actual_effect;
            assert(pt_host_pair_acquire(&f.transaction) == PT_HOST_PAIR_RETAINED);
            assert(f.transaction.record[unknown_slot].acquire == PT_HOST_PAIR_RECORDED_UNKNOWN);
            assert(f.provider.calls[ACQUIRE][0] == 1);
            assert(f.provider.calls[ACQUIRE][1] == unknown_slot);
            assert(f.provider.actual_acquisitions[unknown_slot] == actual_effect);
            if (unknown_slot == 1) assert(f.provider.actual_acquisitions[0] == 1);
            assert(f.provider.calls[REMOVE_SERVER][0] == 0 && f.provider.calls[REMOVE_SERVER][1] == 0);
            assert(f.provider.calls[RETURN_RESOURCE][0] == 0 && f.provider.calls[RETURN_RESOURCE][1] == 0);
            if (!actual_effect)
                assert(snapshot_same(&f.provider.actual[unknown_slot], &f.provider.initial[unknown_slot]));
            else assert(f.provider.actual[unknown_slot].resource_held == 1);
            assert_unknown_stops_effects(&f);
        }
    ++passed_groups;
}

static void group_false_claims_and_full_sibling_drift(void)
{
    struct fixture f;
    unsigned slot, claim, operation;
    /* A raw success without effects, or raw absence with real effects. */
    for (slot = 0; slot < 2; ++slot)
        for (claim = 0; claim < 2; ++claim) {
            prepare(&f); initialize(&f);
            f.provider.behavior[ACQUIRE][slot].result = claim ? PT_HOST_PAIR_NO_EFFECTS : PT_HOST_PAIR_EFFECT_DONE;
            f.provider.behavior[ACQUIRE][slot].perform_effect = claim;
            assert(pt_host_pair_acquire(&f.transaction) == PT_HOST_PAIR_RETAINED);
            assert(f.provider.actual_acquisitions[slot] == claim);
            assert(f.provider.calls[REMOVE_SERVER][0] == 0 && f.provider.calls[RETURN_RESOURCE][0] == 0);
            assert_unknown_stops_effects(&f);
        }
    for (operation = REMOVE_SERVER; operation <= RETURN_RESOURCE; ++operation)
        for (claim = 0; claim < 2; ++claim) {
            prepare(&f); initialize(&f); acquire_both(&f);
            f.provider.behavior[operation][0].result = claim ? PT_HOST_PAIR_NO_EFFECTS : PT_HOST_PAIR_EFFECT_DONE;
            f.provider.behavior[operation][0].perform_effect = claim;
            assert(pt_host_pair_restore_once(&f.transaction, &f.input.registration,
                f.input.binding[0].ticket) == PT_HOST_PAIR_RETAINED);
            assert(f.provider.calls[operation][0] == 1);
            if (operation == REMOVE_SERVER) {
                assert(f.provider.actual_removals[0] == claim);
                assert(f.provider.calls[RETURN_RESOURCE][0] == 0);
            } else {
                assert(f.provider.actual_removals[0] == 1);
                assert(f.provider.actual_returns[0] == claim);
            }
            assert_unknown_stops_effects(&f);
        }
    /* Only a non-enum sibling member changes. Every trace has both complete
     * sibling snapshots, so ownership booleans alone cannot hide this drift. */
    for (operation = ACQUIRE; operation <= RETURN_RESOURCE; ++operation) {
        struct pt_host_pair_snapshot sibling_before;
        prepare(&f); initialize(&f);
        if (operation != ACQUIRE) acquire_both(&f);
        sibling_before = f.provider.actual[1];
        f.provider.behavior[operation][0].drift_sibling = 1;
        if (operation == ACQUIRE)
            assert(pt_host_pair_acquire(&f.transaction) == PT_HOST_PAIR_RETAINED);
        else assert(pt_host_pair_restore_once(&f.transaction, &f.input.registration,
            f.input.binding[0].ticket) == PT_HOST_PAIR_RETAINED);
        assert(!snapshot_same(&sibling_before, &f.provider.actual[1]));
        assert(sibling_before.resource_owner == f.provider.actual[1].resource_owner);
        assert(sibling_before.server_installed == f.provider.actual[1].server_installed);
        assert(sibling_before.image.programmed_latch != f.provider.actual[1].image.programmed_latch);
        assert_unknown_stops_effects(&f);
    }
    ++passed_groups;
}

static void assert_selector_refused(struct fixture *f,
    const struct pt_host_pair_registration *registration, uint64_t ticket)
{
    struct audit audit = capture_audit(f);
    unsigned intent0 = f->transaction.record[0].restore_intent;
    unsigned intent1 = f->transaction.record[1].restore_intent;
    unsigned close_intent = f->transaction.close_intent;
    assert(pt_host_pair_lookup(&f->transaction, registration, ticket) == NULL);
    assert(pt_host_pair_restore_once(&f->transaction, registration, ticket) == PT_HOST_PAIR_INVALID);
    assert(f->transaction.record[0].restore_intent == intent0);
    assert(f->transaction.record[1].restore_intent == intent1);
    assert(f->transaction.close_intent == close_intent);
    assert_audit_same(f, &audit, 1);
}

static void group_foreign_zero_duplicate_and_sibling_binding_drift(void)
{
    struct fixture f;
    struct pt_host_pair_registration registration;
    unsigned field;
    prepare(&f); initialize(&f); acquire_both(&f);
    for (field = 0; field < 4; ++field) {
        registration = f.input.registration;
        if (field == 0) registration.owner = &tokens[20];
        if (field == 1) registration.queue = &tokens[21];
        if (field == 2) ++registration.session;
        if (field == 3) ++registration.generation;
        assert_selector_refused(&f, &registration, f.input.binding[0].ticket);
    }
    assert_selector_refused(&f, &f.input.registration, 0);
    assert_selector_refused(&f, &f.input.registration, UINT64_C(0x7700000000));
    assert_identity(&f);
    for (field = 0; field < 2; ++field) {
        prepare(&f);
        f.input.binding[1].ticket = field ? f.input.binding[0].ticket : 0;
        assert(pt_host_pair_init(&f.transaction, &f.input, &f.ops) == 0);
        assert_no_mutating_calls(&f);
        assert(f.provider.trace_used == 0);
    }
    prepare(&f);
    /* Different opaque resource tokens do not legitimize the same counter. */
    f.input.binding[1].chip = f.input.binding[0].chip;
    f.input.binding[1].bit = f.input.binding[0].bit;
    assert(f.input.binding[0].resource != f.input.binding[1].resource);
    assert(pt_host_pair_init(&f.transaction, &f.input, &f.ops) == 0);
    assert_no_mutating_calls(&f);
    assert(f.provider.trace_used == 0);
    prepare(&f);
    /* One resource token may represent two separately addressed counters. */
    f.input.binding[1].resource = f.input.binding[0].resource;
    f.input.binding[1].chip = f.input.binding[0].chip;
    f.input.binding[1].bit = 1U - f.input.binding[0].bit;
    f.provider.binding[1] = f.input.binding[1];
    initialize(&f); acquire_both(&f);
    assert(pt_host_pair_close_once(&f.transaction) == PT_HOST_PAIR_RESTORED);
    assert(f.provider.actual_acquisitions[0] == 1 && f.provider.actual_acquisitions[1] == 1);
    assert(f.provider.actual_removals[0] == 1 && f.provider.actual_removals[1] == 1);
    assert(f.provider.actual_returns[0] == 1 && f.provider.actual_returns[1] == 1);
    assert(snapshot_same(&f.provider.actual[0], &f.provider.initial[0]));
    assert(snapshot_same(&f.provider.actual[1], &f.provider.initial[1]));
    assert_identity(&f);
    /* Every copied sibling binding field participates in selector admission.
     * Test while slot zero is actually owned; refusal must precede intent. */
    for (field = 0; field < 14; ++field) {
        struct pt_host_pair_binding *sibling;
        prepare(&f); initialize(&f); acquire_both(&f);
        sibling = &f.transaction.record[1].binding;
        switch (field) {
        case 0: sibling->registration.owner = &tokens[20]; break;
        case 1: sibling->registration.queue = &tokens[21]; break;
        case 2: ++sibling->registration.session; break;
        case 3: ++sibling->registration.generation; break;
        case 4: sibling->ticket = 0; break;
        case 5: sibling->ticket = f.input.binding[0].ticket; break;
        case 6: ++sibling->first; break;
        case 7: sibling->frequency = 715909U; break;
        case 8: sibling->resource = &tokens[20]; break;
        case 9: sibling->server = &tokens[20]; break;
        case 10: sibling->server_data = &tokens[20]; break;
        case 11: sibling->server_code = foreign_server; break;
        case 12: sibling->chip = 0; break;
        case 13: sibling->bit = 0; break;
        }
        assert_selector_refused(&f, &f.input.registration, f.input.binding[0].ticket);
        assert(binding_same(&f.transaction.record[0].binding, &f.input.binding[0]));
        assert(input_same(&f.transaction.input, &f.input));
        assert(input_same(&f.transaction.original, &f.input));
    }
    ++passed_groups;
}

static void assert_reinit_refused_without_writes(struct fixture *f)
{
    unsigned char bytes[sizeof(f->transaction)];
    struct audit audit = capture_audit(f);
    memcpy(bytes, &f->transaction, sizeof(bytes));
    assert(pt_host_pair_init(&f->transaction, &f->input, &f->ops) == 0);
    assert(memcmp(bytes, &f->transaction, sizeof(bytes)) == 0);
    assert_audit_same(f, &audit, 1);
}

static void group_nonzero_and_retained_reinit(void)
{
    struct fixture f;
    prepare(&f);
    f.transaction.initial[1].delivery_queued = 1;
    assert_reinit_refused_without_writes(&f);
    assert_no_mutating_calls(&f);
    prepare(&f); initialize(&f);
    assert_reinit_refused_without_writes(&f);
    prepare(&f); initialize(&f);
    f.provider.behavior[ACQUIRE][0].result = PT_HOST_PAIR_EFFECT_UNKNOWN;
    f.provider.behavior[ACQUIRE][0].perform_effect = 1;
    assert(pt_host_pair_acquire(&f.transaction) == PT_HOST_PAIR_RETAINED);
    assert_reinit_refused_without_writes(&f);
    assert(f.provider.actual[0].resource_held == 1);
    assert_unknown_stops_effects(&f);
    ++passed_groups;
}

static void group_incomplete_facts_refuse_before_effects(void)
{
    struct fixture f;
    struct audit audit;
    unsigned slot, field, restore_slot;
    for (slot = 0; slot < 2; ++slot)
        for (field = 0; field < 6; ++field) {
            prepare(&f); initialize(&f);
            switch (field) {
            case 0: f.provider.actual[slot].image_complete = 0; break;
            case 1: f.provider.actual[slot].whole_exclusion = 0; break;
            case 2: f.provider.actual[slot].full_return = 0; break;
            case 3: f.provider.actual[slot].callbacks_inflight = 1; break;
            case 4: f.provider.actual[slot].delivery_queued = 1; break;
            case 5: ++f.provider.actual[slot].image.programmed_latch; break;
            }
            assert(pt_host_pair_acquire(&f.transaction) == PT_HOST_PAIR_REFUSED);
            assert_no_mutating_calls(&f);
            assert(f.transaction.record[0].acquire == PT_HOST_PAIR_NOT_CALLED);
            assert(f.transaction.record[1].acquire == PT_HOST_PAIR_NOT_CALLED);
            assert(f.transaction.record[0].restore_intent == 0);
            assert(f.transaction.record[1].restore_intent == 0);
            audit = capture_audit(&f);
            assert(pt_host_pair_acquire(&f.transaction) == PT_HOST_PAIR_INVALID);
            assert_audit_same(&f, &audit, 1);
            assert_identity(&f);
        }
    /* After both actual leases exist, every lost fact or image drift on either
     * the selected lease or its sibling blocks the very first removal. */
    for (restore_slot = 0; restore_slot < 2; ++restore_slot)
        for (slot = 0; slot < 2; ++slot)
            for (field = 0; field < 6; ++field) {
                struct pt_host_pair_snapshot provider_before[2];
                prepare(&f); initialize(&f); acquire_both(&f);
                switch (field) {
                case 0: f.provider.actual[slot].image_complete = 0; break;
                case 1: f.provider.actual[slot].whole_exclusion = 0; break;
                case 2: f.provider.actual[slot].full_return = 0; break;
                case 3: f.provider.actual[slot].callbacks_inflight = 1; break;
                case 4: f.provider.actual[slot].delivery_queued = 1; break;
                case 5: ++f.provider.actual[slot].image.programmed_latch; break;
                }
                provider_before[0] = f.provider.actual[0];
                provider_before[1] = f.provider.actual[1];
                audit = capture_audit(&f);
                assert(pt_host_pair_restore_once(&f.transaction, &f.input.registration,
                    f.input.binding[restore_slot].ticket) == PT_HOST_PAIR_RETAINED);
                assert(f.transaction.fault == 1);
                assert_audit_same(&f, &audit, 0);
                assert(snapshot_same(&provider_before[0], &f.provider.actual[0]));
                assert(snapshot_same(&provider_before[1], &f.provider.actual[1]));
                assert(f.transaction.record[0].remove_server == PT_HOST_PAIR_NOT_CALLED);
                assert(f.transaction.record[1].remove_server == PT_HOST_PAIR_NOT_CALLED);
                assert(f.transaction.record[0].return_resource == PT_HOST_PAIR_NOT_CALLED);
                assert(f.transaction.record[1].return_resource == PT_HOST_PAIR_NOT_CALLED);
                assert(f.provider.calls[REMOVE_SERVER][0] == 0 && f.provider.calls[REMOVE_SERVER][1] == 0);
                assert(f.provider.calls[RETURN_RESOURCE][0] == 0 && f.provider.calls[RETURN_RESOURCE][1] == 0);
                assert(f.provider.actual_removals[0] == 0 && f.provider.actual_removals[1] == 0);
                assert(f.provider.actual_returns[0] == 0 && f.provider.actual_returns[1] == 0);
                assert_unknown_stops_effects(&f);
                assert(snapshot_same(&provider_before[0], &f.provider.actual[0]));
                assert(snapshot_same(&provider_before[1], &f.provider.actual[1]));
            }
    ++passed_groups;
}

static void group_probes_are_read_only_and_cannot_clear_unknown(void)
{
    struct fixture f;
    struct pt_host_pair_snapshot observed, provider_before[2];
    unsigned char transaction_bytes[sizeof(f.transaction)];
    struct audit before;
    unsigned slot;
    prepare(&f); initialize(&f);
    f.provider.behavior[ACQUIRE][1].result = -31;
    f.provider.behavior[ACQUIRE][1].perform_effect = 1;
    assert(pt_host_pair_acquire(&f.transaction) == PT_HOST_PAIR_RETAINED);
    for (slot = 0; slot < 2; ++slot) {
        memcpy(transaction_bytes, &f.transaction, sizeof(transaction_bytes));
        provider_before[0] = f.provider.actual[0];
        provider_before[1] = f.provider.actual[1];
        before = capture_audit(&f);
        assert(pt_host_pair_probe(&f.transaction, &f.input.registration,
            f.input.binding[slot].ticket, &observed) == PT_HOST_PAIR_OBSERVED);
        assert(snapshot_same(&observed, &provider_before[slot]));
        assert(snapshot_same(&provider_before[0], &f.provider.actual[0]));
        assert(snapshot_same(&provider_before[1], &f.provider.actual[1]));
        assert(memcmp(transaction_bytes, &f.transaction, sizeof(transaction_bytes)) == 0);
        assert_audit_same(&f, &before, 0);
        assert(f.provider.calls[INSPECT][slot] == before.calls[INSPECT][slot] + 1);
        assert(f.provider.calls[INSPECT][1U - slot] == before.calls[INSPECT][1U - slot]);
        assert(f.provider.trace_used == before.trace_used + 1);
    }
    /* An external mock repair can report a pristine full-return snapshot.
     * Observation must still leave both recorded outcomes and sticky fault. */
    f.provider.actual[0] = f.provider.initial[0];
    f.provider.actual[1] = f.provider.initial[1];
    memcpy(transaction_bytes, &f.transaction, sizeof(transaction_bytes));
    before = capture_audit(&f);
    assert(pt_host_pair_probe(&f.transaction, &f.input.registration,
        f.input.binding[1].ticket, &observed) == PT_HOST_PAIR_OBSERVED);
    assert(snapshot_same(&observed, &f.provider.initial[1]));
    assert(memcmp(transaction_bytes, &f.transaction, sizeof(transaction_bytes)) == 0);
    assert_audit_same(&f, &before, 0);
    before = capture_audit(&f);
    assert(pt_host_pair_probe(&f.transaction, &f.input.registration, 0,
        &observed) == PT_HOST_PAIR_INVALID);
    assert_audit_same(&f, &before, 1);
    assert_unknown_stops_effects(&f);
    ++passed_groups;
}

int main(void)
{
    group_success_and_repeated_cleanup();
    group_close_reverse_order();
    group_no_effect_acquisition_refusals();
    group_remove_no_effect_and_unknown();
    group_resource_return_no_effect_and_unknown();
    group_unknown_acquisitions();
    group_false_claims_and_full_sibling_drift();
    group_foreign_zero_duplicate_and_sibling_binding_drift();
    group_nonzero_and_retained_reinit();
    group_incomplete_facts_refuse_before_effects();
    group_probes_are_read_only_and_cannot_clear_unknown();
    assert(passed_groups == 11);
    printf("HOST two-lease fixture assertion groups: %u\n", passed_groups);
    puts("HOST_TWO_LEASE_TRANSACTION_FIXTURE_SOURCE_V1_PASS");
    return 0;
}
