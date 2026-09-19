#ifndef PT_EDITOR_H
#define PT_EDITOR_H
#include "pattern.h"
#include "playback.h"
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
enum pt_editor_action {PT_UI_NONE,PT_UI_SAVE,PT_UI_QUIT,PT_UI_PLAY,PT_UI_PATTERN,PT_UI_STOP,PT_UI_AUDITION,PT_UI_LOAD,PT_UI_SAVE_AS,PT_UI_EXPORT_MOD,PT_UI_NEW};
struct pt_editor_selection {unsigned active,marking,pattern,r0,r1,c0,c1,anchor_row,anchor_channel;};
struct pt_editor {
    struct pt_project *project;
    struct pt_pattern_history history;
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
int pt_editor_init(struct pt_editor *,struct pt_project *);
enum pt_editor_action pt_editor_key(struct pt_editor *,unsigned raw,unsigned qualifier);
enum pt_editor_action pt_editor_click(struct pt_editor *,int x,int y);
void pt_editor_status(struct pt_editor *,const char *);
void pt_editor_saved(struct pt_editor *);
int pt_editor_dirty(const struct pt_editor *);
/* Current half-open selection, normalized across reverse/cross-page marking. */
int pt_editor_selection(const struct pt_editor *,struct pt_editor_selection *);
void pt_editor_note(const struct pt_event *,char out[4]);
#endif
