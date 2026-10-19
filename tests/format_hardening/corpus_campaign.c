#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ledger.h"
#include "wav.h"
#include "svx.h"
#include "raw.h"
#include "mod_project.h"

static uint8_t *read_file(const char *dir,const char *name,size_t *length)
{
    char path[2048];FILE *f;long n;uint8_t *data;
    assert(snprintf(path,sizeof(path),"%s/%s",dir,name)>0);f=fopen(path,"rb");assert(f && !fseek(f,0,SEEK_END));n=ftell(f);assert(n>=0 && n<=1048576);rewind(f);
    *length=(size_t)n;data=malloc(*length+1);assert(data && fread(data,1,*length,f)==*length && !fclose(f));return data;
}
struct reader {const uint8_t *data;size_t n,calls,fail;};
static int read_at(void *ctx,size_t pos,uint8_t *out,size_t n)
{
    struct reader *r=ctx;assert(pos<=r->n && n<=r->n-pos && n<=1092);
    if(++r->calls==r->fail)return 0;
    memcpy(out,r->data+pos,n);return 1;
}
static void expected_pcm(const char *dir,const char *name,const struct pt_pcm *p)
{
    size_t n,i;uint8_t *b=read_file(dir,name,&n);assert(n==4*(size_t)p->frames*p->channels);
    for(i=0;i<n;i+=4) {
        uint32_t u=((uint32_t)b[i]<<24)|((uint32_t)b[i+1]<<16)|((uint32_t)b[i+2]<<8)|b[i+3];
        int32_t v=u>INT32_MAX?(int32_t)((int64_t)u-4294967296LL):(int32_t)u;
        assert(p->data[i/4]==v);
    }free(b);
}
static void project_case(const char *id,const char *fmt,const uint8_t *data,size_t n,int valid)
{
    struct fh_ledger l={0};struct pt_allocator a={&l,fh_alloc,fh_free};struct pt_document d,before;
    uint8_t *saved;size_t size,w;enum pt_project_result r;
    pt_document_init(&d,&a);assert(pt_document_new(&d,4,SIZE_MAX)==PT_PROJECT_OK);d.dirty=1;before=d;fh_snapshot(&l);
    if(valid)fh_drop_snapshot(&l);
    r=pt_document_load(&d,data,n,1048576);
    if(!valid) {assert(r!=PT_PROJECT_OK && !memcmp(&before,&d,sizeof(d)));fh_check_snapshot(&l);fh_drop_snapshot(&l);}
    else {
        struct pt_document re;size_t out_size,w2;uint8_t *again;
        assert(r==PT_PROJECT_OK && !d.dirty);assert(pt_project_size(&d.project,&size)==PT_PROJECT_OK);saved=malloc(size);assert(saved);
        assert(pt_project_encode(&d.project,saved,size,&w)==PT_PROJECT_OK && w==size);
        pt_document_init(&re,&a);assert(pt_document_load(&re,saved,w,1048576)==PT_PROJECT_OK);
        assert(pt_project_size(&re.project,&out_size)==PT_PROJECT_OK && out_size==size);again=malloc(size);assert(again);
        assert(pt_project_encode(&re.project,again,size,&w2)==PT_PROJECT_OK && w2==size && !memcmp(saved,again,size));
        if(!strcmp(fmt,"PTG"))assert(size==n && !memcmp(saved,data,n));
        if(!strcmp(fmt,"MOD")) {
            struct pt_mod_export_report report;assert(pt_mod_export_analyse(&d.project,&report)==PT_PROJECT_OK && !report.issues && report.classification==PT_CONVERSION_LOSSLESS && report.bytes==n);
            assert(pt_mod_export_direct(&d.project,again,size,&w2)==PT_PROJECT_OK && w2==n && !memcmp(data,again,n));
        }
        free(again);free(saved);pt_document_release(&re);
    }
    pt_document_release(&d);assert(!l.live);
    printf("{\"case\":\"%s\",\"api\":\"document_load\",\"valid\":%s,\"result\":%d,\"peakBytes\":%lu,\"liveBytes\":0}\n",id,valid?"true":"false",r,(unsigned long)l.peak);
}
static void sample_case(const char *dir,const char *id,const char *fmt,const uint8_t *data,size_t n,int valid,unsigned bits,unsigned ch,unsigned rate,unsigned frames,unsigned le,unsigned uns,const char *oracle)
{
    struct pt_wav_info wi,bwi;struct pt_svx_info si,bsi;struct reader reader={data,n,0,0};
    struct pt_raw_format raw={rate,(uint8_t)bits,(uint8_t)ch,(uint8_t)le,(uint8_t)uns};
    struct pt_pcm pcm;int32_t *values;size_t count=(size_t)frames*ch,i,reads=0;int r=-1;
    memset(&wi,0xa7,sizeof(wi));bwi=wi;memset(&si,0xa7,sizeof(si));bsi=si;
    if(!strcmp(fmt,"WAV")) {
        r=pt_wav_inspect(data,n,&wi);assert((r==PT_WAV_OK)==valid);
        if(valid) {struct pt_wav_info other;assert(pt_wav_inspect_reader(read_at,&reader,n,&other)==PT_WAV_OK && wi.frames==other.frames && wi.rate==other.rate && wi.data_offset==other.data_offset && wi.data_bytes==other.data_bytes && wi.channels==other.channels && wi.bits==other.bits);assert(wi.bits==bits && wi.channels==ch && wi.rate==rate && wi.frames==frames);}
        else assert(!memcmp(&wi,&bwi,sizeof(wi)));
    } else if(!strcmp(fmt,"SVX")) {
        r=pt_svx_inspect(data,n,&si);assert((r==PT_SVX_OK)==valid);
        if(valid) {struct pt_svx_info other;assert(pt_svx_inspect_reader(read_at,&reader,n,&other)==PT_SVX_OK && si.frames==other.frames && si.rate==other.rate && si.loop_start==other.loop_start && si.loop_end==other.loop_end && si.volume==other.volume && si.cycles==other.cycles && si.offset==other.offset && si.bytes==other.bytes && si.compression==other.compression && !memcmp(si.name,other.name,sizeof(si.name)));assert(si.rate==rate && si.frames==frames && si.loop_start==2 && si.loop_end==10);}
        else assert(!memcmp(&si,&bsi,sizeof(si)));
    } else {uint32_t f=999;assert(pt_raw_frames(n,&raw,&f)==PT_RAW_OK && valid && f==frames);r=0;}
    if(!valid) {printf("{\"case\":\"%s\",\"api\":\"inspect\",\"valid\":false,\"result\":%d}\n",id,r);return;}
    reads=reader.calls;
    for(i=1;i<=reads+1;++i) {
        reader.calls=0;reader.fail=i;
        if(!strcmp(fmt,"WAV")) {struct pt_wav_info o;memset(&o,0xa7,sizeof(o));r=pt_wav_inspect_reader(read_at,&reader,n,&o);assert(i>reads?r==PT_WAV_OK:r==PT_WAV_TRUNCATED);if(i<=reads)assert(!memcmp(&o,&bwi,sizeof(o)));}
        else if(!strcmp(fmt,"SVX")) {struct pt_svx_info o;memset(&o,0xa7,sizeof(o));r=pt_svx_inspect_reader(read_at,&reader,n,&o);assert(i>reads?r==PT_SVX_OK:r==PT_SVX_TRUNCATED);if(i<=reads)assert(!memcmp(&o,&bsi,sizeof(o)));}
    }
    values=malloc((count+1)*sizeof(*values));assert(values);for(i=0;i<=count;++i)values[i]=0x123456;
    pcm=(struct pt_pcm){values,count,frames,rate,(uint8_t)ch,(uint8_t)bits};
    if(!strcmp(fmt,"WAV"))r=pt_wav_decode(data,n,&pcm);
    else if(!strcmp(fmt,"SVX"))r=pt_svx_decode(data,n,&pcm);
    else r=pt_raw_decode(data,n,&raw,&pcm);
    assert(r==0 && values[count]==0x123456);expected_pcm(dir,oracle,&pcm);
    if(!strcmp(fmt,"SVX")) {
        reader.calls=reader.fail=0;assert(pt_svx_decode_reader(read_at,&reader,n,&pcm)==PT_SVX_OK);expected_pcm(dir,oracle,&pcm);reads=reader.calls;
        for(i=1;i<=reads+1;++i) {reader.calls=0;reader.fail=i;r=pt_svx_decode_reader(read_at,&reader,n,&pcm);assert(i>reads?r==PT_SVX_OK:r==PT_SVX_TRUNCATED);assert(values[count]==0x123456);}
    }
    /* Re-encode verifies supported semantics rather than preserving unknown optional chunks. */
    {size_t size,w;uint8_t *out;
        if(!strcmp(fmt,"WAV"))assert(pt_wav_size(&pcm,&size)==PT_WAV_OK);
        else if(!strcmp(fmt,"SVX"))assert(pt_svx_size(&pcm,&si,&size)==PT_SVX_OK);
        else assert(pt_raw_size(&pcm,&raw,&size)==PT_RAW_OK);
        out=malloc(size+1);assert(out);out[size]=0x55;
        if(!strcmp(fmt,"WAV"))assert(pt_wav_encode(&pcm,out,size,&w)==PT_WAV_OK && w==size && pt_wav_decode(out,size,&pcm)==PT_WAV_OK);
        else if(!strcmp(fmt,"SVX"))assert(pt_svx_encode(&pcm,&si,out,size,&w)==PT_SVX_OK && w==size && pt_svx_decode(out,size,&pcm)==PT_SVX_OK);
        else assert(pt_raw_encode(&pcm,&raw,out,size,&w)==PT_RAW_OK && w==n && !memcmp(data,out,n));
        assert(out[size]==0x55 && values[count]==0x123456);expected_pcm(dir,oracle,&pcm);free(out);
    }
    if(!strcmp(fmt,"RAW")) {
        struct pt_raw_format bad=raw;uint32_t f=999;
        bad.rate=0;assert(pt_raw_frames(n,&bad,&f)==PT_RAW_INVALID && f==999);bad=raw;bad.bits=32;assert(pt_raw_frames(n,&bad,&f)==PT_RAW_INVALID && f==999);
        bad=raw;bad.channels=3;assert(pt_raw_frames(n,&bad,&f)==PT_RAW_INVALID && f==999);bad=raw;bad.little_endian=2;assert(pt_raw_frames(n,&bad,&f)==PT_RAW_INVALID && f==999);
        if(bits*ch>8)assert(pt_raw_frames(n-1,&raw,&f)==PT_RAW_INVALID && f==999);
    }
    free(values);printf("{\"case\":\"%s\",\"api\":\"sample_decode_roundtrip\",\"valid\":true,\"readerCount\":%lu,\"faultIndices\":%lu,\"sentinel\":true}\n",id,(unsigned long)reads,(unsigned long)reads);
}
int main(int argc,char **argv)
{
    FILE *f;char path[2048],line[2048],id[100],fmt[10],name[150],oracle[150];size_t total=0,cases=0;
    assert(argc==2);assert(snprintf(path,sizeof(path),"%s/cases.tsv",argv[1])>0);f=fopen(path,"r");assert(f);
    while(fgets(line,sizeof(line),f)) {
        unsigned valid,bits,ch,rate,frames,le,uns;uint8_t *data,*snapshot;size_t n;
        assert(sscanf(line,"%99s %9s %u %u %u %u %u %u %u %149s %149s",id,fmt,&valid,&bits,&ch,&rate,&frames,&le,&uns,name,oracle)==11);
        data=read_file(argv[1],name,&n);snapshot=malloc(n+1);assert(snapshot);memcpy(snapshot,data,n);total+=n;assert(++cases<=512 && total<=67108864);
        fprintf(stderr,"CASE %s\n",id);
        if(!strcmp(fmt,"PTG") || !strcmp(fmt,"MOD") || !strcmp(fmt,"PP20"))project_case(id,fmt,data,n,(int)valid);
        else sample_case(argv[1],id,fmt,data,n,(int)valid,bits,ch,rate,frames,le,uns,oracle);
        assert(!memcmp(data,snapshot,n));free(snapshot);free(data);
    }
    assert(!fclose(f));printf("CORPUS_CAMPAIGN PASS cases=%lu bytes=%lu\n",(unsigned long)cases,(unsigned long)total);return 0;
}
