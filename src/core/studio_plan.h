#ifndef PT_STUDIO_PLAN_H
#define PT_STUDIO_PLAN_H
#include "render_commands.h"
#include "studio_mix.h"
/* Immutable bindings captured by the owner thread from validated project masters.
 * Provider must return exactly this PCM content/format for key+version. Descriptor
 * identity is matched, never pointer subtraction or dereferencing foreign PCM.
 * Binding descriptors and plan remain alive/disjoint throughout dispatch. */
struct pt_studio_binding {const struct pt_pcm *pcm;uint64_t key,version;};
/* Apply a successful render plan to a dedicated Studio session with `voices`
 * channels (1..16). Caller has rendered the preceding interval and kept command
 * state in sync. No allocation here; provider acquisition may allocate/pin.
 * Any error stops ALL voices, releasing current/pending pins. This is fail-stop,
 * not rollback: discard command state and abort playback, never retry the plan.
 * No scheduling, project editing, output transport or hardware access. */
enum pt_pcm_result pt_studio_dispatch(struct pt_studio_mix *,unsigned voices,
    const struct pt_render_plan *,const struct pt_studio_binding *,unsigned bindings);
#endif
