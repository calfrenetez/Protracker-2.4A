#include <string.h>
#include "sample_file.h"
struct svx_source {struct pt_pcm pcm;uint8_t header[88];};
static void be16(uint8_t *p,unsigned v) {p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v;}
static void be32(uint8_t *p,uint32_t v) {be16(p,v>>16);be16(p+2,v&65535);}
static int generate_svx(void *context,size_t offset,void *output,size_t n)
{
    const struct svx_source *s=context;uint8_t *out=output;size_t i;
    for(i=0;i<n;++i,++offset)
        out[i]=offset<88?s->header[offset]:offset-88<s->pcm.frames?(uint8_t)s->pcm.data[offset-88]:0;
    return 1;
}
enum pt_save_result pt_sample_svx_save(const char *path,const struct pt_pcm *pcm,
    const struct pt_svx_info *info,const struct pt_allocator *a)
{
    struct svx_source s;size_t total;
    if(pt_svx_size(pcm,info,&total)!=PT_SVX_OK)return PT_SAVE_INVALID;
    memset(&s,0,sizeof(s));s.pcm=*pcm;
    memcpy(s.header,"FORM",4);be32(s.header+4,(uint32_t)total-8);
    memcpy(s.header+8,"8SVXVHDR",8);be32(s.header+16,20);
    be32(s.header+20,info->loop_end?info->loop_start:pcm->frames);
    be32(s.header+24,info->loop_end?info->loop_end-info->loop_start:0);
    be32(s.header+28,info->cycles);be16(s.header+32,pcm->rate);s.header[34]=1;
    be32(s.header+36,info->volume);memcpy(s.header+40,"NAME",4);
    be32(s.header+44,32);memcpy(s.header+48,info->name,32);
    memcpy(s.header+80,"BODY",4);be32(s.header+84,pcm->frames);
    return pt_file_save_generated(path,total,generate_svx,&s,a);
}
