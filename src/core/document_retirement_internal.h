#ifndef PT_DOCUMENT_RETIREMENT_INTERNAL_H
#define PT_DOCUMENT_RETIREMENT_INTERNAL_H
#include "document.h"
/* Serialized private HOST composition. These are direct, callback-free queries
 * of the genuine factory's private registry, not caller-provided permissions.
 * Core-only compilation has no dependency on the editor implementation. */
#if defined(PT_DOCUMENT_ENROLLMENT_HOST)
#if defined(__m68k__) || defined(__amigaos__) || defined(__AMIGA__) || defined(AMIGA)
#error "Document enrollment is a private serialized HOST prototype only"
#endif
int pt_document_registered_project_replacement_allowed(const struct pt_project *);
int pt_document_registered_output_disjoint(const void *,size_t);
#endif
/* The genuine factory checks this before traversing a project during a checked
 * release callback. No status getter or public owner clone can clear it. */
int pt_document_retirement_project_busy(const struct pt_project *);
/* Private typed replacement composition. These fixed seven-entry helpers
 * observe full-capacity ownership only; they are not factory/release authority. */
int pt_document_owned_storage_spans(const struct pt_document *,void *[7],size_t [7]);
int pt_document_storage_output_disjoint(const struct pt_document *,const void *,size_t);
int pt_document_retirement_callback_active(void);
#if defined(PT_DOCUMENT_TYPED_REPLACEMENT)
#if defined(PT_DOCUMENT_ENROLLMENT_HOST)
#error Typed replacement and historical pretyped HOST enrollment are separate compositions
#endif
int pt_document_typed_replacement_project_allowed(const struct pt_project *);
int pt_document_typed_replacement_output_disjoint(const void *,size_t);
#endif
#endif
