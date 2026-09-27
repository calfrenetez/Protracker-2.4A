#include "recovery_file.h"
#include <string.h>
#define HEADER 44U
static void put64(uint8_t *p,uint64_t v)
{unsigned i;for(i=0;i<8;++i)p[i]=(uint8_t)(v>>(56-8*i));}
static uint64_t get64(const uint8_t *p)
{unsigned i;uint64_t v=0;for(i=0;i<8;++i)v=(v<<8)|p[i];return v;}
static int valid_info(const struct pt_recovery_info *m,size_t *length)
{
    size_t n;
    if(!m || !m->document_id || !m->timestamp || m->revision<=m->saved_revision)return 0;
    for(n=0;n<=PT_RECOVERY_SOURCE_MAX && m->source[n];++n)
        if((unsigned char)m->source[n]<32)return 0;
    if(n>PT_RECOVERY_SOURCE_MAX)return 0;
    *length=n;return 1;
}
enum pt_save_result pt_recovery_file_save(const char *path,const struct pt_project *p,
    const struct pt_recovery_info *m,const struct pt_allocator *a)
{
    struct workspace {struct pt_project project;struct pt_extension extension[];} *w;
    uint8_t metadata[HEADER+PT_RECOVERY_SOURCE_MAX];size_t n,bytes;unsigned i;
    enum pt_save_result result;uint32_t caps;
    if(!path || !a || !a->allocate || !a->release || !valid_info(m,&n) ||
       pt_project_validate(p,&caps)!=PT_PROJECT_OK || p->extension_count>=4090)return PT_SAVE_INVALID;
    for(i=0;i<p->extension_count;++i)if(p->extensions[i].id==PT_RECOVERY_EXTENSION)return PT_SAVE_INVALID;
    bytes=sizeof(*w)+((size_t)p->extension_count+1)*sizeof(struct pt_extension);
    w=a->allocate(a->context,bytes);if(!w)return PT_SAVE_MEMORY;
    memset(metadata,0,sizeof(metadata));memcpy(metadata,"PTREC001",8);
    put64(metadata+8,m->document_id);put64(metadata+16,m->revision);
    put64(metadata+24,m->saved_revision);put64(metadata+32,m->timestamp);
    metadata[40]=(uint8_t)(n>>8);metadata[41]=(uint8_t)n;
    memcpy(metadata+HEADER,m->source,n);
    w->project=*p;
    if(p->extension_count)memcpy(w->extension,p->extensions,p->extension_count*sizeof(*w->extension));
    w->extension[p->extension_count]=(struct pt_extension){PT_RECOVERY_EXTENSION,(uint32_t)(HEADER+n),1,metadata};
    w->project.extensions=w->extension;w->project.extension_count++;
    result=pt_project_file_save(path,&w->project,a);a->release(a->context,w);return result;
}
int pt_recovery_project_info(const struct pt_project *p,struct pt_recovery_info *out)
{
    const struct pt_extension *e=NULL;const uint8_t *b;struct pt_recovery_info m;
    size_t n,checked;unsigned i;uint32_t caps;
    if(!out || pt_project_validate(p,&caps)!=PT_PROJECT_OK)return 0;
    for(i=0;i<p->extension_count;++i)if(p->extensions[i].id==PT_RECOVERY_EXTENSION) {
        if(e)return 0;
        e=p->extensions+i;
    }
    if(!e || e->version!=1 || e->length<HEADER || e->length>HEADER+PT_RECOVERY_SOURCE_MAX)return 0;
    b=e->data;n=((size_t)b[40]<<8)|b[41];
    if(memcmp(b,"PTREC001",8) || b[42] || b[43] || e->length!=HEADER+n || n>PT_RECOVERY_SOURCE_MAX)return 0;
    memset(&m,0,sizeof(m));m.document_id=get64(b+8);m.revision=get64(b+16);
    m.saved_revision=get64(b+24);m.timestamp=get64(b+32);memcpy(m.source,b+HEADER,n);
    if(!valid_info(&m,&checked) || checked!=n)return 0;
    *out=m;return 1;
}
enum pt_project_result pt_recovery_file_load(struct pt_document *d,const char *path,
    uint64_t expected,size_t limit,size_t budget,struct pt_recovery_info *out)
{
    struct pt_document staged;struct pt_recovery_info m;enum pt_project_result result;unsigned i;
    if(!d || !out || !expected)return PT_PROJECT_INVALID;
    pt_document_init(&staged,&d->allocator);
    result=pt_project_file_load(&staged,path,limit,budget);
    if(result==PT_PROJECT_OK) {
        if(!pt_recovery_project_info(&staged.project,&m) || m.document_id!=expected)result=PT_PROJECT_INVALID;
        else {
            for(i=0;i<staged.project.extension_count;++i)if(staged.project.extensions[i].id==PT_RECOVERY_EXTENSION)break;
            --staged.project.extension_count;
            memmove(staged.project.extensions+i,staged.project.extensions+i+1,
                (staged.project.extension_count-i)*sizeof(*staged.project.extensions));
            staged.dirty=1;pt_document_release(d);*d=staged;*out=m;return PT_PROJECT_OK;
        }
    }
    pt_document_release(&staged);return result;
}
