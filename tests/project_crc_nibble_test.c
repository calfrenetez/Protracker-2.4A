/* SOURCE proposal: Root alone compiles/runs. Include the selected production
 * translation unit to exercise its private primitive without a new public API.
 * The independent reference deliberately retains the original bitwise body. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/core/project.c"

static uint32_t crc_reference_byte(uint32_t state,uint8_t value)
{
    unsigned bit;state^=value;
    for(bit=0;bit<8;++bit)state=(state>>1)^((state&1)?0xedb88320UL:0);
    return state;
}
static uint32_t crc_reference_project(const uint8_t *data,size_t bytes)
{
    size_t i;uint32_t state=0xffffffffUL;
    for(i=0;i<bytes;++i)state=crc_reference_byte(state,(uint8_t)(i>=20&&i<24?0:data[i]));
    return state^0xffffffffUL;
}
static void crc_transition_cases(void)
{
    static const uint32_t states[]={0U,0xffffffffUL,0x12345678UL,0x80000001UL,
        0xaaaaaaaaUL,0x55555555UL,0x000000ffUL,0xff000000UL};
    unsigned nibble,bit,value;size_t i;uint32_t reference;
    assert(sizeof(project_crc32_nibble)==64);
    for(nibble=0;nibble<16;++nibble){reference=nibble;
        for(bit=0;bit<4;++bit)reference=(reference>>1)^((reference&1)?0xedb88320UL:0);
        assert(project_crc32_nibble[nibble]==reference);
    }
    for(i=0;i<sizeof(states)/sizeof(states[0]);++i)
        for(value=0;value<256;++value)
            assert(project_crc32_byte(states[i],(uint8_t)value)==crc_reference_byte(states[i],(uint8_t)value));
    for(bit=0;bit<32;++bit)
        for(value=0;value<256;++value){uint32_t state=(uint32_t)1U<<bit;
            assert(project_crc32_byte(state,(uint8_t)value)==crc_reference_byte(state,(uint8_t)value));
        }
    assert(checksum((const uint8_t *)"",0)==0U);
    assert(checksum((const uint8_t *)"123456789",9)==0xcbf43926UL);
}
struct crc_test_reader {
    const uint8_t *data;size_t bytes,calls,fail,maximum,next;unsigned sequential;
};
static int crc_test_read(void *context,size_t offset,uint8_t *output,size_t bytes)
{
    struct crc_test_reader *r=context;
    assert(bytes<=r->maximum&&offset<=r->bytes&&bytes<=r->bytes-offset);
    if(r->sequential){assert(offset==r->next);r->next+=bytes;}
    if(++r->calls==r->fail)return 0;
    memcpy(output,r->data+offset,bytes);return 1;
}
static void crc_split_case(const uint8_t *data,size_t bytes,size_t split,uint32_t expected)
{
    struct stream_writer writer;assert(split<=bytes);memset(&writer,0,sizeof(writer));
    writer.crc=0xffffffffUL;
    assert(stream_bytes(&writer,data,split)&&stream_bytes(&writer,data+split,bytes-split));
    assert(writer.pos==bytes&&!writer.used&&writer.crc==(expected^0xffffffffUL));
}
static void crc_buffer_cases(void)
{
    static uint8_t data[4097],before[4097];
    static const size_t lengths[]={0,1,2,8,9,19,20,21,22,23,24,25,31,32,255,256,1023,1024,1025,2048,4097};
    static const size_t boundaries[]={0,1,19,20,21,22,23,24,25,31,32,255,256,1023,1024,1025,2047,2048,4096,4097};
    size_t i,j,split,calls;uint32_t expected,observed;struct crc_test_reader reader;
    for(i=0;i<sizeof(data);++i)data[i]=(uint8_t)(i*37U+11U);
    memcpy(before,data,sizeof(data));
    for(i=0;i<sizeof(lengths)/sizeof(lengths[0]);++i){size_t bytes=lengths[i];
        expected=crc_reference_project(data,bytes);assert(checksum(data,bytes)==expected);
        if(bytes<=32)for(split=0;split<=bytes;++split)crc_split_case(data,bytes,split,expected);
        for(j=0;j<sizeof(boundaries)/sizeof(boundaries[0]);++j)
            if(boundaries[j]<=bytes)crc_split_case(data,bytes,boundaries[j],expected);
        crc_split_case(data,bytes,bytes,expected);
        reader=(struct crc_test_reader){data,bytes,0,0,1024,0,1};observed=0x5a5aa5a5UL;
        assert(reader_checksum(crc_test_read,&reader,bytes,&observed)&&observed==expected);
        calls=reader.calls;assert(calls==(bytes+1023)/1024&&reader.next==bytes);
        for(j=1;j<=calls;++j){reader=(struct crc_test_reader){data,bytes,0,j,1024,0,1};observed=0x5a5aa5a5UL;
            assert(!reader_checksum(crc_test_read,&reader,bytes,&observed)&&observed==0x5a5aa5a5UL&&reader.calls==j);
        }
    }
    assert(!memcmp(data,before,sizeof(data)));
    expected=checksum(data,sizeof(data));
    for(i=20;i<24;++i)data[i]^=0xff;
    assert(checksum(data,sizeof(data))==expected&&crc_reference_project(data,sizeof(data))==expected);
    memcpy(data,before,sizeof(data));data[19]^=1;
    assert(checksum(data,sizeof(data))!=expected&&checksum(data,sizeof(data))==crc_reference_project(data,sizeof(data)));
    memcpy(data,before,sizeof(data));data[24]^=1;
    assert(checksum(data,sizeof(data))!=expected&&checksum(data,sizeof(data))==crc_reference_project(data,sizeof(data)));
    memcpy(data,before,sizeof(data));assert(!memcmp(data,before,sizeof(data)));
}
struct crc_test_sink {uint8_t *data;size_t capacity,used;};
static int crc_test_collect(void *context,const uint8_t *data,size_t bytes)
{
    struct crc_test_sink *sink=context;
    assert(bytes<=1024&&sink->used<=sink->capacity&&bytes<=sink->capacity-sink->used);
    memcpy(sink->data+sink->used,data,bytes);sink->used+=bytes;return 1;
}
static void *crc_test_array(size_t count,size_t width)
{
    void *result;assert(width&&count<=SIZE_MAX/width);
    result=calloc(count?count:1,width);assert(result);return result;
}
static void crc_fixed_golden(const char *path)
{
    FILE *file=fopen(path,"rb");long length;uint8_t *data,*encoded_copy,*stream_copy;
    size_t bytes,written;struct pt_project_requirements need={0},positional={0};
    struct pt_project_storage storage={0};struct pt_project project;
    struct crc_test_reader reader;struct crc_test_sink sink;
    assert(file&&!fseek(file,0,SEEK_END));length=ftell(file);assert(length>=32&&length<=1048576);
    rewind(file);bytes=(size_t)length;data=malloc(bytes);encoded_copy=malloc(bytes);stream_copy=malloc(bytes);
    assert(data&&encoded_copy&&stream_copy&&fread(data,1,bytes,file)==bytes&&fgetc(file)==EOF&&!fclose(file));
    assert(checksum(data,bytes)==u32(data+20)&&crc_reference_project(data,bytes)==u32(data+20));
    assert(pt_project_probe(data,bytes,&need)==PT_PROJECT_OK);
    reader=(struct crc_test_reader){data,bytes,0,0,1092,0,0};
    assert(pt_project_probe_reader(crc_test_read,&reader,bytes,&positional)==PT_PROJECT_OK&&!memcmp(&need,&positional,sizeof(need)));
    storage.order_capacity=need.orders;storage.orders=crc_test_array(need.orders,sizeof(*storage.orders));
    storage.event_capacity=need.events;storage.events=crc_test_array(need.events,sizeof(*storage.events));
    storage.sample_capacity=need.samples;storage.samples=crc_test_array(need.samples,sizeof(*storage.samples));
    storage.pcm_capacity=need.pcm_values;storage.pcm=crc_test_array(need.pcm_values,sizeof(*storage.pcm));
    storage.slice_capacity=need.slices;storage.slices=crc_test_array(need.slices,sizeof(*storage.slices));
    storage.extension_capacity=need.extensions;storage.extensions=crc_test_array(need.extensions,sizeof(*storage.extensions));
    storage.extension_bytes=need.extension_bytes;storage.extension_data=crc_test_array(need.extension_bytes,1);
    reader.calls=0;
    assert(pt_project_decode_reader(crc_test_read,&reader,bytes,&storage,&project)==PT_PROJECT_OK);
    assert(pt_project_encode(&project,encoded_copy,bytes,&written)==PT_PROJECT_OK&&written==bytes&&!memcmp(data,encoded_copy,bytes));
    sink=(struct crc_test_sink){stream_copy,bytes,0};
    assert(pt_project_stream(&project,crc_test_collect,&sink,&written)==PT_PROJECT_OK&&written==bytes&&sink.used==bytes&&!memcmp(data,stream_copy,bytes));
    free(storage.orders);free(storage.events);free(storage.samples);free(storage.pcm);free(storage.slices);
    free(storage.extensions);free(storage.extension_data);free(data);free(encoded_copy);free(stream_copy);
}
int main(int argc,char **argv)
{
    assert(argc==3);crc_transition_cases();crc_buffer_cases();crc_fixed_golden(argv[1]);crc_fixed_golden(argv[2]);
    puts("PROJECT CRC NIBBLE PASS: independent bitwise transitions/all 256 bytes/state basis; split/hole/read failures; mixed/song fixed goldens, exact encode/stream/positional decode; no reduced validation");
    return 0;
}
