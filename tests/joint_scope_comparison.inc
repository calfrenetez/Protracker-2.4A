/* HOST-only dual predicate and temporary shape controls; exact restoration. */

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
static unsigned long pt_scope_comparisons[3][2];
static unsigned pt_scope_adverse_done[2];
static struct pt_editor_mixed_causal_lineage_source_prepare *pt_scope_compare(struct pt_editor_mixed_causal_prepare *s)
{
 struct pt_editor_mixed_causal_lineage_source_prepare *old=pt_scope_reference(s),*new=pt_scope_proposed(s);
 unsigned mode=s->source_mode==2?0:s->source_mode==3?1:2;
 if(old!=new)abort();
 ++pt_scope_comparisons[mode][!!old];
 return new;
}
static void pt_scope_adverse(struct pt_editor_mixed_causal_prepare *s,unsigned mode)
{
 unsigned n=s->guard_count,m=s->source_mode,i;
 const struct pt_editor_mixed_causal_source_inputs *input=s->source_inputs;
 const struct pt_editor_mixed_causal_prepare_inputs *preparation=s->source_preparation;
 struct pt_editor_mixed_causal_lineage_prepare *owner=s->lineage_owner;
 unsigned char *before=malloc(sizeof(*s));
 if(!before)abort();
 memcpy(before,s,sizeof(*s));
 s->guard_count=0;(void)pt_scope_compare(s);s->guard_count=n;
 s->guard_count=PT_EDITOR_MIXED_READERS_GUARDS+1;(void)pt_scope_compare(s);s->guard_count=n;
 s->source_inputs=NULL;(void)pt_scope_compare(s);s->source_inputs=input;
 s->source_preparation=NULL;(void)pt_scope_compare(s);s->source_preparation=preparation;
 s->lineage_owner=NULL;(void)pt_scope_compare(s);s->lineage_owner=owner;
 for(i=0;i<5;++i){s->source_mode=i;(void)pt_scope_compare(s);}
 s->source_mode=m;
 for(i=0;i<n;++i){
    size_t bytes=s->guards[i].bytes,values[4]={0,1,bytes?bytes-1:0,SIZE_MAX};unsigned k;
    for(k=0;k<4;++k){s->guards[i].bytes=values[k];(void)pt_scope_compare(s);}
    s->guards[i].bytes=bytes;
 }
 if(mode==1){
    struct pt_editor_mixed_causal_rolling_state *r=s->rolling_owner;
    size_t bytes=r->wrapper.bytes;unsigned joint=r->joint;
    s->rolling_owner=NULL;(void)pt_scope_compare(s);s->rolling_owner=r;
    r->wrapper.bytes=0;(void)pt_scope_compare(s);r->wrapper.bytes=bytes;
    r->joint=2;(void)pt_scope_compare(s);r->joint=joint;
    /* The original outer parent refusal must precede an inaccessible rolling
     * header even when an isolated numeric guard names that header. */
    {
       long page=sysconf(_SC_PAGESIZE);size_t length;void *unreadable;
       struct pt_sampler_storage_span first;
       if(page<=0)abort();
       length=((sizeof(*r)+(size_t)page-1)/(size_t)page)*(size_t)page;
       unreadable=mmap(NULL,length,PROT_NONE,MAP_PRIVATE|MAP_ANON,-1,0);
       if(unreadable==MAP_FAILED)abort();
       memcpy(&first,s->guards,sizeof(first));
       s->guard_count=1;s->rolling_owner=unreadable;
       s->guards[0]=(struct pt_sampler_storage_span){unreadable,sizeof(*r)};
       if(pt_scope_compare(s))abort();
       memcpy(s->guards,&first,sizeof(first));s->rolling_owner=r;s->guard_count=n;
       if(munmap(unreadable,length))abort();
    }
 }
 if(memcmp(before,s,sizeof(*s)))abort();
 free(before);
}
static struct pt_editor_mixed_causal_lineage_source_prepare *lineage_source_scope(struct pt_editor_mixed_causal_prepare *s)
{
 struct pt_editor_mixed_causal_lineage_source_prepare *result=pt_scope_compare(s);
 unsigned mode=s->source_mode==2?0:s->source_mode==3?1:2;
 if(result&&mode<2&&!pt_scope_adverse_done[mode]){
    pt_scope_adverse_done[mode]=1;pt_scope_adverse(s,mode);
 }
 return result;
}
static void __attribute__((destructor)) pt_scope_report(void)
{
 const char *path=getenv("PT_SCOPE_REPORT");FILE *f;unsigned i;
 if(!path||(f=fopen(path,"wb"))==NULL)abort();
 for(i=0;i<3;++i)if(fprintf(f,"%u %lu %lu\n",i,pt_scope_comparisons[i][0],pt_scope_comparisons[i][1])<0)abort();
 if(fclose(f))abort();
}
