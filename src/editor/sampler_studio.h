#ifndef PT_SAMPLER_STUDIO_H
#define PT_SAMPLER_STUDIO_H
#include "sampler.h"
#include "../core/studio_mix.h"
/* Borrowed provider context: outlives session. Keys are slot+1, version is the
 * sampler generation captured by the control thread. Close sessions before
 * destroying/reinitializing sampler/context or replacing the document. */
struct pt_sampler_studio {struct pt_sampler *sampler;struct pt_project *project;};
struct pt_studio_source pt_sampler_studio_source(struct pt_sampler_studio *);
#endif
