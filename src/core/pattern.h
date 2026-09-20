#ifndef PT_PATTERN_H
#define PT_PATTERN_H
#include "project.h"

struct pt_event_update { uint32_t index; struct pt_event event; };
struct pt_event_change { uint32_t index; struct pt_event before,after; };
/* External immutable resources can join the same chronological journal.
 * apply validates before mutation; discard releases only command ownership. */
struct pt_edit_resource {
    void *context;
    int (*apply)(void *,struct pt_project *,int);
    void (*discard)(void *);
};
enum pt_command_kind {PT_COMMAND_EVENTS,PT_COMMAND_CHANNEL,PT_COMMAND_RESOURCE};
struct pt_pattern_command {
    size_t offset,count;uint32_t before_revision,after_revision;
    uint8_t kind,channel;
    struct pt_channel channel_before,channel_after;
    struct pt_edit_resource resource;
};
struct pt_pattern_history {
    struct pt_event *bound_events;
    unsigned bound_channels,bound_patterns;
    struct pt_pattern_command *commands;
    struct pt_event_change *changes;
    size_t command_capacity,change_capacity,count,cursor,used;
    uint32_t revision,saved_revision,next_revision;
};
struct pt_block {struct pt_event *events;size_t capacity;uint8_t rows,channels;};
enum pt_edit_result {PT_EDIT_OK,PT_EDIT_INVALID,PT_EDIT_CAPACITY,PT_EDIT_END,
                     PT_EDIT_CONFLICT,PT_EDIT_UNSUPPORTED,PT_EDIT_ALIAS,PT_EDIT_PAULA_LIMIT,PT_EDIT_CANCELLED};

/* Bind history to one validated project's event storage. The caller serializes
 * editor/replayer access. Budgets are caller-owned memory, not a RAM assumption.
 * Changes apply atomically after validation; undo checks for outside edits.
 */
enum pt_edit_result pt_pattern_history_init(struct pt_pattern_history *,const struct pt_project *,
    struct pt_pattern_command *,size_t,struct pt_event_change *,size_t);
enum pt_edit_result pt_pattern_apply(struct pt_project *,struct pt_pattern_history *,
    const struct pt_event_update *,size_t);
/* Channel settings share chronological undo and dirty revisions with notes.
 * The candidate is copied before journal eviction; project fields may alias it. */
enum pt_edit_result pt_pattern_channel_apply(struct pt_project *,struct pt_pattern_history *,
    unsigned,const struct pt_channel *);
enum pt_edit_result pt_pattern_undo(struct pt_project *,struct pt_pattern_history *,int);
/* Ownership transfers only on success. apply returns nonzero on success. */
enum pt_edit_result pt_pattern_resource_apply(struct pt_project *,struct pt_pattern_history *,const struct pt_edit_resource *);
void pt_pattern_history_release(struct pt_pattern_history *);
void pt_pattern_mark_saved(struct pt_pattern_history *);
int pt_pattern_dirty(const struct pt_pattern_history *);
/* Half-open row/channel selections. Paste does not clip silently. */
enum pt_edit_result pt_pattern_copy(const struct pt_project *,unsigned,unsigned,unsigned,unsigned,unsigned,struct pt_block *);
enum pt_edit_result pt_pattern_paste(struct pt_project *,struct pt_pattern_history *,unsigned,unsigned,unsigned,
    const struct pt_block *,struct pt_event_update *,size_t);
enum pt_edit_result pt_pattern_transpose(struct pt_project *,struct pt_pattern_history *,unsigned,unsigned,unsigned,unsigned,unsigned,
    int,struct pt_event_update *,size_t);
enum pt_edit_result pt_pattern_clone(struct pt_project *,struct pt_pattern_history *,unsigned,unsigned,
    struct pt_event_update *,size_t);
#endif
