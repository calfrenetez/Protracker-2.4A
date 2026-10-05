#ifndef PT_RENDER_STORAGE_INTERNAL_H
#define PT_RENDER_STORAGE_INTERNAL_H
#include "project.h"
/* Private metadata-only complete declared project storage guard. Borrow a
 * genuine live project with readable descriptor tables; keep those descriptors
 * and all named storage alive and immutable. No semantic values, validation,
 * allocation or callbacks. Includes full PCM capacities, unused samples and
 * unknown extension payloads. Malformed bounds/wrapped or missing positive
 * spans refuse. This does not prove pointer residency or semantic validity. */
int pt_render_project_storage_output_disjoint(const struct pt_project *,const void *,size_t);
#endif
