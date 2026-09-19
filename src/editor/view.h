#ifndef PT_EDITOR_VIEW_H
#define PT_EDITOR_VIEW_H
#include "editor.h"
#define PT_VIEW_WIDTH 640
#define PT_VIEW_HEIGHT 512
#define PT_VIEW_PLANE_BYTES (80UL*512)
/* Four ordinary non-interleaved 640x512 bitplanes, 80 bytes per row. */
struct pt_canvas {uint8_t *planes[4];};
extern const uint16_t pt_view_palette[16];
void pt_editor_draw(const struct pt_editor *,struct pt_canvas *,const uint8_t font[580]);
#endif
