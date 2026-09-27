#define PT_WAVETABLE_NATIVE
#include "amigus_wavetable_cache_test.c"
#include "../src/editor/sampler_wavetable.h"
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
    assert(f);assert(wavetable_fixture_main()==0);init(f,PT_AMIGUS_WAVETABLE);
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
