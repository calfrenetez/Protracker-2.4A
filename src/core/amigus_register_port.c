#include "amigus_register_port.h"
#include <string.h>
static int owned(struct pt_amigus_register_port *p)
{
    if(!p || !p->io.owned || p->io.owned(p->io.context)!=1) {if(p){p->fault=1;p->aligned=0;}return 0;}
    return 1;
}
static int fault(struct pt_amigus_register_port *p) {p->fault=1;p->aligned=0;return -1;}
int pt_amigus_register_port_init(struct pt_amigus_register_port *p,const struct pt_amigus_register_io *io,unsigned cap)
{
    if(!p || !io || !io->owned || !io->read16 || !io->write16 || !io->write32 || cap<6 || cap>65535)return 0;
    memset(p,0,sizeof(*p));p->io=*io;p->capacity_words=cap;return 1;
}
int pt_amigus_register_capacity(void *v)
{
    struct pt_amigus_register_port *p=v;uint16_t used,flags;
    if(!owned(p) || p->fault || !p->aligned)return -1;
    if(p->io.read16(p->io.context,0x10,&used)!=1 || used>p->capacity_words ||
       p->io.read16(p->io.context,0x00,&flags)!=1)return fault(p);
    /* FULL is an independent refusal, even when usage arithmetic leaves space.
     * Native Mini observations reported FULL with4094 words, not4096. */
    if(flags&2)return 0;
    return (int)((p->capacity_words-used)/2);
}
int pt_amigus_register_write3(void *v,const uint32_t *words)
{
    struct pt_amigus_register_port *p=v;unsigned i;int free;
    if(!words || !owned(p) || p->fault || !p->aligned)return -1;
    free=pt_amigus_register_capacity(p);if(free<3)return fault(p);
    for(i=0;i<3;++i) {
        /* Recheck between longword stores: an incomplete stereo triplet must
         * retain a failed owner until reset rather than issue another store. */
        if(i && pt_amigus_register_capacity(p)<1)return fault(p);
        if(p->io.write32(p->io.context,0x0c,words[i])!=1)return fault(p);
    }
    return 1;
}
int pt_amigus_register_reset(void *v)
{
    struct pt_amigus_register_port *p=v;uint16_t rate,mask,used;
    if(!owned(p))return -1;
    p->aligned=0;p->fault=1;p->start_phase=0;
    if(p->io.write16(p->io.context,0x06,0)!=1 ||
       p->io.write16(p->io.context,0x00,7)!=1 ||
       p->io.write16(p->io.context,0x02,7)!=1 ||
       p->io.write16(p->io.context,0x08,0)!=1)return -1;
    if(p->io.read16(p->io.context,0x06,&rate)!=1 ||
       p->io.read16(p->io.context,0x02,&mask)!=1 ||
       p->io.read16(p->io.context,0x10,&used)!=1)return -1;
    if(used>p->capacity_words)return -1;
    if((rate&0x8000) || (mask&7) || used)return 0;
    p->fault=0;p->aligned=1;return 1;
}
int pt_amigus_register_drain(void *v)
{
    struct pt_amigus_register_port *p=v;uint16_t used;
    if(!owned(p) || p->fault || !p->aligned)return -1;
    if(p->io.read16(p->io.context,0x10,&used)!=1 || used>p->capacity_words)return fault(p);
    return used==0;
}

int pt_amigus_register_start(void *v)
{
    struct pt_amigus_register_port *p=v;uint16_t format,rate,mask,used;
    if(!owned(p) || p->fault || !p->aligned)return -1;
    if(p->start_phase==3)return 1;
    if(p->io.read16(p->io.context,0x06,&rate)!=1 ||
       p->io.read16(p->io.context,0x02,&mask)!=1)return fault(p);
    if(mask&7)return fault(p);
    if(p->start_phase<2) {
        if(rate&0x8000)return fault(p);
        if(p->io.read16(p->io.context,0x10,&used)!=1 ||
           used>p->capacity_words || used%6)return fault(p);
        if(!used)return 0;
        if(!p->start_phase) {
            if(p->io.write16(p->io.context,0x04,5)!=1)return fault(p);
            p->start_phase=1;
        }
        if(p->io.read16(p->io.context,0x04,&format)!=1)return fault(p);
        if(format!=5)return 0;
        /* No interpolation, unsigned, endian swap or channel swap flags. */
        if(p->io.write16(p->io.context,0x06,0x8007)!=1)return fault(p);
        p->start_phase=2;
        return 0;
    }
    if(p->io.read16(p->io.context,0x04,&format)!=1 || format!=5)return fault(p);
    if(!(rate&0x8000))return 0;
    if(rate!=0x8007)return fault(p);
    p->start_phase=3;return 1;
}
