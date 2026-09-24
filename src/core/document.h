#ifndef PT_DOCUMENT_H
#define PT_DOCUMENT_H
#include "project.h"
#include "mod_inspect.h"
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
void pt_document_release(struct pt_document *);
/* Create a staged empty song, 1..16 channels, one order/pattern and 31 empty
 * classic sample slots. First four routes are Paula; extra routes are AmiGUS.
 * Failure preserves the existing document and all of its storage. */
enum pt_project_result pt_document_new(struct pt_document *,unsigned channels,size_t budget);
/* Validates, budgets and allocates a separate complete candidate. Only success
 * releases the old project and swaps it in. budget is caller policy, not an
 * assumption about installed/free RAM; allocation failure is always handled.
 * PP20 is detected by content, fully validated/decompressed, then strict MOD
 * validation is applied. Budget includes temporary decompressed bytes. */
enum pt_project_result pt_document_load(struct pt_document *,const uint8_t *,size_t,size_t);
/* Stable uncompressed MOD or enhanced-project source; bounded reads. Finish verifies
 * end/close and return1 before replacing the old document. On failure caller
 * cleans up input; old document is retained. No source callbacks may reenter. */
enum pt_project_result pt_document_load_mod_reader(struct pt_document *,pt_mod_read,void *,size_t,size_t,int (*)(void *));
enum pt_project_result pt_document_load_project_reader(struct pt_document *,pt_project_read,void *,size_t,size_t,int (*)(void *));
#endif
