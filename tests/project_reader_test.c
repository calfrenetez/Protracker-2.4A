#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "project.h"
struct input {uint8_t *data;size_t length,calls,fail;};
static int read_at(void *context,size_t offset,uint8_t *out,size_t count)
{
    struct input *in=context;assert(count<=1092 && offset<=in->length && count<=in->length-offset);
    if(++in->calls==in->fail)return 0;
    memcpy(out,in->data+offset,count);return 1;
}
static void repair_crc(struct input *in)
{
    uint32_t crc=0xffffffffUL;size_t i;unsigned j;
    memset(in->data+20,0,4);
    for(i=0;i<in->length;++i) {crc^=in->data[i];for(j=0;j<8;++j)crc=(crc>>1)^((crc&1)?0xedb88320UL:0);}
    crc^=0xffffffffUL;for(i=0;i<4;++i)in->data[20+i]=(uint8_t)(crc>>(24-i*8));
}
static void compare(struct input *in,size_t length)
{
    struct pt_project_requirements expected={0},actual={0},unchanged;
    enum pt_project_result a=pt_project_probe(in->data,length,&expected),b;
    memset(&actual,0xa5,sizeof(actual));unchanged=actual;in->calls=0;in->fail=0;
    b=pt_project_probe_reader(read_at,in,length,&actual);assert(a==b);
    if(a==PT_PROJECT_OK)assert(!memcmp(&expected,&actual,sizeof(actual)));
    else assert(!memcmp(&unchanged,&actual,sizeof(actual)));
}
int main(int argc,char **argv)
{
    FILE *f;long length;struct input in={0};uint8_t *original;size_t i,reads;
    struct pt_project_requirements need,unchanged;
    assert(argc==2);f=fopen(argv[1],"rb");assert(f && !fseek(f,0,SEEK_END));length=ftell(f);assert(length>32);rewind(f);
    in.length=(size_t)length;in.data=malloc(in.length);original=malloc(in.length);assert(in.data && original);
    assert(fread(in.data,1,in.length,f)==in.length && !fclose(f));memcpy(original,in.data,in.length);
    {
        struct pt_project_storage st={0};struct pt_project project,old;
        uint8_t *encoded=malloc(in.length);size_t written,calls;
        assert(encoded && pt_project_probe(in.data,in.length,&need)==PT_PROJECT_OK);
        st.order_capacity=need.orders;st.orders=calloc(need.orders,sizeof(*st.orders));
        st.event_capacity=need.events;st.events=calloc(need.events,sizeof(*st.events));
        st.sample_capacity=need.samples;st.samples=calloc(need.samples,sizeof(*st.samples));
        st.pcm_capacity=need.pcm_values;st.pcm=calloc(need.pcm_values,sizeof(*st.pcm));
        st.slice_capacity=need.slices;st.slices=calloc(need.slices,sizeof(*st.slices));
        st.extension_capacity=need.extensions;st.extensions=calloc(need.extensions,sizeof(*st.extensions));
        st.extension_bytes=need.extension_bytes;st.extension_data=calloc(need.extension_bytes,1);
        in.calls=0;assert(pt_project_decode_reader(read_at,&in,in.length,&st,&project)==PT_PROJECT_OK);calls=in.calls;
        assert(pt_project_encode(&project,encoded,in.length,&written)==PT_PROJECT_OK && written==in.length && !memcmp(encoded,in.data,in.length));
        memset(&project,0xa5,sizeof(project));old=project;
        for(i=1;i<=calls;++i) {in.calls=0;in.fail=i;assert(pt_project_decode_reader(read_at,&in,in.length,&st,&project)!=PT_PROJECT_OK);assert(!memcmp(&project,&old,sizeof(project)));}
        in.fail=0;free(encoded);free(st.orders);free(st.events);free(st.samples);free(st.pcm);free(st.slices);free(st.extensions);free(st.extension_data);
    }
    compare(&in,in.length);reads=in.calls;
    memset(&need,0xa5,sizeof(need));unchanged=need;
    for(i=1;i<=reads;++i) {in.calls=0;in.fail=i;assert(pt_project_probe_reader(read_at,&in,in.length,&need)!=PT_PROJECT_OK);assert(!memcmp(&need,&unchanged,sizeof(need)));}
    for(i=0;i<in.length;i+=97)compare(&in,i);
    for(i=0;i<in.length;i+=31) {
        memcpy(in.data,original,in.length);in.data[i]^=0x80;compare(&in,in.length);
        repair_crc(&in);compare(&in,in.length);
    }
    /* Exercise every byte of the file/chunk headers with a valid CRC. */
    for(i=0;i<128;++i) {memcpy(in.data,original,in.length);in.data[i]^=0x01;repair_crc(&in);compare(&in,in.length);}
    free(original);free(in.data);
    puts("PROJECT READER PASS: exact master re-encode, decoder failure staging, bounded reads, original parser parity, truncation, CRC and repaired mutations, every read failure leaves requirements unchanged");return 0;
}
