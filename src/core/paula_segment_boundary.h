#ifndef PT_PAULA_SEGMENT_BOUNDARY_H
#define PT_PAULA_SEGMENT_BOUNDARY_H
#if defined(__m68k__) || defined(__amigaos__) || defined(__AMIGA__) || defined(AMIGA)
#error "The finite segment boundary admission is a HOST model only"
#endif
#include "render_commands.h"
#include "scheduled_readers.h"
#define PT_PAULA_BOUNDARY_DEPENDENCIES 3U
#define PT_PAULA_BOUNDARY_EVENTS 4U
struct pt_paula_segment_boundary_scope {
    unsigned track,dependency_count;
    const struct pt_pcm *dependency[PT_PAULA_BOUNDARY_DEPENDENCIES];
    uint64_t frame[PT_PAULA_BOUNDARY_EVENTS];
};
enum pt_paula_segment_boundary_result {PT_PAULA_BOUNDARY_OK,PT_PAULA_BOUNDARY_INVALID,
    PT_PAULA_BOUNDARY_ALIAS,PT_PAULA_BOUNDARY_STALE,PT_PAULA_BOUNDARY_UNSUPPORTED};
/* Allocation-free shape admission for an already validated immutable renderer
 * project and a fixed one-track SEGMENT/REPEAT/REPEAT/STOP profile. Accepts only
 * the complete ordered plan: primary+CONTROL, or a lone STOP. CONTROL must have
 * exactly the primary's PCM/pending source, phase, cycle, ranges and flags;
 * only its final step/gains replace the primary's values. No PCM is read or
 * advanced. The configured four strictly increasing frames are never rebased.
 * Every non-NULL current/pending descriptor must belong to the predeclared
 * bounded dependency set; this module acquires or releases no source resource.
 *
 * Initial admission requires NULL keys. Continuations compare every field of
 * an immutable original key and a fresh key supplied by the caller's genuine
 * positive getter. Equality/shape is NOT adoption/retirement/ownership proof.
 * The real finite reader API must still enforce progress, current versions,
 * exact key, representation/geometry capability and transfer. This function is
 * not a song preflight or a certificate, and its success changes no queue.
 *
 * All supplied objects must be fully addressable. Numeric fixed extent and
 * alignment/alias checks precede typed reads. Known descriptor objects are
 * guarded before copying descriptor metadata. Checked capacity*sizeof(int32_t)
 * protects every known complete master allocation, including unused capacity;
 * capacity is already an element count and is never multiplied by channels.
 * Nonzero missing, unrepresentable, misaligned or wrapped storage refuses.
 * Zero capacity with a non-NULL data pointer or nonempty frames also
 * refuses; genuine data-NULL/frame-zero/capacity-zero metadata is allowed
 * as an unused storage shape only, never a full PCM or voice certificate.
 * No PCM payload/capacity padding is read. Metadata checks do not establish
 * currentness, representation capability, sample validity or ownership.
 * Unknown cache/control extents remain caller-disjoint obligations.
 * Refusal preserves output and all inputs. No callbacks, allocation, hardware,
 * native output, automatic fetch boundary, receipt or disposal is introduced.
 */
enum pt_paula_segment_boundary_result pt_paula_segment_boundary_admit(
    const struct pt_paula_segment_boundary_scope *,unsigned index,uint64_t frame,
    const struct pt_render_plan *,const struct pt_readers_key *original,
    const struct pt_readers_key *actual,struct pt_render_action *);
#endif
