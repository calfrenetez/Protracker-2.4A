#include "native_exec_memory.h"
#include "../src/native/editor_paula.h"
#include <proto/dos.h>
#include <string.h>
/* Actual prepared16/24 dispatch, zero PCM with nonzero logical volume; classic8
 * zero-leading segment refusal is checked without output. Numerical
 * progression only: no native timer/deadline/listening/frontend claim. */
static void *fast_alloc(void *c,size_t n){(void)c;return native_allocate(n);}
static void fast_free(void *c,void *p){(void)c;native_release(p);}
static int stop_bounded(struct pt_native_editor_paula *n)
{unsigned i;for(i=0;i<50;++i){if(pt_editor_paula_stop(&n->binding))return 1;Delay(1);}return 0;}
static int fixture(unsigned bits)
{
    struct pt_allocator a={NULL,fast_alloc,fast_free};struct pt_document doc;
    struct pt_editor *ed=native_allocate(sizeof(*ed));struct pt_native_editor_paula n={0};
    int32_t *pcm=native_allocate(32*sizeof(*pcm));struct pt_render_options options={0};
    struct pt_render_interval span;struct pt_paula_preflight_report report={0};enum pt_paula_song_result r;
    unsigned i,j,polls;int result=0,initialized=0,attached=0;
#define CHECK(c) do{if(!(c)){printf("PREPARED PAULA FAIL bits=%u line=%u; cleanup required\n",bits,(unsigned)__LINE__);result=20;goto done;}}while(0)
    memset(pcm,0,32*sizeof(*pcm));pt_document_init(&doc,&a);
    CHECK(pt_document_new(&doc,16,SIZE_MAX)==PT_PROJECT_OK);
    for(i=0;i<16;++i)doc.project.channels.track[i].route=PT_AMIGUS;
    doc.project.channels.track[4].route=doc.project.channels.track[7].route=
        doc.project.channels.track[10].route=doc.project.channels.track[15].route=PT_PAULA;
    doc.project.channels.track[4].pan=0;
    doc.project.samples[0].pcm=doc.project.samples[1].pcm=(struct pt_pcm){pcm,32,32,8000,1,(uint8_t)bits};
    doc.project.samples[0].volume=64;
    doc.project.events[4]=(struct pt_event){428,0,PT_NOTE_PERIOD,1,0,0,0,0};
    doc.project.events[16*3+15].effect=15;doc.project.events[16*3+15].parameter=0;
    CHECK(pt_editor_init(ed,&doc.project));initialized=1;ed->sampler.allocator=a;
    options.rate=48000;options.bits=24;options.tracks=1U<<4;options.gain_q16=65536;
    options.tick_limit=100;options.frame_limit=100000;
    CHECK(pt_native_editor_paula_attach(&n,ed));attached=1;
    CHECK(pt_native_editor_paula_begin(&n,&options,32)==PT_PAULA_SONG_PREPARING);
    CHECK(!n.binding.song && n.active && pt_editor_prepare_change(ed));
    CHECK(!n.active && !n.engine.output.reservation.port);
    CHECK(pt_native_editor_paula_begin(&n,&options,32)==PT_PAULA_SONG_PREPARING);
    polls=0;do{r=pt_native_editor_paula_advance(&n,&report);CHECK(++polls<4096);if(r==PT_PAULA_SONG_PREPARING && !n.engine.ready)Delay(1);}while(r==PT_PAULA_SONG_PREPARING);
    printf("PREPARE bits=%u result=%u capability=%u render=%u action=%u channel=%u kind=%u begun=%u ready=%u\n",bits,(unsigned)r,(unsigned)report.result,(unsigned)report.render_result,report.action,report.channel,(unsigned)report.kind,n.begun,n.engine.ready);
    if(bits==8) {
        CHECK(r==PT_PAULA_SONG_CAPABILITY && report.result==PT_PAULA_OPERATION);
        CHECK(report.channel==4 && report.kind==PT_RENDER_SEGMENT && n.failed);
        CHECK(!n.engine.output.held[0] && !n.engine.cache.cache.bytes && !ed->sampler.current[0]);
        goto done;
    }
    CHECK(r==PT_PAULA_SONG_OK && n.binding.song && !n.engine.output.held[0]);
    CHECK(ed->sampler.current[0] && !ed->sampler.current[1] && !n.engine.cache.cache.bytes);
    CHECK((TypeOfMem(doc.project.samples[0].pcm.data)&(MEMF_FAST|MEMF_CHIP))==MEMF_FAST);
    for(i=0;i<20 && !n.engine.output.held[0];++i) {
        CHECK(pt_editor_paula_next(&n.binding,&span)==PT_PAULA_SONG_OK);
        while(span.frames){unsigned chunk=span.frames>256?256:span.frames;CHECK(pt_editor_paula_consume(&n.binding,chunk)==PT_PAULA_SONG_OK);span.frames-=chunk;}
        polls=0;do{r=pt_editor_paula_stage(&n.binding);CHECK(++polls<100);}while(r==PT_PAULA_SONG_PREPARING);
        CHECK(r==PT_PAULA_SONG_OK);
        /* Inspect all published/staged bytes BEFORE any actual reader starts. */
        for(j=0;j<PT_CACHE_SLOTS;++j)if(n.engine.cache.cache.entry[j].data) {
            size_t k;struct pt_cache_entry *entry=&n.engine.cache.cache.entry[j];
            CHECK(entry->bytes==32 && (TypeOfMem(entry->data)&MEMF_CHIP));
            for(k=0;k<entry->bytes;++k)CHECK(!((const uint8_t *)entry->data)[k]);
        }
        CHECK(pt_editor_paula_complete(&n.binding)==PT_PAULA_SONG_OK);
    }
    CHECK(n.engine.output.held[0] && n.engine.cache.cache.bytes==32);
    CHECK((pt_native_paula_output_dma()&15)==1);
    CHECK(doc.project.samples[0].pcm.bits==bits && !ed->sampler.current[1]);
    for(i=0;i<32;++i)CHECK(!doc.project.samples[0].pcm.data[i]);
    Delay(1); /* Actual reader persists through task turn; no timing acceptance. */
 done:
    if(attached && !stop_bounded(&n)){puts("PREPARED PAULA HOLD: reader/cache/device unresolved; all storage retained");return 21;}
    if(attached && !pt_editor_paula_detach(&n.binding)){puts("PREPARED PAULA HOLD: detach unresolved; storage retained");return 22;}
    if(initialized && !pt_editor_dispose(ed)){puts("PREPARED PAULA HOLD: editor disposal unresolved; storage retained");return 23;}
    if(pt_native_paula_output_dma()&15){puts("PREPARED PAULA HOLD: DMA remains active; storage retained");return 24;}
    if(n.active || n.binding.song || n.engine.cache.sampler || n.engine.output.reservation.port){puts("PREPARED PAULA HOLD: final contexts retained");return 25;}
    pt_document_release(&doc);native_release(pcm);native_release(ed);
    if(result){native_memory_finish();return result;}
    if(bits==8)puts("PREPARED PAULA PASS: classic8 zero-leading segment refusal before master promotion/cache/output; early cancellation and editor/device closure");
    else printf("PREPARED PAULA PASS: %u-bit Fast master; selected zero Chip32; real prepared WRITE; early cancellation and editor stop/device closure; unused master unpromoted\n",bits);
#undef CHECK
    return 0;
}
int main(void)
{int r;native_memory_start();r=fixture(8);if(r)return r;r=fixture(16);if(r)return r;r=fixture(24);if(r)return r;native_memory_finish();return 0;}
