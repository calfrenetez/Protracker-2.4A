#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "io_faults.h"
#include "ledger.h"
#include "file_save.h"
#include "file_load.h"
#include "project_file.h"
#include "mod_file.h"
#include "sample_file.h"
#include "mod_import.h"
#include "project_import.h"
#include "pp20_import.h"
#include "mod_project.h"

static uint8_t payload[10003];
static const char *const save_names[]={"allocated","generated","streamed","PTG","MOD-direct","MOD-round8","MOD-tpdf8","WAV","RAW","SVX"};
static struct pt_document source;
static struct pt_document source_before;
static struct fh_ledger *source_ledger;
static size_t source_live;
static int32_t master[10003];
static struct pt_pcm pcm={master,10003,10003,22050,1,8};
static struct pt_raw_format raw={22050,8,1,0,0};
static struct pt_svx_info svx={.loop_start=2,.loop_end=10000,.volume=65536};
struct producer {unsigned calls,fail;int mismatch;};
static void guard_source(void)
{
    size_t i;
    assert(source_ledger && source_ledger->live==source_live);
    assert(!memcmp(&source_before,&source,sizeof(source)));
    fh_check_snapshot(source_ledger);
    for(i=0;i<10003;++i)assert(master[i]==(int32_t)(i%256)-128);
}
static void snapshot_source(struct fh_ledger *ledger)
{
    assert(!source_ledger);
    source_ledger=ledger;source_live=ledger->live;
    memcpy(&source_before,&source,sizeof(source));fh_snapshot(ledger);
    guard_source();
}
static void drop_source_snapshot(void)
{
    guard_source();fh_drop_snapshot(source_ledger);source_ledger=NULL;
}
static int generate(void *ctx,size_t offset,void *out,size_t n)
{
    struct producer *p=ctx;++p->calls;if(p->calls==p->fail)return 0;
    memcpy(out,payload+offset,n);if(p->mismatch && p->calls>3)((uint8_t *)out)[0]^=1;return 1;
}
static int produce(void *ctx,pt_file_sink sink,void *sc)
{
    struct producer *p=ctx;size_t at=0;++p->calls;
    if(p->calls==p->fail) {assert(sink(sc,payload,17));return 0;}
    while(at<sizeof(payload)) {size_t n=sizeof(payload)-at;if(n>1103)n=1103;if(!sink(sc,payload+at,n))return 0;at+=n;}
    return 1;
}
static enum pt_save_result save_unguarded(int kind,const char *path,struct pt_allocator *a,struct producer *p)
{
    switch(kind) {
    case 0:return pt_file_save_new_allocated(path,payload,sizeof(payload),a);
    case 1:return pt_file_save_generated(path,sizeof(payload),generate,p,a);
    case 2:return pt_file_save_streamed(path,sizeof(payload),produce,p,a);
    case 3:return pt_project_file_save(path,&source.project,a);
    case 4:case 5:case 6:return pt_mod_file_save(path,&source.project,(unsigned)kind-4,a);
    case 7:return pt_sample_wav_save(path,&pcm,a);
    case 8:return pt_sample_raw_save(path,&pcm,&raw,a);
    case 9:return pt_sample_svx_save(path,&pcm,&svx,a);
    default:assert(0);return PT_SAVE_INVALID;
    }
}
static enum pt_save_result save(int kind,const char *path,struct pt_allocator *a,struct producer *p)
{
    enum pt_save_result result;
    guard_source();result=save_unguarded(kind,path,a,p);guard_source();
    return result;
}
static uint8_t *read_file(const char *path,size_t *n)
{
    FILE *f=fopen(path,"rb");long end;uint8_t *data;assert(f && !fseek(f,0,SEEK_END));end=ftell(f);assert(end>=0 && end<=1048576);rewind(f);
    *n=(size_t)end;data=malloc(*n+1);assert(data && fread(data,1,*n,f)==*n && !fclose(f));return data;
}
static void exact(const char *path,const void *expected,size_t n)
{size_t count;uint8_t *b=read_file(path,&count);assert(count==n && !memcmp(b,expected,n));free(b);}
static void write_file(const char *path,const void *p,size_t n)
{FILE *f=fopen(path,"wb");assert(f && fwrite(p,1,n,f)==n && !fclose(f));}
static void path_at(char *path,size_t n,const char *dir,const char *name)
{int r=snprintf(path,n,"%s/%s",dir,name);assert(r>0 && (size_t)r<n);}
static void no_residue(const char *dir,const char *allowed)
{
    DIR *d=opendir(dir);struct dirent *entry;assert(d);
    while((entry=readdir(d)))if(strcmp(entry->d_name,".") && strcmp(entry->d_name,".."))assert(allowed && !strcmp(entry->d_name,allowed));
    assert(!closedir(d));
}
static void report(const char *path,const char *event,size_t index,int mode,int result,struct fh_ledger *l)
{
    guard_source();
    printf("{\"path\":\"%s\",\"event\":\"%s\",\"index\":%lu,\"mode\":%d,\"result\":%d,\"ioTriggered\":%lu,\"allocationCalls\":%lu,\"allocationTriggered\":%lu,\"released\":%lu,\"peakBytes\":%lu,\"liveBytes\":%lu,\"liveDescriptors\":%lu,\"retainedTemp\":%s}\n",path,event,(unsigned long)index,mode,result,(unsigned long)fh_io.triggered,(unsigned long)l->calls,(unsigned long)l->triggered,(unsigned long)l->releases,(unsigned long)l->peak,(unsigned long)l->live,(unsigned long)fh_io.open_live,fh_io.retained[0]?"true":"false");
    guard_source();
}
static void save_campaign(const char *dir,unsigned depth)
{
    int kind,op,mode;char path[1600],conflict[1700];size_t i,n,counts[FH_CALLS];
    struct fh_ledger l={0};struct pt_allocator a={&l,fh_alloc,fh_free};
    assert(depth==8 || depth==16 || depth==24);
    path_at(path,sizeof(path),dir,"output");
    for(kind=0;kind<10;++kind) {
        char label[64];const char *name=save_names[kind];int label_bytes;
        struct producer p={0};uint8_t *expected;struct pt_project before=source.project;
        /* High precision uses project save and only the two explicit MOD modes. */
        if(depth!=8 && kind!=3 && kind!=5 && kind!=6)continue;
        if(depth!=8) {
            label_bytes=snprintf(label,sizeof(label),"%s-%ubit",name,depth);
            assert(label_bytes>0 && (size_t)label_bytes<sizeof(label));name=label;
        }
        fh_reset(&l,0);fh_io_reset(0,0,0);assert(save(kind,path,&a,&p)==PT_SAVE_OK && !l.live && !fh_io.open_live);
        assert(l.calls==1 && l.calls<=FH_MAX_ALLOCATIONS);memcpy(counts,fh_io.calls,sizeof(counts));expected=read_file(path,&n);assert(!unlink(path));
        report(name,"control",0,0,PT_SAVE_OK,&l);
        /* Each successful syscall index, independently, on real production I/O. */
        for(op=0;op<FH_CALLS;++op)for(i=1;i<=counts[op]+1;++i) {
            enum pt_save_result result;int sentinel=i>counts[op];
            p=(struct producer){0};fh_reset(&l,0);fh_io_reset(op,i,FH_ERROR);
            result=save(kind,path,&a,&p);assert(!l.live && !fh_io.open_live);
            assert(fh_io.triggered==(size_t)!sentinel);
            if(sentinel || op==FH_UNLINK) {assert(result==PT_SAVE_OK);exact(path,expected,n);assert(!unlink(path));}
            else {
                enum pt_save_result expected_phase=PT_SAVE_INVALID;
                if(op==FH_OPEN)expected_phase=i==1?PT_SAVE_BEGIN:PT_SAVE_VERIFY;
                if(op==FH_READ)expected_phase=PT_SAVE_VERIFY;
                if(op==FH_WRITE)expected_phase=PT_SAVE_WRITE;
                if(op==FH_SYNC)expected_phase=PT_SAVE_FINISH;
                if(op==FH_CLOSE)expected_phase=i==1?PT_SAVE_FINISH:PT_SAVE_VERIFY;
                if(op==FH_LINK)expected_phase=PT_SAVE_PUBLISH;
                assert(result==expected_phase);assert(access(path,F_OK)!=0);
            }
            if(op==FH_UNLINK && !sentinel) {assert(fh_io.retained[0]);exact(fh_io.retained,expected,n);assert(!unlink(fh_io.retained));}
            report(name,fh_names[op],i,FH_ERROR,result,&l);no_residue(dir,NULL);
            assert(!memcmp(&before,&source.project,sizeof(before)));
        }
        /* Progress versus failure, including read EINTR and write EINTR refusal. */
        for(op=FH_READ;op<=FH_WRITE;++op)for(mode=FH_ZERO;mode<=FH_EINTR;++mode) {
            enum pt_save_result result;int success=mode==FH_SHORT || (mode==FH_EINTR && op==FH_READ);
            p=(struct producer){0};fh_reset(&l,0);fh_io_reset(op,1,mode);
            result=save(kind,path,&a,&p);assert(fh_io.triggered==1 && !l.live && !fh_io.open_live);
            if(success) {assert(result==PT_SAVE_OK);exact(path,expected,n);assert(!unlink(path));}
            else {assert(result==((op==FH_READ)?PT_SAVE_VERIFY:PT_SAVE_WRITE));assert(access(path,F_OK)!=0);}
            report(name,fh_names[op],1,mode,result,&l);no_residue(dir,NULL);
        }
        for(mode=FH_MISMATCH;mode<=FH_EXTRA;++mode) {
            enum pt_save_result result;p=(struct producer){0};fh_reset(&l,0);fh_io_reset(FH_OPEN,2,mode);
            result=save(kind,path,&a,&p);assert(result==PT_SAVE_VERIFY && fh_io.triggered==1 && !l.live && !fh_io.open_live);
            assert(access(path,F_OK)!=0);report(name,"verify-staging",2,mode,result,&l);no_residue(dir,NULL);
        }
        /* Existing destination and the destination-created-at-link race. */
        for(mode=0;mode<2;++mode) {
            enum pt_save_result result;if(!mode)write_file(path,"OLD",3);
            p=(struct producer){0};fh_reset(&l,0);fh_io_reset(FH_LINK,mode?1:0,FH_RACE);
            result=save(kind,path,&a,&p);assert(result==PT_SAVE_PUBLISH && !l.live && !fh_io.open_live);
            exact(path,mode?"RACE":"OLD",mode?4:3);report(name,mode?"race":"existing",1,FH_RACE,result,&l);
            assert(!unlink(path));no_residue(dir,NULL);
        }
        /* A conflicting exclusive temp belongs to someone else. */
        assert(snprintf(conflict,sizeof(conflict),"%s.pttmp-%lu-0",path,(unsigned long)getpid())>0);write_file(conflict,"UNOWNED",7);
        p=(struct producer){0};fh_reset(&l,0);fh_io_reset(0,0,0);
        assert(save(kind,path,&a,&p)==PT_SAVE_OK);exact(conflict,"UNOWNED",7);exact(path,expected,n);
        assert(!unlink(path) && !unlink(conflict));no_residue(dir,NULL);
        /* Fail allocation 1, then the nontriggered N+1 sentinel. */
        for(i=1;i<=2;++i) {
            enum pt_save_result result;p=(struct producer){0};fh_reset(&l,i);fh_io_reset(0,0,0);
            result=save(kind,path,&a,&p);assert(!l.live && !fh_io.open_live);
            if(i==1)assert(result==PT_SAVE_MEMORY && l.triggered==1 && !fh_io.calls[FH_OPEN]);
            else {assert(result==PT_SAVE_OK && !l.triggered);exact(path,expected,n);assert(!unlink(path));}
            report(name,"allocate",i,0,result,&l);no_residue(dir,NULL);
        }
        /* Exact workspace budget boundary from successful ledger high-water. */
        fh_reset(&l,0);fh_io_reset(0,0,0);p=(struct producer){0};assert(save(kind,path,&a,&p)==PT_SAVE_OK);assert(!unlink(path));
        {size_t workspace=l.peak;for(i=workspace-1;i<=workspace+1;++i) {
            enum pt_save_result result;l.limit=i;fh_reset(&l,0);fh_io_reset(0,0,0);p=(struct producer){0};result=save(kind,path,&a,&p);
            if(i<workspace)assert(result==PT_SAVE_MEMORY && l.triggered==1);
            else {assert(result==PT_SAVE_OK);exact(path,expected,n);assert(!unlink(path));}
            assert(!l.live);report(name,"workspace-budget",i,0,result,&l);
        }l.limit=0;}
        /* Precommit cleanup denial retains the primary write failure and exact owned artifact. */
        p=(struct producer){0};fh_reset(&l,0);fh_io_reset(FH_WRITE,2,FH_ERROR);fh_io.deny_cleanup=1;fh_io.short_writes=1;
        {enum pt_save_result result=save(kind,path,&a,&p);
            assert(result==PT_SAVE_WRITE && fh_io.triggered==1 && !l.live && !fh_io.open_live && fh_io.retained[0]);
            assert(access(path,F_OK)!=0 && access(fh_io.retained,F_OK)==0);report(name,"cleanup-denied",2,FH_ERROR,result,&l);
            fh_io.deny_cleanup=0;assert(!unlink(fh_io.retained));no_residue(dir,NULL);}
        free(expected);
    }
    if(depth!=8)return;
    for(kind=1;kind<=2;++kind)for(i=1;i<=(kind==1?6U:2U);++i) {
        enum pt_save_result result;struct producer p={0,(unsigned)i,0};fh_reset(&l,0);fh_io_reset(0,0,0);
        result=save(kind,path,&a,&p);assert(result==(i<=(kind==1?3U:1U)?PT_SAVE_WRITE:PT_SAVE_VERIFY));
        assert(!l.live && !fh_io.open_live && access(path,F_OK)!=0);report(save_names[kind],"producer-stop",i,0,result,&l);no_residue(dir,NULL);
    }
}

static enum pt_project_result load(int kind,struct pt_document *d,const char *path,size_t budget)
{
    if(kind==0)return pt_mod_file_load(d,path,1048576,budget);
    if(kind==1)return pt_project_file_load(d,path,1048576,budget);
    return pt_pp20_file_load(d,path,1048576,budget);
}
static void load_campaign(const char *dir,const char *corpus)
{
    const char *names[]={"mod-16-2.mod","ptg-rich.ptg","pp20-skip-7.pp20"};
    int kind,op,mode;size_t i,N,counts[FH_CALLS],required,old_n;char path[1600];uint8_t *old_bytes;(void)dir;
    path_at(path,sizeof(path),corpus,"ptg-rich.ptg");old_bytes=read_file(path,&old_n);
    for(kind=0;kind<3;++kind) {
        struct fh_ledger l={0};struct pt_allocator a={&l,fh_alloc,fh_free};struct pt_document d,before;
        path_at(path,sizeof(path),corpus,names[kind]);pt_document_init(&d,&a);fh_io_reset(0,0,0);
        assert(load(kind,&d,path,SIZE_MAX)==PT_PROJECT_OK);N=l.calls;assert(N<=FH_MAX_ALLOCATIONS);required=d.allocated_bytes;
        memcpy(counts,fh_io.calls,sizeof(counts));report(names[kind],"control",0,0,PT_PROJECT_OK,&l);pt_document_release(&d);assert(!l.live);
        for(op=-1;op<FH_CALLS;++op)for(i=1;i<=(op<0?N:counts[op])+1;++i) {
            enum pt_project_result result;size_t old_live;int sentinel=i>(op<0?N:counts[op]);
            fh_reset(&l,0);assert(pt_document_load(&d,old_bytes,old_n,SIZE_MAX)==PT_PROJECT_OK);d.dirty=1;before=d;old_live=l.live;fh_snapshot(&l);
            fh_reset(&l,op<0?i:0);fh_io_reset(op,op<0?0:i,FH_ERROR);
            /* Drop snapshots before a legitimate successful replacement releases old storage. */
            if(sentinel)fh_drop_snapshot(&l);
            result=load(kind,&d,path,SIZE_MAX);
            if(sentinel)assert(result==PT_PROJECT_OK && !l.triggered && !fh_io.triggered);
            else {
                assert(result!=PT_PROJECT_OK && !memcmp(&before,&d,sizeof(d)) && l.live==old_live);fh_check_snapshot(&l);fh_drop_snapshot(&l);
                assert(op<0?l.triggered==1:fh_io.triggered==1);
            }
            assert(!fh_io.open_live);report(names[kind],op<0?"allocate":fh_names[op],i,FH_ERROR,result,&l);
            pt_document_release(&d);assert(!l.live);
        }
        for(mode=FH_ZERO;mode<=FH_EINTR;++mode) {
            enum pt_project_result result;size_t old_live;int success=mode==FH_SHORT || mode==FH_EINTR;
            fh_reset(&l,0);assert(pt_document_load(&d,old_bytes,old_n,SIZE_MAX)==PT_PROJECT_OK);before=d;old_live=l.live;fh_snapshot(&l);fh_reset(&l,0);fh_io_reset(FH_READ,1,mode);
            if(success)fh_drop_snapshot(&l);
            result=load(kind,&d,path,SIZE_MAX);assert(fh_io.triggered==1 && !fh_io.open_live);
            if(success)assert(result==PT_PROJECT_OK);
            else {assert(result!=PT_PROJECT_OK && !memcmp(&before,&d,sizeof(d)) && l.live==old_live);fh_check_snapshot(&l);fh_drop_snapshot(&l);}
            report(names[kind],"read",1,mode,result,&l);pt_document_release(&d);assert(!l.live);
        }
        /* PP20's budget also includes the decompressed MOD temporary. */
        if(kind==2)required+=2124;
        for(i=required-1;i<=required+1;++i) {
            enum pt_project_result result;size_t old_live;
            fh_reset(&l,0);assert(pt_document_load(&d,old_bytes,old_n,SIZE_MAX)==PT_PROJECT_OK);
            before=d;old_live=l.live;fh_snapshot(&l);
            fh_reset(&l,0);fh_io_reset(0,0,0);
            /* Legitimate replacement may free the old document on success. */
            if(i>=required)fh_drop_snapshot(&l);
            result=load(kind,&d,path,i);
            if(i<required) {
                assert(result==PT_PROJECT_CAPACITY && !memcmp(&before,&d,sizeof(d)) && l.live==old_live);
                fh_check_snapshot(&l);fh_drop_snapshot(&l);
            } else assert(result==PT_PROJECT_OK);
            report(names[kind],"document-budget",i,0,result,&l);pt_document_release(&d);assert(!l.live);
        }
    }
    free(old_bytes);
}
static void generic_load_campaign(const char *dir)
{
    char path[1600];struct fh_ledger l={0};struct pt_allocator a={&l,fh_alloc,fh_free};size_t n,counts[FH_CALLS],i;uint8_t *bytes;int op,mode;
    path_at(path,sizeof(path),dir,"input");write_file(path,payload,sizeof(payload));fh_io_reset(0,0,0);
    assert(pt_file_load(path,sizeof(payload),&a,&bytes,&n)==PT_LOAD_OK && n==sizeof(payload) && !memcmp(bytes,payload,n));memcpy(counts,fh_io.calls,sizeof(counts));fh_free(&l,bytes);
    for(op=0;op<FH_CALLS;++op)for(i=1;i<=counts[op]+1;++i) {
        enum pt_load_result result;bytes=(uint8_t *)(uintptr_t)1;n=777;fh_reset(&l,0);fh_io_reset(op,i,FH_ERROR);
        result=pt_file_load(path,sizeof(payload),&a,&bytes,&n);
        if(i>counts[op]) {assert(result==PT_LOAD_OK && !fh_io.triggered);fh_free(&l,bytes);}
        else assert(result!=PT_LOAD_OK && bytes==(uint8_t *)(uintptr_t)1 && n==777 && fh_io.triggered==1);
        assert(!l.live && !fh_io.open_live);report("file-load",fh_names[op],i,0,result,&l);
    }
    for(mode=FH_ZERO;mode<=FH_EINTR;++mode) {
        enum pt_load_result result;bytes=(uint8_t *)(uintptr_t)1;n=777;fh_reset(&l,0);fh_io_reset(FH_READ,1,mode);
        result=pt_file_load(path,sizeof(payload),&a,&bytes,&n);assert(fh_io.triggered==1);
        if(mode==FH_ZERO)assert(result==PT_LOAD_IO && bytes==(uint8_t *)(uintptr_t)1 && n==777);
        else {assert(result==PT_LOAD_OK && n==sizeof(payload) && !memcmp(bytes,payload,n));fh_free(&l,bytes);}
        assert(!l.live && !fh_io.open_live);report("file-load","read",1,mode,result,&l);
    }
    assert(!unlink(path));
}
int main(int argc,char **argv)
{
    struct fh_ledger l={0};struct pt_allocator a={&l,fh_alloc,fh_free};size_t i,n;uint8_t *b;unsigned depth;
    assert(argc==3);for(i=0;i<sizeof(payload);++i) {payload[i]=(uint8_t)(i*37+19);master[i]=(int32_t)(i%256)-128;}
    pt_document_init(&source,&a);{char path[1600];path_at(path,sizeof(path),argv[2],"mod-16-2.mod");b=read_file(path,&n);}
    assert(pt_document_load(&source,b,n,SIZE_MAX)==PT_PROJECT_OK);free(b);snapshot_source(&l);
    save_campaign(argv[1],8);
    for(depth=16;depth<=24;depth+=8) {
        char path[1600],name[32];int name_bytes=snprintf(name,sizeof(name),"ptg-%u.ptg",depth);
        assert(name_bytes>0 && (size_t)name_bytes<sizeof(name));path_at(path,sizeof(path),argv[2],name);b=read_file(path,&n);
        /* Snapshots stay live until immediately before genuine replacement. */
        drop_source_snapshot();assert(pt_document_load(&source,b,n,SIZE_MAX)==PT_PROJECT_OK);free(b);
        assert(source.project.sample_count==1 && source.project.samples[0].pcm.bits==depth && source.project.samples[0].pcm.channels==1);
        snapshot_source(&l);save_campaign(argv[1],depth);
    }
    load_campaign(argv[1],argv[2]);generic_load_campaign(argv[1]);
    drop_source_snapshot();pt_document_release(&source);
    assert(!l.live && !fh_io.open_live);no_residue(argv[1],NULL);
    puts("IO_CAMPAIGN PASS: syscall indices, allocation ledgers, immutable masters, no-replace and commit-point cleanup");return 0;
}
