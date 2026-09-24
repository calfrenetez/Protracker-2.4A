#include "project_file.h"
struct producer {const struct pt_project *project;pt_file_sink sink;void *context;size_t total;};
static int forward(void *context,const uint8_t *data,size_t n)
{struct producer *p=context;return p->sink(p->context,data,n);}
static int produce(void *context,pt_file_sink sink,void *sink_context)
{
    struct producer *p=context;size_t written=0;p->sink=sink;p->context=sink_context;
    return pt_project_stream(p->project,forward,p,&written)==PT_PROJECT_OK && written==p->total;
}
enum pt_save_result pt_project_file_save(const char *path,const struct pt_project *project,const struct pt_allocator *a)
{
    struct producer p;
    if(pt_project_size(project,&p.total)!=PT_PROJECT_OK)return PT_SAVE_INVALID;
    p.project=project;p.sink=0;p.context=0;
    return pt_file_save_streamed(path,p.total,produce,&p,a);
}
