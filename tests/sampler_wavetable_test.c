#define PT_WAVETABLE_NATIVE
#include "amigus_wavetable_cache_test.c"
#include "../src/editor/sampler_wavetable_internal.h"
static unsigned allocations,refuse;
static void *allocate_master(void *ctx,size_t bytes)
{void *p;(void)ctx;if(refuse)return NULL;p=malloc(bytes);if(p)++allocations;return p;}
static void release_master(void *ctx,void *p)
{(void)ctx;if(p){assert(allocations);--allocations;free(p);}}
static enum pt_cache_result sample_load(struct pt_sampler_wavetable *s,unsigned slot,struct pt_cache_lease *lease)
{
    uint8_t staging[3];struct pt_playback_format format={16,0,0,0};
    return pt_sampler_wavetable_acquire(s,slot,&format,staging,sizeof(staging),lease);
}
static void exact_save(struct pt_project *p,const uint8_t *saved,size_t size)
{
    uint8_t *output=malloc(size);size_t used;assert(output);
    assert(pt_project_encode(p,output,size,&used)==PT_PROJECT_OK);
    assert(used==size && !memcmp(output,saved,size));free(output);
}
static void sampler_upload_fixture(void)
{
    struct fixture *f=malloc(sizeof(*f));struct pt_allocator a={NULL,allocate_master,release_master};
    struct pt_document d;struct pt_sampler sampler;struct pt_sampler_wavetable bridge={0},saved_bridge;
    struct pt_sampler_upload_job job={0};struct pt_playback_format format={16,0,0,0};
    struct pt_cache_lease out={31,999},hit;struct pt_sample_version *pin=NULL;struct pt_pcm pcm;
    struct pt_pattern_history history;struct pt_pattern_command commands[4];struct pt_event_change changes[4];
    struct pt_amigus_reservation reservation;enum pt_cache_result result;
    uint8_t staging[3],expected[8];int32_t data[4];unsigned bits,mode,prepared,writes;uint32_t address,bytes;
    assert(f);
    for(bits=8;bits<=24;bits+=8)for(prepared=0;prepared<2;++prepared)for(mode=0;mode<16;++mode) {
        data[0]=bits==8?127:bits==16?32767:8388607;data[1]=-data[0]-1;data[2]=1;data[3]=-1;
        init(f,PT_AMIGUS_WAVETABLE);assert(pt_amigus_wavetable_cache_attach(&f->cache,&f->reservation,16,112,112,f,bus_owned,bus_write));
        pt_document_init(&d,&a);assert(pt_document_new(&d,4,SIZE_MAX)==PT_PROJECT_OK);
        d.project.samples[0].pcm=(struct pt_pcm){data,4,4,48000,1,bits};
        pt_sampler_init(&sampler,&a,1024*1024);
        assert(pt_pattern_history_init(&history,&d.project,commands,4,changes,4)==PT_EDIT_OK);
        assert(pt_sampler_wavetable_bind(&bridge,&sampler,&d.project,&f->cache));
        assert(pt_playback_pcm_pack(&d.project.samples[0].pcm,&format,expected,sizeof(expected))==PT_PCM_OK);
        if(prepared) {
            assert(pt_sampler_pin(&sampler,&d.project,0,sampler.generation,&pcm,&pin)==PT_EDIT_OK);
            if(mode==15)sampler.budget=0; /* Retaining a prepared pin allocates nothing. */
            result=pt_sampler_upload_begin_prepared(&job,&bridge,0,sampler.generation,bridge.version,pin,&format,&out);
            pt_sampler_unpin(pin);pin=NULL;
        }else {
            sampler.budget=0;assert(pt_sampler_upload_begin(&job,&bridge,0,&format,&out)==PT_CACHE_CAPACITY && !job.bridge && !sampler.bytes);
            sampler.budget=1024*1024;refuse=1;
            assert(pt_sampler_upload_begin(&job,&bridge,0,&format,&out)==PT_CACHE_CAPACITY && !job.bridge && !sampler.bytes);refuse=0;
            result=pt_sampler_upload_begin(&job,&bridge,0,&format,&out);
        }
        assert(result==PT_CACHE_PENDING && job.pin && out.serial==999 && !f->writes);
        assert(job.upload.upload.source==&job.pcm && job.pcm.data!=data && job.pcm.data[0]==data[0]);
        assert(pt_sampler_upload_step(&job,staging,2,&out)==PT_CACHE_PENDING && !f->writes);
        saved_bridge=bridge;
        switch(mode) {
        case 1:pt_sampler_upload_cancel(&job);break;
        case 2:
            assert(pt_sampler_edit(&sampler,&d.project,&history,0,PT_PCM_GAIN,0,4,500)==PT_EDIT_OK);
            pt_pattern_history_release(&history);pt_sampler_release(&sampler);
            assert(sampler.bytes && job.pcm.data[0]==data[0]);break;
        case 3:pt_sampler_release(&sampler);assert(sampler.bytes && job.pcm.data[0]==data[0]);break;
        case 4:pt_document_release(&d);pt_sampler_release(&sampler);assert(sampler.bytes);pt_sampler_upload_cancel(&job);assert(!sampler.bytes);break;
        case 5:--d.project.samples[0].pcm.capacity;break;
        case 6:++bridge.version;break;
        case 7:assert(!pt_sampler_wavetable_close(&bridge));break;
        case 8:f->healthy=0;break;
        case 9:f->fail=f->writes+1;break;
        case 10:reservation=f->reservation;f->cache.reservation=&reservation;break;
        case 11:bridge.sampler=NULL;break;
        case 12:d.project.channels.selected=1;break;
        case 13:++sampler.generation;assert(pt_sampler_wavetable_sync(&bridge));break;
        case 14:d.project.samples[0].pcm.data=data;break;
        }
        if(mode==0 || mode==12 || mode==15) {
            do{result=pt_sampler_upload_step(&job,staging,sizeof(staging),&out);}while(result==PT_CACHE_PENDING);
            assert(result==PT_CACHE_LOAD && !job.pin && !job.bridge);
            assert(pt_sampler_wavetable_location(&bridge,out,&address,&bytes) && bytes==8 && !memcmp(f->ram+address,expected,8));
            writes=f->writes;
            assert(pt_sampler_upload_begin(&job,&bridge,0,&format,&hit)==PT_CACHE_HIT && !job.pin && f->writes==writes);
            pt_sampler_upload_cancel(&job);assert(pt_sampler_wavetable_unpin(&bridge,hit));
            assert(pt_sampler_wavetable_unpin(&bridge,out));out=(struct pt_cache_lease){31,999};
        }else if(mode!=1 && mode!=4) {
            assert(pt_sampler_upload_step(&job,staging,2,&out)==(mode==9?PT_CACHE_TRANSFER:PT_CACHE_INVALID));
            assert(!job.pin && !job.bridge && !f->cache.cache.bytes && out.serial==999);
            if(mode==2 || mode==3)assert(!sampler.bytes);
        }
        pt_sampler_upload_cancel(&job);bridge=saved_bridge;f->cache.reservation=&f->reservation;
        assert(pt_sampler_wavetable_close(&bridge) && pt_amigus_reservation_close(&f->reservation));
        pt_pattern_history_release(&history);pt_sampler_release(&sampler);pt_document_release(&d);assert(!allocations && !sampler.bytes);
    }
    free(f);puts("SAMPLER UPLOAD JOB PASS: exact8/16/24 pins, stable descriptor, bounded steps, release/edit/history/cancel guards, hits and memory refusal; injected only");
}
static unsigned metadata_owned_calls;
static int metadata_owned(void *context)
{++metadata_owned_calls;return bus_owned(context);}
static void sampler_metadata_fixture(void)
{
    struct fixture *f=malloc(sizeof(*f));struct pt_allocator a={NULL,allocate_master,release_master};
    struct pt_document d;struct pt_sampler sampler;struct pt_sampler_wavetable bridge={0};
    struct pt_cache_lease lease;struct pt_sample_version *master;
    uint8_t cache_before[sizeof(struct pt_sample_cache)],ram_before[4096];
    int32_t data[4],value;uint16_t order;unsigned bits,writes,held_allocations,generation;
    size_t held_bytes;
    assert(f);
    for(bits=8;bits<=24;bits+=8) {
        data[0]=bits==8?127:bits==16?32767:8388607;data[1]=-data[0]-1;data[2]=1;data[3]=-1;
        init(f,PT_AMIGUS_WAVETABLE);
        assert(pt_amigus_wavetable_cache_attach(&f->cache,&f->reservation,16,112,112,f,bus_owned,bus_write));
        pt_document_init(&d,&a);assert(pt_document_new(&d,4,SIZE_MAX)==PT_PROJECT_OK);
        d.project.samples[0].pcm=(struct pt_pcm){data,4,4,48000,1,bits};
        pt_sampler_init(&sampler,&a,1024*1024);
        assert(pt_sampler_wavetable_bind(&bridge,&sampler,&d.project,&f->cache));
        assert(sample_load(&bridge,0,&lease)==PT_CACHE_LOAD);
        master=sampler.current[0];assert(master);
        held_bytes=sampler.bytes;held_allocations=allocations;writes=f->writes;generation=sampler.generation;
        memcpy(cache_before,&f->cache.cache,sizeof(cache_before));memcpy(ram_before,f->ram,sizeof(ram_before));
        f->cache.owned=metadata_owned;metadata_owned_calls=0;
        assert(pt_sampler_wavetable_prepared_metadata_current(&bridge));
        /* Intentional test-only poison: this query is NOT a semantic validator.
         * Restore before any ordinary source consumer or cleanup. */
        value=d.project.samples[0].pcm.data[0];order=d.project.orders[0];
        d.project.samples[0].pcm.data[0]=INT32_MAX;d.project.orders[0]=UINT16_MAX;
        assert(pt_project_validate(&d.project,NULL)!=PT_PROJECT_OK);
        assert(pt_sampler_wavetable_prepared_metadata_current(&bridge));
        d.project.samples[0].pcm.data[0]=value;d.project.orders[0]=order;
        ++sampler.generation;
        assert(!pt_sampler_wavetable_prepared_metadata_current(&bridge));
        sampler.generation=generation;
        /* Matching source metadata cannot authorize even a currently unowned
         * fake backend. No ownership callback or sticky fault is touched. */
        f->healthy=0;
        assert(pt_sampler_wavetable_prepared_metadata_current(&bridge));
        assert(!metadata_owned_calls && !f->cache.faulted);
        assert(!memcmp(cache_before,&f->cache.cache,sizeof(cache_before)));
        assert(!memcmp(ram_before,f->ram,sizeof(ram_before)) && f->writes==writes);
        assert(sampler.current[0]==master && sampler.bytes==held_bytes && allocations==held_allocations);
        /* The original live gate still invokes ownership and latches a fault. */
        assert(!pt_amigus_wavetable_cache_current(&f->cache));
        assert(metadata_owned_calls==1 && f->cache.faulted);
        assert(pt_sampler_wavetable_prepared_metadata_current(&bridge) && metadata_owned_calls==1);
        assert(!memcmp(cache_before,&f->cache.cache,sizeof(cache_before)));
        assert(pt_sampler_wavetable_unpin(&bridge,lease));
        assert(pt_sampler_wavetable_close(&bridge) && pt_amigus_reservation_close(&f->reservation));
        pt_sampler_release(&sampler);pt_document_release(&d);assert(!sampler.bytes && !allocations);
    }
    free(f);puts("SAMPLER METADATA PASS:8/16/24 source-only query, poisoned payload not scanned, stale refusal preserves active cache, zero callback/allocation/write; backend ownership remains separate; fake bus only");
}
static int sampler_fixture_main(void)
{
    struct fixture *f=malloc(sizeof(*f));
    struct pt_allocator allocator={NULL,allocate_master,release_master};
    struct pt_document document,donor;struct pt_sampler sampler;
    struct pt_sampler_wavetable bridge={0};
    struct pt_pattern_history history;struct pt_pattern_command commands[16];struct pt_event_change changes[16];
    struct pt_cache_lease old,newer,hit,undo,redo,metadata,grown,out={31,999};
    int32_t data[]={257,-513,1025,-2049};uint8_t *saved,held[8];size_t size,used;
    uint32_t address,bytes;unsigned writes;uint64_t version;
    assert(f);assert(wavetable_fixture_main()==0);sampler_upload_fixture();sampler_metadata_fixture();init(f,PT_AMIGUS_WAVETABLE);
    assert(pt_amigus_wavetable_cache_attach(&f->cache,&f->reservation,16,112,112,f,bus_owned,bus_write));
    pt_document_init(&document,&allocator);assert(pt_document_new(&document,4,SIZE_MAX)==PT_PROJECT_OK);
    document.project.samples[0].pcm=(struct pt_pcm){data,4,4,48000,1,24};
    assert(pt_project_size(&document.project,&size)==PT_PROJECT_OK);saved=malloc(size);assert(saved);
    assert(pt_project_encode(&document.project,saved,size,&used)==PT_PROJECT_OK && used==size);
    pt_sampler_init(&sampler,&allocator,1024*1024);
    assert(pt_pattern_history_init(&history,&document.project,commands,16,changes,16)==PT_EDIT_OK);
    /* A new provider cannot adopt somebody else's already populated cache. */
    assert(load(f,1,1,&hit)==PT_CACHE_LOAD);assert(pt_amigus_wavetable_cache_unpin(&f->cache,hit));
    assert(!pt_sampler_wavetable_bind(&bridge,&sampler,&document.project,&f->cache));
    assert(pt_cache_clear(&f->cache.cache));f->writes=0;
    assert(pt_sampler_wavetable_bind(&bridge,&sampler,&document.project,&f->cache));
    assert(!pt_sampler_wavetable_bind(&bridge,&sampler,&document.project,&f->cache));
    sampler.budget=0;assert(sample_load(&bridge,0,&out)==PT_CACHE_CAPACITY && out.serial==999 && !sampler.bytes);
    sampler.budget=1024*1024;refuse=1;assert(sample_load(&bridge,0,&out)==PT_CACHE_CAPACITY);refuse=0;
    assert(!f->writes && !sampler.bytes);exact_save(&document.project,saved,size);
    assert(sample_load(&bridge,0,&old)==PT_CACHE_LOAD);
    assert(pt_sampler_wavetable_location(&bridge,old,&address,&bytes) && address==16 && bytes==8);
    memcpy(held,f->ram+address,8);assert(held[0]==0 && held[1]==1 && held[2]==255 && held[3]==254);
    writes=f->writes;assert(sample_load(&bridge,0,&hit)==PT_CACHE_HIT && writes==f->writes);
    assert(pt_sampler_wavetable_unpin(&bridge,hit));exact_save(&document.project,saved,size);
    refuse=1;assert(pt_sampler_edit(&sampler,&document.project,&history,0,PT_PCM_GAIN,0,4,2000)==PT_EDIT_CAPACITY);refuse=0;
    assert(sample_load(&bridge,0,&hit)==PT_CACHE_HIT && f->writes==writes);
    assert(pt_sampler_wavetable_unpin(&bridge,hit));exact_save(&document.project,saved,size);
    assert(pt_sampler_edit(&sampler,&document.project,&history,0,PT_PCM_GAIN,0,4,2000)==PT_EDIT_OK);
    assert(!pt_sampler_wavetable_location(&bridge,old,&address,&bytes));
    assert(sample_load(&bridge,0,&newer)==PT_CACHE_LOAD);assert(!memcmp(held,f->ram+16,8));
    assert(pt_pattern_undo(&document.project,&history,-1)==PT_EDIT_OK);exact_save(&document.project,saved,size);
    assert(sample_load(&bridge,0,&undo)==PT_CACHE_LOAD);
    assert(pt_sampler_wavetable_location(&bridge,undo,&address,&bytes));assert(!memcmp(f->ram+address,held,8));
    assert(pt_pattern_undo(&document.project,&history,1)==PT_EDIT_OK);
    assert(sample_load(&bridge,0,&redo)==PT_CACHE_LOAD);
    version=bridge.version;
    assert(pt_sampler_loop(&sampler,&document.project,&history,0,PT_LOOP_FORWARD,0,4,0)==PT_EDIT_OK);
    assert(sample_load(&bridge,0,&metadata)==PT_CACHE_LOAD && bridge.version>version);
    version=bridge.version;
    assert(pt_sampler_add_slot(&sampler,&document.project,&history)==PT_EDIT_OK);
    assert(sample_load(&bridge,0,&grown)==PT_CACHE_LOAD && bridge.version>version);
    assert(bridge.table==document.project.samples && bridge.count==document.project.sample_count);
    assert(pt_sampler_wavetable_unpin(&bridge,grown));
    assert(pt_pattern_undo(&document.project,&history,-1)==PT_EDIT_OK); /* Slot/table undo. */
    f->fail=f->writes+2;
    assert(sample_load(&bridge,0,&out)==PT_CACHE_TRANSFER && out.serial==999);f->fail=0;
    assert(!memcmp(held,f->ram+16,8));
    assert(sample_load(&bridge,0,&grown)==PT_CACHE_LOAD);assert(pt_sampler_wavetable_unpin(&bridge,grown));
    assert(pt_pattern_undo(&document.project,&history,-1)==PT_EDIT_OK); /* Loop metadata undo. */
    assert(pt_pattern_undo(&document.project,&history,-1)==PT_EDIT_OK); /* Gain undo. */
    exact_save(&document.project,saved,size);
    /* Replace slot zero with another source, then undo to its true24 master. */
    pt_document_init(&donor,&allocator);assert(pt_document_new(&donor,4,SIZE_MAX)==PT_PROJECT_OK);
    donor.project.samples[0].pcm=(struct pt_pcm){data,4,4,48000,1,16};
    assert(pt_sampler_import_slot(&sampler,&document.project,&history,0,&donor.project,0)==PT_EDIT_OK);
    assert(sample_load(&bridge,0,&grown)==PT_CACHE_LOAD);
    assert(pt_sampler_wavetable_location(&bridge,grown,&address,&bytes));
    assert(f->ram[address]==1 && f->ram[address+1]==1); /* 257 in signed16. */
    assert(pt_sampler_wavetable_unpin(&bridge,grown));
    assert(pt_pattern_undo(&document.project,&history,-1)==PT_EDIT_OK);exact_save(&document.project,saved,size);
    /* Overflow refuses a new revision instead of resurrecting old keys. */
    bridge.version=UINT64_MAX;
    assert(pt_sampler_attributes(&sampler,&document.project,&history,0,"Renamed",64,0)==PT_EDIT_OK);
    assert(sample_load(&bridge,0,&out)==PT_CACHE_INVALID && !bridge.version && out.serial==999);
    assert(!pt_sampler_wavetable_close(&bridge));assert(!pt_amigus_reservation_close(&f->reservation));
    assert(sample_load(&bridge,0,&out)==PT_CACHE_INVALID);
    assert(pt_sampler_wavetable_unpin(&bridge,old));assert(pt_sampler_wavetable_unpin(&bridge,newer));
    assert(pt_sampler_wavetable_unpin(&bridge,undo));assert(pt_sampler_wavetable_unpin(&bridge,redo));
    assert(pt_sampler_wavetable_unpin(&bridge,metadata));
    assert(pt_sampler_wavetable_close(&bridge));assert(pt_amigus_reservation_close(&f->reservation));
    assert(pt_pattern_undo(&document.project,&history,-1)==PT_EDIT_OK);exact_save(&document.project,saved,size);
    pt_pattern_history_release(&history);pt_sampler_release(&sampler);assert(!sampler.bytes);
    pt_document_release(&document);
    /* Rebind another document only after all old leases/cache resources end. */
    init(f,PT_AMIGUS_WAVETABLE);
    assert(pt_amigus_wavetable_cache_attach(&f->cache,&f->reservation,16,112,112,f,bus_owned,bus_write));
    pt_sampler_init(&sampler,&allocator,1024*1024);
    assert(pt_sampler_wavetable_bind(&bridge,&sampler,&donor.project,&f->cache));
    assert(sample_load(&bridge,0,&grown)==PT_CACHE_LOAD && bridge.version==1);
    assert(pt_sampler_wavetable_location(&bridge,grown,&address,&bytes) && f->ram[address]==1 && f->ram[address+1]==1);
    assert(pt_sampler_wavetable_unpin(&bridge,grown));assert(pt_sampler_wavetable_close(&bridge));
    assert(pt_amigus_reservation_close(&f->reservation));pt_sampler_release(&sampler);assert(!sampler.bytes);
    pt_document_release(&donor);assert(!allocations && data[0]==257 && data[1]==-513);
    free(saved);free(f);puts("SAMPLER WAVETABLE PASS: revision rebuilds, edits/undo/table changes, pinned old playback, master save and failure cleanup; fake bus only");return 0;
}
#ifndef PT_SAMPLER_WAVETABLE_NATIVE
int main(void) {return sampler_fixture_main();}
#endif
