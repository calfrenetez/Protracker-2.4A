/* HOST-only stopped workflow request admission. Real UI usage scans precede
 * genuine three-source/cache/unpublished queue ownership. Byte images are only
 * observations, never live owner copies, restored controls or release authority. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/editor/editor.h"
#include "../src/editor/prepared_paula_mutation_barrier.h"
#include "../src/editor/sampler_internal.h"
#include "../src/editor/sampler_paula_internal.h"

enum {FRAMES=17,PACKED=18,RECORDS=64,IMAGES=48,BUDGET=1024*1024,STEPS=128};
struct allocation {void *data;size_t bytes;unsigned chip;};
struct image {const void *data;size_t bytes;uint8_t *copy;};
struct fixture {
    struct pt_allocator allocator;struct pt_document document;struct pt_editor *editor;
    struct pt_sampler_paula bridge;struct pt_sample_version *pin[3];struct pt_pcm pcm[3];
    int32_t input[3][FRAMES];uint32_t markers[3][2];struct pt_sample saved[3];
    struct allocation allocation[RECORDS];size_t fast_bytes,chip_bytes;
    unsigned fast_calls,fast_frees,chip_calls,chip_frees,backend_calls;
    void *queue;size_t queue_bytes;unsigned queue_calls,queue_frees;
    void *work[3];size_t capacity[3];struct pt_prepared_paula_mutation_barrier *barrier;
    struct pt_prepared_paula_sources *sources;struct pt_prepared_paula_lifetime *lifetime;
    uint8_t *project_gold;size_t project_bytes;unsigned steps;
};
struct counts {size_t fast,chip;unsigned fa,ff,ca,cf,qa,qf,backend;};
static struct counts counts(const struct fixture *f)
{return (struct counts){f->fast_bytes,f->chip_bytes,f->fast_calls,f->fast_frees,
    f->chip_calls,f->chip_frees,f->queue_calls,f->queue_frees,f->backend_calls};}
static void counts_equal(struct counts a,struct counts b)
{assert(a.fast==b.fast&&a.chip==b.chip&&a.fa==b.fa&&a.ff==b.ff&&a.ca==b.ca&&
    a.cf==b.cf&&a.qa==b.qa&&a.qf==b.qf&&a.backend==b.backend);}
static void *allocate(struct fixture *f,size_t bytes,unsigned chip)
{
    unsigned i;size_t used=chip?f->chip_bytes:f->fast_bytes;void *p;
    if(chip)++f->chip_calls;else ++f->fast_calls;
    if(used>BUDGET||bytes>BUDGET-used)return NULL;
    p=malloc(bytes);assert(p);
    for(i=0;i<RECORDS&&f->allocation[i].data;++i){}
    assert(i<RECORDS);f->allocation[i]=(struct allocation){p,bytes,chip};
    if(chip)f->chip_bytes+=bytes;else f->fast_bytes+=bytes;
    memset(p,chip?0xa5:0xcc,bytes);return p;
}
static void release(struct fixture *f,void *p,unsigned chip)
{
    unsigned i;if(!p)return;
    for(i=0;i<RECORDS&&f->allocation[i].data!=p;++i){}
    assert(i<RECORDS&&f->allocation[i].chip==chip);
    if(chip){assert(f->chip_bytes>=f->allocation[i].bytes);f->chip_bytes-=f->allocation[i].bytes;++f->chip_frees;}
    else {assert(f->fast_bytes>=f->allocation[i].bytes);f->fast_bytes-=f->allocation[i].bytes;++f->fast_frees;}
    memset(f->allocation+i,0,sizeof(f->allocation[i]));free(p);
}
static void *fast_allocate(void *c,size_t n){return allocate(c,n,0);}
static void fast_release(void *c,void *p){release(c,p,0);}
static void *chip_allocate(void *c,size_t n){return allocate(c,n,1);}
static void chip_release(void *c,void *p,size_t n)
{
    struct fixture *f=c;unsigned i;
    for(i=0;i<RECORDS&&f->allocation[i].data!=p;++i){}
    assert(i<RECORDS&&f->allocation[i].bytes==n);release(f,p,1);
}
static void *queue_allocate(void *c,size_t n)
{
    struct fixture *f=c;void *p=fast_allocate(c,n);
    assert(!f->queue&&!f->queue_bytes);++f->queue_calls;
    if(p){f->queue=p;f->queue_bytes=n;}return p;
}
static void queue_release(void *c,void *p)
{
    struct fixture *f=c;assert(p&&p==f->queue&&f->queue_bytes);
    f->queue=NULL;f->queue_bytes=0;++f->queue_frees;fast_release(c,p);
}
static int backend_clock(void *c,uint64_t *t,uint32_t *hz)
{struct fixture *f=c;(void)t;(void)hz;++f->backend_calls;assert(0);return 0;}
static int backend_submit(void *c,const struct pt_readers_event *e)
{struct fixture *f=c;(void)e;++f->backend_calls;assert(0);return 0;}
static enum pt_readers_reply backend_command(void *c,uint64_t t,struct pt_readers_command_receipt *o)
{struct fixture *f=c;(void)t;(void)o;++f->backend_calls;assert(0);return PT_READERS_UNCERTAIN;}
static enum pt_readers_reply backend_reader(void *c,const struct pt_readers_domain *d,struct pt_readers_reader_receipt *o)
{struct fixture *f=c;(void)d;(void)o;++f->backend_calls;assert(0);return PT_READERS_UNCERTAIN;}
static unsigned cache_pins(const struct fixture *f)
{unsigned i,n=0;for(i=0;i<PT_CACHE_SLOTS;++i)n+=f->bridge.cache.entry[i].pins;return n;}
static uint8_t signed8(int32_t value,unsigned bits)
{
    int64_t n=value;unsigned shift=bits-8;
    if(shift){int64_t half=INT64_C(1)<<(shift-1);n=n>=0?(n+half)>>shift:-(((-n)+half)>>shift);}
    if(n>127)n=127;
    if(n< -128)n=-128;
    return (uint8_t)(int8_t)n;
}
static void cache_equal(const struct fixture *f)
{
    unsigned i,j,k,seen=0;uint8_t expected[PACKED];
    assert(f->chip_bytes==3*PACKED&&cache_pins(f)==3);
    for(i=0;i<PT_CACHE_SLOTS;++i){const struct pt_cache_entry *e=f->bridge.cache.entry+i;
        if(!e->data)continue;
        assert(e->bytes==PACKED&&e->pins==1&&e->valid&&e->key&&e->serial);
        for(j=0;j<3;++j){for(k=0;k<FRAMES;++k)expected[k]=signed8(f->input[j][k],8+8*j);
            expected[FRAMES]=0;if(!memcmp(e->data,expected,PACKED))break;}
        assert(j<3&&!(seen&(1U<<j)));seen|=1U<<j;
    }
    assert(seen==7);
}
static uint8_t *encode(const struct pt_project *p,size_t *bytes)
{
    uint8_t *out;size_t written;assert(pt_project_size(p,bytes)==PT_PROJECT_OK);
    out=malloc(*bytes);assert(out);
    assert(pt_project_encode(p,out,*bytes,&written)==PT_PROJECT_OK&&written==*bytes);return out;
}
static void master_save_equal(const struct fixture *f)
{
    unsigned i;size_t bytes;uint8_t *out=encode(&f->document.project,&bytes);
    assert(bytes==f->project_bytes&&!memcmp(out,f->project_gold,bytes));free(out);
    assert(!memcmp(f->saved,f->document.project.samples,sizeof(f->saved)));
    for(i=0;i<3;++i){const struct pt_sample *s=f->document.project.samples+i;
        assert(s->pcm.data==f->pcm[i].data&&s->pcm.bits==8+8*i&&s->pcm.frames==FRAMES&&
            !memcmp(s->pcm.data,f->input[i],sizeof(f->input[i]))&&s->slice_count==2&&
            !memcmp(s->slices,f->markers[i],sizeof(f->markers[i])));}
}
/* Full real UI scan and checkbox/selection. No forged preview or selected map. */
static void preview(struct fixture *f,unsigned copy)
{
    struct pt_editor *e=f->editor;unsigned i;
    assert(!e->workflow.pending_apply&&!e->workflow.busy&&!e->workflow.transaction);
    assert(pt_editor_key(e,0x28,9)==PT_UI_NONE&&e->panel==PT_WORKFLOW_MANAGER);
    for(i=0;i<STEPS&&e->workflow.scanning;++i)(void)pt_editor_workflow_idle(e);
    assert(i<STEPS&&!e->workflow.scanning&&e->workflow.usage_valid&&
        pt_sample_usage_current(&e->workflow.usage,e->project,e->history.revision,e->sampler.generation));
    for(i=0;i<PT_PROJECT_SAMPLES;++i)assert(!e->workflow.usage.selected[i]);
    assert(e->workflow.usage.rows[2].flags&PT_USAGE_ELIGIBLE);
    assert(pt_editor_click(e,copy?50:8,304)==PT_UI_NONE&&e->sample==3);
    if(copy){assert(!e->workflow.usage.selected[2]);
        assert(pt_editor_key(e,0x27,9)==PT_UI_NONE&&e->panel==PT_WORKFLOW_TOOLBOX);
        assert(e->sample_range_slot==3&&!e->sample_marking&&e->sample_start==0&&e->sample_end==FRAMES);
    }else assert(e->workflow.usage.selected[2]);
    assert(!e->workflow.pending_apply&&!e->workflow.busy&&!e->workflow.transaction&&
        !e->workflow.scanning&&!e->workflow.resolving&&!e->workflow.wave_building);
    master_save_equal(f);
}
static void image_add(struct image *images,unsigned *n,const void *data,size_t bytes)
{
    struct image *a;if(!bytes)return;
    assert(data&&*n<IMAGES);a=images+(*n)++;*a=(struct image){data,bytes,malloc(bytes)};
    assert(a->copy);memcpy(a->copy,data,bytes);
}
static void refuse_request(struct fixture *f,unsigned copy)
{
    struct pt_editor *e=f->editor;struct counts before=counts(f);
    struct image images[IMAGES];unsigned n=0,i,j,count;uint32_t revision=e->history.revision;
    struct pt_sampler_storage_span spans[PT_SAMPLER_VERSION_SPANS];
    struct pt_prepared_paula_sources_status a={0},b={0};
    assert(!e->workflow.pending_apply&&!e->workflow.transaction&&!e->workflow.busy);
    assert(pt_prepared_paula_sources_get(f->sources,&a)==PT_PREPARED_PAULA_SOURCES_OK&&
        a.phase==PT_PREPARED_PAULA_SOURCES_PREPARED&&a.completed==3&&
        a.master_pins==3&&a.chip_leases==3&&!a.master_job&&!a.chip_job);
    image_add(images,&n,&e->sampler,sizeof(e->sampler));
    image_add(images,&n,&e->workflow.usage,sizeof(e->workflow.usage));
    image_add(images,&n,&f->bridge,sizeof(f->bridge));
    image_add(images,&n,f->work[1],f->capacity[1]);image_add(images,&n,f->work[2],f->capacity[2]);
    image_add(images,&n,f->queue,f->queue_bytes);
    for(i=0;i<3;++i){assert(pt_sampler_version_spans(f->pin[i],spans,PT_SAMPLER_VERSION_SPANS,&count));
        for(j=0;j<count;++j)image_add(images,&n,spans[j].data,spans[j].bytes);}
    for(i=0;i<PT_CACHE_SLOTS;++i)if(f->bridge.cache.entry[i].data)
        image_add(images,&n,f->bridge.cache.entry[i].data,f->bridge.cache.entry[i].bytes);
    /* These are the actual Cleanup/Copy input paths, never a fake hook call. */
    assert(pt_editor_key(e,copy?0x33:0x22,0)==PT_UI_NONE);
    assert(!e->workflow.pending_apply&&!e->workflow.transaction&&!e->workflow.busy&&
        !e->workflow.scanning&&!e->workflow.resolving&&!e->workflow.wave_building);
    assert(e->history.revision==revision&&!e->history.count&&!e->history.cursor);
    assert(!strcmp(e->status,"PLAYBACK STOP UNCONFIRMED - RETRY BEFORE CHANGING PROJECT"));
    for(i=0;i<n;++i){assert(!memcmp(images[i].data,images[i].copy,images[i].bytes));free(images[i].copy);}
    assert(pt_prepared_paula_sources_get(f->sources,&b)==PT_PREPARED_PAULA_SOURCES_OK&&!memcmp(&a,&b,sizeof(a)));
    counts_equal(before,counts(f));master_save_equal(f);cache_equal(f);
    /* Captured copy-range cursor metadata/status may change; pending/job/usage,
     * owned resources and data do not. Never clear pending to help cleanup. */
}
static enum pt_prepared_paula_mutation_result step(struct fixture *f)
{
    enum pt_prepared_paula_mutation_result r;assert(++f->steps<=STEPS);
    r=pt_prepared_paula_mutation_barrier_step(f->barrier);assert(!f->backend_calls);return r;
}
static struct fixture *make(unsigned copy)
{
    struct fixture *f=calloc(1,sizeof(*f));struct pt_prepared_paula_mutation_config c={0};
    unsigned i,j;assert(f);f->allocator=(struct pt_allocator){f,fast_allocate,fast_release};
    pt_document_init(&f->document,&f->allocator);
    assert(pt_document_new(&f->document,4,SIZE_MAX)==PT_PROJECT_OK);
    for(i=0;i<3;++i){unsigned bits=8+8*i;int32_t low=-(int32_t)(UINT32_C(1)<<(bits-1));
        struct pt_sample *s=f->document.project.samples+i;
        for(j=0;j<FRAMES;++j)f->input[i][j]=(int32_t)(((uint64_t)j*139+17*i)%((UINT64_C(1)<<bits)-1))+low;
        f->input[i][0]=low;f->input[i][1]=-low-1;
        f->input[i][2]=i?(int32_t)(UINT32_C(1)<<(bits-9)):1;f->input[i][3]=-f->input[i][2];
        f->markers[i][0]=2;f->markers[i][1]=10;
        s->pcm=(struct pt_pcm){f->input[i],FRAMES,FRAMES,48000,1,(uint8_t)bits};
        s->slices=f->markers[i];s->slice_count=2;s->volume=48;s->name[0]=(char)('A'+i);}
    f->editor=calloc(1,sizeof(*f->editor));assert(f->editor&&pt_editor_init(f->editor,&f->document.project));
    f->editor->sampler.allocator=f->allocator;f->editor->sampler.budget=BUDGET;
    f->editor->sample=1;f->editor->panel=5;
    for(i=0;i<3;++i)assert(pt_sampler_pin(&f->editor->sampler,&f->document.project,i,0,f->pcm+i,f->pin+i)==PT_EDIT_OK&&f->pin[i]);
    memcpy(f->saved,f->document.project.samples,sizeof(f->saved));
    f->project_gold=encode(&f->document.project,&f->project_bytes);
    preview(f,copy); /* Actual completed usage, before any custody attaches. */
    assert(pt_sampler_paula_bind(&f->bridge,&f->editor->sampler,&f->document.project,f,chip_allocate,chip_release,3*PACKED));
    f->capacity[0]=pt_prepared_paula_mutation_barrier_size();f->capacity[1]=pt_prepared_paula_sources_size();
    f->capacity[2]=pt_prepared_paula_lifetime_size();
    for(i=0;i<3;++i){f->work[i]=calloc(1,f->capacity[i]);assert(f->work[i]);}
    f->barrier=f->work[0];f->sources=f->work[1];f->lifetime=f->work[2];
    c.editor=f->editor;c.bridge=&f->bridge;c.sources=f->sources;c.source_capacity=f->capacity[1];
    c.lifetime=f->lifetime;c.lifetime_capacity=f->capacity[2];c.track=0;
    for(i=0;i<3;++i)c.sample[i]=i;
    c.queue.allocator=(struct pt_allocator){f,queue_allocate,queue_release};
    c.queue.grid=(struct pt_scheduled_grid){100,7,1000000,48000};c.queue.session=8101+copy;
    c.queue.frame=100;c.queue.control_budget=BUDGET;c.queue.period=124;c.queue.volume=48;
    c.queue.context_count=1;c.queue.context[0]=(struct pt_prepared_paula_context_extent){f,sizeof(*f)};
    c.backend=(struct pt_readers_backend){f,sizeof(*f),{PT_SCHEDULED_REQUIRED,1,1},PT_READERS_VERSION,
        PT_READERS_REQUIRED,1,backend_clock,backend_submit,backend_command,backend_command,backend_reader,backend_reader};
    assert(pt_prepared_paula_mutation_barrier_begin(f->barrier,f->capacity[0],&c)==PT_PREPARED_PAULA_MUTATION_OK);
    for(;;){enum pt_prepared_paula_mutation_result r=step(f);
        if(r==PT_PREPARED_PAULA_MUTATION_READY)break;
        assert(r==PT_PREPARED_PAULA_MUTATION_PENDING);master_save_equal(f);}
    assert(pt_prepared_paula_mutation_barrier_enqueue_once(f->barrier)==PT_PREPARED_PAULA_MUTATION_QUEUED);
    assert(f->queue&&f->queue_calls==1&&f->queue_frees==0);cache_equal(f);return f;
}
static void cleanup_detach(struct fixture *f)
{
    unsigned i;struct counts before;
    /* The first refused editor call already latched cancel. Cleanup is later,
     * top-level actual local C->R stop and bounded captured source draining. */
    for(;;){enum pt_prepared_paula_mutation_result r=step(f);
        if(r==PT_PREPARED_PAULA_MUTATION_CLEANED)break;
        assert(r==PT_PREPARED_PAULA_MUTATION_PENDING);master_save_equal(f);}
    assert(!f->chip_bytes&&!cache_pins(f)&&!f->queue&&f->queue_calls==f->queue_frees);
    before=counts(f);
    assert(pt_prepared_paula_mutation_barrier_detach(f->barrier)==PT_PREPARED_PAULA_MUTATION_CLOSED);
    counts_equal(before,counts(f));assert(!f->editor->change_ready&&!f->editor->before_change&&!f->editor->before_change_context);
    for(i=0;i<3;++i){free(f->work[i]);f->work[i]=NULL;}
    f->barrier=NULL;f->sources=NULL;f->lifetime=NULL;
}
static void finish(struct fixture *f)
{
    unsigned i;assert(pt_editor_dispose(f->editor));
    for(i=0;i<3;++i){pt_sampler_unpin(f->pin[i]);f->pin[i]=NULL;}
    assert(!f->editor->sampler.bytes&&!f->editor->sampler.live_count&&!f->editor->sampler.pending_count);
    pt_document_release(&f->document);assert(!f->fast_bytes&&!f->chip_bytes&&!f->backend_calls);
    for(i=0;i<RECORDS;++i)assert(!f->allocation[i].data);
    free(f->project_gold);free(f->editor);free(f);
}
static void successful_request_and_undo(struct fixture *f,unsigned copy)
{
    struct pt_editor *e=f->editor;unsigned i;uint32_t revision=e->history.revision;
    size_t history=e->history.count;
    assert(!history&&!e->history.cursor&&!e->workflow.pending_apply&&!e->workflow.transaction);
    preview(f,copy); /* Fresh genuine scan after separate detach, if old tags expire. */
    assert(pt_editor_key(e,copy?0x33:0x22,0)==PT_UI_NONE&&e->workflow.busy&&e->workflow.transaction);
    assert(e->workflow.pending_apply==(copy?2U:1U)&&e->history.revision==revision);
    for(i=0;i<STEPS;++i){(void)pt_editor_workflow_idle(e);
        if(!e->workflow.busy&&!e->workflow.scanning&&!e->workflow.resolving&&!e->workflow.wave_building)break;}
    assert(i<STEPS&&!e->workflow.pending_apply&&!e->workflow.transaction&&
        e->history.count==history+1&&e->history.cursor==1&&e->history.revision!=revision);
    assert(e->workflow.stats.affected_slots==1&&!e->workflow.stats.master_bytes_released);
    if(copy){const struct pt_sample *s=e->project->samples+3;
        assert(e->sample==4&&e->workflow.stats.destination_slot==3&&!e->workflow.stats.appended&&
            s->pcm.bits==24&&s->pcm.channels==1&&s->pcm.frames==FRAMES&&
            s->pcm.data!=f->pcm[2].data&&!memcmp(s->pcm.data,f->input[2],sizeof(f->input[2]))&&
            s->slice_count==2&&s->slices!=f->markers[2]&&!memcmp(s->slices,f->markers[2],sizeof(f->markers[2])));
        assert(!memcmp(f->saved,e->project->samples,sizeof(f->saved)));
    }else {assert(e->workflow.stats.slots_freed==1&&!e->project->samples[2].pcm.frames);
        assert(!memcmp(f->saved,e->project->samples,2*sizeof(*f->saved)));}
    assert(pt_editor_key(e,0x31,8)==PT_UI_NONE&&e->history.count==history+1&&
        !e->history.cursor&&e->history.revision==revision);
    master_save_equal(f);assert(!f->backend_calls&&!f->chip_bytes&&!f->queue);
}
int main(void)
{
    unsigned copy;for(copy=0;copy<2;++copy){struct fixture *f=make(copy);
        refuse_request(f,copy);cleanup_detach(f);
        assert(!f->editor->workflow.pending_apply&&!f->editor->workflow.transaction);
        successful_request_and_undo(f,copy);finish(f);}
    puts("PREPARED PAULA WORKFLOW HOST PASS:2 cases;Cleanup/Copy veto before pending;real completed usage and queued custody;authentic cleanup/detach;postdetach transaction+one undo;zero ledgers;NO NATIVE AUTHORITY");
    return 0;
}
