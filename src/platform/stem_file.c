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
enum pt_render_file_result pt_stem_file_new(const char *path,const struct pt_project *project,
    const struct pt_render_options *options,unsigned grouped,pt_render_progress notify,void *ctx,
    struct pt_stem_report *out,enum pt_render_result *detail)
{
    struct pt_stem_report report;struct pt_render_options one;struct pt_render_report measured[16];
    enum pt_stem_result planned;enum pt_render_file_result result=PT_RENDER_FILE_RENDER;
    char stage[1300],file[1400];unsigned i,owned=0;int made=0;
    if(detail)*detail=PT_RENDER_INVALID;
    if(!path || !*path || strlen(path)>1200 || !project || !options || !out || !detail)return PT_RENDER_FILE_INVALID;
    memset(&report,0,sizeof(report));planned=pt_stems_plan(&project->channels,options->tracks,grouped,&report.plan);
    if(planned!=PT_STEM_OK) {*detail=planned==PT_STEM_MIDI?PT_RENDER_ROUTE:PT_RENDER_INVALID;return result;}
    one=*options;
    /* Validate all stems before creating any staging; global flow stays intact. */
    for(i=0;i<report.plan.count;++i) {
        one.tracks=report.plan.item[i].tracks;*detail=pt_render_measure(project,&one,notify,ctx,measured+i);
        if(*detail!=PT_RENDER_OK)return result;
        if(i && (measured[i].frames!=measured[0].frames || measured[i].ticks!=measured[0].ticks || measured[i].end!=measured[0].end)) {*detail=PT_RENDER_INVALID;return result;}
    }
    if(!access(path,F_OK))return PT_RENDER_FILE_BEGIN;
    for(i=0;i<32;++i) {
        snprintf(stage,sizeof(stage),"%s.ptstems-%lu-%u",path,(unsigned long)getpid(),i);
        made=directory_new(stage);if(made)break;
    }
    if(made!=1)return PT_RENDER_FILE_BEGIN;
    for(i=0;i<report.plan.count;++i) {
        one.tracks=report.plan.item[i].tracks;filename(file,sizeof(file),stage,report.plan.item+i);
        result=pt_render_file_new(file,project,&one,notify,ctx,report.audio+i,detail);
        if(result!=PT_RENDER_FILE_OK)goto fail;
        ++owned;
        if(report.audio[i].frames!=measured[i].frames || report.audio[i].ticks!=measured[i].ticks || report.audio[i].end!=measured[i].end) {
            *detail=PT_RENDER_INVALID;result=PT_RENDER_FILE_VERIFY;goto fail;
        }
    }
    if(notify && !notify(ctx,PT_RENDER_VERIFY,measured[0].ticks,measured[0].frames)) {*detail=PT_RENDER_CANCELLED;result=PT_RENDER_FILE_RENDER;goto fail;}
    if(!publish(stage,path)) {result=PT_RENDER_FILE_PUBLISH;goto fail;}
    *out=report;return PT_RENDER_FILE_OK;
fail:
    for(i=0;i<owned;++i) {filename(file,sizeof(file),stage,report.plan.item+i);if(unlink(file))fprintf(stderr,"Stem WAV retained: %s\n",file);}
    directory_remove(stage);return result;
}
