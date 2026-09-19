#ifndef PT_EDITOR_VIEW_H
#define PT_EDITOR_VIEW_H
#include "editor.h"
#define PT_VIEW_WIDTH 640
#define PT_VIEW_HEIGHT 512
#define PT_VIEW_PLANE_BYTES (80UL*512)
/* Four ordinary non-interleaved 640x512 bitplanes, 80 bytes per row. */
struct pt_canvas {uint8_t *planes[4];};
#define PT_VIEW_DIRTY_MAX 28
struct pt_view_rect {unsigned x,y,width,height;};
struct pt_view_cache {
    unsigned valid,page,pattern,first_row,position,sample,editing,panel,row,field,selected,dirty,new_channels,new_pending;
    size_t sample_bytes;
    struct pt_project project;
    struct pt_sample sample_meta;
    struct pt_event events[PT_EDITOR_ROWS][4];
    struct pt_playback playback;
    struct pt_editor_selection selection;
    char status[76];
};
/* Zero-initialize cache, and invalidate it after exposure/modal dialogs.
   The returned rectangles cover every changed pixel. Playback ticks can also
   use pt_editor_draw_playback directly; its next cached update is harmless. */
unsigned pt_editor_draw_update(const struct pt_editor *,struct pt_canvas *,const uint8_t font[580],
                              struct pt_view_cache *,struct pt_view_rect rects[PT_VIEW_DIRTY_MAX]);
extern const uint16_t pt_view_palette[16];
void pt_editor_draw(const struct pt_editor *,struct pt_canvas *,const uint8_t font[580]);
void pt_editor_draw_playback(const struct pt_editor *,struct pt_canvas *,const uint8_t font[580]);
#endif
