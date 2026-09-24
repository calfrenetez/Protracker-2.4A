#include <string.h>
#include "sample_file.h"
#include "wav.h"
struct wav_source {struct pt_pcm pcm;size_t payload;uint8_t header[44];};
static void le16(uint8_t *p,unsigned v) {p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static void le32(uint8_t *p,uint32_t v) {le16(p,v&65535);le16(p+2,v>>16);}
static int generate(void *context,size_t offset,void *output,size_t n)
{
    const struct wav_source *s=context;uint8_t *out=output;size_t i;
    unsigned width=s->pcm.bits/8;
    for(i=0;i<n;++i,++offset) {
        size_t at;uint32_t value;
        if(offset<44) {out[i]=s->header[offset];continue;}
        at=offset-44;if(at>=s->payload) {out[i]=0;continue;}
        value=width==1?(uint32_t)(s->pcm.data[at]+128):(uint32_t)s->pcm.data[at/width];
        out[i]=(uint8_t)(value>>((at%width)*8));
    }
    return 1;
}
enum pt_save_result pt_sample_wav_save(const char *path,const struct pt_pcm *pcm,const struct pt_allocator *a)
{
    struct wav_source s;size_t total;unsigned width;
    if(pt_wav_size(pcm,&total)!=PT_WAV_OK)return PT_SAVE_INVALID;
    memset(&s,0,sizeof(s));s.pcm=*pcm;width=pcm->bits/8;
    s.payload=(size_t)pcm->frames*pcm->channels*width;
    memcpy(s.header,"RIFF",4);le32(s.header+4,(uint32_t)total-8);
    memcpy(s.header+8,"WAVEfmt ",8);le32(s.header+16,16);le16(s.header+20,1);
    le16(s.header+22,pcm->channels);le32(s.header+24,pcm->rate);
    le32(s.header+28,pcm->rate*pcm->channels*width);
    le16(s.header+32,pcm->channels*width);le16(s.header+34,pcm->bits);
    memcpy(s.header+36,"data",4);le32(s.header+40,(uint32_t)s.payload);
    return pt_file_save_generated(path,total,generate,&s,a);
}
