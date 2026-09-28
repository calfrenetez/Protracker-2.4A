#ifndef PT_EDITOR_CAPTURE_H
#define PT_EDITOR_CAPTURE_H
#include "editor.h"
#include "../core/amigus_capture.h"
/* Serialized injected recording owner; no native input or UI controls. Zero-init,
 * attach only without another editor guard, and keep editor/allocator/adapter
 * contexts alive until detach succeeds. This conservative single guard excludes
 * simultaneous editor playback. Never manipulate embedded owners independently.
 * A project change/dispose requests Finish but is vetoed until polling confirms
 * stop AND the nonempty recording is explicitly published or discarded. Edits
 * cannot silently discard a completed recording or destroy its allocator context. */
struct pt_editor_capture {
    struct pt_editor *editor;
    struct pt_amigus_capture device;
    struct pt_capture ready;
    enum pt_capture_fault fault; /* Sticky until explicit discard or next start. */
    unsigned publishing;
};
int pt_editor_capture_attach(struct pt_editor_capture *,struct pt_editor *);
int pt_editor_capture_start(struct pt_editor_capture *,struct pt_amigus_reservation *,const struct pt_capture_input *,unsigned bits,unsigned channels,uint32_t rate,uint32_t frames,size_t budget);
enum pt_capture_poll pt_editor_capture_step(struct pt_editor_capture *);
void pt_editor_capture_finish(struct pt_editor_capture *);
int pt_editor_capture_busy(const struct pt_editor_capture *);
const struct pt_pcm *pt_editor_capture_pcm(const struct pt_editor_capture *);
/* One undoable append, selects the new sample. Allocation failure preserves the
 * stopped recording, old project/history/selection and permits a later retry. */
enum pt_edit_result pt_editor_capture_publish(struct pt_editor_capture *,const char *name);
/* Explicit discard requests abort without I/O; returns0 while shutdown pending.
 * Continue step before retrying. Detach also explicitly discards any recording. */
int pt_editor_capture_discard(struct pt_editor_capture *);
int pt_editor_capture_detach(struct pt_editor_capture *);
#endif
