#define main output_mock_fixture
#include "paula_output_test.c"
#undef main
static unsigned chip_owned;
ULONG AvailMem(ULONG flags){assert(flags==MEMF_CHIP);return 512UL*1024+32;}
void *AllocMem(ULONG bytes,ULONG flags){assert(bytes==32 && flags==(MEMF_CHIP|MEMF_PUBLIC));if(chip_owned)return NULL;chip_owned=1;return chip.bytes;}
void FreeMem(void *p,ULONG bytes){assert(p==chip.bytes && bytes==32 && chip_owned && !(dma&15));chip_owned=0;}
static void *fast_alloc(void *c,size_t n){(void)c;return malloc(n);}
static void fast_free(void *c,void *p){(void)c;free(p);}
#include "../src/native/editor_paula.h"
static void prepared_fixture(unsigned bits,unsigned mapped)
{
    struct pt_allocator a={NULL,fast_alloc,fast_free};struct pt_document doc;
    struct pt_editor *ed=malloc(sizeof(*ed));struct pt_native_editor_paula n={0};
    struct pt_render_options options={0};struct pt_render_interval span;
    struct pt_paula_preflight_report report={0};
    enum pt_paula_song_result r;unsigned i,polls;int32_t pcm[32],saved[32];
    reset();assert(ed);for(i=0;i<32;++i)pcm[i]=(int32_t)i+1;memcpy(saved,pcm,sizeof(saved));
    pt_document_init(&doc,&a);assert(pt_document_new(&doc,mapped?16:4,SIZE_MAX)==PT_PROJECT_OK);
    if(mapped) {
        if(bits!=8 || mapped==2)memset(pcm,0,sizeof(pcm));
        memcpy(saved,pcm,sizeof(saved));
        for(i=0;i<16;++i)doc.project.channels.track[i].route=PT_AMIGUS;
        doc.project.channels.track[4].route=doc.project.channels.track[7].route=
            doc.project.channels.track[10].route=doc.project.channels.track[15].route=PT_PAULA;
        doc.project.channels.track[4].pan=0;
        doc.project.samples[1].pcm=(struct pt_pcm){pcm,32,32,8000,1,(uint8_t)bits};
    }
    doc.project.samples[0].pcm=(struct pt_pcm){pcm,32,32,8000,1,(uint8_t)bits};
    doc.project.samples[0].volume=64;doc.project.events[mapped?4:0]=(struct pt_event){428,0,PT_NOTE_PERIOD,1,0,0,0,0};
    doc.project.events[mapped?16*3+15:4*3].effect=15;doc.project.events[mapped?16*3+15:4*3].parameter=0;
    assert(pt_editor_init(ed,&doc.project));ed->sampler.allocator=a;
    options.rate=48000;options.bits=24;options.tracks=mapped?1U<<4:1;options.gain_q16=65536;
    options.tick_limit=100;options.frame_limit=100000;
    assert(pt_native_editor_paula_attach(&n,ed));
    assert(pt_native_editor_paula_begin(&n,&options,32)==PT_PAULA_SONG_PREPARING);
    assert(!n.binding.song && n.active && pt_editor_prepare_change(ed));
    assert(!n.active && !n.engine.output.reservation.port); /* Cancel before song exists. */
    assert(pt_native_editor_paula_begin(&n,&options,32)==PT_PAULA_SONG_PREPARING);
    polls=0;do{r=pt_native_editor_paula_advance(&n,&report);assert(++polls<4096);}while(r==PT_PAULA_SONG_PREPARING);
    if(mapped==2) {
        /* Zero-leading classic8 is a segment/repeat, not the supported one-shot. */
        assert(bits==8 && r==PT_PAULA_SONG_CAPABILITY && report.result==PT_PAULA_OPERATION);
        assert(report.channel==4 && report.kind==PT_RENDER_SEGMENT && n.failed);
        assert(!n.engine.output.held[0] && !chip_owned && !ed->sampler.current[0]);
        for(polls=0;polls<8 && !pt_editor_prepare_change(ed);++polls){}
        assert(polls<8 && !n.active);
        goto done;
    }
    assert(r==PT_PAULA_SONG_OK && n.binding.song && !n.engine.output.held[0]);
    for(i=0;i<20 && !n.engine.output.held[0];++i) {
    assert(pt_editor_paula_next(&n.binding,&span)==PT_PAULA_SONG_OK);
    while(span.frames){unsigned chunk=span.frames>256?256:span.frames;assert(pt_editor_paula_consume(&n.binding,chunk)==PT_PAULA_SONG_OK);span.frames-=chunk;}
    polls=0;do{r=pt_editor_paula_stage(&n.binding);assert(++polls<100);}while(r==PT_PAULA_SONG_PREPARING);
    assert(r==PT_PAULA_SONG_OK && pt_editor_paula_complete(&n.binding)==PT_PAULA_SONG_OK);
    }
    assert(n.engine.output.held[0] && n.engine.cache.cache.bytes==32);
    hold_dma=1;assert(!pt_editor_prepare_change(ed) && n.binding.song && n.active);
    assert(!pt_editor_dispose(ed) && n.engine.cache.cache.bytes==32);
    hold_dma=0;dma&=~15U;delay_free=1;
    assert(!pt_editor_prepare_change(ed) && !n.binding.song && n.binding.release_pending && n.active);
    assert(!n.engine.cache.sampler && n.engine.output.reservation.port && !chip_owned);
    assert(!pt_editor_dispose(ed) && !pt_editor_paula_detach(&n.binding));
    assert(pt_native_editor_paula_begin(&n,&options,32)==PT_PAULA_SONG_INVALID);
    for(i=0;i<16;++i)if(requests[i].q && (requests[i].q->io_Command==ADCMD_FREE || requests[i].q->io_Command==ADCMD_LOCK))requests[i].pending=0;
    delay_free=0;assert(pt_editor_prepare_change(ed) && !n.active && !n.binding.release_pending && !live);
    assert(doc.project.samples[0].pcm.bits==bits && !memcmp(doc.project.samples[0].pcm.data,saved,sizeof(saved)));
    /* Refused capability never starts a reader and must still use the barrier. */
    options.rate=123;assert(pt_native_editor_paula_begin(&n,&options,32)==PT_PAULA_SONG_PREPARING);
    polls=0;do{r=pt_native_editor_paula_advance(&n,NULL);assert(++polls<4096);}while(r==PT_PAULA_SONG_PREPARING);
    assert(r!=PT_PAULA_SONG_OK && n.failed && !n.engine.output.held[0]);
    assert(pt_native_editor_paula_advance(&n,NULL)==PT_PAULA_SONG_INVALID);
    for(polls=0;polls<8 && !pt_editor_prepare_change(ed);++polls){}
    assert(polls<8 && !n.active);
 done:
    assert(pt_editor_dispose(ed) && pt_editor_paula_detach(&n.binding));
    pt_document_release(&doc);free(ed);
}
int main(void){unsigned mapped;for(mapped=0;mapped<2;++mapped){prepared_fixture(8,mapped);prepared_fixture(16,mapped);prepared_fixture(24,mapped);}prepared_fixture(8,2);puts("NATIVE EDITOR PAULA HOST PASS: prepared device output; early cancellation, retained DMA and final FREE barrier, classic8 segment refusal");return 0;}
