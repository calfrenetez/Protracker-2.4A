#ifndef PT_EDITOR_MIDI_SETUP_H
#define PT_EDITOR_MIDI_SETUP_H
#include "editor.h"
#include "../platform/midi_endpoints.h"

struct pt_midi_setup;
enum pt_midi_setup_target {PT_MIDI_SETUP_INPUT=1,PT_MIDI_SETUP_OUTPUT,PT_MIDI_SETUP_CLOCK};
enum pt_midi_setup_result {PT_MIDI_SETUP_OK=0,PT_MIDI_SETUP_INVALID,PT_MIDI_SETUP_BUSY,
    PT_MIDI_SETUP_STALE,PT_MIDI_SETUP_ACTIVE,PT_MIDI_SETUP_VETO,PT_MIDI_SETUP_CAPACITY,
    PT_MIDI_SETUP_CONFLICT,PT_MIDI_SETUP_IN_USE,PT_MIDI_SETUP_HELD};
enum pt_midi_setup_presence {PT_MIDI_SETUP_UNCONFIGURED=0,PT_MIDI_SETUP_UNKNOWN,PT_MIDI_SETUP_MISSING,PT_MIDI_SETUP_PRESENT};
/* Copied presentation data only, never connection/activation authority. Input
 * names must describe clusters with senders; output names clusters with receivers.
 * The current receiver-only CAMD control registry provides NO input inventory.
 * Unknown direction requires count0; configured names remain visible/missing.
 * No native discovery/library operation is invoked by any function here. */
struct pt_midi_setup_discovery {
    unsigned input_known,output_known,input_count,output_count;
    struct pt_midi_endpoint_name input[PT_MIDI_PORTS],output[PT_MIDI_PORTS];
};
struct pt_midi_setup_view {
    char input[PT_MIDI_ENDPOINT],output[PT_MIDI_ENDPOINT];
    uint32_t flags;unsigned track,midi_channel,route;
    enum pt_midi_setup_presence input_presence,output_presence;
};
/* owned_bytes counts only initialized exclusively owned journal records. A
 * returned known-alias allocation is a separate retained uncertain obligation;
 * get returns HELD without writing an observation in that state. */
struct pt_midi_setup_status {size_t owned_bytes,budget;unsigned commands,draft;};

/* Serialized original editor-task metadata controller, not a MIDI backend.
 * Supply zeroed aligned complete storage, original initialized editor/project,
 * and a separate caller policy byte budget for actual fixed journal allocations.
 * The genuine editor sampler allocator is captured; its context and this owner
 * outlive ALL retained journal records, including undone redo entries. Release
 * the editor journal before close; never reinitialize/replace editor/project/
 * history/controller at the same addresses while a draft/record survives. Pointer,
 * shape and revision checks detect ordinary changes, not same-address ABA.
 * All sources, allocator contexts and owner storage remain alive; callbacks are
 * synchronous, serialized, nonreentrant and return fresh disjoint aligned blocks.
 * A returned known alias is retained HELD, never initialized or freed as fresh.
 * No PCM/master accounting is changed; this budget is ordinary metadata storage.
 */
size_t pt_midi_setup_size(void);
size_t pt_midi_setup_alignment(void);
size_t pt_midi_setup_command_bytes(void);
enum pt_midi_setup_result pt_midi_setup_init(struct pt_midi_setup *,size_t,
    struct pt_editor *,size_t history_byte_budget);
/* Begin/edit/cancel only affect the draft. Names include exactly their final
 * NUL, 1..64 bytes; empty means no configured endpoint. No silent truncation.
 * Clock supports only existing saved bits0/1. OUTPUT uses an active track index
 * but never changes its P/A/M route, MIDI channel or any other/inactive slot. */
enum pt_midi_setup_result pt_midi_setup_begin(struct pt_midi_setup *,enum pt_midi_setup_target,unsigned track);
enum pt_midi_setup_result pt_midi_setup_name(struct pt_midi_setup *,const char *,size_t);
enum pt_midi_setup_result pt_midi_setup_flags(struct pt_midi_setup *,uint32_t);
enum pt_midi_setup_result pt_midi_setup_cancel(struct pt_midi_setup *);
/* Invalid/stale/no-op drafts never call the editor hook or allocate. A visible
 * same name is a no-op even if its existing bytes after NUL are nonzero. Actual
 * changes require stopped playback, no active workflow/recording operation,
 * existing positive editor change admission
 * and a fresh post-hook snapshot check. No automatic Stop, queued Apply, open,
 * connect, route change, Program Change or message is supplied. External live
 * MIDI ownership must be covered by that existing editor barrier; playback.active
 * alone is not proof of external quiet. One real chronological resource command
 * stores full64 before/after bytes (or flags) and refuses outside-edit conflicts.
 */
enum pt_midi_setup_result pt_midi_setup_commit(struct pt_midi_setup *);
enum pt_midi_setup_result pt_midi_setup_get(struct pt_midi_setup *,struct pt_midi_setup_status *);
enum pt_midi_setup_result pt_midi_setup_view(struct pt_midi_setup *,unsigned,
    const struct pt_midi_setup_discovery *,struct pt_midi_setup_view *);
/* Read only original controller/record ownership, not former editor/project or
 * released history. IN_USE while any actual journal record survives. No release,
 * forced history clear or recovery for HELD. Successful close consumes owner. */
enum pt_midi_setup_result pt_midi_setup_close(struct pt_midi_setup *);
#endif
