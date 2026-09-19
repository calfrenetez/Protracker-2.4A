#include <string.h>
#include "document.h"
#include "mod_project.h"
void pt_document_init(struct pt_document *d,const struct pt_allocator *a)
{ memset(d,0,sizeof(*d));if(a)d->allocator=*a; }
void pt_document_release(struct pt_document *d)
{
    struct pt_allocator a=d->allocator;
    if(a.release) {
        void *p[7];unsigned i;
        p[0]=d->storage.orders;p[1]=d->storage.events;p[2]=d->storage.samples;
        p[3]=d->storage.pcm;p[4]=d->storage.slices;p[5]=d->storage.extensions;p[6]=d->storage.extension_data;
        for(i=0;i<7;++i)if(p[i])a.release(a.context,p[i]);
    }
    pt_document_init(d,&a);
}
enum pt_project_result pt_document_load(struct pt_document *d,const uint8_t *data,size_t length,size_t budget)
{
    struct pt_project_requirements need;struct pt_document next;enum pt_project_result r;
    size_t count[7],width[7],bytes[7],total=0;void *p[7]={0};unsigned i;int enhanced;
    if(!d || !data || !d->allocator.allocate || !d->allocator.release)return PT_PROJECT_INVALID;
    enhanced=length>=5 && !memcmp(data,"PT24G",5);
    r=enhanced?pt_project_probe(data,length,&need):pt_mod_project_probe(data,length,&need);
    if(r!=PT_PROJECT_OK)return r;
    count[0]=need.orders;width[0]=sizeof(uint16_t);
    count[1]=need.events;width[1]=sizeof(struct pt_event);
    count[2]=need.samples;width[2]=sizeof(struct pt_sample);
    count[3]=need.pcm_values;width[3]=sizeof(int32_t);
    count[4]=need.slices;width[4]=sizeof(uint32_t);
    count[5]=need.extensions;width[5]=sizeof(struct pt_extension);
    count[6]=need.extension_bytes;width[6]=1;
    for(i=0;i<7;++i) {
        if(count[i]>SIZE_MAX/width[i])return PT_PROJECT_CAPACITY;
        bytes[i]=count[i]*width[i];
        if(bytes[i]>SIZE_MAX-total)return PT_PROJECT_CAPACITY;
        total+=bytes[i];
    }
    if(total>budget)return PT_PROJECT_CAPACITY;
    pt_document_init(&next,&d->allocator);
    for(i=0;i<7;++i)if(bytes[i]) {
        p[i]=d->allocator.allocate(d->allocator.context,bytes[i]);
        if(!p[i]) {
            unsigned j;for(j=0;j<i;++j)if(p[j])d->allocator.release(d->allocator.context,p[j]);
            return PT_PROJECT_CAPACITY;
        }
    }
    next.storage.orders=p[0];next.storage.order_capacity=count[0];
    next.storage.events=p[1];next.storage.event_capacity=count[1];
    next.storage.samples=p[2];next.storage.sample_capacity=count[2];
    next.storage.pcm=p[3];next.storage.pcm_capacity=count[3];
    next.storage.slices=p[4];next.storage.slice_capacity=count[4];
    next.storage.extensions=p[5];next.storage.extension_capacity=count[5];
    next.storage.extension_data=p[6];next.storage.extension_bytes=count[6];
    r=enhanced?pt_project_decode(data,length,&next.storage,&next.project):
               pt_mod_project_decode(data,length,&next.storage,&next.project);
    if(r!=PT_PROJECT_OK) {pt_document_release(&next);return r;}
    next.loaded=1;next.allocated_bytes=total;
    pt_document_release(d);*d=next;return PT_PROJECT_OK;
}
