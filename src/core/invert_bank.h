#ifndef PT_INVERT_BANK_H
#define PT_INVERT_BANK_H
#include "document.h"
#include "invert_pcm.h"
/* A render-owned bank; entries preserve instrument indexing, while only selected
 * samples receive mutable PCM copies. Masters must outlive the bank and remain
 * stable. Use the caller's Fast-memory allocator and explicit byte budget.
 * Zero-initialize before open. Failure preserves the empty bank and masters. */
struct pt_invert_bank {
    struct pt_invert_pcm *entries;
    int32_t *storage;
    struct pt_allocator allocator;
    size_t count, allocated_bytes;
};
enum pt_invert_bank_result { PT_INVERT_BANK_OK, PT_INVERT_BANK_INVALID,
                            PT_INVERT_BANK_BUDGET, PT_INVERT_BANK_MEMORY };
enum pt_invert_bank_result pt_invert_bank_open(struct pt_invert_bank *,
    const struct pt_sample *,size_t count,const uint8_t *selected,
    size_t budget,const struct pt_allocator *);
/* Restore private copies before another pass; reset channel clocks separately. */
enum pt_pcm_result pt_invert_bank_reset(struct pt_invert_bank *);
void pt_invert_bank_close(struct pt_invert_bank *);
#endif
