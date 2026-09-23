#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#ifdef __linux__
#include <fcntl.h>
#endif
#ifdef __amigaos__
#include <proto/dos.h>
#endif
#include "stem_file.h"
#include "document.h"
struct batch {
    struct pt_stem_report report;struct pt_render_options one;struct pt_render_report measured[16];
    char stage[1300],file[1400];
};
static int directory_new(const char *path)
{
#ifdef __amigaos__
    BPTR lock=CreateDir((STRPTR)path);
    if(!lock)return IoErr()==ERROR_OBJECT_EXISTS?0:-1;
    UnLock(lock);return 1;
#else
    if(!mkdir(path,0700))return 1;
    return errno==EEXIST?0:-1;
#endif
}
static void directory_remove(const char *path)
{
#ifdef __amigaos__
    if(!DeleteFile((STRPTR)path))fprintf(stderr,"Stem staging retained: %s\n",path);
#else
    if(rmdir(path))fprintf(stderr,"Stem staging retained: %s\n",path);
#endif
}
static int publish(const char *stage,const char *path)
{
#ifdef __amigaos__
    return Rename((STRPTR)stage,(STRPTR)path)!=0;
#elif defined(__APPLE__)
    return renamex_np(stage,path,RENAME_EXCL)==0;
#elif defined(__linux__)
    return renameat2(AT_FDCWD,stage,AT_FDCWD,path,RENAME_NOREPLACE)==0;
#else
    (void)stage;(void)path;return 0; /* Never fall back to overwriting rename. */
#endif
}
static void filename(char *out,size_t size,const char *dir,const struct pt_stem *stem)
{
    snprintf(out,size,"%s/%s-%02u.wav",dir,stem->group?"group":"track",stem->group?stem->group:stem->channel+1);
}
static enum pt_render_file_result batch_new(const char *path,const struct pt_project *project,
    const struct pt_render_options *options,unsigned grouped,pt_render_progress notify,void *ctx,
    struct pt_stem_report *out,enum pt_render_result *detail,const struct pt_allocator *allocator,struct batch *w)
{
    enum pt_stem_result planned;enum pt_render_file_result result=PT_RENDER_FILE_RENDER;
    unsigned i,owned=0;int made=0;
    if(detail)*detail=PT_RENDER_INVALID;
    if(!path || !*path || strlen(path)>1200 || !project || !options || !out || !detail)return PT_RENDER_FILE_INVALID;
    memset(&w->report,0,sizeof(w->report));planned=pt_stems_plan(&project->channels,options->tracks,grouped,&w->report.plan);
    if(planned!=PT_STEM_OK) {*detail=planned==PT_STEM_MIDI?PT_RENDER_ROUTE:PT_RENDER_INVALID;return result;}
    w->one=*options;
    /* Validate all stems before creating any staging; global flow stays intact. */
    for(i=0;i<w->report.plan.count;++i) {
        w->one.tracks=w->report.plan.item[i].tracks;*detail=(allocator?pt_render_measure_allocated(project,&w->one,notify,ctx,w->measured+i,allocator):pt_render_measure(project,&w->one,notify,ctx,w->measured+i));
        if(*detail!=PT_RENDER_OK)return result;
        if(i && (w->measured[i].frames!=w->measured[0].frames || w->measured[i].ticks!=w->measured[0].ticks || w->measured[i].end!=w->measured[0].end)) {*detail=PT_RENDER_INVALID;return result;}
    }
    if(!access(path,F_OK))return PT_RENDER_FILE_BEGIN;
    for(i=0;i<32;++i) {
        snprintf(w->stage,sizeof(w->stage),"%s.ptstems-%lu-%u",path,(unsigned long)getpid(),i);
        made=directory_new(w->stage);if(made)break;
    }
    if(made!=1)return PT_RENDER_FILE_BEGIN;
    for(i=0;i<w->report.plan.count;++i) {
        w->one.tracks=w->report.plan.item[i].tracks;filename(w->file,sizeof(w->file),w->stage,w->report.plan.item+i);
        result=pt_render_file_new_allocated(w->file,project,&w->one,notify,ctx,w->report.audio+i,detail,allocator);
        if(result!=PT_RENDER_FILE_OK)goto fail;
        ++owned;
        if(w->report.audio[i].frames!=w->measured[i].frames || w->report.audio[i].ticks!=w->measured[i].ticks || w->report.audio[i].end!=w->measured[i].end) {
            *detail=PT_RENDER_INVALID;result=PT_RENDER_FILE_VERIFY;goto fail;
        }
    }
    if(notify && !notify(ctx,PT_RENDER_VERIFY,w->measured[0].ticks,w->measured[0].frames)) {*detail=PT_RENDER_CANCELLED;result=PT_RENDER_FILE_RENDER;goto fail;}
    if(!publish(w->stage,path)) {result=PT_RENDER_FILE_PUBLISH;goto fail;}
    *out=w->report;return PT_RENDER_FILE_OK;
fail:
    for(i=0;i<owned;++i) {filename(w->file,sizeof(w->file),w->stage,w->report.plan.item+i);if(unlink(w->file))fprintf(stderr,"Stem WAV retained: %s\n",w->file);}
    directory_remove(w->stage);return result;
}

enum pt_render_file_result pt_stem_file_new(const char *path,const struct pt_project *project,
    const struct pt_render_options *options,unsigned grouped,pt_render_progress notify,void *ctx,
    struct pt_stem_report *out,enum pt_render_result *detail)
{
    struct batch w;return batch_new(path,project,options,grouped,notify,ctx,out,detail,NULL,&w);
}
enum pt_render_file_result pt_stem_file_new_allocated(const char *path,const struct pt_project *project,
    const struct pt_render_options *options,unsigned grouped,pt_render_progress notify,void *ctx,
    struct pt_stem_report *out,enum pt_render_result *detail,const struct pt_allocator *allocator)
{
    struct batch *w;enum pt_render_file_result result;
    if(!allocator)return pt_stem_file_new(path,project,options,grouped,notify,ctx,out,detail);
    if(detail)*detail=PT_RENDER_INVALID;
    if(!path || !*path || strlen(path)>1200 || !project || !options || !out || !detail || !allocator->allocate || !allocator->release)return PT_RENDER_FILE_INVALID;
    w=allocator->allocate(allocator->context,sizeof(*w));
    if(!w) {*detail=PT_RENDER_MEMORY;return PT_RENDER_FILE_RENDER;}
    result=batch_new(path,project,options,grouped,notify,ctx,out,detail,allocator,w);
    allocator->release(allocator->context,w);return result;
}
