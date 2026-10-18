#include "editor_document_switch.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

struct switch_alignment {char byte;struct pt_native_document_switch value;};
struct document_alignment {char byte;struct pt_document value;};
struct editor_alignment {char byte;struct pt_editor value;};
struct span {uintptr_t first,last;};

static int region(const void *pointer,size_t bytes,struct span *out)
{
    uintptr_t first=(uintptr_t)pointer;
    if(!pointer || !bytes || bytes>UINTPTR_MAX-first)return 0;
    out->first=first;out->last=first+bytes;return 1;
}
static int apart(struct span a,struct span b)
{return a.last<=b.first || b.last<=a.first;}
static int zero(const void *pointer,size_t bytes)
{
    const unsigned char *p=pointer;size_t i;
    for(i=0;i<bytes;++i)if(p[i])return 0;
    return 1;
}
static int old_storage_apart(const struct pt_document *d,struct span known)
{
    const void *p[7];size_t count[7],width[7],bytes;struct span s;unsigned i;
    p[0]=d->storage.orders;count[0]=d->storage.order_capacity;width[0]=sizeof(uint16_t);
    p[1]=d->storage.events;count[1]=d->storage.event_capacity;width[1]=sizeof(struct pt_event);
    p[2]=d->storage.samples;count[2]=d->storage.sample_capacity;width[2]=sizeof(struct pt_sample);
    p[3]=d->storage.pcm;count[3]=d->storage.pcm_capacity;width[3]=sizeof(int32_t);
    p[4]=d->storage.slices;count[4]=d->storage.slice_capacity;width[4]=sizeof(uint32_t);
    p[5]=d->storage.extensions;count[5]=d->storage.extension_capacity;width[5]=sizeof(struct pt_extension);
    p[6]=d->storage.extension_data;count[6]=d->storage.extension_bytes;width[6]=1;
    for(i=0;i<7;++i) {
        if(count[i]>SIZE_MAX/width[i])return 0;
        bytes=count[i]*width[i];
        if(!bytes) {if(p[i])return 0;continue;}
        if(!region(p[i],bytes,&s) || !apart(s,known))return 0;
    }
    return 1;
}
static int geometry(struct pt_native_document_switch *c,size_t capacity,
    const struct pt_native_document_switch_inputs *in,const uint8_t *source,size_t bytes)
{
    struct span control,document,editor,context,input;unsigned have_context;
    if(!c || !in || capacity<sizeof(*c) ||
       (uintptr_t)c%offsetof(struct switch_alignment,value) ||
       !region(c,capacity,&control) || !in->document || !in->editor ||
       in->document_capacity<sizeof(*in->document) || in->editor_capacity<sizeof(*in->editor) ||
       (uintptr_t)in->document%offsetof(struct document_alignment,value) ||
       (uintptr_t)in->editor%offsetof(struct editor_alignment,value) ||
       !region(in->document,in->document_capacity,&document) ||
       !region(in->editor,in->editor_capacity,&editor) ||
       !apart(control,document) || !apart(control,editor) || !apart(document,editor))return 0;
    if(in->editor->project!=&in->document->project || !in->document->loaded ||
       !pt_document_replacement_allowed(in->document) ||
       in->allocator_context!=in->document->allocator.context ||
       !in->document->allocator.allocate || !in->document->allocator.release)return 0;
    have_context=in->allocator_context!=NULL;
    if(have_context) {
        if(!region(in->allocator_context,in->allocator_context_capacity,&context) ||
           !apart(context,control) || !apart(context,document) || !apart(context,editor))return 0;
    } else if(in->allocator_context_capacity)return 0;
    if(!old_storage_apart(in->document,control) ||
       !old_storage_apart(in->document,document) || !old_storage_apart(in->document,editor) ||
       (have_context && !old_storage_apart(in->document,context)))return 0;
    if(source || bytes) {
        if(!region(source,bytes,&input) || !apart(input,control) || !apart(input,document) ||
           !apart(input,editor) || (have_context && !apart(input,context)) ||
           !old_storage_apart(in->document,input))return 0;
    }
    return zero(c,sizeof(*c));
}
static int original(const struct pt_native_document_switch *c)
{
    return !memcmp(c->inputs.document,&c->original_document,sizeof(c->original_document)) &&
           !memcmp(c->inputs.editor,&c->original_editor,sizeof(c->original_editor));
}
static enum pt_native_document_switch_result hold(struct pt_native_document_switch *c)
{c->busy=0;c->result=PT_NATIVE_DOCUMENT_SWITCH_HELD;return c->result;}
static int empty_candidate(const struct pt_document *d)
{
    struct pt_document empty;
    memset(&empty,0,sizeof(empty));empty.allocator=d->allocator;
    return !memcmp(d,&empty,sizeof(empty));
}
static enum pt_native_document_switch_result prepare(struct pt_native_document_switch *c,
    size_t capacity,const struct pt_native_document_switch_inputs *in,
    const uint8_t *source,size_t bytes,unsigned channels,size_t budget,unsigned loading)
{
    enum pt_project_result r;enum pt_document_release_result released;
    struct pt_native_document_switch_inputs accepted;
    if(!c || capacity<sizeof(*c) || (uintptr_t)c%offsetof(struct switch_alignment,value))
        return PT_NATIVE_DOCUMENT_SWITCH_INVALID;
    if(c->self==c && c->busy)return PT_NATIVE_DOCUMENT_SWITCH_BUSY;
    if(!geometry(c,capacity,in,source,bytes))return PT_NATIVE_DOCUMENT_SWITCH_INVALID;
    accepted=*in;
    memset(c,0,sizeof(*c));c->self=c;c->capacity=capacity;c->inputs=accepted;
    memcpy(&c->original_document,accepted.document,sizeof(c->original_document));
    memcpy(&c->original_editor,accepted.editor,sizeof(c->original_editor));
    pt_document_init(&c->candidate,&accepted.document->allocator);c->busy=1;
    r=loading?pt_document_load(&c->candidate,source,bytes,budget):
              pt_document_new(&c->candidate,channels,budget);
    c->preparation_result=r;
    if(!original(c))return hold(c);
    if(r==PT_PROJECT_OK) {
        if(!c->candidate.loaded || !pt_document_replacement_allowed(&c->candidate))return hold(c);
        c->candidate_snapshot=c->candidate;c->busy=0;
        c->result=PT_NATIVE_DOCUMENT_SWITCH_PREPARED;return c->result;
    }
    if(!pt_document_replacement_allowed(&c->candidate))return hold(c);
    released=pt_document_release_checked(&c->candidate);
    if(released!=PT_DOCUMENT_RELEASED || !original(c) || !empty_candidate(&c->candidate))return hold(c);
    c->busy=0;c->result=PT_NATIVE_DOCUMENT_SWITCH_FAILED;return c->result;
}
enum pt_native_document_switch_result pt_native_document_switch_new(
    struct pt_native_document_switch *c,size_t capacity,
    const struct pt_native_document_switch_inputs *in,unsigned channels,size_t budget)
{
    if(!channels || channels>PT_CHANNEL_LIMIT)return PT_NATIVE_DOCUMENT_SWITCH_INVALID;
    return prepare(c,capacity,in,NULL,0,channels,budget,0);
}
enum pt_native_document_switch_result pt_native_document_switch_load(
    struct pt_native_document_switch *c,size_t capacity,
    const struct pt_native_document_switch_inputs *in,const uint8_t *source,size_t bytes,size_t budget)
{
    if(!source || !bytes)return PT_NATIVE_DOCUMENT_SWITCH_INVALID;
    return prepare(c,capacity,in,source,bytes,0,budget,1);
}
enum pt_native_document_switch_result pt_native_document_switch_state(struct pt_native_document_switch *c)
{
    if(!c)return PT_NATIVE_DOCUMENT_SWITCH_INVALID;
    if(c->self!=c)return zero(c,sizeof(*c))?PT_NATIVE_DOCUMENT_SWITCH_EMPTY:PT_NATIVE_DOCUMENT_SWITCH_INVALID;
    if(c->busy)return PT_NATIVE_DOCUMENT_SWITCH_BUSY;
    if(c->result==PT_NATIVE_DOCUMENT_SWITCH_PREPARED &&
       (!original(c) || memcmp(&c->candidate,&c->candidate_snapshot,sizeof(c->candidate))))return hold(c);
    return c->result;
}
enum pt_native_document_switch_result pt_native_document_switch_admit_publication(struct pt_native_document_switch *c)
{
    enum pt_native_document_switch_result r=pt_native_document_switch_state(c);
    if(r!=PT_NATIVE_DOCUMENT_SWITCH_PREPARED)return r;
    /* No native stop/adoption authority exists in this increment. Never call
     * stop/dispose/release the original domain or accept a caller bool. */
    return PT_NATIVE_DOCUMENT_SWITCH_STOP_UNQUALIFIED;
}
enum pt_native_document_switch_result pt_native_document_switch_cancel(struct pt_native_document_switch *c)
{
    enum pt_native_document_switch_result r=pt_native_document_switch_state(c);
    enum pt_document_release_result released;
    if(r!=PT_NATIVE_DOCUMENT_SWITCH_PREPARED)return r;
    c->busy=1;released=pt_document_release_checked(&c->candidate);
    if(released!=PT_DOCUMENT_RELEASED || !original(c) || !empty_candidate(&c->candidate))return hold(c);
    c->busy=0;c->result=PT_NATIVE_DOCUMENT_SWITCH_CANCELED;return c->result;
}
int pt_native_document_switch_reset(struct pt_native_document_switch *c)
{
    if(!c || c->self!=c || c->busy ||
       (c->result!=PT_NATIVE_DOCUMENT_SWITCH_FAILED && c->result!=PT_NATIVE_DOCUMENT_SWITCH_CANCELED) ||
       !empty_candidate(&c->candidate))return 0;
    memset(c,0,sizeof(*c));return 1;
}
