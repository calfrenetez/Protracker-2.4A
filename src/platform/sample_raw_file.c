#include "sample_file.h"
struct raw_source {struct pt_pcm pcm;struct pt_raw_format format;};
static int generate_raw(void *context,size_t offset,void *output,size_t n)
{
    const struct raw_source *s=context;uint8_t *out=output;size_t i;
    unsigned width=s->format.bits/8;
    for(i=0;i<n;++i,++offset) {
        unsigned byte=(unsigned)(offset%width);
        uint32_t value=(uint32_t)(s->pcm.data[offset/width]+(s->format.unsigned8?128:0));
        out[i]=(uint8_t)(value>>(8*(s->format.little_endian?byte:width-1-byte)));
    }
    return 1;
}
enum pt_save_result pt_sample_raw_save(const char *path,const struct pt_pcm *pcm,
    const struct pt_raw_format *format,const struct pt_allocator *a)
{
    struct raw_source s;size_t total;
    if(pt_raw_size(pcm,format,&total)!=PT_RAW_OK)return PT_SAVE_INVALID;
    s.pcm=*pcm;s.format=*format;
    return pt_file_save_generated(path,total,generate_raw,&s,a);
}
