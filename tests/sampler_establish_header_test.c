/* Only raw bytes are compared; no forged sampler topology is traversed. */
#include "../src/editor/sampler_establish_inventory_internal.h"
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#ifdef PT_ESTABLISH_HEADER_NATIVE
#define HEADER_SEEDS 2U
#else
#define HEADER_SEEDS 16U
#endif
static int reference(const struct pt_sampler *left,const struct pt_sampler *right)
{
    const unsigned char *a=(const unsigned char *)left,*b=(const unsigned char *)right;
    struct range {size_t offset,bytes;} fields[]={
#define FIELD(name) {offsetof(struct pt_sampler,name),sizeof(left->name)}
        FIELD(current),FIELD(bytes),FIELD(live_head),FIELD(live_count),FIELD(pending_head),FIELD(pending_count)
#undef FIELD
    };
    size_t i,k;unsigned allowed;
    for(i=0;i<sizeof(*left);++i){
        allowed=0;
        for(k=0;k<sizeof(fields)/sizeof(fields[0]);++k)
            if(i>=fields[k].offset && i-fields[k].offset<fields[k].bytes)allowed=1;
        if(!allowed&&a[i]!=b[i])return 0;
    }
    return 1;
}
int main(int argc,char **argv)
{
    struct pt_sampler original,changed,saved;
    unsigned seed,bit;size_t i,j,first_protected=sizeof(original);unsigned checks=0;
    FILE *stream;char initial[32];size_t bytes;int failed;
    if(argc!=2||!argv[1]||!argv[1][0]||strlen(argv[1])>255)return 20;
    stream=fopen(argv[1],"rb");if(!stream)return 20;
    bytes=fread(initial,1,sizeof(initial),stream);failed=ferror(stream);
    if(fclose(stream))failed=1;
    if(failed||bytes!=12||memcmp(initial,"NOT STARTED\n",12))return 20;
    (void)pt_sampler_establish_inventory_current;
    for(seed=0;seed<HEADER_SEEDS;++seed){
        for(i=0;i<sizeof(original);++i)((unsigned char *)&original)[i]=(unsigned char)(seed*37U+i*11U);
        memcpy(&saved,&original,sizeof(saved));
        assert(pt_establish_inventory_header_equal(&original,&original));
        for(i=0;i<sizeof(original);++i)for(bit=0;bit<8;++bit){
            memcpy(&changed,&original,sizeof(changed));((unsigned char *)&changed)[i]^=(unsigned char)(1U<<bit);
            assert(pt_establish_inventory_header_equal(&changed,&original)==reference(&changed,&original));
            assert(pt_establish_inventory_header_equal(&original,&changed)==reference(&original,&changed));
            if(!reference(&changed,&original)&&first_protected==sizeof(original))first_protected=i;
            assert(!memcmp(&original,&saved,sizeof(original)));++checks;
        }
        assert(first_protected<sizeof(original));
        /* All permitted bytes may differ simultaneously; a protected byte
         * must still refuse in either direction, with both inputs untouched. */
        memcpy(&changed,&original,sizeof(changed));
        for(i=0;i<sizeof(original);++i){struct pt_sampler probe;memcpy(&probe,&original,sizeof(probe));((unsigned char *)&probe)[i]^=1;if(reference(&probe,&original))((unsigned char *)&changed)[i]^=0xff;}
        assert(reference(&changed,&original)&&pt_establish_inventory_header_equal(&changed,&original));
        ((unsigned char *)&changed)[first_protected]^=1;
        assert(!reference(&changed,&original)&&!pt_establish_inventory_header_equal(&changed,&original)&&!pt_establish_inventory_header_equal(&original,&changed));
        for(j=0;j<sizeof(original);++j)assert(((unsigned char *)&original)[j]==((unsigned char *)&saved)[j]);
    }
    assert(checks==HEADER_SEEDS*8U*sizeof(original));
    assert(!pt_establish_inventory_field_byte(0,SIZE_MAX,1));
    assert(!pt_establish_inventory_field_byte(SIZE_MAX,0,SIZE_MAX));
    assert(pt_establish_inventory_field_byte(SIZE_MAX,SIZE_MAX,1));
    stream=fopen(argv[1],"wb");if(!stream)return 20;
    failed=fputs("ESTABLISH HEADER DONE\n",stream)<0;if(fclose(stream))failed=1;
    if(failed)return 20;
#ifdef PT_ESTABLISH_HEADER_NATIVE
    puts("ESTABLISH HEADER NATIVE BYTE EQUIVALENCE PASS");
#else
    puts("ESTABLISH HEADER BYTE EQUIVALENCE PASS");
#endif
    return 0;
}
