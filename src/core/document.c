#include <string.h>
#include "document.h"
#include "mod_project.h"
#include "pp20.h"
#include "document_retirement_internal.h"
#include <stdint.h>

#define DOCUMENT_STAGE_RECORDS 16U
#define DOCUMENT_STAGE_ALLOCATIONS 8U
struct document_stage {
    struct pt_document *document,*next;
    struct pt_document original;
    struct pt_allocator allocator;
    void *old[7],*accepted[DOCUMENT_STAGE_ALLOCATIONS],*unproven;
    size_t old_bytes[7],bytes[DOCUMENT_STAGE_ALLOCATIONS],unproven_bytes;
    const void *input;size_t input_bytes;
    const struct pt_project_requirements *requirements;
    struct pt_project_requirements requirements_snapshot;
    unsigned occupied,quarantined,fault,unknown_span;
    unsigned char owned[DOCUMENT_STAGE_ALLOCATIONS],release_called[DOCUMENT_STAGE_ALLOCATIONS];
};
static struct document_stage document_stage_records[DOCUMENT_STAGE_RECORDS];
static struct document_stage *stage_active,*stage_adopting;
static int document_stage_output_apart(const void *,size_t);
static int document_stage_busy(const struct pt_document *);
static struct document_stage *document_stage_for_next(const struct pt_document *);
static int document_stage_discard(struct document_stage *);

static struct pt_document *release_active;
static unsigned release_reentry;
static struct {
    struct pt_document *document;
    void *pointer[7];
    size_t bytes[7];
    unsigned valid;
} release_quarantine;
static int document_overlap(const void *a,size_t an,const void *b,size_t bn)
{
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return an&&bn&&x<y+bn&&y<x+an;
}
static int document_numeric_span(const void *p,size_t bytes)
{return p&&bytes&&bytes<=UINTPTR_MAX-(uintptr_t)p;}
static int document_project_allowed(const struct pt_document *d)
{
#if defined(PT_DOCUMENT_TYPED_REPLACEMENT)
    if(!pt_document_typed_replacement_project_allowed(&d->project))return 0;
#endif
#if defined(PT_DOCUMENT_ENROLLMENT_HOST)
    return pt_document_registered_project_replacement_allowed(&d->project);
#else
    (void)d;return 1;
#endif
}
static int document_output_apart(const void *p,size_t bytes)
{
#if defined(PT_DOCUMENT_TYPED_REPLACEMENT)
    if(!pt_document_typed_replacement_output_disjoint(p,bytes))return 0;
#endif
    unsigned i;
    if(!document_numeric_span(p,bytes)||!document_stage_output_apart(p,bytes))return 0;
    if(release_quarantine.valid){
        if(document_overlap(p,bytes,release_quarantine.document,sizeof(struct pt_document)))return 0;
        for(i=0;i<7;++i)
            if(document_overlap(p,bytes,release_quarantine.pointer[i],release_quarantine.bytes[i]))return 0;
    }
#if defined(PT_DOCUMENT_ENROLLMENT_HOST)
    return pt_document_registered_output_disjoint(p,bytes);
#else
    return 1;
#endif
}
static int document_spans(const struct pt_document *d,void *p[7],size_t b[7],unsigned strict)
{
    const struct pt_project_storage *s=&d->storage;size_t count[7],width[7],total=0;unsigned i,j;
    p[0]=s->orders;p[1]=s->events;p[2]=s->samples;p[3]=s->pcm;
    p[4]=s->slices;p[5]=s->extensions;p[6]=s->extension_data;
    count[0]=s->order_capacity;count[1]=s->event_capacity;count[2]=s->sample_capacity;
    count[3]=s->pcm_capacity;count[4]=s->slice_capacity;count[5]=s->extension_capacity;count[6]=s->extension_bytes;
    width[0]=sizeof(uint16_t);width[1]=sizeof(struct pt_event);width[2]=sizeof(struct pt_sample);
    width[3]=sizeof(int32_t);width[4]=sizeof(uint32_t);width[5]=sizeof(struct pt_extension);width[6]=1;
    for(i=0;i<7;++i){
        if(count[i]>SIZE_MAX/width[i])return 0;
        b[i]=count[i]*width[i];
        if((!p[i]&&b[i])||(strict&&p[i]&&!b[i]))return 0;
        /* Legacy partial-staging cleanup has no complete capacity record. It
         * receives a point refusal only and remains outside checked-release
         * acceptance. Never use this route to claim an unknown extent owned. */
        if(p[i]&&!b[i])b[i]=1;
        if(b[i]>SIZE_MAX-total)return 0;
        total+=b[i];
        if(b[i]&&(!document_output_apart(p[i],b[i])||
           document_overlap(p[i],b[i],d,sizeof(*d))))return 0;
        for(j=0;j<i;++j)if(document_overlap(p[i],b[i],p[j],b[j]))return 0;
    }
    return !strict||total==d->allocated_bytes;
}
int pt_document_owned_storage_spans(const struct pt_document *d,void *out[7],size_t bytes[7])
{
    void *p[7];size_t b[7];unsigned i;
    if(!d||!out||!bytes||!document_output_apart(out,7*sizeof(*out))||
       !document_output_apart(bytes,7*sizeof(*bytes))||
       document_overlap(out,7*sizeof(*out),bytes,7*sizeof(*bytes))||
       document_overlap(out,7*sizeof(*out),d,sizeof(*d))||
       document_overlap(bytes,7*sizeof(*bytes),d,sizeof(*d))||
       !document_spans(d,p,b,1))return 0;
    for(i=0;i<7;++i)if(document_overlap(out,7*sizeof(*out),p[i],b[i])||
        document_overlap(bytes,7*sizeof(*bytes),p[i],b[i]))return 0;
    memcpy(out,p,sizeof p);memcpy(bytes,b,sizeof b);return 1;
}
int pt_document_storage_output_disjoint(const struct pt_document *d,const void *out,size_t bytes)
{
    void *p[7];size_t b[7];unsigned i;
    if(!d||!document_numeric_span(out,bytes)||!document_output_apart(out,bytes)||
       document_overlap(out,bytes,d,sizeof(*d))||!document_spans(d,p,b,1))return 0;
    for(i=0;i<7;++i)if(document_overlap(out,bytes,p[i],b[i]))return 0;
    return 1;
}
int pt_document_retirement_callback_active(void)
{return release_active!=NULL||stage_active!=NULL;}

static int document_entry_allowed(struct pt_document *d,unsigned strict)
{
    void *p[7];size_t b[7];
    if(!d||release_active||document_stage_busy(d)||release_quarantine.document==d||
       !document_project_allowed(d)||!document_output_apart(d,sizeof(*d)))return 0;
    return document_spans(d,p,b,strict);
}
int pt_document_retirement_project_busy(const struct pt_project *p)
{
    unsigned i;
    if(!p)return 0;
    if((release_active&&p==&release_active->project)||
       (release_quarantine.valid&&p==&release_quarantine.document->project))return 1;
    for(i=0;i<DOCUMENT_STAGE_RECORDS;++i)
        if(document_stage_records[i].occupied&&p==&document_stage_records[i].document->project)return 1;
    return 0;
}
int pt_document_replacement_allowed(const struct pt_document *d)
{return document_entry_allowed((struct pt_document *)d,1);}
static int document_reentrant(void)
{
    if(stage_active){stage_active->fault=1;return 1;}
    if(!release_active)return 0;
    release_reentry=1;return 1;
}
static void document_retire_entry(struct pt_document *d,unsigned i)
{
    switch(i){
    case 0:d->storage.orders=NULL;d->storage.order_capacity=0;break;
    case 1:d->storage.events=NULL;d->storage.event_capacity=0;break;
    case 2:d->storage.samples=NULL;d->storage.sample_capacity=0;break;
    case 3:d->storage.pcm=NULL;d->storage.pcm_capacity=0;break;
    case 4:d->storage.slices=NULL;d->storage.slice_capacity=0;break;
    case 5:d->storage.extensions=NULL;d->storage.extension_capacity=0;break;
    default:d->storage.extension_data=NULL;d->storage.extension_bytes=0;break;
    }
}
enum pt_document_release_result pt_document_release_checked(struct pt_document *d)
{
    struct pt_document expected;struct pt_allocator allocator;void *p[7];size_t b[7];unsigned i;
    if(document_reentrant())return PT_DOCUMENT_RELEASE_REFUSED;
    if(release_quarantine.valid)return release_quarantine.document==d?PT_DOCUMENT_RELEASE_UNCERTAIN:PT_DOCUMENT_RELEASE_REFUSED;
    if(!document_entry_allowed(d,1)||!document_spans(d,p,b,1))return PT_DOCUMENT_RELEASE_REFUSED;
    memcpy(&allocator,&d->allocator,sizeof allocator);
    for(i=0;i<7;++i)if(p[i]&&!allocator.release)return PT_DOCUMENT_RELEASE_REFUSED;
    /* Save the entire original obligations before the first arbitrary callback.
     * On uncertainty they remain private; a copied header cannot grant retry. */
    release_quarantine.document=d;
    memcpy(release_quarantine.pointer,p,sizeof p);memcpy(release_quarantine.bytes,b,sizeof b);
    release_quarantine.valid=1;release_active=d;release_reentry=0;
    memset(&d->project,0,sizeof d->project);d->loaded=0;d->dirty=0;
    for(i=0;i<7;++i)if(p[i]){
        document_retire_entry(d,i);d->allocated_bytes-=b[i];
        memcpy(&expected,d,sizeof expected);
        allocator.release(allocator.context,p[i]);
        if(release_reentry||memcmp(d,&expected,sizeof expected)||!document_project_allowed(d)){
            release_active=NULL;return PT_DOCUMENT_RELEASE_UNCERTAIN;
        }
    }
    release_active=NULL;memset(&release_quarantine,0,sizeof release_quarantine);
    memset(d,0,sizeof(*d));memcpy(&d->allocator,&allocator,sizeof allocator);return PT_DOCUMENT_RELEASED;
}

void pt_document_init(struct pt_document *d,const struct pt_allocator *a)
{
    if(document_reentrant()||!d||document_stage_busy(d)||release_quarantine.document==d||
       !document_project_allowed(d)||!document_output_apart(d,sizeof(*d)))return;
    memset(d,0,sizeof(*d));if(a)d->allocator=*a;
}
void pt_document_release(struct pt_document *d)
{
    struct pt_allocator a;struct document_stage *stage;
    if(document_reentrant())return;
    stage=document_stage_for_next(d);
    if(stage){(void)document_stage_discard(stage);return;}
    if(!document_entry_allowed(d,0))return;
    if(document_entry_allowed(d,1)){(void)pt_document_release_checked(d);return;}
    /* Legacy partial-staging cleanup preserves its ordinary fresh-allocation
     * contract; it is not checked-retirement or alias-ownership acceptance. */
    memcpy(&a,&d->allocator,sizeof a);
    if(a.release) {
        void *p[7];unsigned i;
        p[0]=d->storage.orders;p[1]=d->storage.events;p[2]=d->storage.samples;
        p[3]=d->storage.pcm;p[4]=d->storage.slices;p[5]=d->storage.extensions;p[6]=d->storage.extension_data;
        for(i=0;i<7;++i)if(p[i])a.release(a.context,p[i]);
    }
    pt_document_init(d,&a);
}

static int document_registered_apart(const void *p,size_t bytes)
{
#if defined(PT_DOCUMENT_ENROLLMENT_HOST)
    return pt_document_registered_output_disjoint(p,bytes);
#else
    (void)p;(void)bytes;return 1;
#endif
}
static int document_stage_busy(const struct pt_document *d)
{
    unsigned i;
    for(i=0;i<DOCUMENT_STAGE_RECORDS;++i)
        if(document_stage_records[i].occupied&&document_stage_records[i].document==d&&
           stage_adopting!=document_stage_records+i)return 1;
    return 0;
}
static int document_stage_output_apart(const void *out,size_t bytes)
{
    unsigned i,j;const struct document_stage *s;
    if(document_overlap(out,bytes,document_stage_records,sizeof document_stage_records))return 0;
    for(i=0;i<DOCUMENT_STAGE_RECORDS;++i){
        s=document_stage_records+i;if(!s->occupied)continue;
        if(s->unknown_span)return 0; /* No representable unknown extent: fail closed. */
        if(stage_adopting!=s){
            if(document_overlap(out,bytes,s->document,sizeof(*s->document)))return 0;
            for(j=0;j<7;++j)if(document_overlap(out,bytes,s->old[j],s->old_bytes[j]))return 0;
        }
        if(s->next&&document_overlap(out,bytes,s->next,sizeof(*s->next)))return 0;
        for(j=0;j<DOCUMENT_STAGE_ALLOCATIONS;++j)
            if(s->accepted[j]&&document_overlap(out,bytes,s->accepted[j],s->bytes[j]))return 0;
        if(s->unproven&&document_overlap(out,bytes,s->unproven,s->unproven_bytes))return 0;
    }
    return 1;
}
static struct document_stage *document_stage_for_next(const struct pt_document *d)
{
    unsigned i;
    for(i=0;i<DOCUMENT_STAGE_RECORDS;++i)
        if(document_stage_records[i].occupied&&!document_stage_records[i].quarantined&&
           document_stage_records[i].next==d)return document_stage_records+i;
    return NULL;
}
static int document_stage_current(const struct document_stage *s)
{
    unsigned i;
    if(!s||!s->occupied||s->quarantined||s->fault||
       memcmp(s->document,&s->original,sizeof s->original)||
       (s->requirements&&memcmp(s->requirements,&s->requirements_snapshot,sizeof s->requirements_snapshot))||
       !document_project_allowed(s->document)||
       !document_registered_apart(s->document,sizeof(*s->document)))return 0;
    /* Original full numeric obligations were captured before callbacks. No
     * descriptor traversal is needed for this original-header comparison. */
    for(i=0;i<7;++i)
        if(s->old_bytes[i]&&!document_registered_apart(s->old[i],s->old_bytes[i]))return 0;
    /* Every earlier issued, still-owned requested span must remain unregistered
     * after each arbitrary allocation/release callback and before any write.
     * Already consumed entries are never reclassified or followed after free. */
    for(i=0;i<DOCUMENT_STAGE_ALLOCATIONS;++i)
        if(s->owned[i]&&!document_registered_apart(s->accepted[i],s->bytes[i]))return 0;
    return 1;
}
static void document_stage_quarantine(struct document_stage *s,void *returned,size_t bytes)
{
    s->quarantined=1;s->unproven=returned;s->unproven_bytes=bytes;
    if(returned&&!document_numeric_span(returned,bytes))s->unknown_span=1;
    s->next=NULL;s->input=NULL;s->input_bytes=0;s->requirements=NULL;
    if(stage_active==s)stage_active=NULL;
    if(stage_adopting==s)stage_adopting=NULL;
    /* Original document/context and known/unproven outstanding allocations
     * remain living. No public init, copied header or retry retires this record. */
}
static struct document_stage *document_stage_begin(struct pt_document *d,
    struct pt_document *next,const void *input,size_t input_bytes)
{
    struct document_stage *s=NULL;unsigned i;
    if(document_reentrant()||!document_entry_allowed(d,1)||
       !d->allocator.allocate||!d->allocator.release||!next||
       !document_output_apart(next,sizeof(*next))||
       document_overlap(next,sizeof(*next),d,sizeof(*d))||
       (input_bytes&&!document_numeric_span(input,input_bytes)))return NULL;
    for(i=0;i<DOCUMENT_STAGE_RECORDS;++i)
        if(!document_stage_records[i].occupied){s=document_stage_records+i;break;}
    if(!s)return NULL;
    memset(s,0,sizeof(*s));
    if(!document_spans(d,s->old,s->old_bytes,1))return NULL;
    memcpy(&s->original,d,sizeof s->original);
    memcpy(&s->allocator,&d->allocator,sizeof s->allocator);
    s->document=d;s->next=next;s->input=input;s->input_bytes=input_bytes;s->occupied=1;
    memset(next,0,sizeof(*next));memcpy(&next->allocator,&s->allocator,sizeof s->allocator);
    return s;
}
static int document_stage_known_alias(const struct document_stage *s,const void *p,size_t bytes)
{
    unsigned i;
    if(document_overlap(p,bytes,s->document,sizeof(*s->document))||
       document_overlap(p,bytes,s->next,sizeof(*s->next))||
       document_overlap(p,bytes,document_stage_records,sizeof document_stage_records)||
       document_overlap(p,bytes,s->input,s->input_bytes)||
       (s->requirements&&document_overlap(p,bytes,s->requirements,sizeof(*s->requirements)))||
       (s->allocator.context&&document_overlap(p,bytes,s->allocator.context,1)))return 1;
    for(i=0;i<7;++i)if(document_overlap(p,bytes,s->old[i],s->old_bytes[i]))return 1;
    for(i=0;i<DOCUMENT_STAGE_ALLOCATIONS;++i)
        if(s->accepted[i]&&document_overlap(p,bytes,s->accepted[i],s->bytes[i]))return 1;
    return 0;
}
static enum pt_project_result document_stage_take(struct document_stage *s,unsigned index,size_t bytes)
{
    void *p;
    if(index>=DOCUMENT_STAGE_ALLOCATIONS||!bytes||s->accepted[index]||
       !document_stage_current(s)){
        document_stage_quarantine(s,NULL,0);return PT_PROJECT_INVALID;
    }
    stage_active=s;p=s->allocator.allocate(s->allocator.context,bytes);stage_active=NULL;
    if(!document_stage_current(s)){
        document_stage_quarantine(s,p,bytes);return PT_PROJECT_INVALID;
    }
    if(!p)return document_stage_discard(s)?PT_PROJECT_CAPACITY:PT_PROJECT_INVALID;
    if(!document_numeric_span(p,bytes)){
        document_stage_quarantine(s,p,bytes);return PT_PROJECT_ALIAS;
    }
    if(document_stage_known_alias(s,p,bytes)){
        /* A returned known borrowed/duplicate address grants no new ownership.
         * Only earlier accepted entries may be retired, each once. */
        return document_stage_discard(s)?PT_PROJECT_ALIAS:PT_PROJECT_INVALID;
    }
    if(!document_output_apart(p,bytes)||!document_registered_apart(p,bytes)){
        document_stage_quarantine(s,p,bytes);return PT_PROJECT_ALIAS;
    }
    s->accepted[index]=p;s->bytes[index]=bytes;s->owned[index]=1;
    return PT_PROJECT_OK;
}
static int document_stage_drop(struct document_stage *s,unsigned index)
{
    void *p;
    if(!s->owned[index])return 1;
    if(!document_stage_current(s)||
       !document_registered_apart(s->accepted[index],s->bytes[index])){
        document_stage_quarantine(s,NULL,0);return 0;
    }
    p=s->accepted[index];s->owned[index]=0;s->release_called[index]=1;
    stage_active=s;s->allocator.release(s->allocator.context,p);stage_active=NULL;
    if(!document_stage_current(s)){
        document_stage_quarantine(s,NULL,0);return 0;
    }
    s->accepted[index]=NULL;s->bytes[index]=0;return 1;
}
static int document_stage_discard(struct document_stage *s)
{
    unsigned i;struct pt_document *next=s->next;struct pt_allocator allocator;
    memcpy(&allocator,&s->allocator,sizeof allocator);
    if(!document_stage_current(s)){document_stage_quarantine(s,NULL,0);return 0;}
    for(i=0;i<DOCUMENT_STAGE_ALLOCATIONS;++i)if(!document_stage_drop(s,i))return 0;
    memset(s,0,sizeof(*s));
    if(next){memset(next,0,sizeof(*next));memcpy(&next->allocator,&allocator,sizeof allocator);}
    return 1;
}
static int document_stage_storage_matches(const struct document_stage *s)
{
    const struct pt_project_storage *t=&s->next->storage;
    const void *p[7]={t->orders,t->events,t->samples,t->pcm,t->slices,t->extensions,t->extension_data};
    size_t n[7]={t->order_capacity,t->event_capacity,t->sample_capacity,t->pcm_capacity,t->slice_capacity,t->extension_capacity,t->extension_bytes};
    size_t w[7]={sizeof(uint16_t),sizeof(struct pt_event),sizeof(struct pt_sample),sizeof(int32_t),sizeof(uint32_t),sizeof(struct pt_extension),1};
    size_t total=0;unsigned i;
    if(memcmp(&s->next->allocator,&s->allocator,sizeof s->allocator))return 0;
    for(i=0;i<7;++i){
        if(n[i]>SIZE_MAX/w[i]||n[i]*w[i]!=s->bytes[i]||p[i]!=s->accepted[i]||
           (p[i]&&!s->owned[i])||s->bytes[i]>SIZE_MAX-total)return 0;
        total+=s->bytes[i];
    }
    return total==s->next->allocated_bytes;
}
/* Reader callbacks share the genuine pre-probe stage and its issued ledger.
 * A positional read may target real staging extension bytes. Such a destination
 * is allowed only as a complete subspan of a still-owned accepted allocation. */
struct document_reader_guard {
    struct document_stage *stage;
    pt_project_read read;
    int (*finish)(void *);
    void *context;
};
static int document_stage_callback_current(const struct document_stage *s)
{
    return document_stage_current(s)&&s->next&&
        document_registered_apart(s->next,sizeof(*s->next))&&
        document_stage_storage_matches(s);
}
static int document_stage_reader_output(const struct document_stage *s,const void *out,size_t bytes)
{
    uintptr_t address=(uintptr_t)out;unsigned i;
    if(!document_numeric_span(out,bytes))return 0;
    for(i=0;i<DOCUMENT_STAGE_ALLOCATIONS;++i)if(s->owned[i]){
        uintptr_t begin=(uintptr_t)s->accepted[i];
        if(address>=begin&&address-begin<=s->bytes[i]&&
           bytes<=s->bytes[i]-(address-begin))return 1;
    }
    return document_output_apart(out,bytes);
}
static int document_guarded_read(void *context,size_t offset,uint8_t *out,size_t bytes)
{
    struct document_reader_guard *g=context;struct document_stage *s=g->stage;
    struct pt_document expected;int result;
    if(!document_stage_callback_current(s)||!document_stage_reader_output(s,out,bytes)){
        document_stage_quarantine(s,NULL,0);return 0;
    }
    memcpy(&expected,s->next,sizeof expected);
    stage_active=s;result=g->read(g->context,offset,out,bytes);stage_active=NULL;
    /* Even a zero return can change ownership. Refuse before decoder resumes. */
    if(!document_stage_callback_current(s)||memcmp(s->next,&expected,sizeof expected)){
        document_stage_quarantine(s,NULL,0);return 0;
    }
    return result;
}
static int document_guarded_finish(struct document_reader_guard *g)
{
    struct document_stage *s=g->stage;struct pt_document expected;int result;
    if(!document_stage_callback_current(s)){
        document_stage_quarantine(s,NULL,0);return 0;
    }
    memcpy(&expected,s->next,sizeof expected);
    stage_active=s;result=g->finish(g->context);stage_active=NULL;
    if(!document_stage_callback_current(s)||memcmp(s->next,&expected,sizeof expected)){
        document_stage_quarantine(s,NULL,0);return 0;
    }
    return result;
}
static enum pt_project_result document_reader_failure(struct document_stage *s,enum pt_project_result result)
{
    /* Current ordinary read/finish failure consumes each issued entry once.
     * Sticky callback uncertainty makes discard retain the living obligations. */
    return document_stage_discard(s)?result:PT_PROJECT_INVALID;
}

static enum pt_project_result document_stage_publish(struct pt_document *d,struct pt_document *next)
{
    struct document_stage *s=document_stage_for_next(next);
    struct pt_document empty;unsigned i;enum pt_document_release_result r;
    if(!s||s->document!=d||!document_stage_current(s)||!document_stage_storage_matches(s)){
        if(s)document_stage_quarantine(s,NULL,0);
        return PT_PROJECT_INVALID;
    }
    /* Packed scratch is positively retired before old release/publication.
     * No allocator callback may run after assigning the new public header. */
    if(!document_stage_drop(s,7))return PT_PROJECT_INVALID;
    stage_adopting=s;r=pt_document_release_checked(d);stage_adopting=NULL;
    if(r!=PT_DOCUMENT_RELEASED){document_stage_quarantine(s,NULL,0);return PT_PROJECT_INVALID;}
    memset(&empty,0,sizeof empty);memcpy(&empty.allocator,&s->allocator,sizeof s->allocator);
    if(memcmp(d,&empty,sizeof empty)||!document_project_allowed(d)){
        document_stage_quarantine(s,NULL,0);return PT_PROJECT_INVALID;
    }
    /* After old retirement, never traverse its released descriptor arrays. */
    for(i=0;i<7;++i)if(s->bytes[i]&&!document_registered_apart(s->accepted[i],s->bytes[i])){
        document_stage_quarantine(s,NULL,0);return PT_PROJECT_INVALID;
    }
    *d=*next;memset(s,0,sizeof(*s));return PT_PROJECT_OK;
}


static enum pt_project_result allocate_staging_record(struct document_stage *s,
    const struct pt_project_requirements *need,size_t budget)
{
    struct pt_document *next=s->next;
    size_t count[7],width[7],bytes[7],total=0;unsigned i;enum pt_project_result r;
    s->requirements=need;memcpy(&s->requirements_snapshot,need,sizeof s->requirements_snapshot);
    count[0]=need->orders;width[0]=sizeof(uint16_t);
    count[1]=need->events;width[1]=sizeof(struct pt_event);
    count[2]=need->samples;width[2]=sizeof(struct pt_sample);
    count[3]=need->pcm_values;width[3]=sizeof(int32_t);
    count[4]=need->slices;width[4]=sizeof(uint32_t);
    count[5]=need->extensions;width[5]=sizeof(struct pt_extension);
    count[6]=need->extension_bytes;width[6]=1;
    for(i=0;i<7;++i){
        if(count[i]>SIZE_MAX/width[i])return document_stage_discard(s)?PT_PROJECT_CAPACITY:PT_PROJECT_INVALID;
        bytes[i]=count[i]*width[i];
        if(bytes[i]>SIZE_MAX-total)return document_stage_discard(s)?PT_PROJECT_CAPACITY:PT_PROJECT_INVALID;
        total+=bytes[i];
    }
    if(total>budget)return document_stage_discard(s)?PT_PROJECT_CAPACITY:PT_PROJECT_INVALID;
    for(i=0;i<7;++i)if(bytes[i]){
        r=document_stage_take(s,i,bytes[i]);if(r!=PT_PROJECT_OK)return r;
        /* Publish a full capacity/charge only after the returned span is accepted.
         * Partial failure cleanup uses the private ledger, never raw next fields. */
        switch(i){
        case 0:next->storage.orders=s->accepted[i];next->storage.order_capacity=count[i];break;
        case 1:next->storage.events=s->accepted[i];next->storage.event_capacity=count[i];break;
        case 2:next->storage.samples=s->accepted[i];next->storage.sample_capacity=count[i];break;
        case 3:next->storage.pcm=s->accepted[i];next->storage.pcm_capacity=count[i];break;
        case 4:next->storage.slices=s->accepted[i];next->storage.slice_capacity=count[i];break;
        case 5:next->storage.extensions=s->accepted[i];next->storage.extension_capacity=count[i];break;
        default:next->storage.extension_data=s->accepted[i];next->storage.extension_bytes=count[i];break;
        }
        next->allocated_bytes+=bytes[i];
    }
    s->requirements=NULL;
    return PT_PROJECT_OK;
}
static enum pt_project_result allocate_staging(struct pt_document *d,
    const struct pt_project_requirements *need,size_t budget,struct pt_document *next)
{
    struct document_stage *s=document_stage_begin(d,next,NULL,0);
    if(!s)return PT_PROJECT_INVALID;
    return allocate_staging_record(s,need,budget);
}
static enum pt_project_result load_unpacked_record(struct pt_document *d,
    const uint8_t *data,size_t length,size_t budget,struct pt_document *next,
    struct document_stage *s)
{
    struct pt_project_requirements need;enum pt_project_result r;int enhanced;
    enhanced=length>=8&&!memcmp(data,"PT24G\r\n\032",8);
    r=enhanced?pt_project_probe(data,length,&need):pt_mod_project_probe(data,length,&need);
    if(r!=PT_PROJECT_OK){if(s&&!document_stage_discard(s))return PT_PROJECT_INVALID;return r;}
    if(!s)s=document_stage_begin(d,next,data,length);
    if(!s)return PT_PROJECT_INVALID;
    r=allocate_staging_record(s,&need,budget);if(r!=PT_PROJECT_OK)return r;
    if(!document_stage_current(s)){document_stage_quarantine(s,NULL,0);return PT_PROJECT_INVALID;}
    r=enhanced?pt_project_decode(data,length,&next->storage,&next->project):
               pt_mod_project_decode(data,length,&next->storage,&next->project);
    if(r!=PT_PROJECT_OK){pt_document_release(next);return r;}
    next->loaded=1;return document_stage_publish(d,next);
}
static enum pt_project_result load_unpacked(struct pt_document *d,const uint8_t *data,size_t length,size_t budget)
{
    struct pt_document next;
    if(document_reentrant()||!document_entry_allowed(d,1))return PT_PROJECT_INVALID;
    if(!data||!d->allocator.allocate||!d->allocator.release)return PT_PROJECT_INVALID;
    return load_unpacked_record(d,data,length,budget,&next,NULL);
}

enum pt_project_result pt_document_load_mod_reader(struct pt_document *d,pt_mod_read read,void *context,size_t length,size_t budget,int (*finish)(void *))
{
    struct pt_project_requirements need;struct pt_document next;struct document_stage *s;
    struct document_reader_guard guarded;enum pt_project_result r;
    if(document_reentrant()||!document_entry_allowed(d,1))return PT_PROJECT_INVALID;
    if(!read||!finish||!d->allocator.allocate||!d->allocator.release)return PT_PROJECT_INVALID;
    s=document_stage_begin(d,&next,NULL,0);if(!s)return PT_PROJECT_INVALID;
    guarded=(struct document_reader_guard){s,read,finish,context};
    r=pt_mod_project_probe_reader(document_guarded_read,&guarded,length,&need);
    if(r!=PT_PROJECT_OK)return document_reader_failure(s,r);
    r=allocate_staging_record(s,&need,budget);if(r!=PT_PROJECT_OK)return r;
    r=pt_mod_project_decode_reader(document_guarded_read,&guarded,length,&next.storage,&next.project);
    if(r!=PT_PROJECT_OK)return document_reader_failure(s,r);
    if(document_guarded_finish(&guarded)!=1)return document_reader_failure(s,PT_PROJECT_INVALID);
    next.loaded=1;return document_stage_publish(d,&next);
}

enum pt_project_result pt_document_load_project_reader(struct pt_document *d,pt_project_read read,void *context,size_t length,size_t budget,int (*finish)(void *))
{
    struct pt_project_requirements need;struct pt_document next;struct document_stage *s;
    struct document_reader_guard guarded;enum pt_project_result r;
    if(document_reentrant()||!document_entry_allowed(d,1))return PT_PROJECT_INVALID;
    if(!read||!finish||!d->allocator.allocate||!d->allocator.release)return PT_PROJECT_INVALID;
    s=document_stage_begin(d,&next,NULL,0);if(!s)return PT_PROJECT_INVALID;
    guarded=(struct document_reader_guard){s,read,finish,context};
    r=pt_project_probe_reader(document_guarded_read,&guarded,length,&need);
    if(r!=PT_PROJECT_OK)return document_reader_failure(s,r);
    r=allocate_staging_record(s,&need,budget);if(r!=PT_PROJECT_OK)return r;
    r=pt_project_decode_reader(document_guarded_read,&guarded,length,&next.storage,&next.project);
    if(r!=PT_PROJECT_OK)return document_reader_failure(s,r);
    if(document_guarded_finish(&guarded)!=1)return document_reader_failure(s,PT_PROJECT_INVALID);
    next.loaded=1;return document_stage_publish(d,&next);
}


enum pt_project_result pt_document_load(struct pt_document *d,const uint8_t *data,size_t length,size_t budget)
{
    struct pt_document next;struct document_stage *s;
    uint8_t *plain;size_t size,written;enum pt_project_result result,r;
    if(document_reentrant()||!document_entry_allowed(d,1))return PT_PROJECT_INVALID;
    if(!data||!d->allocator.allocate||!d->allocator.release)return PT_PROJECT_INVALID;
    if(length>=4&&!memcmp(data,"PX20",4))return PT_PROJECT_UNSUPPORTED;
    if(length<4||memcmp(data,"PP20",4))return load_unpacked(d,data,length,budget);
    if(pt_pp20_probe(data,length,&size)!=PT_PP20_OK)return PT_PROJECT_INVALID;
    if(size>budget)return PT_PROJECT_CAPACITY;
    s=document_stage_begin(d,&next,data,length);if(!s)return PT_PROJECT_INVALID;
    r=document_stage_take(s,7,size);if(r!=PT_PROJECT_OK)return r;plain=s->accepted[7];
    if(!document_stage_current(s)){document_stage_quarantine(s,NULL,0);return PT_PROJECT_INVALID;}
    if(pt_pp20_decode(data,length,plain,size,&written)!=PT_PP20_OK||written!=size){
        result=PT_PROJECT_INVALID;
    }else{
        /* Probe strict MOD before using the shared seven-slot staging record.
         * Nested compression/enhanced packed projects remain unsupported. */
        struct pt_project_requirements need;
        result=pt_mod_project_probe(plain,size,&need);
        if(result==PT_PROJECT_OK)return load_unpacked_record(d,plain,size,budget-size,&next,s);
    }
    if(!document_stage_discard(s))return PT_PROJECT_INVALID;
    return result;
}
enum pt_project_result pt_document_new(struct pt_document *d,unsigned channels,size_t budget)
{
    struct pt_document next;struct pt_project_requirements need;
    enum pt_project_result r;unsigned i;
    if(document_reentrant()||!document_entry_allowed(d,1))return PT_PROJECT_INVALID;
    if(!d->allocator.allocate||!d->allocator.release||!channels||channels>PT_CHANNEL_LIMIT)return PT_PROJECT_INVALID;
    memset(&need,0,sizeof need);need.orders=1;need.events=(size_t)64*channels;need.samples=31;
    r=allocate_staging(d,&need,budget,&next);if(r!=PT_PROJECT_OK)return r;
    if(!document_stage_current(document_stage_for_next(&next))){
        document_stage_quarantine(document_stage_for_next(&next),NULL,0);return PT_PROJECT_INVALID;
    }
    *next.storage.orders=0;memset(next.storage.events,0,need.events*sizeof(struct pt_event));
    memset(next.storage.samples,0,(size_t)31*sizeof(struct pt_sample));
    next.project.orders=next.storage.orders;next.project.events=next.storage.events;next.project.samples=next.storage.samples;
    next.project.order_count=1;next.project.pattern_count=1;next.project.sample_count=31;next.project.bpm=125;next.project.speed=6;
    pt_channels_init(&next.project.channels);pt_channels_resize(&next.project.channels,channels);
    for(i=0;i<31;++i){
        next.project.samples[i].pcm.bits=8;next.project.samples[i].pcm.channels=1;next.project.samples[i].pcm.rate=PT_CLASSIC_RATE;
    }
    next.loaded=1;return document_stage_publish(d,&next);
}
/* Probe, packed reads and finish reuse the same guarded stage before unpacking. */
enum pt_project_result pt_document_load_pp20_reader(struct pt_document *d,pt_pp20_read read,void *context,size_t length,size_t budget,int (*finish)(void *))
{
    struct pt_document next;struct document_stage *s;struct document_reader_guard guarded;
    uint8_t *plain;size_t size,written;enum pt_pp20_result packed;enum pt_project_result result,r;
    if(document_reentrant()||!document_entry_allowed(d,1))return PT_PROJECT_INVALID;
    if(!read||!finish||!d->allocator.allocate||!d->allocator.release)return PT_PROJECT_INVALID;
    s=document_stage_begin(d,&next,NULL,0);if(!s)return PT_PROJECT_INVALID;
    guarded=(struct document_reader_guard){s,read,finish,context};
    packed=pt_pp20_probe_reader(document_guarded_read,&guarded,length,&size);
    if(packed!=PT_PP20_OK)return document_reader_failure(s,packed==PT_PP20_UNSUPPORTED?PT_PROJECT_UNSUPPORTED:PT_PROJECT_INVALID);
    if(size>budget)return document_reader_failure(s,PT_PROJECT_CAPACITY);
    r=document_stage_take(s,7,size);if(r!=PT_PROJECT_OK)return r;plain=s->accepted[7];
    if(!document_stage_current(s)){document_stage_quarantine(s,NULL,0);return PT_PROJECT_INVALID;}
    packed=pt_pp20_decode_reader(document_guarded_read,&guarded,length,plain,size,&written);
    if(packed!=PT_PP20_OK||written!=size)return document_reader_failure(s,PT_PROJECT_INVALID);
    if(document_guarded_finish(&guarded)!=1)return document_reader_failure(s,PT_PROJECT_INVALID);
    {
        struct pt_project_requirements need;
        result=pt_mod_project_probe(plain,size,&need);
        if(result==PT_PROJECT_OK)return load_unpacked_record(d,plain,size,budget-size,&next,s);
    }
    return document_reader_failure(s,result);
}
