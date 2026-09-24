#include "sample_import.h"
#include "raw_import.h"
#include "mod_import.h"
#include "pp20_import.h"
static enum pt_project_result load_mod_source(void *context,struct pt_document *d,size_t budget)
{return pt_pp20_file_candidate((const char *)context)?pt_pp20_file_load(d,(const char *)context,64UL*1024*1024,budget):pt_mod_file_load(d,(const char *)context,64UL*1024*1024,budget);}
enum pt_edit_result pt_editor_sample_file_import(struct pt_editor *e,const char *path,int raw,int source_only,int *preview)
{
    const char *name=path,*part;
    if(preview)*preview=0;
    if(!e || !path || !e->sample)return PT_EDIT_INVALID;
    if(raw) {
        for(part=path;*part;++part)if(*part=='/' || *part==':')name=part+1;
        pt_editor_prepare_change(e);
        return pt_raw_file_import(path,64UL*1024*1024,&e->sampler,e->project,&e->history,e->sample-1,name,&e->raw_format);
    }
    if(!source_only && pt_wav_file_candidate(path)) {
        for(part=path;*part;++part)if(*part=='/' || *part==':')name=part+1;
        pt_editor_prepare_change(e);
        return pt_wav_file_import(path,64UL*1024*1024,&e->sampler,e->project,&e->history,e->sample-1,name);
    }
    if(!source_only && pt_svx_file_candidate(path)) {
        for(part=path;*part;++part)if(*part=='/' || *part==':')name=part+1;
        pt_editor_prepare_change(e);
        return pt_svx_file_import(path,64UL*1024*1024,&e->sampler,e->project,&e->history,e->sample-1,name);
    }
    if(pt_mod_file_candidate(path) || pt_pp20_file_candidate(path)) {
        if(preview)*preview=1;
        return pt_editor_source_load_with(e,load_mod_source,(void *)path);
    }
    /* All supported formats above use bounded readers. Unknown data must not
     * allocate a whole-file buffer merely to discover that it is unsupported. */
    return PT_EDIT_UNSUPPORTED;
}
