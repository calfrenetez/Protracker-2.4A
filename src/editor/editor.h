#ifndef PT_EDITOR_H
#define PT_EDITOR_H
#include "pattern.h"
#define PT_EDITOR_ROWS 20
enum pt_editor_action {PT_UI_NONE,PT_UI_SAVE,PT_UI_QUIT};
struct pt_editor {
    struct pt_project *project;
    struct pt_pattern_history history;
    struct pt_pattern_command commands[128];
    struct pt_event_change changes[2048];
    unsigned pattern,row,first_row,field,sample,octave,editing,quit_pending,position,panel;
    char status[76];
};
/* Initialize only after a complete project load; no audio backend is invoked. */
int pt_editor_init(struct pt_editor *,struct pt_project *);
enum pt_editor_action pt_editor_key(struct pt_editor *,unsigned raw,unsigned qualifier);
enum pt_editor_action pt_editor_click(struct pt_editor *,int x,int y);
void pt_editor_status(struct pt_editor *,const char *);
void pt_editor_saved(struct pt_editor *);
int pt_editor_dirty(const struct pt_editor *);
void pt_editor_note(const struct pt_event *,char out[4]);
#endif
