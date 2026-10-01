#ifndef PT_ENGINE_NATIVE
#define main output_mock_fixture
#include "paula_output_test.c"
#undef main
static unsigned chip_owned,chip_frees;
ULONG AvailMem(ULONG flags){assert(flags==MEMF_CHIP);return 512UL*1024+32;}
void *AllocMem(ULONG bytes,ULONG flags){assert(bytes==32 && flags==(MEMF_CHIP|MEMF_PUBLIC));if(chip_owned)return NULL;chip_owned=1;return chip.bytes;}
void FreeMem(void *p,ULONG bytes){assert(p==chip.bytes && bytes==32 && chip_owned && !(dma&15));chip_owned=0;++chip_frees;}
#endif
#include "../src/native/paula_engine.h"
#include "../src/core/document.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
static void *fast_alloc(void *c,size_t n){(void)c;return malloc(n);}
static void fast_free(void *c,void *p){(void)c;free(p);}
static int engine_close(struct pt_native_paula_engine *e)
{unsigned n;for(n=0;n<50;++n){if(pt_native_paula_engine_close(e))return 1;
#ifdef PT_ENGINE_NATIVE
Delay(1);
#endif
}return 0;}
static int engine_fixture(unsigned bits)
{
    struct pt_allocator a={NULL,fast_alloc,fast_free};struct pt_document doc;
    struct pt_sampler sampler;struct pt_native_paula_engine e={0};
    struct pt_paula_voice_request voice={0,32,400,0};int32_t *master=malloc(32*sizeof(*master));
    int32_t saved[32];unsigned i,n;int ready=0,result=0;size_t used;
    #ifndef PT_ENGINE_NATIVE
    reset();
    #endif
    assert(master);pt_document_init(&doc,&a);pt_sampler_init(&sampler,&a,1024*1024);
#ifdef PT_ENGINE_NATIVE
#define ENGINE_CHECK(c) do{if(!(c)){printf("PAULA ENGINE FAIL line=%u bits=%u; attempting confirmed cleanup\n",(unsigned)__LINE__,bits);result=20;goto done;}}while(0)
#else
#define ENGINE_CHECK(c) assert(c)
#endif
    for(i=0;i<32;++i)master[i]=i==0?-(1L<<(bits-1)):i==31?(1L<<(bits-1))-1:(int32_t)i;
    memcpy(saved,master,sizeof(saved));
    ENGINE_CHECK(pt_document_new(&doc,4,SIZE_MAX)==PT_PROJECT_OK);
    doc.project.samples[0].pcm=(struct pt_pcm){master,32,32,48000,1,(uint8_t)bits};
    ENGINE_CHECK(pt_native_paula_engine_begin(&e,&sampler,&doc.project,32));
    ENGINE_CHECK(!e.cache.sampler && !e.voices.bridge);
    ENGINE_CHECK(engine_close(&e)); /* Cancel before acquisition/binding. */
    ENGINE_CHECK(pt_native_paula_engine_begin(&e,&sampler,&doc.project,32));
    for(n=0;n<50;++n){ready=pt_native_paula_engine_advance(&e);if(ready)break;
#ifdef PT_ENGINE_NATIVE
Delay(1);
#endif
    }
    ENGINE_CHECK(ready==1 && e.ready && !e.cache.cache.bytes);
    ENGINE_CHECK(pt_native_paula_engine_advance(&e)==1);
    for(i=0;i<4;++i)ENGINE_CHECK(pt_paula_voices_trigger(&e.voices,i,0,0,&voice)==PT_PAULA_VOICE_ACTIVE);
    ENGINE_CHECK(e.cache.cache.bytes==32);used=sampler.bytes;
    for(i=0;i<4;++i)ENGINE_CHECK(pt_paula_voices_control(&e.voices,i,500,0)==PT_PAULA_VOICE_ACTIVE);
    ENGINE_CHECK(sampler.bytes==used && doc.project.samples[0].pcm.bits==bits);
    ENGINE_CHECK(!memcmp(doc.project.samples[0].pcm.data,saved,sizeof(saved)));
#ifndef PT_ENGINE_NATIVE
    /* An attached song must retain the whole engine; public close cannot bypass it. */
    e.voices.song_owner=&sampler;ENGINE_CHECK(!pt_native_paula_engine_close(&e));
    ENGINE_CHECK(e.output.ready && e.cache.cache.bytes==32 && e.closing);
    e.voices.song_owner=NULL;
    hold_dma=1;ENGINE_CHECK(!pt_native_paula_engine_close(&e));
    ENGINE_CHECK(!e.output.pending[0] && e.output.held[0] && e.cache.cache.bytes==32 && chip_owned);
    dma&=~15U;
#endif
#ifdef PT_ENGINE_NATIVE
 done:
#endif
    if(!engine_close(&e)){puts("PAULA ENGINE HOLD: reader/cache/device unresolved; storage retained");return 21;}
    if(!result && (e.cache.sampler || e.voices.bridge || e.output.reservation.port))result=23;
    if(!result && (doc.project.samples[0].pcm.bits!=bits || memcmp(doc.project.samples[0].pcm.data,saved,sizeof(saved))))result=22;
    pt_sampler_release(&sampler);
    if(doc.project.samples)doc.project.samples[0].pcm=(struct pt_pcm){0};
    free(master);pt_document_release(&doc);
    if(result)return result;
    printf("PAULA ENGINE PASS: %u-bit master, selected Chip32 shared by four silent device readers, confirmed stop before cache/device release\n",bits);
#undef ENGINE_CHECK
    return 0;
}
int main(void)
{
#ifdef PT_ENGINE_NATIVE
    native_memory_start();
#endif
    {int r;r=engine_fixture(8);if(r)return r;r=engine_fixture(16);if(r)return r;r=engine_fixture(24);if(r)return r;}
#ifdef PT_ENGINE_NATIVE
    assert(!(pt_native_paula_output_dma()&15));native_memory_finish();
#else
    assert(!live && !chip_owned && chip_frees==3);
#endif
    return 0;
}
