#ifndef PT_EDITOR_PAULA_READERS_PREPARE_H
#define PT_EDITOR_PAULA_READERS_PREPARE_H
#include "editor_mixed_establish.h"
#include "paula_readers_song.h"

enum pt_editor_readers_result {
    PT_EDITOR_READERS_INVALID=0,PT_EDITOR_READERS_PENDING,PT_EDITOR_READERS_DONE,
    PT_EDITOR_READERS_WAIT_ACTIVE,PT_EDITOR_READERS_WAIT_PRESSURE,
    PT_EDITOR_READERS_CANCELLED,PT_EDITOR_READERS_ESTABLISH_ERROR,
    PT_EDITOR_READERS_PRODUCER_ERROR,PT_EDITOR_READERS_FAULT,PT_EDITOR_READERS_CLOSED
};
enum pt_editor_readers_phase {
    PT_EDITOR_READERS_EMPTY=0,PT_EDITOR_READERS_MASTERS,PT_EDITOR_READERS_HANDOFF,
    PT_EDITOR_READERS_PRODUCER,PT_EDITOR_READERS_FAILED,PT_EDITOR_READERS_FINISHED
};
struct pt_editor_paula_readers_prepare_inputs {
    struct pt_editor_mixed *binding;
    struct pt_editor_mixed_establish *establish;
    struct pt_paula_readers_song_config config;
    struct pt_sampler_storage_span contexts;
    void *workspace;size_t workspace_capacity;
};
/* Opt-in HOST_ONLY software composition, serialized editor thread, single use.
 * Attach the genuine ordinary mixed binding first. All controls are readable,
 * addressable ordinary storage, zero-init where required, never copied or edited.
 * Keep inputs, establish, editor, binding, workspace and the WHOLE contexts span
 * alive/immutable until close returns1. contexts contains this controller and
 * every non-NULL allocator/Chip/backend opaque context with its entire extent;
 * backend.context_bytes is checked, other unknown extents remain caller duty.
 * Callback context starts must lie outside this controller and inputs. Other
 * establishment/binding/editor controls and source capacities are disjoint from
 * contexts and each other. Original immutable inputs may reside in contexts,
 * disjoint from this controller and every callback context's declared extent.
 * Workspace uses the genuine producer size/alignment and its entire declared
 * capacity is guarded before any write/callback. No prepared certificate.
 *
 * Begin adopts genuine cancellable 8/16/24 master establishment. One step does
 * one establishment step OR confirmed-stop + private hook + genuine workspace
 * producer construction without yielding OR one existing producer step. During
 * masters/handoff or any failed call the optional producer status stays unchanged. Work1..4096 is
 * the existing bounded item/byte contract, not a wall-clock or stack promise.
 * Completed masters remain sampler-owned and authoritative for saves.
 *
 * Explicit publish/services retain the original grid/session/absolute_start.
 * Each receipt service performs at most one backend callback. Publication uses
 * the existing one clock read + at most one submit; no timer or immediate start.
 * Failure/cancel latches; no auto retry/rebuild/forced release. Stop/edit/dispose
 * invokes producer cancel (no musical STOP/poll) then close. Submitted/uncertain
 * command and reader domains retain the private hook and veto edits/disposal
 * until independently exact proofs drain through explicit services. NULL receipt
 * outputs support stale-source draining. No direct underlying-owner calls.
 * Terminal get/step/close read only this controller after confirmed close.
 * No classic PLAY/UI/native output/MMIO/DMA/placement/timing/audio qualification.
 */
struct pt_editor_paula_readers_prepare;
struct pt_editor_readers_callback {struct pt_editor_paula_readers_prepare *owner;};
#define PT_EDITOR_READERS_ALLOCATIONS 66U
struct pt_editor_paula_readers_prepare {
    const struct pt_editor_paula_readers_prepare_inputs *inputs;
    struct pt_editor_paula_readers_prepare_inputs saved;
    struct pt_sampler_storage_span parents[3];
    struct pt_allocator original_allocator,allocator;
    struct pt_paula_readers_song_config producer_config;
    struct pt_paula_readers_song *producer;
    struct pt_editor_readers_callback callback;
    struct pt_project project_header;
    struct pt_sampler sampler_header;
    struct pt_editor *editor;
    struct pt_project *project;
    struct pt_sampler_storage_span allocation[PT_EDITOR_READERS_ALLOCATIONS];
    uint32_t revision,generation;
    unsigned phase,busy,closing,close_call,producer_adopted,reentries;
    enum pt_editor_readers_result result,first_error;
    enum pt_establish_result establish_result;
    enum pt_paula_readers_song_result producer_result;
};
enum pt_editor_readers_result pt_editor_paula_readers_prepare_begin(
    struct pt_editor_paula_readers_prepare *,const struct pt_editor_paula_readers_prepare_inputs *);
enum pt_editor_readers_result pt_editor_paula_readers_prepare_get(struct pt_editor_paula_readers_prepare *);
enum pt_editor_readers_result pt_editor_paula_readers_prepare_step(
    struct pt_editor_paula_readers_prepare *,unsigned,struct pt_paula_readers_song_status *);
enum pt_scheduled_result pt_editor_paula_readers_prepare_publish(struct pt_editor_paula_readers_prepare *);
enum pt_scheduled_result pt_editor_paula_readers_prepare_service_command(
    struct pt_editor_paula_readers_prepare *,unsigned,unsigned,struct pt_readers_command_receipt *);
enum pt_scheduled_result pt_editor_paula_readers_prepare_service_reader(
    struct pt_editor_paula_readers_prepare *,unsigned,unsigned,struct pt_readers_reader_receipt *);
enum pt_editor_readers_result pt_editor_paula_readers_prepare_terminal_stop(struct pt_editor_paula_readers_prepare *);
int pt_editor_paula_readers_prepare_close(struct pt_editor_paula_readers_prepare *);
#endif
