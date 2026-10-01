#ifndef PT_NATIVE_PAULA_ENGINE_H
#define PT_NATIVE_PAULA_ENGINE_H
#include "paula_output.h"
#include "paula_memory.h"
/* Zero-init, noncopyable, serialized frontend-owned native engine. All storage,
 * sampler/project/allocator contexts survive pending or failed close. Bind this
 * voice owner to the existing editor song/mixed barrier BEFORE enabling edits
 * during playback. Close attached song first, then this engine, then sampler.
 * Begin/advance reserve channels and bind selective Chip copies; no voices start.
 * Advance never waits. No timer/UI/IRQ installation or automatic retry. */
struct pt_native_paula_engine {
    struct pt_native_paula_output output;
    struct pt_sampler_paula cache;
    struct pt_paula_voices voices;
    struct pt_sampler *sampler;struct pt_project *project;
    size_t budget;unsigned ready,closing,failed;
};
static inline int pt_native_paula_engine_close(struct pt_native_paula_engine *e)
{
    if(!e)return 0;
    e->closing=1;e->ready=0;
    /* Song ownership refuses public voice close; never bypass that barrier. */
    if(e->voices.bridge && !pt_paula_voices_close(&e->voices))return 0;
    if(e->cache.sampler && !pt_sampler_paula_close(&e->cache))return 0;
    if(!pt_native_paula_output_close(&e->output))return 0;
    e->sampler=NULL;e->project=NULL;e->budget=0;e->closing=e->failed=0;
    return 1;
}
static inline int pt_native_paula_engine_begin(struct pt_native_paula_engine *e,
    struct pt_sampler *sampler,struct pt_project *project,size_t chip_budget)
{
    if(!e || e->sampler || e->project || e->ready || e->closing || e->failed ||
       e->voices.bridge || e->cache.sampler || !sampler || !sampler->allocator.allocate ||
       !sampler->allocator.release || !chip_budget || chip_budget>UINT32_MAX ||
       pt_project_validate(project,NULL)!=PT_PROJECT_OK)return 0;
    if(!pt_native_paula_output_open(&e->output))return 0;
    e->sampler=sampler;e->project=project;e->budget=chip_budget;return 1;
}
/* 1 ready,0 reservation pending,-1 failed/closing. Failure retains resources;
 * explicit close is required. Ready is idempotent and rechecks reservation. */
static inline int pt_native_paula_engine_advance(struct pt_native_paula_engine *e)
{
    struct pt_paula_voice_api api;int r;
    if(!e || !e->sampler || e->closing || e->failed)return -1;
    r=pt_native_paula_output_advance(&e->output);
    if(r<0){e->failed=1;return -1;}if(!r)return 0;
    if(e->ready)return 1;
    if(!pt_native_paula_output_api(&e->output,&api) ||
       !pt_sampler_paula_bind(&e->cache,e->sampler,e->project,NULL,
           pt_paula_chip_allocate,pt_paula_chip_release,e->budget) ||
       !pt_paula_voices_bind(&e->voices,&e->cache,&api) ||
       !pt_paula_voices_bind_quiesce(&e->voices,pt_native_paula_output_quiesce,&e->output)) {
        e->failed=1;return -1;
    }
    e->ready=1;return 1;
}
#endif
