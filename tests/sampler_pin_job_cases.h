#include "../src/editor/sampler_internal.h"
static void pin_job_fixture(const struct pt_allocator *a)
{
    struct pt_document d;struct pt_sampler s;struct pt_sampler_pin_job job={0};
    struct pt_sample original;struct pt_pcm pcm,unchanged;struct pt_sample_version *token;
    int32_t *data=a->allocate(a->context,3000*sizeof(int32_t));
    uint32_t *markers=a->allocate(a->context,1100*sizeof(uint32_t));
    unsigned bits,mode,i,ready,steps;size_t reserved,copied;enum pt_edit_result result;
    assert(data && markers);for(i=0;i<1100;++i)markers[i]=i;
    pt_document_init(&d,a);assert(pt_document_new(&d,4,SIZE_MAX)==PT_PROJECT_OK);
    for(bits=8;bits<=24;bits+=8)for(mode=0;mode<7;++mode) {
        uint8_t *saved=NULL,*encoded=NULL;size_t length=0,written;
        for(i=0;i<3000;++i)data[i]=bits==8?(int32_t)(i%255)-128:bits==16?(int32_t)i*3-4000:(int32_t)i*257-100000;
        d.project.samples[0].pcm=(struct pt_pcm){data,3000,1500,48000,2,(uint8_t)bits};
        d.project.samples[0].slices=markers;d.project.samples[0].slice_count=1100;original=d.project.samples[0];
        assert(pt_project_validate(&d.project,NULL)==PT_PROJECT_OK);
        if(mode==6) {
            assert(pt_project_size(&d.project,&length)==PT_PROJECT_OK);
            saved=a->allocate(a->context,length);encoded=a->allocate(a->context,length);assert(saved && encoded);
            assert(pt_project_encode(&d.project,saved,length,&written)==PT_PROJECT_OK && written==length);
        }
        pt_sampler_init(&s,a,1024*1024);
        s.budget=0;assert(pt_sampler_pin_job_begin(&job,&s,&d.project,0,s.generation)==PT_EDIT_CAPACITY && !job.value && !s.bytes);
        s.budget=1024*1024;
        assert(pt_sampler_pin_job_begin(&job,&s,&d.project,0,s.generation)==PT_EDIT_OK && job.value);
        reserved=s.bytes;assert(reserved>=16400 && !s.current[0] && d.project.samples[0].pcm.data==data);
        assert(pt_sampler_pin_job_begin(&job,&s,&d.project,0,s.generation)==PT_EDIT_INVALID && s.bytes==reserved);
        memset(&unchanged,0x5a,sizeof(unchanged));pcm=unchanged;token=(struct pt_sample_version *)(uintptr_t)1;ready=7;
        assert(pt_sampler_pin_job_step(&job,0,&pcm,&token,&ready)==PT_EDIT_INVALID && ready==7);
        assert(pt_sampler_pin_job_step(&job,4097,&pcm,&token,&ready)==PT_EDIT_INVALID && ready==7);
        if(mode) {
            /* Odd byte counts are allowed; partial words never become visible. */
            assert(pt_sampler_pin_job_step(&job,1,&pcm,&token,&ready)==PT_EDIT_OK && !ready);
            assert(job.copied_values==1 && !s.current[0] && !memcmp(&pcm,&unchanged,sizeof(pcm)) && token==(struct pt_sample_version *)(uintptr_t)1);
        }
        if(mode==2)++s.generation;
        if(mode==3)d.project.samples=NULL;
        if(mode==4)++d.project.samples[0].volume;
        if(mode>=2 && mode<=4) {
            ready=7;assert(pt_sampler_pin_job_step(&job,4096,&pcm,&token,&ready)==PT_EDIT_CONFLICT && ready==7);
            d.project.samples=job.table;d.project.samples[0]=original;s.generation=job.generation;
            assert(pt_sampler_pin_job_step(&job,4096,&pcm,&token,&ready)==PT_EDIT_CONFLICT); /* Poisoned until cancel. */
        }
        if(mode>=5) {
            steps=0;
            do {
                copied=job.copied_values+job.copied_slices;
                result=pt_sampler_pin_job_step(&job,4096,&pcm,&token,&ready);assert(result==PT_EDIT_OK && ++steps<10);
                if(!ready) {
                    assert(job.copied_values+job.copied_slices-copied<=4096 && s.bytes==reserved);
                    assert(!s.current[0] && d.project.samples[0].pcm.data==data && d.project.samples[0].slices==markers);
                    assert(!memcmp(&pcm,&unchanged,sizeof(pcm)) && token==(struct pt_sample_version *)(uintptr_t)1);
                    if(mode==5 && job.copied_slices)break; /* Cancel in marker copy. */
                }
            }while(!ready);
            if(mode==5)assert(!ready && job.copied_values==12000 && job.copied_slices>0);
            else {
                struct pt_sample_version *second=NULL;struct pt_pcm again;
                assert(ready && !job.value && s.current[0]==token && pcm.bits==bits && pcm.data!=data);
                assert(!memcmp(pcm.data,data,12000) && !memcmp(d.project.samples[0].slices,markers,4400));
                assert(pt_project_encode(&d.project,encoded,length,&written)==PT_PROJECT_OK && written==length && !memcmp(saved,encoded,length));
                a->release(a->context,encoded);a->release(a->context,saved);
                /* Existing immutable master reuse reserves no new allocation. */
                s.budget=0;
                assert(pt_sampler_pin_job_begin(&job,&s,&d.project,0,s.generation)==PT_EDIT_OK && s.bytes==reserved);
                assert(pt_sampler_pin_job_step(&job,1,&again,&second,&ready)==PT_EDIT_OK && ready && second==token && again.data==pcm.data);
                pt_sampler_unpin(second);pt_sampler_release(&s);assert(s.bytes==reserved);
                assert(!memcmp(pcm.data,data,12000));pt_sampler_unpin(token);assert(!s.bytes);
                d.project.samples[0]=original;
            }
        }
        pt_sampler_pin_job_cancel(&job);pt_sampler_pin_job_cancel(&job);
        assert(!job.value && !s.bytes && !s.current[0] && d.project.samples[0].pcm.data==data);
        pt_sampler_release(&s);
    }
    pt_document_release(&d);a->release(a->context,markers);a->release(a->context,data);
    puts("PIN JOB PASS: bounded PCM/marker copy, atomic 8/16/24-bit publication, cancellation, stale poison, budget/reuse and pinned lifetime");
}
