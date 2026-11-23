#include "routed_preflight.h"
#include "../core/render_storage_internal.h"
#include <string.h>

enum phase { MEASURE, NEXT, CONSUME, COMPLETE, PLAN_BEGIN, PLAN_STEP, PASSED, FAILED, TAKEN };

struct pt_routed_preflight {
    struct pt_routed_preflight *self;
    struct pt_routed_preflight **owner;
    size_t capacity;
    struct pt_routed_preflight_inputs inputs;
    struct pt_project project_header;
    struct pt_render_sequence *sequence;
    struct pt_render_interval interval;
    struct pt_routed_render_plan next;
    struct pt_mixed_plan_origin origins[PT_MIXED_PLAN_RECORDS];
    struct pt_mixed_plan_inputs plan_inputs;
    struct pt_mixed_plan_normalizer *normalizer;
    struct pt_mixed_plan_quantized_batch batch;
    struct pt_routed_preflight_report report;
    enum phase phase;
    unsigned closing;
};

static int apart(const void *left, size_t left_bytes, const void *right, size_t right_bytes)
{
    uintptr_t a = (uintptr_t)left, b = (uintptr_t)right;
    return a && b && left_bytes <= UINTPTR_MAX - a && right_bytes <= UINTPTR_MAX - b &&
        (a + left_bytes <= b || b + right_bytes <= a);
}

static int owns(const struct pt_routed_preflight *audit)
{
    return audit && audit->self == audit && !audit->closing && audit->owner && *audit->owner == audit;
}

static int current(const struct pt_routed_preflight *audit, uint32_t revision, uint32_t generation)
{
    const size_t cursor = offsetof(struct pt_project, channels) + offsetof(struct pt_channels, selected);
    const size_t after = cursor + sizeof(audit->project_header.channels.selected);
    const struct pt_project *project = audit->inputs.project;
    const unsigned char *actual = (const unsigned char *)project;
    const unsigned char *original = (const unsigned char *)&audit->project_header;
    return revision == audit->inputs.revision && generation == audit->inputs.generation &&
        project->channels.selected < project->channels.count && !memcmp(actual, original, cursor) &&
        !memcmp(actual + after, original + after, sizeof(*project) - after);
}

static int output_apart(const struct pt_routed_preflight *audit, const void *out, size_t bytes)
{
    return apart(audit, audit->capacity, out, bytes) &&
        apart(audit->owner, sizeof(*audit->owner), out, bytes) &&
        apart(audit->inputs.normalizer_workspace, audit->inputs.normalizer_capacity, out, bytes) &&
        pt_render_project_storage_output_disjoint(audit->inputs.project, out, bytes) &&
        (!audit->sequence || pt_render_sequence_control_output_disjoint(audit->sequence, out, bytes));
}

size_t pt_routed_preflight_workspace_size(void)
{
    return sizeof(struct pt_routed_preflight);
}

size_t pt_routed_preflight_workspace_alignment(void)
{
    return _Alignof(struct pt_routed_preflight);
}

enum pt_routed_preflight_result pt_routed_preflight_begin(void *storage, size_t capacity,
    const struct pt_routed_preflight_inputs *inputs, struct pt_routed_preflight **out)
{
    struct pt_routed_preflight *audit = storage;
    struct pt_render_sequence *sequence = NULL;
    size_t i;
    const unsigned char *bytes = storage;

    if (capacity < sizeof(*audit) || (uintptr_t)audit % _Alignof(struct pt_routed_preflight) ||
        (uintptr_t)inputs % _Alignof(struct pt_routed_preflight_inputs) ||
        (uintptr_t)out % _Alignof(struct pt_routed_preflight *) ||
        !apart(storage, capacity, inputs, sizeof(*inputs)) || !apart(storage, capacity, out, sizeof(*out)) ||
        !apart(inputs, sizeof(*inputs), out, sizeof(*out)))
        return PT_ROUTED_PREFLIGHT_INVALID;
    if (inputs->normalizer_capacity < pt_mixed_plan_normalizer_workspace_size() ||
        (uintptr_t)inputs->normalizer_workspace % pt_mixed_plan_normalizer_workspace_alignment() ||
        !apart(storage, capacity, inputs->normalizer_workspace, inputs->normalizer_capacity) ||
        !apart(inputs, sizeof(*inputs), inputs->normalizer_workspace, inputs->normalizer_capacity) ||
        !apart(out, sizeof(*out), inputs->normalizer_workspace, inputs->normalizer_capacity) ||
        !pt_render_project_storage_output_disjoint(inputs->project, inputs, sizeof(*inputs)) ||
        !pt_render_project_storage_output_disjoint(inputs->project, storage, capacity) ||
        !pt_render_project_storage_output_disjoint(inputs->project, out, sizeof(*out)) ||
        !pt_render_project_storage_output_disjoint(inputs->project, inputs->normalizer_workspace,
                                                   inputs->normalizer_capacity) ||
        !pt_paula_render_caps_valid(&inputs->caps) || inputs->options.row_range ||
        (inputs->options.rate != 44100 && inputs->options.rate != 48000) ||
        (inputs->format.bits != 8 && inputs->format.bits != 16) || inputs->format.channel ||
        inputs->format.word_pad || inputs->format.little_endian > 1 || *out)
        return PT_ROUTED_PREFLIGHT_INVALID;
    for (i = 0; i < sizeof(*audit); ++i)
        if (bytes[i])
            return PT_ROUTED_PREFLIGHT_INVALID;

    audit->self = audit;
    if (pt_render_sequence_begin_routed(inputs->project, &inputs->options, &inputs->allocator,
                                       &sequence) != PT_RENDER_OK) {
        memset(audit, 0, sizeof(*audit));
        return PT_ROUTED_PREFLIGHT_REFUSED;
    }
    audit->owner = out;
    audit->capacity = capacity;
    audit->inputs = *inputs;
    memcpy(&audit->project_header, inputs->project, sizeof(audit->project_header));
    audit->sequence = sequence;
    *out = audit;
    return PT_ROUTED_PREFLIGHT_PENDING;
}

static void complete_boundary(struct pt_routed_preflight *audit)
{
    audit->report.render = pt_render_sequence_complete_routed(audit->sequence, &audit->next);
    if (audit->report.render != PT_RENDER_OK)
        return;
    audit->report.frame = audit->next.frame;
    audit->phase = audit->next.end ? PASSED : NEXT;
    if (audit->next.audio.count)
        audit->phase = PLAN_BEGIN;
}

static void begin_plan(struct pt_routed_preflight *audit)
{
    memset(&audit->report.plan, 0, sizeof(audit->report.plan));
    audit->plan_inputs = (struct pt_mixed_plan_inputs){
        .project = audit->inputs.project, .plan = &audit->next.audio, .origins = audit->origins,
        .caps = &audit->inputs.caps, .format = &audit->inputs.format, .rate = audit->inputs.options.rate,
        .frame = audit->next.frame, .revision = audit->inputs.revision, .generation = audit->inputs.generation
    };
    audit->report.plan.result = pt_mixed_plan_normalizer_begin_quantized_in_workspace(
        audit->inputs.normalizer_workspace, audit->inputs.normalizer_capacity, &audit->plan_inputs,
        &audit->normalizer);
    audit->phase = audit->report.plan.result == PT_MIXED_PLAN_PENDING ? PLAN_STEP : FAILED;
}

static void audit_plan(struct pt_routed_preflight *audit, unsigned work)
{
    enum pt_mixed_plan_result result = pt_mixed_plan_normalizer_step(audit->normalizer,
        audit->inputs.revision, audit->inputs.generation, work);
    if (result == PT_MIXED_PLAN_PENDING)
        return;
    if (result == PT_MIXED_PLAN_READY)
        result = pt_mixed_plan_normalizer_get_quantized(audit->normalizer, audit->inputs.revision,
            audit->inputs.generation, &audit->batch);
    (void)pt_mixed_plan_normalizer_report(audit->normalizer, &audit->report.plan);
    audit->report.plan.result = result;
    if (!pt_mixed_plan_normalizer_close(&audit->normalizer) || result != PT_MIXED_PLAN_READY) {
        audit->phase = FAILED;
        return;
    }
    memcpy(audit->origins, audit->batch.normalized.next, sizeof(audit->origins));
    ++audit->report.audio_boundaries;
    audit->phase = audit->next.end ? PASSED : NEXT;
}

enum pt_routed_preflight_result pt_routed_preflight_step(struct pt_routed_preflight *audit,
    uint32_t revision, uint32_t generation, unsigned work, struct pt_routed_preflight_report *report)
{
    unsigned ready = 0;
    uint32_t frames;
    if (!owns(audit) || !work || work > 256 || audit->phase == TAKEN)
        return PT_ROUTED_PREFLIGHT_INVALID;
    if (!current(audit, revision, generation))
        return PT_ROUTED_PREFLIGHT_STALE;
    if (report && ((uintptr_t)report % _Alignof(struct pt_routed_preflight_report) ||
                   !output_apart(audit, report, sizeof(*report))))
        return PT_ROUTED_PREFLIGHT_INVALID;

    switch (audit->phase) {
    case MEASURE:
        audit->report.render = pt_render_sequence_prepare(audit->sequence, work, &ready);
        if (ready)
            audit->phase = NEXT;
        break;
    case NEXT:
        audit->report.render = pt_render_sequence_next(audit->sequence, &audit->interval);
        audit->phase = audit->interval.frames ? CONSUME : COMPLETE;
        break;
    case CONSUME:
        frames = audit->interval.frames > work ? work : audit->interval.frames;
        audit->report.render = pt_render_sequence_consume(audit->sequence, frames);
        audit->interval.frames -= frames;
        if (!audit->interval.frames)
            audit->phase = COMPLETE;
        break;
    case COMPLETE:
        complete_boundary(audit);
        break;
    case PLAN_BEGIN:
        begin_plan(audit);
        break;
    case PLAN_STEP:
        audit_plan(audit, work);
        break;
    case PASSED:
    case FAILED:
    case TAKEN:
        break;
    }
    if (audit->report.render != PT_RENDER_OK)
        audit->phase = FAILED;
    if (report)
        *report = audit->report;
    if (audit->phase == PASSED)
        return PT_ROUTED_PREFLIGHT_READY;
    return audit->phase == FAILED ? PT_ROUTED_PREFLIGHT_REFUSED : PT_ROUTED_PREFLIGHT_PENDING;
}

int pt_routed_preflight_take(struct pt_routed_preflight *audit, uint32_t revision, uint32_t generation,
    struct pt_render_sequence **out)
{
    if (!owns(audit) || audit->phase != PASSED || !current(audit, revision, generation) ||
        (uintptr_t)out % _Alignof(struct pt_render_sequence *) || !output_apart(audit, out, sizeof(*out)) || *out)
        return 0;
    if (pt_render_sequence_rewind(audit->sequence) != PT_RENDER_OK) {
        audit->phase = FAILED;
        return 0;
    }
    *out = audit->sequence;
    audit->sequence = NULL;
    audit->phase = TAKEN;
    return 1;
}

int pt_routed_preflight_close(struct pt_routed_preflight **slot)
{
    struct pt_routed_preflight *audit;
    if (!slot || (uintptr_t)slot % _Alignof(struct pt_routed_preflight *))
        return 0;
    audit = *slot;
    if (!audit)
        return 1;
    if (!owns(audit) || audit->owner != slot || !pt_mixed_plan_normalizer_close(&audit->normalizer))
        return 0;
    audit->closing = 1;
    *slot = NULL;
    pt_render_sequence_close(audit->sequence);
    memset(audit, 0, sizeof(*audit));
    return 1;
}
