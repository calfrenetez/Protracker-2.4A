#ifndef PT_EDITOR_WORKFLOW_H
#define PT_EDITOR_WORKFLOW_H
#include "sampler_workflow.h"
#include "../core/event_resource.h"
#include "wave_summary.h"
#define PT_WORKFLOW_MANAGER 12
#define PT_WORKFLOW_TOOLBOX 13
#define PT_WORKFLOW_ROWS 10
/* Embedded in the editor: native editor allocation is authoritative Fast RAM.
 * All jobs run on the owner task between input polls, never in audio interrupts. */
struct pt_editor_workflow {
    struct pt_sample_usage_scan scan;
    struct pt_sample_usage_preview usage;
    struct pt_sampler_workflow *transaction;
    struct pt_sampler_workflow_stats stats;
    struct pt_event_resource_job resolver;
    struct pt_event_resource_result resource;
    struct pt_event_resource_origin return_origin;
    unsigned scanning,usage_valid,scan_for_copy,busy,pending_apply;
    unsigned highlight,first,sort,filter,searching,view_count,view_sample_count;
    const struct pt_sample *view_table;
    uint8_t view[PT_PROJECT_SAMPLES];
    char find[17];
    int (*recording_busy)(void *);void *recording_context;
    unsigned copy_slot;uint32_t copy_start,copy_end,copy_revision,copy_generation;
    struct pt_wave_summary_job wave_job;
    struct pt_wave_summary wave;
    struct pt_wave_summary_bin wave_bins[2][PT_WAVE_SUMMARY_BINS];
    unsigned wave_building,wave_buffer,wave_cancelled,source_generation;
    enum pt_flow_mode flow_mode;
    unsigned resolving,open_resource,resource_valid,return_valid,return_field,invocation_field;
    uint32_t navigation_revision,navigation_generation;
};
#endif
