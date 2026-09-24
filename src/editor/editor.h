#ifndef PT_EDITOR_H
#define PT_EDITOR_H
#include "pattern.h"
#include "playback.h"
#include "sampler.h"
#include "song.h"
#include "render.h"
#include "recent.h"
#define PT_EDITOR_ROWS 20
/* One grid for parameter rows, command rows and their mouse targets. */
#define PT_EDITOR_CONTROL_Y 2
#define PT_EDITOR_CONTROL_HEIGHT 19
#define PT_EDITOR_COMMAND_BOTTOM (PT_EDITOR_CONTROL_Y+5*PT_EDITOR_CONTROL_HEIGHT)
#define PT_EDITOR_SCOPE_Y (PT_EDITOR_COMMAND_BOTTOM+PT_EDITOR_CONTROL_HEIGHT)
#define PT_EDITOR_SCOPE_BOTTOM (PT_EDITOR_CONTROL_Y+8*PT_EDITOR_CONTROL_HEIGHT)
#define PT_EDITOR_HEADER_Y 234
#define PT_EDITOR_PATTERN_Y 250
#define PT_EDITOR_BOTTOM_Y 491
enum pt_editor_action {PT_UI_NONE,PT_UI_SAVE,PT_UI_QUIT,PT_UI_PLAY,PT_UI_PATTERN,PT_UI_STOP,PT_UI_AUDITION,PT_UI_LOAD,PT_UI_SAVE_AS,PT_UI_EXPORT_MOD,PT_UI_NEW,PT_UI_SAMPLE_LOAD,PT_UI_SAMPLE_SAVE,PT_UI_SAMPLE_SVX,PT_UI_RAW_LOAD,PT_UI_RAW_SAVE,PT_UI_SOURCE_LOAD,PT_UI_RENDER,PT_UI_BOUNCE,PT_UI_STEMS,PT_UI_RECENT_LOAD,PT_UI_RECENT_REMOVE,PT_UI_RECENT_CLEAR,PT_UI_EXPORT_MOD8};
struct pt_editor_selection {unsigned active,marking,pattern,r0,r1,c0,c1,anchor_row,anchor_channel;};
struct pt_editor {
    struct pt_project *project;
    void (*before_change)(void *);
    void *before_change_context;
    struct pt_pattern_history history;
    struct pt_sampler sampler;
    struct pt_song song;
    const struct pt_recent *recent; /* Owned by native application, survives project replacement. */
    unsigned recent_details,recent_selected,export_details,export_dither;
    unsigned song_details,song_tools;
    unsigned render_details,render_pattern,render_bits,render_lead_in,render_groups;
    uint32_t render_rate,render_gain;
    uint16_t render_tracks;
    unsigned render_range,render_first,render_end,render_range_pattern;
    unsigned sample_range_slot,sample_marking;
    uint32_t sample_start,sample_end,sample_anchor;
    uint32_t wave_start,wave_end,wave_frames;
    unsigned wave_slot,number_field,number_fresh;
    char number_text[11];
    unsigned channel_details,note_details,name_entry,name_fresh;
    char name_text[PT_PROJECT_NAME];
    struct pt_document sample_source;
    unsigned source_selected;
    struct pt_raw_format raw_format;
    unsigned format_slot,format_bits,format_filtered;
    uint32_t format_rate;
    uint32_t loop_fade,slice_markers[4096];
    size_t slice_count;
    unsigned slice_pending,slice_slot,slice_generation,slice_threshold,slice_gap_ms,slice_zero,sample_ui;
    struct pt_pattern_command commands[128];
    struct pt_event_change changes[2048];
    unsigned pattern,row,first_row,field,sample,octave,editing,quit_pending,position,panel,load_pending,new_pending,new_channels;
    /* Bounded clipboard/workspace live in the heap-allocated editor, not the
       Amiga stack. A whole 64-row/16-channel pattern fits one undo command. */
    struct pt_editor_selection selection;
    struct pt_block clipboard;
    struct pt_event clipboard_events[1024];
    struct pt_event_update scratch[1024];
    struct pt_playback playback;
    char status[76];
};
/* Initialize only after a complete project load; no audio backend is invoked. */
void pt_editor_render_options(const struct pt_editor *,struct pt_render_options *);
int pt_editor_init(struct pt_editor *,struct pt_project *);
/* Dispose before reinitializing or freeing a live editor. Releases editor-owned replacement arrays;
   the project must no longer be used. Release/destroy its document separately. */
void pt_editor_dispose(struct pt_editor *);
/* Owner-thread synchronous playback release hook. Called before editor project
 * mutations and disposal (also on a refused mutation). Must be idempotent, must
 * not reenter editor mutation, and its context must outlive dispose. Init clears
 * the hook. Native/external mutations must call prepare_change themselves BEFORE
 * changing/releasing project storage. No backend is started by this API. */
void pt_editor_change_guard(struct pt_editor *,void (*)(void *),void *);
void pt_editor_prepare_change(struct pt_editor *);
void pt_editor_sample_all(struct pt_editor *);
/* Synchronous MOD loader into an empty unpublished document; honor budget and
 * allocator, finish source I/O before success, never mutate/reenter the editor.
 * Failure releases staging and preserves the previous source and destination. */
typedef enum pt_project_result (*pt_source_loader)(void *,struct pt_document *,size_t);
enum pt_edit_result pt_editor_source_load_with(struct pt_editor *,pt_source_loader,void *);
enum pt_edit_result pt_editor_source_load(struct pt_editor *,const uint8_t *,size_t);
void pt_editor_wave_bounds(const struct pt_editor *,uint32_t *,uint32_t *);
void pt_editor_sample_result(struct pt_editor *,enum pt_edit_result);
enum pt_editor_action pt_editor_key(struct pt_editor *,unsigned raw,unsigned qualifier);
enum pt_editor_action pt_editor_click(struct pt_editor *,int x,int y);
void pt_editor_status(struct pt_editor *,const char *);
void pt_editor_saved(struct pt_editor *);
int pt_editor_dirty(const struct pt_editor *);
/* Follow normal playback without moving an active edit cursor or audition. */
void pt_editor_follow_playback(struct pt_editor *);
/* Current half-open selection, normalized across reverse/cross-page marking. */
int pt_editor_selection(const struct pt_editor *,struct pt_editor_selection *);
void pt_editor_note(const struct pt_event *,char out[4]);
#endif
