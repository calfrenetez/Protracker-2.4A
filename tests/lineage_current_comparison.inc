/* HOST-only dual predicate, restored adverse controls and early-read guards. */
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
static unsigned long pt_lc_counts[3][2];
static unsigned pt_lc_adverse_done[2],pt_lc_protected_stage,pt_lc_protected_activation;
static int pt_lc_compare(struct pt_editor_mixed_causal_prepare *s)
{
 int a=pt_lc_ref_lineage_current(s),b=pt_lc_proposed(s);
 unsigned mode=s->source_mode==2?0:s->source_mode==3?1:2;
 if(a!=b)abort();
 ++pt_lc_counts[mode][!!a];return b;
}
struct pt_lc_before {
 void *wrapper,*input,*descriptor,*publisher;
 void *input_at,*descriptor_at,*publisher_at;
 size_t wrapper_bytes,input_bytes,descriptor_bytes;
};
static void pt_lc_restore(struct pt_editor_mixed_causal_prepare *s,const struct pt_lc_before *b)
{
 memcpy(s,b->wrapper,b->wrapper_bytes);
 memcpy(b->input_at,b->input,b->input_bytes);
 memcpy(b->descriptor_at,b->descriptor,b->descriptor_bytes);
 memcpy(b->publisher_at,b->publisher,sizeof(struct pt_editor_mixed_causal_source_borrow));
 if(memcmp(s,b->wrapper,b->wrapper_bytes)||memcmp(b->input_at,b->input,b->input_bytes)||
    memcmp(b->descriptor_at,b->descriptor,b->descriptor_bytes)||
    memcmp(b->publisher_at,b->publisher,sizeof(struct pt_editor_mixed_causal_source_borrow)))abort();
}
static void pt_lc_adverse(struct pt_editor_mixed_causal_prepare *s,unsigned mode)
{
 struct pt_lc_before b;unsigned i,k,n=s->guard_count,values[5]={0,3,4,5,UINT_MAX};
 struct pt_editor_mixed_causal_lineage_source_prepare *source=(void *)s;
 struct pt_editor_mixed_causal_lineage_prepare *w=&source->original;
 b.wrapper_bytes=mode?sizeof(struct pt_editor_mixed_causal_rolling_source_prepare):sizeof(*source);
 b.input_bytes=mode?sizeof(struct pt_editor_mixed_causal_rolling_inputs):sizeof(struct pt_editor_mixed_causal_lineage_inputs);
 b.descriptor_bytes=mode?sizeof(struct pt_editor_mixed_causal_rolling_source_inputs):sizeof(struct pt_editor_mixed_causal_lineage_source_inputs);
 b.input_at=(void *)s->source_preparation;b.descriptor_at=(void *)s->source_inputs;b.publisher_at=s->source_publisher;
 b.wrapper=malloc(b.wrapper_bytes);b.input=malloc(b.input_bytes);b.descriptor=malloc(b.descriptor_bytes);
 b.publisher=malloc(sizeof(struct pt_editor_mixed_causal_source_borrow));
 if(!b.wrapper||!b.input||!b.descriptor||!b.publisher||!b.publisher_at)abort();
 memcpy(b.wrapper,s,b.wrapper_bytes);memcpy(b.input,b.input_at,b.input_bytes);
 memcpy(b.descriptor,b.descriptor_at,b.descriptor_bytes);
 memcpy(b.publisher,b.publisher_at,sizeof(struct pt_editor_mixed_causal_source_borrow));
#define PT_LC_CONTROL(...) do{__VA_ARGS__; (void)pt_lc_compare(s);pt_lc_restore(s,&b);}while(0)
 for(i=0;i<5;++i){PT_LC_CONTROL(w->stage=values[i]);PT_LC_CONTROL(w->constructing=values[i]);}
 for(i=0;i<3;++i){PT_LC_CONTROL(w->root_detached=i);PT_LC_CONTROL(w->root_closed=i);}
 PT_LC_CONTROL(s->source_activated=0);PT_LC_CONTROL(s->source_activated=1);
 PT_LC_CONTROL(s->inputs=NULL);PT_LC_CONTROL(w->self=NULL);PT_LC_CONTROL(w->inputs=NULL);
 PT_LC_CONTROL(source->self=NULL);PT_LC_CONTROL(source->inputs=NULL);PT_LC_CONTROL(source->saved.preparation=NULL);
 PT_LC_CONTROL(s->source_inputs=NULL);PT_LC_CONTROL(s->source_preparation=NULL);
 PT_LC_CONTROL(s->lineage_owner=NULL);PT_LC_CONTROL(s->stop_owner=(void *)s);
 PT_LC_CONTROL(s->guard_count=0);PT_LC_CONTROL(s->guard_count=PT_EDITOR_MIXED_READERS_GUARDS+1);
 PT_LC_CONTROL(((unsigned char *)&w->saved.original)[0]^=1);
 for(i=0;i<5;++i){PT_LC_CONTROL(s->source_mode=i);}
 for(i=0;i<n;++i){size_t bytes=s->guards[i].bytes,lengths[4]={0,1,bytes?bytes-1:0,SIZE_MAX};
    for(k=0;k<4;++k){PT_LC_CONTROL(s->guards[i].bytes=lengths[k]);}}
 if(mode){
    struct pt_editor_mixed_causal_rolling_state *r=s->rolling_owner;
    struct pt_editor_mixed_causal_rolling_source_inputs *v=b.descriptor_at;
    struct pt_editor_mixed_causal_rolling_inputs *in=b.input_at;
    long page=sysconf(_SC_PAGESIZE);size_t length;void *unreadable;unsigned input_guard;
    if(page<=0)abort();length=((sizeof(*in)+(size_t)page-1)/(size_t)page)*(size_t)page;
    unreadable=mmap(NULL,length,PROT_NONE,MAP_PRIVATE|MAP_ANON,-1,0);if(unreadable==MAP_FAILED)abort();
    PT_LC_CONTROL(s->rolling_owner=NULL);PT_LC_CONTROL(r->self=NULL);PT_LC_CONTROL(r->inputs=NULL);
    PT_LC_CONTROL(r->preparation=NULL);PT_LC_CONTROL(r->joint=0);PT_LC_CONTROL(r->joint=2);
    PT_LC_CONTROL(r->wrapper.data=NULL);PT_LC_CONTROL(r->wrapper.bytes=0);
    PT_LC_CONTROL(v->preparation=NULL);PT_LC_CONTROL(v->original.preparation=NULL);
    PT_LC_CONTROL(((unsigned char *)&in->original.lineage)[0]^=1);
    PT_LC_CONTROL(((unsigned char *)&in->rolling)[sizeof(in->rolling)-1]^=1);
    PT_LC_CONTROL(s->rolling_owner=unreadable);PT_LC_CONTROL(s->lineage_owner=unreadable);
    PT_LC_CONTROL(s->source_inputs=unreadable);PT_LC_CONTROL(s->source_preparation=unreadable);
    PT_LC_CONTROL(s->rolling_owner=unreadable;s->guard_count=1;
        s->guards[0].data=unreadable;s->guards[0].bytes=sizeof(*r));
    /* A complete numeric preparation parent and consistent captured backlinks
     * let the original scope succeed without reading this protected body.
     * Invalid stage/constructing must therefore refuse before rolling payload.
     * Do not run this forged body with otherwise passing stage checks. */
    for(input_guard=0;input_guard<n;++input_guard)
       if(s->guards[input_guard].data==b.input_at&&s->guards[input_guard].bytes==b.input_bytes)break;
    if(input_guard==n)abort();
    s->source_preparation=unreadable;s->inputs=unreadable;r->inputs=unreadable;
    v->preparation=unreadable;v->original.preparation=unreadable;v->original.original.preparation=unreadable;
    source->saved.preparation=unreadable;source->saved.original.preparation=unreadable;w->inputs=unreadable;
    s->guards[input_guard].data=unreadable;s->guards[input_guard].bytes=sizeof(*in);
    w->stage=5;w->constructing=4;
    if(pt_lc_ref_lineage_scope(s)!=w||lineage_scope(s)!=w||pt_lc_compare(s))abort();
    ++pt_lc_protected_stage;
    w->stage=4;w->constructing=5;
    if(pt_lc_ref_lineage_scope(s)!=w||lineage_scope(s)!=w||pt_lc_compare(s))abort();
    ++pt_lc_protected_stage;
    /* With the original inactive-source inputs condition satisfied, activation
     * refusal also precedes every stage or rolling input payload read. */
    w->stage=4;w->constructing=4;s->source_activated=0;s->inputs=NULL;
    if(pt_lc_ref_lineage_scope(s)||lineage_scope(s)||pt_lc_compare(s))abort();
    ++pt_lc_protected_activation;
    pt_lc_restore(s,&b);
    if(munmap(unreadable,length))abort();
 }
#undef PT_LC_CONTROL
 pt_lc_restore(s,&b);free(b.wrapper);free(b.input);free(b.descriptor);free(b.publisher);
}
static int lineage_current(struct pt_editor_mixed_causal_prepare *s)
{
 int value=pt_lc_compare(s);unsigned mode=s->source_mode==2?0:s->source_mode==3?1:2;
 if(value&&mode<2&&!pt_lc_adverse_done[mode]){pt_lc_adverse_done[mode]=1;pt_lc_adverse(s,mode);}
 return value;
}
static void __attribute__((destructor)) pt_lc_report(void)
{
 const char *path=getenv("PT_LC_REPORT");FILE *f;unsigned i;
 if(!path||(f=fopen(path,"wb"))==NULL)abort();
 for(i=0;i<3;++i)if(fprintf(f,"%u %lu %lu\n",i,pt_lc_counts[i][0],pt_lc_counts[i][1])<0)abort();
 if(fprintf(f,"protected %u %u\n",pt_lc_protected_stage,pt_lc_protected_activation)<0||fclose(f))abort();
}
