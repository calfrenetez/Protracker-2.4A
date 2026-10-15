/* SOURCE proposal only. No compiler, runtime or target acceptance.
 * The accepted finite donor supplies genuine document/sampler/prepared cache/
 * lowerer/core ownership and an independent borrowed-reference HOST backend.
 * Its dormant main and handcrafted firing/positive-case functions are not run. */
#define main pt_delivery4_finite_dormant_main
#include "paula_segment_repeat_readers_test.c"
#undef main
#include "../src/editor/paula_delivery4_task.h"
#include "../src/editor/sampler_paula_internal.h"
struct d4_version_audit {
    struct pt_sampler_storage_span span[PT_SAMPLER_VERSION_SPANS];
    unsigned count;unsigned char *before[PT_SAMPLER_VERSION_SPANS];
};
#define D4_PROJECT_EXTENSIONS 32U
#define D4_KNOWN_SPANS (PT_SCHEDULED_SPANS+16U)
struct d4_known_audit {
    struct pt_sampler_storage_span span[D4_KNOWN_SPANS];
    unsigned count;unsigned char *before[D4_KNOWN_SPANS];
};
struct d4_project_storage {
    uint16_t *orders;int32_t *unused;uint32_t *slices;
    struct pt_extension *extensions;uint8_t *payload;
    size_t order_bytes,extension_bytes,unused_values,payload_bytes;
};
struct d4_fixture {
    struct fixture f;
    struct d4_project_storage project_storage;
    struct d4_known_audit known;
    struct pt_cache_lease extra[PT_PAULA_DELIVERY4_DEPENDENCIES];
    struct pt_paula_segment_source dependency[PT_PAULA_DELIVERY4_DEPENDENCIES];
    struct pt_paula_future_packet packet;
    const struct pt_readers_domain *domain;
    struct d4_version_audit version[PT_PAULA_DELIVERY4_DEPENDENCIES];
    uint8_t cache[PT_PAULA_DELIVERY4_DEPENDENCIES][32];
};
static unsigned d4_cases;
/* Complete genuine version spans are compared as opaque bytes. No layout,
 * numeric private refcount, state repair, owner write or certificate is exposed. */
static void d4_audit_release(struct d4_fixture *x)
{
    unsigned i,j;
    for(i=0;i<PT_PAULA_DELIVERY4_DEPENDENCIES;++i){
        for(j=0;j<x->version[i].count;++j)free(x->version[i].before[j]);
        memset(x->version+i,0,sizeof(x->version[i]));
    }
    for(j=0;j<x->known.count;++j)free(x->known.before[j]);
    memset(&x->known,0,sizeof(x->known));
}
static void d4_known_add(struct d4_fixture *x,const void *data,size_t bytes)
{
    struct d4_known_audit *a=&x->known;unsigned i=a->count;
    if(!bytes)return;
    assert(data&&bytes<=1024*1024&&i<D4_KNOWN_SPANS);
    a->span[i]=(struct pt_sampler_storage_span){data,bytes};
    a->before[i]=malloc(bytes);assert(a->before[i]);memcpy(a->before[i],data,bytes);
    ++a->count;
}
/* Additional valid project extents are genuinely allocated and owned by this
 * fixture before its lowerer/reader construction. Slot3 remains unpromoted:
 * it has no sampler version, cache lease, or fabricated owner counter. */
static void d4_project_construct(struct d4_fixture *x)
{
    struct pt_project *p=&x->f.document.project;struct d4_project_storage *s=&x->project_storage;
    unsigned i;
    assert(pt_sampler_paula_close(&x->f.bridge));
    s->order_bytes=PT_PROJECT_ORDERS*sizeof(*s->orders)+sizeof(struct pt_paula_delivery4_action);
    s->orders=x->f.allocator.allocate(x->f.allocator.context,s->order_bytes);
    s->unused_values=sizeof(struct pt_paula_delivery4_action)/sizeof(*s->unused)+64U;
    s->unused=x->f.allocator.allocate(x->f.allocator.context,s->unused_values*sizeof(*s->unused));
    s->slices=x->f.allocator.allocate(x->f.allocator.context,2U*sizeof(*s->slices));
    s->extension_bytes=D4_PROJECT_EXTENSIONS*sizeof(*s->extensions)+sizeof(struct pt_paula_delivery4_action);
    s->extensions=x->f.allocator.allocate(x->f.allocator.context,s->extension_bytes);
    s->payload_bytes=sizeof(struct pt_paula_delivery4_action)+64U;
    assert(s->payload_bytes<=UINT32_MAX);
    s->payload=x->f.allocator.allocate(x->f.allocator.context,s->payload_bytes);
    assert(s->orders&&s->unused&&s->slices&&s->extensions&&s->payload);
    memset(s->orders,0,s->order_bytes);
    for(i=0;i<s->unused_values;++i)s->unused[i]=(int32_t)(i%32U)-16;
    s->slices[0]=4;s->slices[1]=12;
    memset(s->extensions,0,s->extension_bytes);
    for(i=0;i<D4_PROJECT_EXTENSIONS;++i){s->extensions[i].id=0x78000000U+i;s->extensions[i].version=1;}
    memset(s->payload,0x62,s->payload_bytes);
    s->extensions[0].data=s->payload;s->extensions[0].length=(uint32_t)s->payload_bytes;
    p->orders=s->orders;p->order_count=PT_PROJECT_ORDERS;
    p->extensions=s->extensions;p->extension_count=D4_PROJECT_EXTENSIONS;
    p->samples[3].pcm=(struct pt_pcm){s->unused,s->unused_values,32,8000,1,8};
    p->samples[3].slices=s->slices;p->samples[3].slice_count=2;
    assert(!x->f.sampler.current[3]);
    assert(pt_project_validate(p,NULL)==PT_PROJECT_OK);
    assert(pt_sampler_paula_bind(&x->f.bridge,&x->f.sampler,p,&x->f.chip,chip_allocate,chip_release,96));
}
static void d4_project_release(struct d4_fixture *x)
{
    struct d4_project_storage *s=&x->project_storage;
    x->f.allocator.release(x->f.allocator.context,s->orders);
    x->f.allocator.release(x->f.allocator.context,s->unused);
    x->f.allocator.release(x->f.allocator.context,s->slices);
    x->f.allocator.release(x->f.allocator.context,s->extensions);
    x->f.allocator.release(x->f.allocator.context,s->payload);
    memset(s,0,sizeof(*s));
}
static void d4_audit_capture(struct d4_fixture *x)
{
    unsigned i,j;struct d4_version_audit *a;
    d4_audit_release(x);
    for(i=0;i<PT_PAULA_DELIVERY4_DEPENDENCIES;++i){
        a=x->version+i;
        assert(pt_sampler_version_spans(x->f.pin[i],a->span,PT_SAMPLER_VERSION_SPANS,&a->count));
        assert(a->count&&a->count<=PT_SAMPLER_VERSION_SPANS);
        for(j=0;j<a->count;++j){
            assert(a->span[j].data&&a->span[j].bytes&&a->span[j].bytes<=1024*1024);
            a->before[j]=malloc(a->span[j].bytes);assert(a->before[j]);
            memcpy(a->before[j],a->span[j].data,a->span[j].bytes);
        }
        assert(x->dependency[i].cache_bytes==sizeof(x->cache[i]));
        memcpy(x->cache[i],x->dependency[i].cache_data,sizeof(x->cache[i]));
    }
    d4_known_add(x,x->domain,sizeof(*x->domain));
    d4_known_add(x,x->domain->spans,x->domain->count*sizeof(*x->domain->spans));
    d4_known_add(x,x->domain->binding.context,x->domain->binding.context_bytes);
    for(i=0;i<x->domain->count;++i)
        d4_known_add(x,x->domain->spans[i].data,x->domain->spans[i].bytes);
    d4_known_add(x,x->f.document.project.orders,x->project_storage.order_bytes);
    d4_known_add(x,x->f.document.project.events,64U*4U*sizeof(struct pt_event));
    d4_known_add(x,x->f.document.project.samples,x->f.document.project.sample_count*sizeof(struct pt_sample));
    d4_known_add(x,x->project_storage.extensions,x->project_storage.extension_bytes);
    d4_known_add(x,x->project_storage.payload,x->project_storage.payload_bytes);
    d4_known_add(x,x->project_storage.unused,x->project_storage.unused_values*sizeof(int32_t));
    d4_known_add(x,x->project_storage.slices,2U*sizeof(uint32_t));
}
static void d4_audit_equal(struct d4_fixture *x,const struct d4_fixture *before)
{
    unsigned i,j;const struct d4_version_audit *a;
    assert(!memcmp(x,before,sizeof(*x)));
    for(i=0;i<PT_PAULA_DELIVERY4_DEPENDENCIES;++i){
        a=x->version+i;
        for(j=0;j<a->count;++j)assert(!memcmp(a->before[j],a->span[j].data,a->span[j].bytes));
        assert(!memcmp(x->cache[i],x->dependency[i].cache_data,sizeof(x->cache[i])));
    }
    for(j=0;j<x->known.count;++j)
        assert(!memcmp(x->known.before[j],x->known.span[j].data,x->known.span[j].bytes));
    /* Read-only existing donor observation; no embedded-cache operations. */
    assert(pinned(&x->f)==6);storage(&x->f);
}
static void d4_setup(struct d4_fixture *x)
{
    struct pt_render_action action;struct pt_sampler_paula_job job;
    struct pt_sampler_storage_span spans[PT_SAMPLER_VERSION_SPANS];
    unsigned i,j,n;const uint8_t *data;size_t bytes;
    memset(x,0,sizeof(*x));init(&x->f);d4_project_construct(x);
    assert(pt_project_validate(&x->f.document.project,NULL)==PT_PROJECT_OK);
    assert(x->f.sampler.generation==0);
    open_fixture(&x->f);prepare(&x->f);
    /* The real finite lowerer retains its own three pins/leases and constructs
     * the queue/event/domain. Independently acquire another real HIT lease per
     * source; never inspect private reader-pool state or fabricate READY. */
    for(i=0;i<PT_PAULA_DELIVERY4_DEPENDENCIES;++i){
        struct pt_paula_segment_source *s=x->dependency+i;
        memset(&job,0,sizeof(job));
        assert(pt_sampler_paula_job_begin(&job,&x->f.bridge,0,i,0,x->f.pin[i],x->extra+i)==PT_CACHE_HIT);
        assert(!job.owner);
        assert(pt_sampler_paula_prepared_location(&x->f.bridge,0,x->extra[i],&data,&bytes));
        assert(pt_sampler_version_spans(x->f.pin[i],spans,PT_SAMPLER_VERSION_SPANS,&n));
        s->descriptor=&x->f.document.project.samples[i].pcm;s->master=x->f.pcm[i];
        s->version=x->f.pin[i];s->sampler_owner=&x->f.sampler;
        s->project_identity=&x->f.document.project;s->cache_owner=&x->f.bridge;
        s->cache_version=x->f.bridge.version;s->generation=x->f.sampler.generation;s->sample=i;
        s->lease=x->extra[i];s->cache_data=data;s->cache_bytes=bytes;
        for(j=0;j<n;++j)s->guard[s->guard_count++]=(struct pt_paula_segment_span){spans[j].data,spans[j].bytes};
        s->guard[s->guard_count++]=(struct pt_paula_segment_span){&x->f.bridge,sizeof(x->f.bridge)};
        s->guard[s->guard_count++]=(struct pt_paula_segment_span){&x->f.sampler,sizeof(x->f.sampler)};
        s->guard[s->guard_count++]=(struct pt_paula_segment_span){&x->f.document.project,sizeof(x->f.document.project)};
        assert(s->guard_count<=PT_PAULA_SEGMENT_SOURCE_GUARDS);
    }
    assert(pinned(&x->f)==6);
    action=segment(&x->f);assert(enqueue(&x->f,0,&action,NULL)==0);
    assert(pt_paula_segment_readers_publish(x->f.pool,0)==PT_SCHEDULED_OK);
    assert(x->f.model.count==1&&!x->f.model.effects&&!x->f.model.initial);
    x->domain=x->f.model.reader;assert(x->domain);
    x->packet.ticket=x->f.model.command[0].event.scheduled.ticket;
    x->packet.first=x->f.model.command[0].event.scheduled.first;
    x->packet.last=x->f.model.command[0].event.scheduled.last;
    memcpy(&x->packet.geometry,&x->f.model.command[0].plan,sizeof(x->packet.geometry));
    assert(x->packet.geometry.source_count==1&&x->packet.geometry.source[0].sample==0);
    assert(pt_paula_segment_plan_identity_equal(&x->packet.geometry,x->dependency,x->dependency));
    d4_audit_capture(x);
}
static void d4_finish(struct d4_fixture *x,unsigned reader_first)
{
    struct pt_readers_command_receipt cr;struct model_command *c=x->f.model.command;
    enum pt_scheduled_result r;unsigned i;
    d4_audit_release(x);
    /* The independent model never fired or adopted an output. A real explicit
     * cancel calls its backend, which drops its actual borrowed command address.
     * Reader retirement separately drops its original domain reference. */
    assert(!x->f.model.effects&&!x->f.model.initial&&!x->f.model.pending&&!x->f.model.superseded);
    if(reader_first){
        genuine_quiet(&x->f);collect_reader(&x->f);assert(!x->f.model.reader);
        assert(c->borrowed&&c->plan_borrowed&&pinned(&x->f)==6);storage(&x->f);
    }
    c->receipt.action[0].command=PT_READERS_CANCELLED_BEFORE;
    c->receipt.action[0].reader=PT_READERS_NONE;
    c->receipt.action[0].adoption=PT_READERS_UNADOPTED;
    c->reply=PT_READERS_COMMAND_DETACHED;
    r=pt_paula_segment_readers_poll_command(x->f.pool,0,1,&cr);
    assert(r==PT_SCHEDULED_OK||r==PT_SCHEDULED_BACKEND);
    assert(!c->borrowed&&!c->plan_borrowed);
    if(!reader_first){
        assert(x->f.model.reader&&pinned(&x->f)==6);storage(&x->f);
        genuine_quiet(&x->f);collect_reader(&x->f);assert(!x->f.model.reader);
    }
    assert(pt_paula_segment_readers_close(x->f.pool));x->f.pool=NULL;x->domain=NULL;
    assert(pinned(&x->f)==3);
    for(i=0;i<PT_PAULA_DELIVERY4_DEPENDENCIES;++i)assert(pt_sampler_paula_unpin(&x->f.bridge,x->extra[i]));
    assert(!pinned(&x->f)&&pt_sampler_paula_close(&x->f.bridge)&&!x->f.chip.live&&!x->f.chip.bytes);
    storage(&x->f);pt_pattern_history_release(&x->f.history);pt_sampler_release(&x->f.sampler);
    for(i=0;i<PT_PAULA_DELIVERY4_DEPENDENCIES;++i){
        assert(!memcmp(x->f.pcm[i].data,x->f.original[i],sizeof(x->f.original[i])));
        pt_sampler_unpin(x->f.pin[i]);x->f.pin[i]=NULL;
    }
    /* Leaked transient master pins would keep charged versions/allocations
     * alive here; no private reference counter is read or adjusted. */
    assert(!x->f.sampler.bytes);pt_document_release(&x->f.document);d4_project_release(x);assert(!x->f.fast.live);
}
static enum pt_paula_delivery4_snapshot_result d4_call(struct d4_fixture *x,
    const struct pt_paula_future_packet *packet,const struct pt_readers_domain *domain,
    const struct pt_paula_segment_source dependency[3],struct pt_sample_version *const expected[3],
    const struct pt_scheduled_grid *grid,const uint64_t frame[4],unsigned publication,
    struct pt_paula_delivery4_action *out)
{
    return pt_paula_delivery4_task_snapshot(&x->f.bridge,packet,domain,dependency,expected,
        grid,frame,publication,out);
}
static void d4_authentic(void)
{
    struct d4_fixture x,before;struct pt_paula_delivery4_action out;unsigned i,order;
    for(order=0;order<2;++order){
    d4_setup(&x);memcpy(&before,&x,sizeof(before));
    for(i=0;i<32;++i){
        memset(&out,0x53,sizeof(out));
        assert(d4_call(&x,&x.packet,x.domain,x.dependency,x.f.pin,&x.f.config.grid,x.f.config.frame,0,&out)==PT_PAULA_DELIVERY4_SNAPSHOT_OK);
        assert(!memcmp(&out.packet,&x.packet,sizeof(x.packet)));
        assert(out.domain==x.domain&&out.publication==0&&out.frequency==709379);
        assert(pt_paula_delivery4_window_observe(&out,out.packet.ticket,out.packet.first,out.frequency)==PT_PAULA_DELIVERY4_IN_WINDOW);
        assert(pt_paula_delivery4_window_observe(&out,out.packet.ticket,out.packet.last,out.frequency)==PT_PAULA_DELIVERY4_LATE);
        d4_audit_equal(&x,&before);
    }
    d4_finish(&x,order);++d4_cases;
    }
}
static void d4_genuine_edits(void)
{
    struct d4_fixture x,before;struct pt_paula_delivery4_action out,saved;unsigned i;
    for(i=0;i<3;++i){
        d4_setup(&x);
        assert(pt_sampler_edit(&x.f.sampler,&x.f.document.project,&x.f.history,i,PT_PCM_REVERSE,0,32,0)==PT_EDIT_OK);
        assert(x.f.sampler.generation==1&&x.f.sampler.current[i]!=x.f.pin[i]);
        /* Capture the real post-edit baseline, never restore pre-edit history. */
        d4_audit_capture(&x);memcpy(&before,&x,sizeof(before));
        memset(&out,0x63,sizeof(out));memcpy(&saved,&out,sizeof(saved));
        assert(d4_call(&x,&x.packet,x.domain,x.dependency,x.f.pin,&x.f.config.grid,x.f.config.frame,0,&out)==PT_PAULA_DELIVERY4_SNAPSHOT_STALE);
        assert(!memcmp(&out,&saved,sizeof(out)));d4_audit_equal(&x,&before);
        d4_finish(&x,d4_cases&1U);++d4_cases;
    }
}
static void d4_genuine_cache_invalidation(void)
{
    struct d4_fixture x,before;struct pt_paula_delivery4_action out,saved;
    d4_setup(&x);
    assert(pt_sampler_edit(&x.f.sampler,&x.f.document.project,&x.f.history,2,PT_PCM_REVERSE,0,32,0)==PT_EDIT_OK);
    assert(pt_sampler_paula_sync(&x.f.bridge)&&x.f.bridge.version==2&&x.f.sampler.generation==1);
    assert(pt_sampler_paula_prepared_current(&x.f.bridge)&&pinned(&x.f)==6);
    /* Real sync invalidates old generation caches while actual live leases keep
     * their buffers retained. No fabricated invalid bit/version/counter writes. */
    d4_audit_capture(&x);memcpy(&before,&x,sizeof(before));
    memset(&out,0x64,sizeof(out));memcpy(&saved,&out,sizeof(saved));
    assert(d4_call(&x,&x.packet,x.domain,x.dependency,x.f.pin,&x.f.config.grid,x.f.config.frame,0,&out)==PT_PAULA_DELIVERY4_SNAPSHOT_STALE);
    assert(!memcmp(&out,&saved,sizeof(out)));d4_audit_equal(&x,&before);
    d4_finish(&x,d4_cases&1U);++d4_cases;
}
static void d4_dependency_refusals(void)
{
    struct d4_fixture x,before;struct pt_paula_delivery4_action out,saved;
    struct pt_paula_segment_source dependency[3];struct pt_sample_version *expected[3];
    unsigned i,mode;
    for(mode=0;mode<10;++mode)for(i=0;i<3;++i){
        d4_setup(&x);memcpy(&before,&x,sizeof(before));
        memcpy(dependency,x.dependency,sizeof(dependency));memcpy(expected,x.f.pin,sizeof(expected));
        if(mode==0)expected[i]=x.f.pin[(i+1U)%3U];
        else if(mode==1)++dependency[i].generation;
        else if(mode==2)++dependency[i].cache_version;
        else if(mode==3)++dependency[i].lease.serial;
        else if(mode==4)dependency[i].descriptor=x.dependency[(i+1U)%3U].descriptor;
        else if(mode==5)++dependency[i].master.capacity;
        else if(mode==6)--dependency[i].guard_count;
        else if(mode==7)dependency[i].guard[0].data=x.dependency[(i+1U)%3U].guard[0].data;
        else if(mode==8)++dependency[i].guard[dependency[i].guard_count-1U].bytes;
        else{struct pt_paula_segment_span guard=dependency[i].guard[0];
            dependency[i].guard[0]=dependency[i].guard[1];dependency[i].guard[1]=guard;}
        memset(&out,0x73,sizeof(out));memcpy(&saved,&out,sizeof(saved));
        assert(d4_call(&x,&x.packet,x.domain,dependency,expected,&x.f.config.grid,x.f.config.frame,0,&out)==PT_PAULA_DELIVERY4_SNAPSHOT_STALE);
        assert(!memcmp(&out,&saved,sizeof(out)));d4_audit_equal(&x,&before);
        d4_finish(&x,d4_cases&1U);++d4_cases;
    }
}
static void d4_domain_and_windows(void)
{
    struct d4_fixture x,before;struct pt_paula_delivery4_action out,saved;
    struct pt_paula_future_packet packet;struct pt_readers_domain domain;
    struct pt_scheduled_grid grid;uint64_t frame[4];unsigned mode,publication;
    enum pt_paula_delivery4_snapshot_result expected;
    for(mode=0;mode<12;++mode){
        d4_setup(&x);memcpy(&before,&x,sizeof(before));memcpy(&packet,&x.packet,sizeof(packet));
        memcpy(&domain,x.domain,sizeof(domain));memcpy(&grid,&x.f.config.grid,sizeof(grid));
        memcpy(frame,x.f.config.frame,sizeof(frame));publication=0;expected=PT_PAULA_DELIVERY4_SNAPSHOT_INVALID;
        if(mode==0)++domain.key.generation;
        else if(mode==1)++domain.key.slot;
        else if(mode==2)packet.ticket=0;
        else if(mode==3)++packet.first;
        else if(mode==4)++packet.last;
        else if(mode==5)++packet.geometry.frame;
        else if(mode==6)++frame[0];
        else if(mode==7)publication=4;
        else if(mode==8)packet.geometry.kind=PT_PAULA_SEGMENT_REPROGRAM;
        else if(mode==9){domain.count=0;expected=PT_PAULA_DELIVERY4_SNAPSHOT_ALIAS;}
        else if(mode==10)++grid.epoch;
        else{++packet.geometry.source[0].lease.serial;expected=PT_PAULA_DELIVERY4_SNAPSHOT_STALE;}
        memset(&out,0x43,sizeof(out));memcpy(&saved,&out,sizeof(saved));
        assert(d4_call(&x,&packet,&domain,x.dependency,x.f.pin,&grid,frame,publication,&out)==expected);
        assert(!memcmp(&out,&saved,sizeof(out)));d4_audit_equal(&x,&before);
        d4_finish(&x,d4_cases&1U);++d4_cases;
    }
}
static void d4_aliases(void)
{
    struct d4_fixture x,before;const void *target;unsigned mode;
    for(mode=0;mode<23;++mode){
        d4_setup(&x);memcpy(&before,&x,sizeof(before));
        if(mode==0)target=x.dependency[0].cache_data;
        else if(mode==1)target=x.dependency[2].cache_data;
        else if(mode==2)target=x.f.pcm[0].data;
        else if(mode==3)target=x.f.pcm[2].data+28;
        else if(mode==4)target=x.version[2].span[0].data;
        else if(mode==5)target=x.domain;
        else if(mode==6)target=x.domain->spans;
        else if(mode==7)target=&x.f.bridge;
        else if(mode==8)target=&x.f.sampler;
        else if(mode==9)target=&x.f.document.project;
        else if(mode==10)target=x.f.document.project.samples;
        else if(mode==11)target=&x.packet;
        else if(mode==12)target=x.dependency;
        else if(mode==13)target=x.f.pin;
        else if(mode==14)target=&x.f.config.grid;
        else if(mode==15)target=x.f.config.frame;
        else if(mode==16)target=x.f.document.project.orders;
        else if(mode==17)target=x.f.document.project.events;
        else if(mode==18)target=x.f.document.project.extensions;
        else if(mode==19)target=x.project_storage.payload;
        else if(mode==20)target=x.project_storage.unused;
        else if(mode==21)target=x.project_storage.unused+x.project_storage.unused_values-8U;
        else target=x.project_storage.slices;
        if(mode==16)assert(x.project_storage.order_bytes>=sizeof(struct pt_paula_delivery4_action));
        if(mode==17)assert(64U*4U*sizeof(struct pt_event)>=sizeof(struct pt_paula_delivery4_action));
        if(mode==18)assert(x.project_storage.extension_bytes>=sizeof(struct pt_paula_delivery4_action));
        if(mode==19)assert(x.project_storage.payload_bytes>=sizeof(struct pt_paula_delivery4_action));
        if(mode==20)assert(x.project_storage.unused_values*sizeof(int32_t)>=sizeof(struct pt_paula_delivery4_action));
        /* Deliberate aligned output aliases, never descriptor/layout casts for
         * activation. The function must refuse before any output byte is read
         * or written; compare only actual complete known source allocations. */
        assert((uintptr_t)target%offsetof(struct {char byte;struct pt_paula_delivery4_action value;},value)==0);
        assert(d4_call(&x,&x.packet,x.domain,x.dependency,x.f.pin,&x.f.config.grid,x.f.config.frame,0,
            (struct pt_paula_delivery4_action *)target)==PT_PAULA_DELIVERY4_SNAPSHOT_ALIAS);
        d4_audit_equal(&x,&before);d4_finish(&x,d4_cases&1U);++d4_cases;
    }
}
static void d4_window_observer_refusals(void)
{
    struct d4_fixture x,before;
    struct pt_paula_delivery4_action out,original,action,saved;
    const struct pt_paula_delivery4_action *input;
    static const enum pt_paula_delivery4_window expected[14]={
        PT_PAULA_DELIVERY4_BAD_WINDOW,PT_PAULA_DELIVERY4_BAD_WINDOW,
        PT_PAULA_DELIVERY4_BAD_WINDOW,PT_PAULA_DELIVERY4_OLD_TICKET,
        PT_PAULA_DELIVERY4_EARLY,PT_PAULA_DELIVERY4_EARLY,
        PT_PAULA_DELIVERY4_IN_WINDOW,PT_PAULA_DELIVERY4_LATE,
        PT_PAULA_DELIVERY4_IN_WINDOW,PT_PAULA_DELIVERY4_BAD_WINDOW,
        PT_PAULA_DELIVERY4_BAD_WINDOW,PT_PAULA_DELIVERY4_BAD_WINDOW,
        PT_PAULA_DELIVERY4_BAD_WINDOW,PT_PAULA_DELIVERY4_BAD_WINDOW
    };
    uint64_t ticket,observed;
    uint32_t frequency;
    unsigned mode;
    d4_setup(&x);memcpy(&before,&x,sizeof(before));
    memset(&out,0x53,sizeof(out));
    assert(d4_call(&x,&x.packet,x.domain,x.dependency,x.f.pin,
        &x.f.config.grid,x.f.config.frame,0,&out)==PT_PAULA_DELIVERY4_SNAPSHOT_OK);
    assert(!memcmp(&out.packet,&x.packet,sizeof(x.packet)));
    assert(out.domain==x.domain&&out.publication==0&&out.frequency==709379);
    assert(out.packet.first>0&&out.packet.first<out.packet.last);
    memcpy(&original,&out,sizeof(original));d4_audit_equal(&x,&before);
    /* Only pure caller-supplied comparison inputs vary below. The original
     * genuine snapshot, three owners and retained storage remain unchanged.
     * Publication3 is a local comparison-field case, not activation authority.
     * Existing genuine71 already checks first and last on successful snapshots. */
    for(mode=0;mode<14;++mode){
        memcpy(&action,&out,sizeof(action));input=&action;
        ticket=out.packet.ticket;observed=out.packet.first;frequency=out.frequency;
        if(mode==0)input=NULL;
        else if(mode==1)frequency=0;
        else if(mode==2)--frequency;
        else if(mode==3)ticket^=UINT64_C(1);
        else if(mode==4)observed=0;
        else if(mode==5)observed=out.packet.first-1;
        else if(mode==6)observed=out.packet.last-1;
        else if(mode==7)observed=UINT64_MAX;
        else if(mode==8)action.publication=3;
        else if(mode==9)action.publication=4;
        else if(mode==10)action.packet.ticket=0;
        else if(mode==11)action.packet.last=action.packet.first;
        else if(mode==12)action.packet.last=action.packet.first-1;
        else action.frequency=0;
        memcpy(&saved,&action,sizeof(saved));
        assert(pt_paula_delivery4_window_observe(input,ticket,observed,frequency)==expected[mode]);
        assert(!memcmp(&action,&saved,sizeof(action)));
        assert(!memcmp(&out,&original,sizeof(out)));
        d4_audit_equal(&x,&before);++d4_cases;
    }
    d4_finish(&x,0);
}

#ifdef PT_DELIVERY4_EXTERNAL_MAIN
static int pt_delivery4_fixture_main(void)
#else
int main(void)
#endif
{
    d4_authentic();d4_genuine_edits();d4_genuine_cache_invalidation();d4_dependency_refusals();
    d4_domain_and_windows();d4_aliases();d4_window_observer_refusals();
    assert(d4_cases==85);
    puts("delivery4 genuine-owner snapshot and window proposal: 85 cases");
    return 0;
}
