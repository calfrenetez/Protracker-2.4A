#ifndef PT_DOCUMENT_H
#define PT_DOCUMENT_H
#include "project.h"
#include "mod_inspect.h"
#include "pp20.h"
struct pt_allocator {
    void *context;
    void *(*allocate)(void *,size_t);
    void (*release)(void *,void *);
};
struct pt_document {
    struct pt_project project;
    struct pt_project_storage storage;
    struct pt_allocator allocator;
    size_t allocated_bytes;
    uint8_t loaded, dirty;
};
void pt_document_init(struct pt_document *,const struct pt_allocator *);
enum pt_document_release_result {
    PT_DOCUMENT_RELEASED,PT_DOCUMENT_RELEASE_REFUSED,PT_DOCUMENT_RELEASE_UNCERTAIN
};
/* Callback-free refusal query; a positive result is instantaneous, not a
 * persistent adoption certificate. Checked release consumes known full-capacity
 * entries once, quarantining uncertainty before arbitrary release callbacks.
 * Original document and allocator context must remain alive on UNCERTAIN.
 * Private HOST owner composition is opt-in; no native/all-path grant follows. */
int pt_document_replacement_allowed(const struct pt_document *);
enum pt_document_release_result pt_document_release_checked(struct pt_document *);
void pt_document_release(struct pt_document *);
/* Staging allocations capture the original allocator/header/full storage
 * extents before callbacks. Only spatially accepted requested-size entries may
 * be written or released; uncertainty retains private bounded obligations.
 * allocate(n) must still return a fresh living disjoint allocation of at least n
 * bytes until its matching release: this API has no heap-capacity census.
 * Reader/finish wrappers retain this same record from before probe, checking
 * old ownership and complete next-header/storage before and after every callback,
 * including return0. Current ordinary failure cleans issued entries once;
 * uncertainty prohibits decoder continuation and keeps obligations living.
 * Arbitrary callback writes/frees, opaque context bounds and global outputs
 * remain contractual gaps. Quarantined document/context must stay alive. */
/* Create a staged empty song, 1..16 channels, one order/pattern and 31 empty
 * classic sample slots. First four routes are Paula; extra routes are AmiGUS.
 * Allocation refusal before old retirement preserves the old document; callback
 * uncertainty retains known obligations and prohibits retry, not rollback. */
enum pt_project_result pt_document_new(struct pt_document *,unsigned channels,size_t budget);
/* Validates, budgets and allocates a separate complete candidate. Only success
 * releases the old project and swaps it in. budget is caller policy, not an
 * assumption about installed/free RAM; allocation failure is always handled.
 * PP20 is detected by content, fully validated/decompressed, then strict MOD
 * validation is applied. Budget includes temporary decompressed bytes. */
enum pt_project_result pt_document_load(struct pt_document *,const uint8_t *,size_t,size_t);
/* Stable synchronous source; bounded guarded reads, including real staging
 * extension-data destinations. Finish verifies end/close and return1 before
 * replacing the old document. No source callback may reenter or change recorded
 * ownership/header/storage. Ordinary failure retains old state; callback
 * uncertainty cannot promise rollback of arbitrary external writes/frees. */
enum pt_project_result pt_document_load_mod_reader(struct pt_document *,pt_mod_read,void *,size_t,size_t,int (*)(void *));
enum pt_project_result pt_document_load_project_reader(struct pt_document *,pt_project_read,void *,size_t,size_t,int (*)(void *));
enum pt_project_result pt_document_load_pp20_reader(struct pt_document *,pt_pp20_read,void *,size_t,size_t,int (*)(void *));
#endif
