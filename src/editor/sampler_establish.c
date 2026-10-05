#include "sampler_establish.h"
#include "../core/render_storage_internal.h"
#include <stddef.h>
#include <string.h>
struct pt_establish_metadata {
    struct pt_project original;
    struct pt_sample *initial,*expected;
    struct pt_extension *extensions;
};
enum {COPY_METADATA=1,VALIDATE_PROJECT,ESTABLISH_MASTERS,ESTABLISH_DONE,ESTABLISH_FAILED};
#define ALIGN_OF(t) offsetof(struct {char prefix;t value;},value)
static int span(const void *p,size_t n)
{return !n||(p&&n<=UINTPTR_MAX-(uintptr_t)p);}
static int apart(const void *a,size_t an,const void *b,size_t bn)
{uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return span(a,an)&&span(b,bn)&&(!an||!bn||x>=y+bn||y>=x+an);}
static int append(size_t *bytes,size_t align,size_t n,size_t *offset)
{
    size_t extra=(*bytes%align)?align-(*bytes%align):0;
    if(extra>SIZE_MAX-*bytes)return 0;
    *bytes+=extra;*offset=*bytes;
    if(n>SIZE_MAX-*bytes)return 0;
    *bytes+=n;return 1;
}
static int current(struct pt_sampler_establish *c)
{
    struct pt_project p;struct pt_establish_metadata *m=c->metadata;
    if(c->project->channels.selected>=c->snapshot.channels.count)return 0;
    memcpy(&p,&c->snapshot,sizeof(p));p.channels.selected=c->project->channels.selected;
    return !memcmp(&p,c->project,sizeof(p))&&
        !memcmp(&c->expected_sampler,c->sampler,sizeof(*c->sampler))&&
        (!m||((!c->copied_samples||!memcmp(m->expected,c->project->samples,c->copied_samples*sizeof(*m->expected)))&&
              (!c->copied_extensions||!memcmp(m->extensions,c->project->extensions,c->copied_extensions*sizeof(*m->extensions)))));
}
static int fresh(struct pt_sampler_establish *c,const void *p,size_t n)
{
    unsigned i;
    if(!p||!n||!apart(p,n,c,sizeof(*c))||!apart(p,n,c->project,sizeof(*c->project))||
       !apart(p,n,c->metadata,c->metadata_bytes)||
       !pt_sampler_output_disjoint(&c->initial_sampler,p,n)||
       !pt_sampler_output_disjoint(c->sampler,p,n)||
       !pt_render_project_storage_output_disjoint(&c->snapshot,p,n)||
       !pt_render_project_storage_output_disjoint(c->project,p,n)||
       !pt_sampler_pin_job_output_disjoint(&c->job,p,n))return 0;
    if(c->metadata&&c->phase>=VALIDATE_PROJECT&&
       !pt_render_project_storage_output_disjoint(&c->metadata->original,p,n))return 0;
    for(i=0;i<c->parent_count;++i)if(!apart(p,n,c->parents[i].data,c->parents[i].bytes))return 0;
    return 1;
}
static enum pt_establish_result failure(struct pt_sampler_establish *c,enum pt_establish_result r)
{
    pt_project_validation_cancel(&c->validation);
    pt_sampler_pin_job_cancel(&c->job);
    c->phase=ESTABLISH_FAILED;c->result=r;return r;
}
static void *guard_allocate(void *context,const struct pt_allocator *a,size_t n)
{
    struct pt_sampler_establish *c=context;struct pt_allocator base=*a;void *p;
    if(!c->busy||c->phase!=ESTABLISH_MASTERS||c->faulted||!current(c))return NULL;
    p=base.allocate(base.context,n);
    /* Never initialize/release a known or ambiguous source/control alias. */
    if(p&&!fresh(c,p,n)){c->faulted=1;c->result=PT_ESTABLISH_ALIAS;return NULL;}
    if(!current(c)){c->faulted=1;c->result=PT_ESTABLISH_STALE;}
    if(c->faulted){if(p)base.release(base.context,p);return NULL;}
    return p;
}
enum pt_establish_result pt_sampler_establish_begin(struct pt_sampler_establish *c,struct pt_sampler *s,
    struct pt_project *p,const struct pt_allocator *a,const struct pt_sampler_storage_span *parents,
    unsigned count,uint32_t revision)
{
    struct pt_allocator base;struct pt_sampler_storage_span saved[PT_ESTABLISH_PARENTS];
    struct pt_establish_metadata *m;size_t bytes=sizeof(*m),first,second,extensions;unsigned i;
    if(c&&c->busy){c->faulted=1;c->result=PT_ESTABLISH_FAULT;return PT_ESTABLISH_FAULT;}
    if(!c||c->active||!s||!p||!a||!a->allocate||!a->release||!s->allocator.allocate||!s->allocator.release||
       count>PT_ESTABLISH_PARENTS-2||!span(parents,count*sizeof(*parents))||
       !apart(c,sizeof(*c),a,sizeof(*a))||!apart(c,sizeof(*c),parents,count*sizeof(*parents))||
       !pt_sampler_output_disjoint(s,c,sizeof(*c))||
       !pt_render_project_storage_output_disjoint(p,c,sizeof(*c)))return PT_ESTABLISH_INVALID;
    for(i=0;i<count;++i){if(!span(parents[i].data,parents[i].bytes))return PT_ESTABLISH_INVALID;saved[i]=parents[i];}
    saved[count]=(struct pt_sampler_storage_span){a,sizeof(*a)};
    saved[count+1]=(struct pt_sampler_storage_span){parents,count*sizeof(*parents)};
    if(!append(&bytes,ALIGN_OF(struct pt_sample),p->sample_count*sizeof(struct pt_sample),&first)||
       !append(&bytes,ALIGN_OF(struct pt_sample),p->sample_count*sizeof(struct pt_sample),&second)||
       !append(&bytes,ALIGN_OF(struct pt_extension),p->extension_count*sizeof(struct pt_extension),&extensions))return PT_ESTABLISH_INVALID;
    base=*a;memset(c,0,sizeof(*c));c->sampler=s;c->project=p;memcpy(&c->snapshot,p,sizeof(*p));
    memcpy(&c->initial_sampler,s,sizeof(*s));memcpy(&c->expected_sampler,s,sizeof(*s));
    c->control_allocator=base;c->parent_count=count+2;memcpy(c->parents,saved,c->parent_count*sizeof(*saved));
    c->revision=revision;c->generation=s->generation;c->active=c->busy=1;c->phase=COPY_METADATA;c->result=PT_ESTABLISH_PENDING;
    m=base.allocate(base.context,bytes);
    if(m&&!fresh(c,m,bytes)){c->faulted=1;c->phase=ESTABLISH_FAILED;c->result=PT_ESTABLISH_ALIAS;c->busy=0;return c->result;}
    if(!current(c)){c->faulted=1;c->result=PT_ESTABLISH_STALE;}
    if(c->faulted||!m){if(m)base.release(base.context,m);c->phase=ESTABLISH_FAILED;
        if(!c->faulted)c->result=PT_ESTABLISH_CAPACITY;
        c->busy=0;return c->result;}
    memset(m,0,sizeof(*m));memcpy(&m->original,&c->snapshot,sizeof(m->original));
    m->initial=(struct pt_sample *)((unsigned char *)m+first);
    m->expected=(struct pt_sample *)((unsigned char *)m+second);
    m->extensions=(struct pt_extension *)((unsigned char *)m+extensions);
    m->original.samples=m->initial;m->original.extensions=m->extensions;
    c->metadata=m;c->metadata_bytes=bytes;c->busy=0;return PT_ESTABLISH_PENDING;
}
enum pt_establish_result pt_sampler_establish_get(struct pt_sampler_establish *c,uint32_t revision,uint32_t generation)
{
    if(!c||!c->active)return PT_ESTABLISH_INVALID;
    if(c->busy){c->faulted=1;c->result=PT_ESTABLISH_FAULT;return c->result;}
    if(c->phase==ESTABLISH_FAILED)return c->result;
    if(c->faulted){c->busy=1;failure(c,(c->result==PT_ESTABLISH_PENDING||c->result==PT_ESTABLISH_READY)?PT_ESTABLISH_FAULT:c->result);c->busy=0;return c->result;}
    if(revision!=c->revision||generation!=c->generation||!current(c)){
        c->busy=1;failure(c,PT_ESTABLISH_STALE);c->busy=0;return c->result;}
    return c->phase==ESTABLISH_DONE?PT_ESTABLISH_READY:PT_ESTABLISH_PENDING;
}
enum pt_establish_result pt_sampler_establish_step(struct pt_sampler_establish *c,uint32_t revision,uint32_t generation,unsigned work)
{
    enum pt_establish_result r;enum pt_project_result valid;enum pt_edit_result edit;
    struct pt_pcm pcm;struct pt_sample_version *pin;unsigned ready=0,i;
    if(!work||work>4096)return PT_ESTABLISH_INVALID;
    r=pt_sampler_establish_get(c,revision,generation);if(r!=PT_ESTABLISH_PENDING)return r;
    c->busy=1;
    if(c->phase==COPY_METADATA){
        for(i=0;i<work;++i){
            if(c->copied_samples<c->snapshot.sample_count){unsigned k=c->copied_samples++;
                memcpy(c->metadata->initial+k,c->project->samples+k,sizeof(struct pt_sample));
                memcpy(c->metadata->expected+k,c->project->samples+k,sizeof(struct pt_sample));
            }else if(c->copied_extensions<c->snapshot.extension_count){unsigned k=c->copied_extensions++;
                memcpy(c->metadata->extensions+k,c->project->extensions+k,sizeof(struct pt_extension));
            }else break;
        }
        if(c->copied_samples==c->snapshot.sample_count&&c->copied_extensions==c->snapshot.extension_count){
            valid=pt_project_validation_begin(&c->validation,c->project,c->revision,c->generation);
            if(valid!=PT_PROJECT_OK)failure(c,PT_ESTABLISH_SEMANTIC);else c->phase=VALIDATE_PROJECT;
        }
    }else if(c->phase==VALIDATE_PROJECT){
        valid=pt_project_validation_step(&c->validation,c->revision,c->generation,work);
        if(valid==PT_PROJECT_OK){
            valid=pt_project_validation_get(&c->validation,c->revision,c->generation,NULL);
            if(valid!=PT_PROJECT_OK)failure(c,PT_ESTABLISH_STALE);
            else {pt_project_validation_cancel(&c->validation);c->phase=ESTABLISH_MASTERS;}
        }else if(valid!=PT_PROJECT_PENDING)failure(c,PT_ESTABLISH_SEMANTIC);
    }else if(c->slot==c->snapshot.sample_count){c->phase=ESTABLISH_DONE;c->result=PT_ESTABLISH_READY;}
    else if(!c->job.owner){
        edit=pt_sampler_pin_job_begin_guarded(&c->job,c->sampler,c->project,c->slot,c->generation,guard_allocate,c);
        if(edit!=PT_EDIT_OK)failure(c,c->faulted?c->result:edit==PT_EDIT_CAPACITY?PT_ESTABLISH_CAPACITY:PT_ESTABLISH_STALE);
        memcpy(&c->expected_sampler,c->sampler,sizeof(*c->sampler));
    }else {
        edit=pt_sampler_pin_job_step(&c->job,work,&pcm,&pin,&ready);
        if(edit!=PT_EDIT_OK)failure(c,PT_ESTABLISH_STALE);
        else if(ready){
            memcpy(c->metadata->expected+c->slot,c->project->samples+c->slot,sizeof(struct pt_sample));
            ++c->slot;pt_sampler_unpin(pin);
            memcpy(&c->expected_sampler,c->sampler,sizeof(*c->sampler));
        }
    }
    if(c->faulted&&c->phase!=ESTABLISH_FAILED)failure(c,(c->result==PT_ESTABLISH_PENDING||c->result==PT_ESTABLISH_READY)?PT_ESTABLISH_FAULT:c->result);
    c->busy=0;return c->result;
}
int pt_sampler_establish_cancel(struct pt_sampler_establish *c)
{
    struct pt_establish_metadata *m;struct pt_allocator a;
    if(!c)return 0;
    if(c->busy){c->faulted=1;c->result=PT_ESTABLISH_FAULT;return 0;}
    if(!c->active)return 1;
    c->busy=1;pt_project_validation_cancel(&c->validation);pt_sampler_pin_job_cancel(&c->job);
    m=c->metadata;a=c->control_allocator;c->metadata=NULL;c->metadata_bytes=0;
    if(m)a.release(a.context,m);
    c->active=0;c->busy=0;return 1;
}
