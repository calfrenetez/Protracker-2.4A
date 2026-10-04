/* Reuse the actual workflow fixture. Execute only new copy-specific boundaries. */
#define main pt_sampler_workflow_existing_main
#include "sampler_workflow_test.c"
#undef main
static void copy_cancel_and_fail(void)
{
    unsigned append,phase,fail;
    for(append=0;append<2;++append) for(phase=0;phase<9;++phase) {
        struct fixture *f=fixture(24,2);struct pt_sample_usage_preview *v;struct pt_sampler_workflow *job=NULL;
        struct pt_sample samples[6];struct pt_project project;struct pt_pattern_history history;
        int32_t pcm[3][5208];unsigned ready=0,i;
        if(append)strcpy(f->samples[5].name,"NAMED EMPTY IS NOT FREE");
        v=scan(f);memcpy(samples,f->samples,sizeof(samples));memcpy(pcm,f->pcm,sizeof(pcm));project=f->p;history=f->history;
        assert(pt_sampler_copy_begin(&f->sampler,&f->p,&f->history,v,0,10,2500,1,&job)==PT_EDIT_OK);
        for(i=0;i<phase;++i)assert(pt_sampler_workflow_step(job,&ready)==PT_EDIT_OK);
        assert(!memcmp(samples,f->samples,sizeof(samples)) && !memcmp(pcm,f->pcm,sizeof(pcm)));
        assert(!f->sampler.current[0] && !f->sampler.current[5] && !f->history.count);
        pt_sampler_workflow_cancel(&job);assert(!job && !f->memory.live && !f->memory.bytes && !f->sampler.bytes);
        assert(!memcmp(&project,&f->p,sizeof(project)) && !memcmp(&history,&f->history,sizeof(history)));
        assert(!memcmp(samples,f->samples,sizeof(samples)) && !memcmp(pcm,f->pcm,sizeof(pcm)));
        free(v);destroy(f);
    }
    for(append=0;append<2;++append) for(fail=1;fail<=3;++fail) {
        struct fixture *f=fixture(24,2);struct pt_sample_usage_preview *v;struct pt_sampler_workflow *job=NULL;
        struct pt_sample samples[6];int32_t pcm[3][5208];struct pt_pattern_history history;
        if(append)strcpy(f->samples[5].name,"NAMED EMPTY IS NOT FREE");
        v=scan(f);memcpy(samples,f->samples,sizeof(samples));memcpy(pcm,f->pcm,sizeof(pcm));history=f->history;f->memory.fail=fail;
        assert(pt_sampler_copy_begin(&f->sampler,&f->p,&f->history,v,0,10,2500,1,&job)==PT_EDIT_CAPACITY);
        assert(!job && !f->memory.live && !f->sampler.bytes && !memcmp(samples,f->samples,sizeof(samples)));
        assert(!memcmp(pcm,f->pcm,sizeof(pcm)) && !memcmp(&history,&f->history,sizeof(history)) && !f->sampler.current[0]);
        free(v);destroy(f);
    }
}
static void copy_typed_output_aliases(void)
{
    struct pointer_tail { int32_t audio[5208]; struct pt_sampler_workflow *out; struct pt_sampler_workflow_stats stats; };
    struct fixture *f=fixture(24,2);struct pointer_tail *m=calloc(1,sizeof(*m)),*prior=malloc(sizeof(*m));
    struct pt_sample source=f->samples[0];struct pt_sample_usage_preview *v;struct pt_sampler_workflow *job=NULL;
    assert(m && prior);memcpy(m->audio,f->pcm[0],sizeof(m->audio));memset(&m->stats,0x4b,sizeof(m->stats));
    f->samples[0].pcm.data=m->audio;f->samples[0].pcm.capacity=sizeof(*m)/sizeof(int32_t);v=scan(f);*prior=*m;
    assert(pt_sampler_copy_begin(&f->sampler,&f->p,&f->history,v,0,10,20,1,&m->out)==PT_EDIT_ALIAS);
    assert(!memcmp(m,prior,sizeof(*m)) && !f->memory.calls && !f->history.count);
    m->out=(struct pt_sampler_workflow *)(void *)m;*prior=*m;
    assert(pt_sampler_copy_begin(&f->sampler,&f->p,&f->history,v,0,10,20,1,&m->out)==PT_EDIT_ALIAS);
    assert(!memcmp(m,prior,sizeof(*m)) && !f->memory.calls && !f->history.count);
    m->out=NULL;*prior=*m;
    assert(pt_sampler_copy_begin(&f->sampler,&f->p,&f->history,v,0,10,2500,1,&job)==PT_EDIT_OK);prepare(job);
    assert(pt_sampler_workflow_commit(&job,1,&m->stats)==PT_EDIT_ALIAS && job);
    assert(!memcmp(m,prior,sizeof(*m)) && !f->history.count && !f->sampler.current[0]);
    pt_sampler_workflow_cancel(&job);assert(!job && !f->memory.live && !f->sampler.bytes);
    f->samples[0]=source;free(v);free(prior);free(m);destroy(f);
}
static void copy_refusals_and_stale(void)
{
    struct fixture *f=fixture(24,2);struct pt_sample_usage_preview *v=scan(f);struct pt_sampler_workflow *job=NULL;
    struct pt_sampler_workflow_stats stats,oldstats;struct pt_pattern_history history=f->history;
    struct pt_sample samples[6];int32_t pcm[3][5208];unsigned ready=55;
    memcpy(samples,f->samples,sizeof(samples));memcpy(pcm,f->pcm,sizeof(pcm));memset(&stats,0x53,sizeof(stats));oldstats=stats;
    assert(pt_sampler_copy_begin(&f->sampler,&f->p,&f->history,v,0,10,10,1,&job)==PT_EDIT_INVALID);
    assert(pt_sampler_copy_begin(&f->sampler,&f->p,&f->history,v,0,10,2601,1,&job)==PT_EDIT_INVALID);
    assert(pt_sampler_copy_begin(&f->sampler,&f->p,&f->history,v,6,10,20,1,&job)==PT_EDIT_INVALID);
    assert(pt_sampler_copy_begin(&f->sampler,&f->p,&f->history,v,0,10,20,0,&job)==PT_EDIT_CONFLICT);
    assert(!job && !f->memory.calls && !memcmp(samples,f->samples,sizeof(samples)));
    assert(!memcmp(pcm,f->pcm,sizeof(pcm)) && !f->memory.calls);
    assert(pt_sampler_copy_begin(&f->sampler,&f->p,&f->history,v,0,10,2500,1,&job)==PT_EDIT_OK);
    assert(pt_sampler_workflow_step(job,(unsigned *)(void *)(f->pcm[0]+5207))==PT_EDIT_ALIAS);
    assert(pt_sampler_workflow_step(job,(unsigned *)(void *)f->markers[0])==PT_EDIT_ALIAS);
    assert(!memcmp(pcm,f->pcm,sizeof(pcm)) && !memcmp(samples,f->samples,sizeof(samples)));
    prepare(job);
    assert(pt_sampler_workflow_commit(&job,0,&stats)==PT_EDIT_CONFLICT && job && !memcmp(&stats,&oldstats,sizeof(stats)));
    assert(!memcmp(pcm,f->pcm,sizeof(pcm)) && !memcmp(&f->history,&history,sizeof(history)));
    ++f->samples[0].volume;
    assert(pt_sampler_workflow_step(job,&ready)==PT_EDIT_CONFLICT && ready==55);
    assert(pt_sampler_workflow_commit(&job,1,&stats)==PT_EDIT_CONFLICT && !memcmp(&stats,&oldstats,sizeof(stats)));
    pt_sampler_workflow_cancel(&job);assert(!job && !f->memory.live && !f->sampler.bytes);
    f->samples[0]=samples[0];free(v);destroy(f);
}
static void copy_changed_table_cancel(void)
{
    unsigned phase;
    for(phase=0;phase<2;++phase) {
        struct fixture *f=fixture(24,2);struct pt_sample_usage_preview *v=scan(f);struct pt_sampler_workflow *job=NULL;
        struct pt_sampler_workflow_stats stats,oldstats;unsigned ready=71;int32_t master[5208];
        memcpy(master,f->pcm[0],sizeof(master));memset(&stats,0x46,sizeof(stats));oldstats=stats;
        assert(pt_sampler_copy_begin(&f->sampler,&f->p,&f->history,v,0,10,2500,1,&job)==PT_EDIT_OK);
        assert(pt_sampler_workflow_step(job,&ready)==PT_EDIT_OK && !ready);
        if(phase)prepare(job);
        f->p.samples=NULL;ready=71;
        assert(pt_sampler_workflow_step(job,&ready)==PT_EDIT_CONFLICT && ready==71);
        assert(pt_sampler_workflow_commit(&job,1,&stats)==(phase?PT_EDIT_CONFLICT:PT_EDIT_INVALID));
        assert(!memcmp(&stats,&oldstats,sizeof(stats)));
        pt_sampler_workflow_cancel(&job);assert(!job && !f->memory.live && !f->sampler.bytes);
        assert(!memcmp(master,f->pcm[0],sizeof(master)) && !f->history.count);
        f->p.samples=f->samples;free(v);destroy(f);
    }
}
static void copy_exclusive_marker_boundary(void)
{
    struct fixture *f=fixture(24,2);struct pt_sample_usage_preview *v=scan(f);struct pt_sampler_workflow *job=NULL;
    struct pt_sampler_workflow_stats stats;int32_t master[5208];memcpy(master,f->pcm[0],sizeof(master));
    assert(pt_sampler_copy_begin(&f->sampler,&f->p,&f->history,v,0,4,2400,1,&job)==PT_EDIT_OK);prepare(job);
    assert(pt_sampler_workflow_commit(&job,1,&stats)==PT_EDIT_OK && stats.destination_slot==5);
    assert(f->samples[5].slice_count==1 && f->samples[5].slices[0]==46); /* 3 excluded; 2400 is exclusive. */
    assert(f->samples[5].loop==PT_LOOP_CROSSFADE && f->samples[5].loop_start==46 && f->samples[5].loop_end==2396);
    assert(!memcmp(master,f->pcm[0],sizeof(master)));
    free(v);destroy(f);
}
int main(void)
{
    copy_cancel_and_fail();copy_typed_output_aliases();copy_refusals_and_stale();copy_changed_table_cancel();copy_exclusive_marker_boundary();
    puts("SAMPLER COPY BOUNDARIES PASS: private gap/append rollback, stale/alias/range refusals and exclusive markers");
    return 0;
}
