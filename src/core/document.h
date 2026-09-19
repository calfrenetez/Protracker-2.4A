#ifndef PT_DOCUMENT_H
#define PT_DOCUMENT_H
#include "project.h"
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
/* Validates, budgets and allocates a separate complete candidate. Only success
 * releases the old project and swaps it in. budget is caller policy, not an
 * assumption about installed/free RAM; allocation failure is always handled. */
enum pt_project_result pt_document_load(struct pt_document *,const uint8_t *,size_t,size_t);
#endif
