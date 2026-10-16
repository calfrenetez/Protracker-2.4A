
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
static unsigned long pt_desc_counts[3][2];
static unsigned pt_desc_adverse_done[2];
static int pt_desc_compare(struct pt_editor_mixed_causal_prepare *s)
{
 int a=pt_desc_ref_source_descriptor(s),b=pt_desc_proposed(s);
 unsigned m=s->source_mode==2?0:s->source_mode==3?1:2;
 if(a!=b)abort();
 ++pt_desc_counts[m][!!a];return b;
}
struct pt_desc_before {
 void *wrapper,*input,*descriptor,*publisher;
 void *input_at,*descriptor_at,*publisher_at;
 size_t wrapper_bytes,input_bytes,descriptor_bytes;
};
static void pt_desc_restore(struct pt_editor_mixed_causal_prepare *s,const struct pt_desc_before *b)
{
 memcpy(s,b->wrapper,b->wrapper_bytes);
 memcpy(b->input_at,b->input,b->input_bytes);
 memcpy(b->descriptor_at,b->descriptor,b->descriptor_bytes);
 memcpy(b->publisher_at,b->publisher,sizeof(struct pt_editor_mixed_causal_source_borrow));
 if(memcmp(s,b->wrapper,b->wrapper_bytes)||memcmp(b->input_at,b->input,b->input_bytes)||
    memcmp(b->descriptor_at,b->descriptor,b->descriptor_bytes)||
    memcmp(b->publisher_at,b->publisher,sizeof(struct pt_editor_mixed_causal_source_borrow)))abort();
}
static void pt_desc_adverse(struct pt_editor_mixed_causal_prepare *s,unsigned mode)
{
 struct pt_desc_before b;unsigned i,k,n=s->guard_count;
 struct pt_editor_mixed_causal_lineage_source_prepare *w=(void *)s;
 struct pt_editor_mixed_causal_source_borrow *publisher=s->source_publisher;
 b.wrapper_bytes=mode?sizeof(struct pt_editor_mixed_causal_rolling_source_prepare):sizeof(*w);
 b.input_bytes=mode?sizeof(struct pt_editor_mixed_causal_rolling_inputs):sizeof(struct pt_editor_mixed_causal_lineage_inputs);
 b.descriptor_bytes=mode?sizeof(struct pt_editor_mixed_causal_rolling_source_inputs):sizeof(struct pt_editor_mixed_causal_lineage_source_inputs);
 b.input_at=(void *)s->source_preparation;b.descriptor_at=(void *)s->source_inputs;b.publisher_at=publisher;
 b.wrapper=malloc(b.wrapper_bytes);b.input=malloc(b.input_bytes);b.descriptor=malloc(b.descriptor_bytes);
 b.publisher=malloc(sizeof(*publisher));if(!b.wrapper||!b.input||!b.descriptor||!b.publisher)abort();
 memcpy(b.wrapper,s,b.wrapper_bytes);memcpy(b.input,b.input_at,b.input_bytes);
 memcpy(b.descriptor,b.descriptor_at,b.descriptor_bytes);memcpy(b.publisher,publisher,sizeof(*publisher));
#define PT_DESC_CONTROL(statement) do{statement;(void)pt_desc_compare(s);pt_desc_restore(s,&b);}while(0)
 PT_DESC_CONTROL(s->guard_count=0);PT_DESC_CONTROL(s->guard_count=PT_EDITOR_MIXED_READERS_GUARDS+1);
 PT_DESC_CONTROL(s->source_serial=0);PT_DESC_CONTROL(s->source_publisher=NULL);
 PT_DESC_CONTROL(s->source_binding=NULL);PT_DESC_CONTROL(s->saved.binding=NULL);
 PT_DESC_CONTROL(s->source_inputs=NULL);PT_DESC_CONTROL(s->source_preparation=NULL);
 PT_DESC_CONTROL(s->lineage_owner=NULL);PT_DESC_CONTROL(s->stop_owner=(void *)s);
 PT_DESC_CONTROL(w->self=NULL);PT_DESC_CONTROL(w->inputs=NULL);PT_DESC_CONTROL(w->saved.preparation=NULL);
 PT_DESC_CONTROL(w->original.self=NULL);PT_DESC_CONTROL(w->original.inputs=NULL);
 PT_DESC_CONTROL(s->inputs=NULL);PT_DESC_CONTROL(s->source_activated=!s->source_activated);
 for(i=0;i<5;++i){PT_DESC_CONTROL(s->source_mode=i);}
 for(i=0;i<3;++i){PT_DESC_CONTROL(s->source_held=i);PT_DESC_CONTROL(s->source_released=i);}
 PT_DESC_CONTROL(s->source_held=0;s->source_released=1;publisher->address=NULL;publisher->serial=0);
 PT_DESC_CONTROL(publisher->address=NULL);PT_DESC_CONTROL(publisher->serial=0);
 PT_DESC_CONTROL(((unsigned char *)b.descriptor_at)[b.descriptor_bytes-1]^=1);
 PT_DESC_CONTROL(((unsigned char *)b.input_at)[b.input_bytes-1]^=1);
 for(i=0;i<n;++i){size_t bytes=s->guards[i].bytes,values[4]={0,1,bytes?bytes-1:0,SIZE_MAX};
    for(k=0;k<4;++k){PT_DESC_CONTROL(s->guards[i].bytes=values[k]);}}
 if(mode){
    struct pt_editor_mixed_causal_rolling_state *r=s->rolling_owner;
    struct pt_editor_mixed_causal_rolling_source_inputs *v=b.descriptor_at;
    long page=sysconf(_SC_PAGESIZE);size_t length;void *unreadable;
    if(page<=0)abort();length=((sizeof(*r)+(size_t)page-1)/(size_t)page)*(size_t)page;
    unreadable=mmap(NULL,length,PROT_NONE,MAP_PRIVATE|MAP_ANON,-1,0);if(unreadable==MAP_FAILED)abort();
    PT_DESC_CONTROL(s->rolling_owner=NULL);PT_DESC_CONTROL(r->self=NULL);PT_DESC_CONTROL(r->inputs=NULL);
    PT_DESC_CONTROL(r->joint=2);PT_DESC_CONTROL(r->wrapper.bytes=0);PT_DESC_CONTROL(r->wrapper.data=NULL);
    PT_DESC_CONTROL(v->preparation=NULL);PT_DESC_CONTROL(v->original.preparation=NULL);
    PT_DESC_CONTROL(s->rolling_owner=unreadable);
    PT_DESC_CONTROL(s->lineage_owner=unreadable);
    PT_DESC_CONTROL(s->source_inputs=unreadable);
    PT_DESC_CONTROL(s->source_preparation=unreadable);
    /* Full parent absence must refuse before any isolated rolling header. */
    PT_DESC_CONTROL(s->rolling_owner=unreadable;s->guard_count=1;s->guards[0]=((struct pt_sampler_storage_span){unreadable,sizeof(*r)}));
    /* Serial refusal precedes publisher pointee reads in both paths. */
    PT_DESC_CONTROL(s->source_serial=0;s->source_publisher=unreadable);
    if(munmap(unreadable,length))abort();
 }
#undef PT_DESC_CONTROL
 pt_desc_restore(s,&b);free(b.wrapper);free(b.input);free(b.descriptor);free(b.publisher);
}
static int source_descriptor(struct pt_editor_mixed_causal_prepare *s)
{
 int value=pt_desc_compare(s);unsigned mode=s->source_mode==2?0:s->source_mode==3?1:2;
 if(value&&mode<2&&!pt_desc_adverse_done[mode]){pt_desc_adverse_done[mode]=1;pt_desc_adverse(s,mode);}
 return value;
}
static void __attribute__((destructor)) pt_desc_report(void)
{
 const char *path=getenv("PT_DESC_REPORT");FILE *f;unsigned i;
 if(!path||(f=fopen(path,"wb"))==NULL)abort();
 for(i=0;i<3;++i)if(fprintf(f,"%u %lu %lu\n",i,pt_desc_counts[i][0],pt_desc_counts[i][1])<0)abort();
 if(fclose(f))abort();
}
