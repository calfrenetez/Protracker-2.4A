#include "mod_file.h"
struct producer {const struct pt_project *project;unsigned policy;pt_file_sink sink;void *context;};
static int forward(void *context,const uint8_t *data,size_t n)
{struct producer *p=context;return p->sink(p->context,data,n);}
static int produce(void *context,pt_file_sink sink,void *sink_context)
{
    struct producer *p=context;p->sink=sink;p->context=sink_context;
    return pt_mod_export_stream(p->project,p->policy,forward,p)==PT_PROJECT_OK;
}
enum pt_save_result pt_mod_file_save(const char *path,const struct pt_project *project,unsigned policy,const struct pt_allocator *a)
{
    struct pt_mod_export_report report;struct producer p;
    if(policy>2 || (policy?pt_mod_export_analyse_round8(project,&report):pt_mod_export_analyse(project,&report))!=PT_PROJECT_OK ||
       (report.issues & ~(policy?PT_EXPORT_PRECISION:0U)))return PT_SAVE_INVALID;
    p.project=project;p.policy=policy;p.sink=0;p.context=0;
    return pt_file_save_streamed(path,report.bytes,produce,&p,a);
}
