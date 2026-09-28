#ifndef PT_SAMPLER_CAPTURE_H
#define PT_SAMPLER_CAPTURE_H
#include "sampler.h"
#include "../core/capture.h"
/* Append finished nonempty capture as one undoable sample, without replacing a
 * master. Caller applies the normal editor change guard first. Success deep-copies
 * exact PCM into the sampler budget and closes capture staging. Failure preserves
 * capture/project/history for retry; both use their existing bounded allocators.
 * Does not stop a capture device or claim supported input formats. */
enum pt_edit_result pt_sampler_capture_append(struct pt_sampler *,struct pt_project *,struct pt_pattern_history *,struct pt_capture *,const char *);
#endif
