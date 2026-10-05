#include "paula_readers_song.h"
#include "sampler_internal.h"
#include "project_snapshot.h"
#include <limits.h>
#include <stddef.h>
#include <string.h>

/* Fixed heap metadata, not a semantic certificate or activation-time state. */
#define SONG_SPANS 6144U
#define SONG_ALLOCS 64U
struct song_span {const void *data;size_t bytes;};
/* Scratch only: populated from actual inputs before the first allocation.
 * No caller-written completion state is ever consumed. */
struct song_begin_workspace {
    struct pt_allocator allocator;
    struct pt_sampler sampler;
    struct pt_project header;
    struct pt_paula_readers_song_config config;
    struct pt_sample sample[PT_PROJECT_SAMPLES];
    struct song_span spans[SONG_SPANS];
    unsigned count;
};
struct song_begin_alignment {char byte;struct song_begin_workspace workspace;};
struct song_allocation {void *data;size_t bytes;unsigned chip;};
struct song_command {
    struct pt_paula_readers_command *holder;
    uint64_t ticket,frame;
    unsigned published;
};
struct song_reader {
    struct pt_paula_readers_reader *holder;
    uint64_t trigger;unsigned action,track;
};
struct song_allocator_context {struct pt_paula_readers_song *owner;};
struct pt_paula_readers_song {
    struct pt_allocator original_allocator,allocator;
    struct pt_paula_readers_song_config config;
    struct pt_paula_readers_config pool_config;
    struct pt_sampler *sampler,sampler_header;
    struct pt_project *project,header;
    struct pt_sample sample[PT_PROJECT_SAMPLES];
    struct pt_sample_version *pins[PT_PROJECT_SAMPLES];
    struct song_span source[SONG_SPANS];unsigned source_count;
    /* Constructor-only borrowed guards; cleared before publishing this owner. */
    struct song_span begin_control[5];unsigned begin_control_count;
    struct song_allocation allocation[SONG_ALLOCS];size_t control_bytes,chip_bytes;
    struct song_allocator_context allocator_context;
    struct song_allocator_context backend_context;
    struct pt_readers_backend backend;
    struct pt_paula_preflight_setup *startup;
    struct pt_paula_preflight *audit;
    struct pt_paula_preflight_report audit_report;
    struct pt_paula_readers_preparation pool_preparation;
    struct pt_render_sequence *sequence;
    struct pt_readers_output *queue;
    struct pt_paula_readers_pool *pool;
    struct pt_render_lookahead lookahead;
    struct pt_render_interval interval;
    struct pt_render_plan plan;
    struct pt_readers_key keys[PT_CHANNEL_LIMIT];
    struct song_command command[PT_PAULA_READERS_SONG_COMMANDS];
    struct song_reader reader[PT_PAULA_READERS_SONG_READERS];
    int8_t map[PT_CHANNEL_LIMIT];
    unsigned prospective[PT_CHANNEL_LIMIT],closed[PT_CHANNEL_LIMIT];
    unsigned lower_track[PT_READERS_ACTIONS],lower_kind[PT_READERS_ACTIONS],lower_count;
    uint32_t revision,generation,remaining;
    uint64_t frame,boundary,terminal,intervals;
    unsigned retain_index,retained,key_index,forecast_ready;
    unsigned pending_index,busy,failed,closing,cancelled,done,stop_requested,stop_boundary,queue_stopped;
    unsigned *release_latch;
    unsigned submit_called;int submit_reply;
    enum pt_paula_readers_song_phase phase;
    enum pt_paula_readers_song_result result;
    enum pt_render_result render_result;
    enum pt_scheduled_result scheduled_result;
};

static int song_span_valid(const void *p,size_t n)
{return !n || (p && (uintptr_t)p<=UINTPTR_MAX-n);}
static int song_apart(const void *a,size_t n,const void *b,size_t m)
{
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return song_span_valid(a,n)&&song_span_valid(b,m)&&
        (!n||!m||(x<=y?n<=y-x:m<=x-y));
}
static int song_add_span(struct song_span *spans,unsigned *count,const void *p,size_t n)
{
    if(!song_span_valid(p,n))return 0;
    if(!n)return 1;
    if(*count==SONG_SPANS)return 0;
    if(spans)spans[*count]=(struct song_span){p,n};
    ++*count;return 1;
}
static int song_source_span(struct song_span *spans,unsigned *count,
    const void *guard,size_t guard_bytes,const void *p,size_t n)
{
    if(guard_bytes&&!song_apart(guard,guard_bytes,p,n))return 0;
    return song_add_span(spans,count,p,n);
}
/* Captures addresses/extents, never values. They remain numeric guards after
 * stale header replacement, so cancellation never walks a former table.
 * A NULL destination streams the same complete enumeration without a ledger.
 * The optional guard refuses any overlap before a caller workspace is written. */
static int song_source_spans(struct song_span *spans,unsigned *count,
    const struct pt_project *p,const struct pt_sampler *s,
    const void *guard,size_t guard_bytes)
{
    unsigned i,j,n;size_t events;
    struct pt_sampler_storage_span version[PT_SAMPLER_VERSION_SPANS]={0};
    if(guard_bytes&&!song_apart(guard,guard_bytes,version,sizeof(version)))return 0;
    if(!song_span_valid(p,sizeof(*p))||!song_span_valid(s,sizeof(*s))||
       p->sample_count>PT_PROJECT_SAMPLES||p->pattern_count>PT_PROJECT_PATTERNS||
       p->order_count>PT_PROJECT_ORDERS||p->extension_count>4090||
       !p->channels.count||p->channels.count>PT_CHANNEL_LIMIT)return 0;
    events=(size_t)p->pattern_count*PT_PROJECT_ROWS*p->channels.count;
    *count=0;
    if(!song_source_span(spans,count,guard,guard_bytes,p,sizeof(*p))||
       !song_source_span(spans,count,guard,guard_bytes,s,sizeof(*s))||
       !song_source_span(spans,count,guard,guard_bytes,p->samples,(size_t)p->sample_count*sizeof(*p->samples))||
       !song_source_span(spans,count,guard,guard_bytes,p->orders,(size_t)p->order_count*sizeof(*p->orders))||
       !song_source_span(spans,count,guard,guard_bytes,p->events,events*sizeof(*p->events))||
       !song_source_span(spans,count,guard,guard_bytes,p->extensions,(size_t)p->extension_count*sizeof(*p->extensions))||
       !song_source_span(spans,count,guard,guard_bytes,s->table,s->table?s->table_bytes:0))return 0;
    for(i=0;i<p->sample_count;++i) {
        const struct pt_sample *v=p->samples+i;
        if(v->pcm.capacity>SIZE_MAX/sizeof(int32_t)||v->slice_count>PT_PROJECT_SLICES||
           !song_source_span(spans,count,guard,guard_bytes,v->pcm.data,v->pcm.capacity*sizeof(int32_t))||
           !song_source_span(spans,count,guard,guard_bytes,v->slices,(size_t)v->slice_count*sizeof(uint32_t)))return 0;
        /* Current ownership remains with the immutable borrowed sampler until
         * our independent retain succeeds. No private version layout is used. */
        if(v->pcm.frames&&!s->current[i])return 0;
    }
    for(i=0;i<PT_PROJECT_SAMPLES;++i)if(s->current[i]) {
        if(!pt_sampler_version_spans(s->current[i],version,PT_SAMPLER_VERSION_SPANS,&n))return 0;
        for(j=0;j<n;++j)if(!song_source_span(spans,count,guard,guard_bytes,version[j].data,version[j].bytes))return 0;
    }
    for(i=0;i<p->extension_count;++i)
        if(!song_source_span(spans,count,guard,guard_bytes,p->extensions[i].data,p->extensions[i].length))return 0;
    return 1;
}
static int song_spans_apart(const struct song_span *s,unsigned count,const void *p,size_t n)
{
    unsigned i;
    if(!song_span_valid(p,n))return 0;
    for(i=0;i<count;++i)if(!song_apart(p,n,s[i].data,s[i].bytes))return 0;
    return 1;
}
/* Fixed control/header comparisons precede every borrowed-descriptor read. */
static int song_current(struct pt_paula_readers_song *s,uint32_t revision)
{
    struct pt_project header;
    if(revision!=s->revision||s->sampler->generation!=s->generation||
       memcmp(s->sampler,&s->sampler_header,sizeof(s->sampler_header))||
       s->project->channels.selected>=s->header.channels.count)return 0;
    memcpy(&header,&s->header,sizeof(header));header.channels.selected=s->project->channels.selected;
    if(!pt_project_snapshot_equal(s->project,&header))return 0;
    return !s->header.sample_count||
        !memcmp(s->project->samples,s->sample,(size_t)s->header.sample_count*sizeof(*s->sample));
}
static int song_output_apart(const struct pt_paula_readers_song *s,const void *out,size_t n)
{
    unsigned i;
    if(!n||!song_apart(out,n,s,sizeof(*s))||
       !song_spans_apart(s->source,s->source_count,out,n)||
       !song_spans_apart(s->begin_control,s->begin_control_count,out,n)||
       !song_apart(out,n,s->config.backend.context,s->config.backend.context_bytes))return 0;
    for(i=0;i<SONG_ALLOCS;++i)if(s->allocation[i].data&&
       !song_apart(out,n,s->allocation[i].data,s->allocation[i].bytes))return 0;
    return 1;
}
static int song_reentry(struct pt_paula_readers_song *s)
{if(s->busy||s->closing){if(s->release_latch)*s->release_latch=1;s->failed=1;return 1;}return 0;}
static enum pt_paula_readers_song_result song_fail(struct pt_paula_readers_song *s,
    enum pt_paula_readers_song_result result)
{s->failed=1;s->phase=PT_PAULA_READERS_SONG_ERROR;s->result=result;return result;}

/* Children use this real allocator; fresh arenas are protected against the
 * WHOLE parent and every registered ordinary/representation allocation before
 * any child sees the pointer. Known aliases are not released as owned blocks. */
static void *song_allocate_kind(struct pt_paula_readers_song *s,size_t n,unsigned chip)
{
    unsigned i;void *p;size_t used=chip?s->chip_bytes:s->control_bytes;
    size_t budget=chip?s->config.readers.chip_budget:s->config.control_budget;
    if(!n||used>budget||n>budget-used||s->failed)return NULL;
    for(i=0;i<SONG_ALLOCS;++i)if(!s->allocation[i].data)break;
    if(i==SONG_ALLOCS)return NULL;
    p=chip?s->config.readers.chip_allocate(s->config.readers.chip_context,n):
        s->original_allocator.allocate(s->original_allocator.context,n);
    if(!p)return NULL;
    if(!song_output_apart(s,p,n)){s->failed=1;return NULL;}
    /* Even a stale callback result is now known fresh/disjoint: use only saved
     * release functions, never source descriptors or mutated arguments. */
    if(!song_current(s,s->revision)||s->failed) {
        if(chip)s->config.readers.chip_release(s->config.readers.chip_context,p,n);
        else s->original_allocator.release(s->original_allocator.context,p);
        s->failed=1;return NULL;
    }
    s->allocation[i]=(struct song_allocation){p,n,chip};
    if(chip)s->chip_bytes+=n;else s->control_bytes+=n;
    return p;
}
static void *song_allocate(void *context,size_t n)
{return song_allocate_kind(((struct song_allocator_context *)context)->owner,n,0);}
static void *song_chip_allocate(void *context,size_t n)
{return song_allocate_kind(((struct song_allocator_context *)context)->owner,n,1);}
static void song_release_kind(struct pt_paula_readers_song *s,void *p,size_t n,unsigned chip)
{
    unsigned i;
    for(i=0;i<SONG_ALLOCS;++i)if(s->allocation[i].data==p&&s->allocation[i].chip==chip)break;
    if(i==SONG_ALLOCS||(chip&&s->allocation[i].bytes!=n)){s->failed=1;return;}
    n=s->allocation[i].bytes;memset(s->allocation+i,0,sizeof(s->allocation[i]));
    if(chip)s->chip_bytes-=n;else s->control_bytes-=n;
    if(chip)s->config.readers.chip_release(s->config.readers.chip_context,p,n);
    else s->original_allocator.release(s->original_allocator.context,p);
    if(!s->closing&&!song_current(s,s->revision))s->failed=1;
}
static void song_release(void *context,void *p)
{song_release_kind(((struct song_allocator_context *)context)->owner,p,0,0);}
static void song_chip_release(void *context,void *p,size_t n)
{song_release_kind(((struct song_allocator_context *)context)->owner,p,n,1);}

/* The real backend remains named in config, including its complete context
 * extent. This separate small wrapper context never aliases child holders or
 * internal output slots. A post-clock failure blocks SUBMIT in the core; actual
 * accepted/uncertain submit and positive independent retirement replies are
 * forwarded faithfully, never relabelled as "no references". */
static int song_backend_clock(void *context,uint64_t *ticks,uint32_t *frequency)
{
    struct pt_paula_readers_song *s=((struct song_allocator_context *)context)->owner;int r;
    if(!s->busy||s->closing||s->failed||!song_current(s,s->revision)){s->failed=1;return 0;}
    r=s->config.backend.read_clock(s->config.backend.context,ticks,frequency);
    if(s->failed||!song_current(s,s->revision)){s->failed=1;return 0;}
    return r;
}
static int song_backend_submit(void *context,const struct pt_readers_event *event)
{
    struct pt_paula_readers_song *s=((struct song_allocator_context *)context)->owner;int r;
    if(!s->busy||s->closing||s->failed||!song_current(s,s->revision)){s->failed=1;return 0;}
    s->submit_called=1;
    r=s->config.backend.submit(s->config.backend.context,event);
    s->submit_reply=r;
    if(!song_current(s,s->revision))s->failed=1;
    return r;
}
static enum pt_readers_reply song_backend_poll_command(void *context,uint64_t ticket,
    struct pt_readers_command_receipt *out)
{
    struct pt_paula_readers_song *s=((struct song_allocator_context *)context)->owner;
    if(!s->busy||s->closing){s->failed=1;return PT_READERS_UNCERTAIN;}
    enum pt_readers_reply r=s->config.backend.poll_command(s->config.backend.context,ticket,out);
    if(!song_current(s,s->revision))s->failed=1;
    return r;
}
static enum pt_readers_reply song_backend_cancel_command(void *context,uint64_t ticket,
    struct pt_readers_command_receipt *out)
{
    struct pt_paula_readers_song *s=((struct song_allocator_context *)context)->owner;
    if(!s->busy||s->closing){s->failed=1;return PT_READERS_UNCERTAIN;}
    enum pt_readers_reply r=s->config.backend.cancel_command(s->config.backend.context,ticket,out);
    if(!song_current(s,s->revision))s->failed=1;
    return r;
}
static enum pt_readers_reply song_backend_poll_reader(void *context,const struct pt_readers_domain *reader,
    struct pt_readers_reader_receipt *out)
{
    struct pt_paula_readers_song *s=((struct song_allocator_context *)context)->owner;
    if(!s->busy||s->closing){s->failed=1;return PT_READERS_UNCERTAIN;}
    enum pt_readers_reply r=s->config.backend.poll_reader(s->config.backend.context,reader,out);
    if(!song_current(s,s->revision))s->failed=1;
    return r;
}
static enum pt_readers_reply song_backend_cancel_reader(void *context,const struct pt_readers_domain *reader,
    struct pt_readers_reader_receipt *out)
{
    struct pt_paula_readers_song *s=((struct song_allocator_context *)context)->owner;
    if(!s->busy||s->closing){s->failed=1;return PT_READERS_UNCERTAIN;}
    enum pt_readers_reply r=s->config.backend.cancel_reader(s->config.backend.context,reader,out);
    if(!song_current(s,s->revision))s->failed=1;
    return r;
}

static struct pt_paula_readers_song_status song_status(const struct pt_paula_readers_song *s)
{
    struct pt_paula_readers_song_status r;unsigned i;
    memset(&r,0,sizeof(r));r.phase=s->failed&&!s->cancelled?PT_PAULA_READERS_SONG_ERROR:s->phase;
    r.result=s->failed?PT_PAULA_READERS_SONG_FAILED:s->result;
    r.boundary_frame=s->boundary;r.terminal_frame=s->terminal;r.intervals=s->intervals;
    r.retained_masters=s->retained;r.done=s->done;r.terminal_stop_requested=s->stop_requested;
    r.cancelled=s->cancelled;r.render_result=s->render_result;r.scheduled_result=s->scheduled_result;
    for(i=0;i<PT_PAULA_READERS_SONG_COMMANDS;++i)if(s->command[i].holder) {
        r.command_mask|=1U<<i;if(s->command[i].published)r.published_mask|=1U<<i;
    }
    for(i=0;i<PT_PAULA_READERS_SONG_READERS;++i)if(s->reader[i].holder)r.reader_mask|=1U<<i;
    return r;
}
size_t pt_paula_readers_song_control_size(void){return sizeof(struct pt_paula_readers_song);}

size_t pt_paula_readers_song_begin_workspace_size(void)
{return sizeof(struct song_begin_workspace);}
size_t pt_paula_readers_song_begin_workspace_alignment(void)
{return offsetof(struct song_begin_alignment,workspace);}

static enum pt_paula_readers_song_result song_begin_in_workspace(const struct pt_allocator *a,
    struct pt_sampler *sampler,struct pt_project *project,
    const struct pt_paula_readers_song_config *config,uint32_t revision,
    void *memory,size_t capacity,struct pt_paula_readers_song **out)
{
    struct song_begin_workspace *saved=NULL;unsigned count;
    struct pt_paula_readers_song *s;enum pt_render_setup_result r;
    struct pt_elapsed_clock clock={0};
    if(!song_span_valid(a,sizeof(*a))||!song_span_valid(config,sizeof(*config))||
       !song_span_valid(out,sizeof(*out))||!a->allocate||!a->release||
       !song_span_valid(memory,capacity)||!memory||
       (uintptr_t)memory%pt_paula_readers_song_begin_workspace_alignment())
        return PT_PAULA_READERS_SONG_INVALID;
    if(capacity<sizeof(struct song_begin_workspace))return PT_PAULA_READERS_SONG_CAPACITY;
    /* Complete streaming guards before ANY caller-workspace write. No callback
     * or semantic PCM/order/event/slice read occurs during these enumerations. */
    if(!song_apart(memory,capacity,a,sizeof(*a))||
       !song_apart(memory,capacity,&clock,sizeof(clock))||
       !song_apart(memory,capacity,config,sizeof(*config))||
       !song_apart(memory,capacity,out,sizeof(*out))||
       !song_apart(memory,capacity,config->backend.context,config->backend.context_bytes)||
       !song_source_spans(NULL,&count,project,sampler,memory,capacity))
        return PT_PAULA_READERS_SONG_INVALID;
    if(!song_apart(out,sizeof(*out),a,sizeof(*a))||
       !song_apart(out,sizeof(*out),&clock,sizeof(clock))||
       !song_apart(out,sizeof(*out),config,sizeof(*config))||
       !song_apart(out,sizeof(*out),config->backend.context,config->backend.context_bytes)||
       !song_source_spans(NULL,&count,project,sampler,out,sizeof(*out)))
        return PT_PAULA_READERS_SONG_INVALID;
    if(*out)return PT_PAULA_READERS_SONG_INVALID;
    if(!config->session||config->absolute_start==UINT64_MAX||!config->grid.generation||
       config->grid.frequency<config->grid.rate||
       config->grid.generation!=config->readers.generation||config->render.rate!=config->grid.rate||
       (config->render.rate!=44100&&config->render.rate!=48000)||
       config->render.pattern_only||config->render.row_range||
       config->readers.maximum_commands!=PT_PAULA_READERS_SONG_COMMANDS||
       config->readers.maximum_readers!=PT_PAULA_READERS_SONG_READERS||
       !config->readers.chip_allocate||!config->readers.chip_release||
       !config->backend.read_clock||!config->backend.submit||!config->backend.poll_command||
       !config->backend.cancel_command||!config->backend.poll_reader||!config->backend.cancel_reader||
       !config->backend.context_bytes||!song_span_valid(config->backend.context,config->backend.context_bytes)||
       pt_elapsed_clock_init(&clock,config->grid.frequency,config->grid.rate,config->grid.epoch,0)!=PT_ELAPSED_OK)
        return PT_PAULA_READERS_SONG_INVALID;
    saved=memory;
    memset(saved,0,sizeof(*saved));
    memcpy(&saved->allocator,a,sizeof(saved->allocator));
    memcpy(&saved->sampler,sampler,sizeof(saved->sampler));
    memcpy(&saved->header,project,sizeof(saved->header));
    memcpy(&saved->config,config,sizeof(saved->config));
    if(saved->header.sample_count)memcpy(saved->sample,project->samples,
        (size_t)saved->header.sample_count*sizeof(*saved->sample));
    if(!song_source_spans(saved->spans,&saved->count,project,sampler,NULL,0))
        return PT_PAULA_READERS_SONG_INVALID;
    if(config->control_budget<sizeof(*s))return PT_PAULA_READERS_SONG_CAPACITY;
    s=saved->allocator.allocate(saved->allocator.context,sizeof(*s));
    if(!s)return PT_PAULA_READERS_SONG_CAPACITY;
    if(!song_spans_apart(saved->spans,saved->count,s,sizeof(*s))||
       !song_apart(s,sizeof(*s),memory,capacity)||!song_apart(s,sizeof(*s),&clock,sizeof(clock))||
       !song_apart(s,sizeof(*s),out,sizeof(*out))||!song_apart(s,sizeof(*s),a,sizeof(*a))||
       !song_apart(s,sizeof(*s),config,sizeof(*config))||
       !song_apart(s,sizeof(*s),saved->config.backend.context,saved->config.backend.context_bytes))
        return PT_PAULA_READERS_SONG_INVALID;
    if(memcmp(a,&saved->allocator,sizeof(*a))||memcmp(config,&saved->config,sizeof(*config))||
       memcmp(sampler,&saved->sampler,sizeof(*sampler))||
       !pt_project_snapshot_equal(project,&saved->header)||
       (saved->header.sample_count&&memcmp(project->samples,saved->sample,
        (size_t)saved->header.sample_count*sizeof(*saved->sample)))||*out) {
        saved->allocator.release(saved->allocator.context,s);return PT_PAULA_READERS_SONG_STALE;
    }
    memset(s,0,sizeof(*s));s->original_allocator=saved->allocator;s->config=saved->config;
    s->project=project;s->header=saved->header;s->sampler=sampler;s->sampler_header=saved->sampler;
    s->generation=sampler->generation;s->revision=revision;s->source_count=saved->count;
    memcpy(s->source,saved->spans,saved->count*sizeof(*s->source));
    if(saved->header.sample_count)memcpy(s->sample,saved->sample,(size_t)saved->header.sample_count*sizeof(*s->sample));
    s->begin_control[0]=(struct song_span){memory,capacity};
    s->begin_control[1]=(struct song_span){a,sizeof(*a)};
    s->begin_control[2]=(struct song_span){config,sizeof(*config)};
    s->begin_control[3]=(struct song_span){out,sizeof(*out)};
    s->begin_control[4]=(struct song_span){&clock,sizeof(clock)};
    s->begin_control_count=5;
    s->control_bytes=sizeof(*s);s->allocator_context.owner=s;
    s->allocator=(struct pt_allocator){&s->allocator_context,song_allocate,song_release};
    s->pool_config=s->config.readers;s->pool_config.chip_context=&s->allocator_context;
    s->pool_config.chip_allocate=song_chip_allocate;s->pool_config.chip_release=song_chip_release;
    s->backend_context.owner=s;s->backend=s->config.backend;
    s->backend.context=&s->backend_context;s->backend.context_bytes=sizeof(s->backend_context);
    s->backend.read_clock=song_backend_clock;s->backend.submit=song_backend_submit;
    s->backend.poll_command=song_backend_poll_command;s->backend.cancel_command=song_backend_cancel_command;
    s->backend.poll_reader=song_backend_poll_reader;s->backend.cancel_reader=song_backend_cancel_reader;
    s->frame=s->boundary=config->absolute_start;s->phase=PT_PAULA_READERS_SONG_INITIAL;
    s->result=PT_PAULA_READERS_SONG_PENDING;s->busy=1;
    r=pt_paula_preflight_setup_begin(project,&s->config.render,NULL,&s->config.caps,1,
        &s->allocator,revision,s->generation,&s->startup);
    s->busy=0;
    if(r!=PT_RENDER_SETUP_PENDING||s->failed||!song_current(s,revision)||
       memcmp(a,&saved->allocator,sizeof(*a))||memcmp(config,&saved->config,sizeof(*config))||*out) {
        s->busy=1;
        if(s->startup)pt_paula_preflight_setup_cancel(&s->startup);
        saved->allocator.release(saved->allocator.context,s);
        return r==PT_RENDER_SETUP_CAPACITY?PT_PAULA_READERS_SONG_CAPACITY:PT_PAULA_READERS_SONG_INVALID;
    }
    memset(s->begin_control,0,sizeof(s->begin_control));s->begin_control_count=0;
    *out=s;return PT_PAULA_READERS_SONG_PENDING;
}

enum pt_paula_readers_song_result pt_paula_readers_song_begin_in_workspace(
    const struct pt_allocator *a,struct pt_sampler *sampler,struct pt_project *project,
    const struct pt_paula_readers_song_config *config,uint32_t revision,
    void *memory,size_t capacity,struct pt_paula_readers_song **out)
{
    return song_begin_in_workspace(a,sampler,project,config,revision,memory,capacity,out);
}

/* Compatibility route only: this local snapshot is intentionally large.
 * The workspace entrypoint above contains no such ledger/sample array. */
enum pt_paula_readers_song_result pt_paula_readers_song_begin(const struct pt_allocator *a,
    struct pt_sampler *sampler,struct pt_project *project,
    const struct pt_paula_readers_song_config *config,uint32_t revision,
    struct pt_paula_readers_song **out)
{
    struct song_begin_workspace saved={0};
    return song_begin_in_workspace(a,sampler,project,config,revision,&saved,sizeof(saved),out);
}

/* Closing genuine retired handles is metadata/allocator work only. A refusal
 * never means retirement; no pointer or key is reconstructed from a receipt. */
static void song_collect(struct pt_paula_readers_song *s)
{
    unsigned i,j;
    for(i=0;i<PT_PAULA_READERS_SONG_COMMANDS;++i)if(s->command[i].holder&&s->command[i].ticket&&
       pt_paula_readers_command_close(s->command[i].holder))memset(s->command+i,0,sizeof(s->command[i]));
    for(i=0;i<PT_PAULA_READERS_SONG_READERS;++i)if(s->reader[i].holder&&
       pt_paula_readers_reader_close(s->reader[i].holder)) {
        for(j=0;j<PT_CHANNEL_LIMIT;++j)if(s->prospective[j]==i+1){s->prospective[j]=0;s->closed[j]=1;}
        memset(s->reader+i,0,sizeof(s->reader[i]));
    }
}
static int song_free_command(struct pt_paula_readers_song *s)
{unsigned i;for(i=0;i<PT_PAULA_READERS_SONG_COMMANDS;++i)if(!s->command[i].holder)return (int)i;return -1;}
static unsigned song_free_readers(struct pt_paula_readers_song *s)
{unsigned i,n=0;for(i=0;i<PT_PAULA_READERS_SONG_READERS;++i)if(!s->reader[i].holder)++n;return n;}
/* Reproduce only slot/request INDEX bookkeeping. All actual geometry/source
 * normalization remains in the genuine lowerer; no duplicated conversions. */
static int song_plan_records(struct pt_paula_readers_song *s)
{
    unsigned i,j,count=0,triggers=0;int first[4]={-1,-1,-1,-1},control[4]={0,0,0,0};
    if(s->plan.count>PT_RENDER_ACTIONS)return -1;
    for(i=0;i<s->plan.count;++i) {
        const struct pt_render_action *a=s->plan.action+i;
        if(a->channel>=s->header.channels.count||
           (a->kind!=PT_RENDER_TRIGGER&&a->kind!=PT_RENDER_SEGMENT&&a->kind!=PT_RENDER_REPEAT&&
            a->kind!=PT_RENDER_STOP&&a->kind!=PT_RENDER_CONTROL))return -1;
        if(s->map[a->channel]<0)continue;
        j=(unsigned)s->map[a->channel];
        if(j>=4||a->kind==PT_RENDER_SEGMENT||a->kind==PT_RENDER_REPEAT)return -1;
        if(first[j]<0)first[j]=(int)i;
        else {
            if(s->plan.action[first[j]].kind!=PT_RENDER_TRIGGER||a->kind!=PT_RENDER_CONTROL||control[j])return -1;
            control[j]=1;
        }
    }
    for(i=0;i<4;++i)if(first[i]>=0) {
        const struct pt_render_action *a=s->plan.action+first[i];
        s->lower_track[count]=a->channel;s->lower_kind[count]=(unsigned)a->kind;
        if(a->kind==PT_RENDER_TRIGGER)++triggers;
        ++count;
    }
    s->lower_count=count;
    return (int)triggers;
}
static enum pt_paula_readers_song_result song_reader_result(struct pt_paula_readers_song *s,
    enum pt_paula_readers_result r)
{
    if(r==PT_PAULA_READERS_CAPACITY||r==PT_PAULA_READERS_BUSY)
        return PT_PAULA_READERS_SONG_WAIT_PRESSURE;
    return song_fail(s,r==PT_PAULA_READERS_STALE?PT_PAULA_READERS_SONG_STALE:PT_PAULA_READERS_SONG_FAILED);
}
static enum pt_paula_readers_song_result song_step_inner(struct pt_paula_readers_song *s,unsigned work)
{
    enum pt_render_setup_result setup;enum pt_paula_readers_result readers;
    unsigned ready=0,i,n;int index;
    switch(s->phase) {
    case PT_PAULA_READERS_SONG_INITIAL:
        setup=pt_paula_preflight_setup_step(s->startup,s->revision,s->generation,work);
        if(setup==PT_RENDER_SETUP_READY)s->phase=PT_PAULA_READERS_SONG_RETAIN;
        else if(setup!=PT_RENDER_SETUP_PENDING)return song_fail(s,setup==PT_RENDER_SETUP_STALE?
            PT_PAULA_READERS_SONG_STALE:PT_PAULA_READERS_SONG_FAILED);
        break;
    case PT_PAULA_READERS_SONG_RETAIN:
        if(s->retain_index<s->header.sample_count) {
            struct pt_pcm pcm;struct pt_sample_version *pin=NULL;i=s->retain_index;
            if(s->sample[i].pcm.frames) {
                if(pt_sampler_pin_current(s->sampler,s->project,i,s->generation,
                   s->sampler_header.current[i],&pcm,&pin)!=PT_EDIT_OK)
                    return song_fail(s,PT_PAULA_READERS_SONG_STALE);
                s->pins[i]=pin;++s->retained;
            }
            ++s->retain_index;break;
        }
        setup=pt_paula_preflight_setup_transfer(&s->startup,s->revision,s->generation,&s->audit);
        if(setup==PT_RENDER_SETUP_CAPACITY)return PT_PAULA_READERS_SONG_WAIT_PRESSURE;
        if(setup!=PT_RENDER_SETUP_READY)return song_fail(s,PT_PAULA_READERS_SONG_FAILED);
        s->phase=PT_PAULA_READERS_SONG_AUDIT;break;
    case PT_PAULA_READERS_SONG_AUDIT:
        switch(pt_paula_preflight_step(s->audit,&s->audit_report)) {
        case PT_PAULA_PENDING:break;
        case PT_PAULA_COMPATIBLE:
            if(s->audit_report.frames>UINT64_MAX-s->config.absolute_start||
               s->config.absolute_start+s->audit_report.frames==UINT64_MAX)
                return song_fail(s,PT_PAULA_READERS_SONG_FAILED);
            s->terminal=s->config.absolute_start+s->audit_report.frames;
            if(pt_channels_paula_map(&s->project->channels,NULL,s->map)!=PT_CHANNEL_OK||
               memcmp(s->map,s->audit_report.map,sizeof(s->map)))return song_fail(s,PT_PAULA_READERS_SONG_FAILED);
            readers=pt_paula_readers_prepare_begin(&s->pool_preparation,&s->allocator,s->sampler,
                s->project,&s->pool_config,s->revision);
            if(readers!=PT_PAULA_READERS_OK)return song_reader_result(s,readers);
            s->phase=PT_PAULA_READERS_SONG_POOL_VALIDATE;break;
        default:return song_fail(s,PT_PAULA_READERS_SONG_FAILED);
        }
        break;
    case PT_PAULA_READERS_SONG_POOL_VALIDATE:
        readers=pt_paula_readers_prepare_step(&s->pool_preparation,s->revision,work);
        if(readers==PT_PAULA_READERS_OK)s->phase=PT_PAULA_READERS_SONG_OUTPUT_SETUP;
        else if(readers!=PT_PAULA_READERS_PENDING)return song_reader_result(s,readers);
        break;
    case PT_PAULA_READERS_SONG_OUTPUT_SETUP:
        if(!s->queue) {
            s->scheduled_result=pt_readers_open(&s->allocator,&s->config.grid,s->config.session,
                &s->backend,PT_PAULA_READERS_SONG_COMMANDS,PT_PAULA_READERS_SONG_READERS,&s->queue);
            if(s->scheduled_result==PT_SCHEDULED_CAPACITY)return PT_PAULA_READERS_SONG_WAIT_PRESSURE;
            if(s->scheduled_result!=PT_SCHEDULED_OK)return song_fail(s,PT_PAULA_READERS_SONG_FAILED);
            break;
        }
        if(!s->pool) {
            readers=pt_paula_readers_prepare_transfer(&s->pool_preparation,s->revision,&s->pool);
            if(readers!=PT_PAULA_READERS_OK)return song_reader_result(s,readers);
            break;
        }
        if(!s->sequence) {
            if(!pt_paula_preflight_transfer(s->audit,&s->sequence))return song_fail(s,PT_PAULA_READERS_SONG_FAILED);
            break;
        }
        pt_paula_preflight_close(&s->audit);s->phase=PT_PAULA_READERS_SONG_NEXT;break;
    case PT_PAULA_READERS_SONG_NEXT:
        s->render_result=pt_render_sequence_next(s->sequence,&s->interval);
        if(s->render_result!=PT_RENDER_OK)return song_fail(s,PT_PAULA_READERS_SONG_FAILED);
        if(s->interval.frames>UINT64_MAX-s->frame||s->frame+s->interval.frames==UINT64_MAX)
            return song_fail(s,PT_PAULA_READERS_SONG_FAILED);
        s->boundary=s->frame+s->interval.frames;s->remaining=s->interval.frames;++s->intervals;
        s->render_result=pt_render_lookahead_begin(&s->lookahead,s->sequence);
        if(s->render_result!=PT_RENDER_OK)return song_fail(s,PT_PAULA_READERS_SONG_FAILED);
        s->forecast_ready=0;s->phase=PT_PAULA_READERS_SONG_FORECAST;break;
    case PT_PAULA_READERS_SONG_FORECAST:
        if(!s->forecast_ready) {
            n=work>256?256:work;
            s->render_result=pt_render_lookahead_step(&s->lookahead,n,&s->plan,&s->forecast_ready);
            if(s->render_result!=PT_RENDER_OK)return song_fail(s,PT_PAULA_READERS_SONG_FAILED);
            break;
        }
        if(s->remaining) {
            n=work>256?256:work;if(n>s->remaining)n=s->remaining;
            s->render_result=pt_render_sequence_consume(s->sequence,n);
            if(s->render_result!=PT_RENDER_OK)return song_fail(s,PT_PAULA_READERS_SONG_FAILED);
            s->remaining-=n;break;
        }
        if(s->interval.end) {
            if(s->plan.count||s->boundary!=s->terminal)return song_fail(s,PT_PAULA_READERS_SONG_FAILED);
            s->render_result=pt_render_lookahead_commit(&s->lookahead);
            if(s->render_result!=PT_RENDER_OK)return song_fail(s,PT_PAULA_READERS_SONG_FAILED);
            s->frame=s->boundary;s->done=1;s->phase=PT_PAULA_READERS_SONG_END;
            return PT_PAULA_READERS_SONG_DONE;
        }
        memset(s->keys,0,sizeof(s->keys));s->key_index=0;s->phase=PT_PAULA_READERS_SONG_KEYS;break;
    case PT_PAULA_READERS_SONG_KEYS:
        /* One track/key lookup per call. A pending replacement is the sole
         * prospective origin; never fall back to an old active same-slot key. */
        /* Validate finite count/kinds/tracks before the first plan walk or key
         * wait. The genuine lowerer still owns source/geometry conversion. */
        if(!s->key_index&&song_plan_records(s)<0)return song_fail(s,PT_PAULA_READERS_SONG_FAILED);
        if(s->key_index<PT_CHANNEL_LIMIT) {
            unsigned track=s->key_index,need=0,trigger=0;
            for(i=0;i<s->plan.count;++i)if(s->plan.action[i].channel==track) {
                if(s->plan.action[i].kind==PT_RENDER_TRIGGER)trigger=1;
                if(s->plan.action[i].kind==PT_RENDER_CONTROL||s->plan.action[i].kind==PT_RENDER_STOP)need=1;
            }
            if(s->map[track]>=0&&need&&!trigger) {
                if(!s->prospective[track]||s->closed[track])return song_fail(s,PT_PAULA_READERS_SONG_FAILED);
                i=s->prospective[track]-1;
                s->scheduled_result=pt_paula_readers_reader_key(s->reader[i].holder,s->keys+track);
                if(s->scheduled_result==PT_SCHEDULED_STALE)return PT_PAULA_READERS_SONG_WAIT_ACTIVE;
                if(s->scheduled_result!=PT_SCHEDULED_OK)return song_fail(s,PT_PAULA_READERS_SONG_FAILED);
            }
            ++s->key_index;break;
        }
        s->phase=PT_PAULA_READERS_SONG_LOWER;break;
    case PT_PAULA_READERS_SONG_LOWER:
        song_collect(s);
        if(s->failed)return song_fail(s,PT_PAULA_READERS_SONG_FAILED);
        if(!song_current(s,s->revision))return song_fail(s,PT_PAULA_READERS_SONG_STALE);
        index=song_plan_records(s);
        if(index<0)return song_fail(s,PT_PAULA_READERS_SONG_FAILED);
        if((unsigned)index>song_free_readers(s))return PT_PAULA_READERS_SONG_WAIT_PRESSURE;
        /* Empty end/other-route boundaries own no command. This also handles
         * a valid project with zero sample slots without invoking the bridge's
         * nonempty-source current predicate. Every kind/track was still checked. */
        if(!s->lower_count) {
            if(!s->stop_boundary) {
                s->render_result=pt_render_lookahead_commit(&s->lookahead);
                if(s->render_result!=PT_RENDER_OK)return song_fail(s,PT_PAULA_READERS_SONG_FAILED);
                s->frame=s->boundary;
            }
            s->phase=s->stop_boundary?PT_PAULA_READERS_SONG_END:PT_PAULA_READERS_SONG_NEXT;
            return s->done?PT_PAULA_READERS_SONG_DONE:PT_PAULA_READERS_SONG_PENDING;
        }
        index=song_free_command(s);
        if(index<0)return PT_PAULA_READERS_SONG_WAIT_PRESSURE;
        s->pending_index=(unsigned)index;
        readers=pt_paula_readers_lower_begin(s->pool,s->queue,s->boundary,s->config.render.rate,
            &s->plan,&s->config.caps,s->keys,&s->command[index].holder);
        if(readers!=PT_PAULA_READERS_OK)return song_reader_result(s,readers);
        if(!s->command[index].holder) {
            if(!s->stop_boundary) {
                s->render_result=pt_render_lookahead_commit(&s->lookahead);
                if(s->render_result!=PT_RENDER_OK)return song_fail(s,PT_PAULA_READERS_SONG_FAILED);
                s->frame=s->boundary;
            }
            s->phase=s->stop_boundary?PT_PAULA_READERS_SONG_END:PT_PAULA_READERS_SONG_NEXT;
            return s->done?PT_PAULA_READERS_SONG_DONE:PT_PAULA_READERS_SONG_PENDING;
        }
        s->phase=PT_PAULA_READERS_SONG_PREPARE;break;
    case PT_PAULA_READERS_SONG_PREPARE:
        readers=pt_paula_readers_step(s->command[s->pending_index].holder,&ready);
        if(readers==PT_PAULA_READERS_OK&&ready)s->phase=PT_PAULA_READERS_SONG_ENQUEUE;
        else if(readers==PT_PAULA_READERS_CAPACITY||readers==PT_PAULA_READERS_BUSY) {
            struct song_command *c=s->command+s->pending_index;
            /* Only local failed preparation is retried by a later explicit
             * step. No transferred ticket/backend attempt is cancelled here. */
            (void)pt_paula_readers_cancel(c->holder);
            if(!pt_paula_readers_command_close(c->holder))return song_fail(s,PT_PAULA_READERS_SONG_FAILED);
            memset(c,0,sizeof(*c));s->phase=PT_PAULA_READERS_SONG_LOWER;
            return PT_PAULA_READERS_SONG_WAIT_PRESSURE;
        }else if(readers!=PT_PAULA_READERS_PENDING&&readers!=PT_PAULA_READERS_OK)return song_reader_result(s,readers);
        break;
    case PT_PAULA_READERS_SONG_ENQUEUE: {
        struct song_command *c=s->command+s->pending_index;
        s->scheduled_result=pt_paula_readers_lower_enqueue(c->holder,&c->ticket);
        if(s->scheduled_result==PT_SCHEDULED_CAPACITY)return PT_PAULA_READERS_SONG_WAIT_PRESSURE;
        if(s->scheduled_result!=PT_SCHEDULED_OK)return song_fail(s,PT_PAULA_READERS_SONG_FAILED);
        c->frame=s->boundary;
        for(i=0;i<s->lower_count;++i) {
            unsigned track=s->lower_track[i];
            if(s->lower_kind[i]==PT_RENDER_TRIGGER) {
                for(n=0;n<PT_PAULA_READERS_SONG_READERS;++n)if(!s->reader[n].holder)break;
                if(n==PT_PAULA_READERS_SONG_READERS||pt_paula_readers_reader(c->holder,i,
                   &s->reader[n].holder)!=PT_PAULA_READERS_OK)return song_fail(s,PT_PAULA_READERS_SONG_FAILED);
                s->reader[n].trigger=c->ticket;s->reader[n].action=i;s->reader[n].track=track;
                s->prospective[track]=n+1;s->closed[track]=0;
            }else if(s->lower_kind[i]==PT_RENDER_STOP)s->closed[track]=1;
        }
        if(!s->stop_boundary) {
            s->render_result=pt_render_lookahead_commit(&s->lookahead);
            if(s->render_result!=PT_RENDER_OK)return song_fail(s,PT_PAULA_READERS_SONG_FAILED);
            s->frame=s->boundary;
        }
        s->phase=PT_PAULA_READERS_SONG_PUBLISH;break;
    }
    case PT_PAULA_READERS_SONG_PUBLISH:return PT_PAULA_READERS_SONG_PENDING;
    case PT_PAULA_READERS_SONG_END:return PT_PAULA_READERS_SONG_DONE;
    default:return s->cancelled?PT_PAULA_READERS_SONG_PENDING:PT_PAULA_READERS_SONG_FAILED;
    }
    return PT_PAULA_READERS_SONG_PENDING;
}

enum pt_paula_readers_song_result pt_paula_readers_song_step(struct pt_paula_readers_song *s,
    uint32_t revision,unsigned work,struct pt_paula_readers_song_status *out)
{
    enum pt_paula_readers_song_result r;
    if(!song_span_valid(s,sizeof(*s))||!work||work>PT_PROJECT_VALIDATION_WORK_MAX||
       (out&&!song_output_apart(s,out,sizeof(*out))))return PT_PAULA_READERS_SONG_INVALID;
    if(song_reentry(s))return PT_PAULA_READERS_SONG_BUSY;
    if(s->failed)return s->result==PT_PAULA_READERS_SONG_STALE?s->result:PT_PAULA_READERS_SONG_FAILED;
    if(s->cancelled)return PT_PAULA_READERS_SONG_PENDING;
    if(!song_current(s,revision))return song_fail(s,PT_PAULA_READERS_SONG_STALE);
    s->busy=1;r=song_step_inner(s,work);
    if(s->failed&&r!=PT_PAULA_READERS_SONG_STALE)r=song_fail(s,PT_PAULA_READERS_SONG_FAILED);
    else if(!song_current(s,revision))r=song_fail(s,PT_PAULA_READERS_SONG_STALE);
    s->result=r;
    if(out&&!s->failed&&song_current(s,revision)&&song_output_apart(s,out,sizeof(*out)))
        *out=song_status(s);
    s->busy=0;return r;
}
enum pt_paula_readers_song_result pt_paula_readers_song_get(struct pt_paula_readers_song *s,
    uint32_t revision,struct pt_paula_readers_song_status *out)
{
    if(!song_span_valid(s,sizeof(*s))||(out&&!song_output_apart(s,out,sizeof(*out))))
        return PT_PAULA_READERS_SONG_INVALID;
    if(song_reentry(s))return PT_PAULA_READERS_SONG_BUSY;
    if(!song_current(s,revision))return PT_PAULA_READERS_SONG_STALE;
    if(out)*out=song_status(s);
    return s->failed?PT_PAULA_READERS_SONG_FAILED:s->result;
}
enum pt_scheduled_result pt_paula_readers_song_publish_next(struct pt_paula_readers_song *s,
    uint32_t revision)
{
    unsigned i,best=PT_PAULA_READERS_SONG_COMMANDS;enum pt_scheduled_result r;
    if(!song_span_valid(s,sizeof(*s)))return PT_SCHEDULED_INVALID;
    if(song_reentry(s))return PT_SCHEDULED_BACKEND;
    if(s->failed||s->cancelled||s->phase!=PT_PAULA_READERS_SONG_PUBLISH)return PT_SCHEDULED_INVALID;
    if(!song_current(s,revision)){song_fail(s,PT_PAULA_READERS_SONG_STALE);return PT_SCHEDULED_STALE;}
    for(i=0;i<PT_PAULA_READERS_SONG_COMMANDS;++i)if(s->command[i].holder&&s->command[i].ticket&&!s->command[i].published&&
       (best==PT_PAULA_READERS_SONG_COMMANDS||s->command[i].frame<s->command[best].frame))best=i;
    if(best==PT_PAULA_READERS_SONG_COMMANDS)return PT_SCHEDULED_INVALID;
    s->busy=1;s->submit_called=0;s->submit_reply=0;
    r=pt_readers_publish(s->queue,s->command[best].ticket);s->scheduled_result=r;
    if(r==PT_SCHEDULED_OK||(s->submit_called&&r==PT_SCHEDULED_BACKEND))s->command[best].published=1;
    if(r==PT_SCHEDULED_OK) {
        s->command[best].published=1;
        s->phase=s->stop_boundary?PT_PAULA_READERS_SONG_END:PT_PAULA_READERS_SONG_NEXT;
        s->result=s->done?PT_PAULA_READERS_SONG_DONE:PT_PAULA_READERS_SONG_PENDING;
    }else if(r!=PT_SCHEDULED_PENDING)song_fail(s,PT_PAULA_READERS_SONG_FAILED);
    if(!song_current(s,revision))song_fail(s,PT_PAULA_READERS_SONG_STALE);
    s->busy=0;return s->failed?PT_SCHEDULED_BACKEND:r;
}
enum pt_scheduled_result pt_paula_readers_song_service_command(struct pt_paula_readers_song *s,
    unsigned index,unsigned cancel,struct pt_readers_command_receipt *out)
{
    struct pt_readers_command_receipt receipt;enum pt_scheduled_result r;unsigned i;
    if(!song_span_valid(s,sizeof(*s))||index>=PT_PAULA_READERS_SONG_COMMANDS||cancel>1||
       (out&&!song_output_apart(s,out,sizeof(*out))))return PT_SCHEDULED_INVALID;
    if(song_reentry(s))return PT_SCHEDULED_BACKEND;
    if(!s->queue||!s->command[index].holder||!s->command[index].ticket)return PT_SCHEDULED_INVALID;
    if(out&&!song_current(s,s->revision))return PT_SCHEDULED_STALE;
    memset(&receipt,0,sizeof(receipt));s->busy=1;
    r=cancel?pt_readers_cancel_command(s->queue,s->command[index].ticket,&receipt):
        pt_readers_poll_command(s->queue,s->command[index].ticket,&receipt);
    if(r==PT_SCHEDULED_BACKEND||r==PT_SCHEDULED_CLOCK||r==PT_SCHEDULED_LATE)
        song_fail(s,PT_PAULA_READERS_SONG_FAILED);
    if(cancel)song_fail(s,PT_PAULA_READERS_SONG_FAILED);
    if(receipt.domain==PT_READERS_COMMAND_DOMAIN)for(i=0;i<receipt.count;++i)
        if(receipt.action[i].command!=PT_READERS_WAITING&&receipt.action[i].command!=PT_READERS_ISSUED)
            song_fail(s,PT_PAULA_READERS_SONG_FAILED);
    song_collect(s);
    if(!song_current(s,s->revision))song_fail(s,PT_PAULA_READERS_SONG_STALE);
    if(out&&!s->failed&&song_output_apart(s,out,sizeof(*out))&&receipt.domain==PT_READERS_COMMAND_DOMAIN)*out=receipt;
    s->busy=0;return s->failed?PT_SCHEDULED_BACKEND:r;
}
enum pt_scheduled_result pt_paula_readers_song_service_reader(struct pt_paula_readers_song *s,
    unsigned index,unsigned cancel,struct pt_readers_reader_receipt *out)
{
    struct pt_readers_reader_receipt receipt;enum pt_scheduled_result r;
    if(!song_span_valid(s,sizeof(*s))||index>=PT_PAULA_READERS_SONG_READERS||cancel>1||
       (out&&!song_output_apart(s,out,sizeof(*out))))return PT_SCHEDULED_INVALID;
    if(song_reentry(s))return PT_SCHEDULED_BACKEND;
    if(!s->queue||!s->reader[index].holder)return PT_SCHEDULED_INVALID;
    if(out&&!song_current(s,s->revision))return PT_SCHEDULED_STALE;
    memset(&receipt,0,sizeof(receipt));s->busy=1;
    r=cancel?pt_readers_cancel_reader(s->queue,s->reader[index].trigger,s->reader[index].action,&receipt):
        pt_readers_poll_reader(s->queue,s->reader[index].trigger,s->reader[index].action,&receipt);
    if(cancel&&r!=PT_SCHEDULED_INVALID&&s->prospective[s->reader[index].track]==index+1)
        s->closed[s->reader[index].track]=1;
    if(r==PT_SCHEDULED_BACKEND||r==PT_SCHEDULED_CLOCK||r==PT_SCHEDULED_LATE)
        song_fail(s,PT_PAULA_READERS_SONG_FAILED);
    if(receipt.domain==PT_READERS_READER_DOMAIN) {
        unsigned track=s->reader[index].track;
        if(s->prospective[track]==index+1&&receipt.state>=PT_READERS_STOP_PENDING)s->closed[track]=1;
        if(receipt.state==PT_READERS_STATE_UNKNOWN)song_fail(s,PT_PAULA_READERS_SONG_FAILED);
    }
    song_collect(s);
    if(!song_current(s,s->revision))song_fail(s,PT_PAULA_READERS_SONG_STALE);
    if(out&&!s->failed&&song_output_apart(s,out,sizeof(*out))&&receipt.domain==PT_READERS_READER_DOMAIN)*out=receipt;
    s->busy=0;return s->failed?PT_SCHEDULED_BACKEND:r;
}
enum pt_paula_readers_song_result pt_paula_readers_song_terminal_stop(struct pt_paula_readers_song *s,
    uint32_t revision)
{
    unsigned i;
    if(!song_span_valid(s,sizeof(*s)))return PT_PAULA_READERS_SONG_INVALID;
    if(song_reentry(s))return PT_PAULA_READERS_SONG_BUSY;
    if(s->failed||s->cancelled||!s->done||s->stop_requested||s->phase!=PT_PAULA_READERS_SONG_END)
        return PT_PAULA_READERS_SONG_INVALID;
    if(!song_current(s,revision))return song_fail(s,PT_PAULA_READERS_SONG_STALE);
    memset(&s->plan,0,sizeof(s->plan));
    for(i=0;i<s->header.channels.count;++i)if(s->prospective[i]&&!s->closed[i]) {
        struct pt_render_action *a=s->plan.action+s->plan.count++;
        a->kind=PT_RENDER_STOP;a->channel=i;
    }
    s->stop_requested=1;s->stop_boundary=1;s->boundary=s->terminal;
    memset(s->keys,0,sizeof(s->keys));s->key_index=0;
    s->phase=PT_PAULA_READERS_SONG_KEYS;s->result=PT_PAULA_READERS_SONG_PENDING;
    return s->result;
}
enum pt_paula_readers_song_result pt_paula_readers_song_cancel(struct pt_paula_readers_song *s)
{
    unsigned i;
    if(!song_span_valid(s,sizeof(*s)))return PT_PAULA_READERS_SONG_INVALID;
    if(song_reentry(s))return PT_PAULA_READERS_SONG_BUSY;
    s->busy=1;s->cancelled=1;s->phase=PT_PAULA_READERS_SONG_DRAIN;
    pt_render_lookahead_cancel(&s->lookahead);
    if(s->startup)(void)pt_paula_preflight_setup_cancel(&s->startup);
    if(s->audit)pt_paula_preflight_close(&s->audit);
    (void)pt_paula_readers_prepare_cancel(&s->pool_preparation);
    for(i=0;i<PT_PAULA_READERS_SONG_COMMANDS;++i)if(s->command[i].holder&&!s->command[i].ticket) {
        (void)pt_paula_readers_cancel(s->command[i].holder);
        if(pt_paula_readers_command_close(s->command[i].holder))memset(s->command+i,0,sizeof(s->command[i]));
    }
    if(s->queue&&!s->queue_stopped){s->queue_stopped=1;s->scheduled_result=pt_readers_stop(s->queue);}
    song_collect(s);s->result=PT_PAULA_READERS_SONG_PENDING;s->busy=0;return s->result;
}
int pt_paula_readers_song_close(struct pt_paula_readers_song **out)
{
    struct pt_paula_readers_song *s;struct pt_allocator a;unsigned i,latch=0;
    struct pt_project *project,header;struct pt_sampler *sampler,sampler_header;
    if(!song_span_valid(out,sizeof(*out))||!*out)return 0;
    s=*out;
    if(!song_span_valid(s,sizeof(*s))||!song_output_apart(s,out,sizeof(*out))||song_reentry(s))return 0;
    if(!s->cancelled&&!s->done)return 0;
    a=s->original_allocator;project=s->project;sampler=s->sampler;
    memcpy(&header,project,sizeof(header));memcpy(&sampler_header,sampler,sizeof(sampler_header));
    s->release_latch=&latch;
    s->busy=1;song_collect(s);
    for(i=0;i<PT_PAULA_READERS_SONG_COMMANDS;++i)if(s->command[i].holder){s->release_latch=NULL;s->busy=0;return 0;}
    for(i=0;i<PT_PAULA_READERS_SONG_READERS;++i)if(s->reader[i].holder){s->release_latch=NULL;s->busy=0;return 0;}
    if(s->queue&&(pt_readers_commands_held(s->queue)||pt_readers_readers_held(s->queue))){s->release_latch=NULL;s->busy=0;return 0;}
    s->closing=1;
    if(s->startup)(void)pt_paula_preflight_setup_cancel(&s->startup);
    if(s->audit)pt_paula_preflight_close(&s->audit);
    (void)pt_paula_readers_prepare_cancel(&s->pool_preparation);
    if(s->sequence){pt_render_sequence_close(s->sequence);s->sequence=NULL;}
    if(s->pool){if(!pt_paula_readers_close(s->pool)){s->release_latch=NULL;s->closing=0;s->busy=0;return 0;}s->pool=NULL;}
    if(s->queue){if(!pt_readers_close(s->queue)){s->release_latch=NULL;s->closing=0;s->busy=0;return 0;}s->queue=NULL;}
    for(i=0;i<s->header.sample_count;++i)if(s->pins[i]){pt_sampler_unpin(s->pins[i]);s->pins[i]=NULL;}
    if(s->control_bytes!=sizeof(*s)||s->chip_bytes||*out!=s){s->release_latch=NULL;s->closing=0;s->busy=0;return 0;}
    *out=NULL;a.release(a.context,s);
    /* No read of the freed parent. The local latch and still-borrowed fixed
     * controls detect closing reentry/control mutation without former walks. */
    return !latch&&!*out&&!memcmp(project,&header,sizeof(header))&&
        !memcmp(sampler,&sampler_header,sizeof(sampler_header));
}
