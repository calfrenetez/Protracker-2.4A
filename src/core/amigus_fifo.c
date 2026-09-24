#include "amigus_fifo.h"
#include <string.h>
int pt_amigus_fifo_init(struct pt_amigus_fifo *f,const struct pt_amigus_fifo_port *p)
{
    if(!f || f->ready || !p || !p->capacity || !p->write3 || !p->reset)return 0;
    f->port=*p;f->ready=1;f->failed=1;
    return pt_amigus_fifo_cancel(f)==1;
}
int pt_amigus_fifo_submit(void *v,const struct pt_pcm *p)
{
    struct pt_amigus_fifo *f=v;
    if(!f || !f->ready || f->failed)return -1;
    if(f->offset<f->count)return 0;
    if(!pt_amigus_pcm_pack_block(&f->pack,p,f->words,384,&f->count))return -1;
    f->offset=0;return 1;
}
int pt_amigus_fifo_poll(void *v)
{
    struct pt_amigus_fifo *f=v;int space;
    if(!f || !f->ready || f->failed)return -1;
    if(f->offset==f->count)return 1;
    space=f->port.capacity(f->port.context);
    if(space<0) {f->failed=1;return -1;}
    if(space<3)return 0;
    if(f->port.write3(f->port.context,f->words+f->offset)!=1) {f->failed=1;return -1;}
    f->offset+=3;return f->offset==f->count?1:0;
}
int pt_amigus_fifo_cancel(void *v)
{
    struct pt_amigus_fifo *f=v;int r;
    if(!f || !f->ready)return -1;
    f->failed=1;r=f->port.reset(f->port.context);
    if(r!=1)return r==0?0:-1;
    memset(&f->pack,0,sizeof(f->pack));f->count=f->offset=0;f->failed=0;return 1;
}
int pt_amigus_fifo_finish(struct pt_amigus_fifo *f,unsigned *padding)
{
    if(!f || !f->ready || f->failed)return -1;
    if(f->offset<f->count)return 0;
    if(!pt_amigus_pcm_pack_finish(&f->pack,f->words,384,&f->count,padding))return -1;
    f->offset=0;return 1;
}
