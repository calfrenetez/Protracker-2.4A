#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <exec/interrupts.h>
#include <proto/exec.h>
#include <proto/dos.h>
#define CIA_BASE_NAME resource
#include <proto/cia.h>
#include "document.h"
#include "mod_project.h"
#include "../src/native/paula.h"
static void *allocate(void *ctx,size_t n) {(void)ctx;return malloc(n);}
static void release(void *ctx,void *p) {(void)ctx;free(p);}
static ULONG dummy_interrupt(void) {return 0;}
#define CHECK(c) do {if(!(c)) {printf("FAIL line %u: %s\n",__LINE__,#c);goto done;}}while(0)
static int claim(struct Library *resource,unsigned bit,struct Interrupt *server)
{return AddICRVector(resource,bit,server)==NULL;}
static void unclaim(struct Library *resource,unsigned bit,struct Interrupt *server)
{RemICRVector(resource,bit,server);}
int main(int argc,char **argv)
{
    struct pt_allocator allocator={NULL,allocate,release};struct pt_document doc;
    struct pt_paula a={0},b={0};struct pt_playback state;struct pt_mod_export_report report;
    struct Library *ciaa=NULL,*ciab=NULL;struct Interrupt holda,holdb;
    FILE *f=NULL;uint8_t *input=NULL,*roundtrip=NULL;long length;size_t written;unsigned own_a=0,own_b=0,i;int rc=20;
    pt_document_init(&doc,&allocator);memset(&holda,0,sizeof(holda));memset(&holdb,0,sizeof(holdb));
    holda.is_Node.ln_Type=NT_INTERRUPT;holda.is_Node.ln_Name="PT24G ownership test";holda.is_Code=(void (*)())dummy_interrupt;holdb=holda;
    CHECK(argc==2);f=fopen(argv[1],"rb");CHECK(f);CHECK(!fseek(f,0,SEEK_END));length=ftell(f);CHECK(length>0);rewind(f);
    input=malloc(length);CHECK(input);CHECK(fread(input,1,length,f)==(size_t)length);fclose(f);f=NULL;
    CHECK(pt_document_load(&doc,input,length,SIZE_MAX)==PT_PROJECT_OK);
    CHECK(pt_mod_export_analyse(&doc.project,&report)==PT_PROJECT_OK && !report.issues);
    CHECK(!pt_paula_play(&a,&doc.project,0,0,0));Delay(20);pt_paula_poll(&a,&state);
    CHECK(state.active && state.ticks>=6 && state.period[0]==428 && state.period[1]==339 && state.volume[0]==24);
    printf("REPLAY initial ticks=%lu row=%u periods=%u,%u,%u,%u\n",(unsigned long)state.ticks,state.row,state.period[0],state.period[1],state.period[2],state.period[3]);
    CHECK(pt_paula_play(&b,&doc.project,0,0,0)!=NULL && !b.started);
    pt_paula_poll(&a,&state);CHECK(state.active);pt_paula_stop(&a);
    CHECK((*(volatile UWORD *)0xdff002 & 15)==0);
    roundtrip=malloc(report.bytes);CHECK(roundtrip);
    CHECK(pt_mod_export_direct(&doc.project,roundtrip,report.bytes,&written)==PT_PROJECT_OK);
    CHECK(written==(size_t)length && !memcmp(input,roundtrip,written));
    /* Repeated starts reset all persistent voice/effect state. */
    for(i=0;i<3;++i) {CHECK(!pt_paula_play(&a,&doc.project,0,0,0));Delay(10);pt_paula_poll(&a,&state);CHECK(state.period[0]==428 && state.ticks>=6);pt_paula_stop(&a);}
    /* Live row publishing changes future replay without replacing DMA sample memory. */
    CHECK(!pt_paula_play(&a,&doc.project,1,0,0));
    doc.project.events[8*4].pitch=214;CHECK(!pt_paula_sync(&a,&doc.project));
    Delay(55);pt_paula_poll(&a,&state);CHECK(state.row>=8 && state.period[0]==214);pt_paula_stop(&a);
    doc.project.events[8*4].pitch=428;
    CHECK(!pt_paula_audition(&a,&doc.project,1,285));Delay(10);pt_paula_poll(&a,&state);CHECK(state.period[0]==285 && !state.period[1]);pt_paula_stop(&a);
    doc.project.events[0].kind=PT_NOTE_OFF;doc.project.events[0].pitch=0;
    CHECK(pt_paula_play(&a,&doc.project,0,0,0)!=NULL && !a.started);
    doc.project.events[0].kind=PT_NOTE_PERIOD;doc.project.events[0].pitch=428;
    /* Observable effect sequence: speed, tempo, volume, delayed cut and
       pattern break. Poll every PAL frame; each effect persists long enough
       to observe without relying on host wall-clock scheduling. */
    memset(doc.project.events,0,256*sizeof(*doc.project.events));
    doc.project.events[0].kind=PT_NOTE_PERIOD;doc.project.events[0].pitch=428;doc.project.events[0].instrument=1;
    doc.project.events[3].effect=15;doc.project.events[3].parameter=3;
    doc.project.events[7].effect=15;doc.project.events[7].parameter=150;
    doc.project.events[8].effect=12;doc.project.events[8].parameter=16;
    doc.project.events[12].effect=14;doc.project.events[12].parameter=0xc1;
    doc.project.events[16].effect=13;
    CHECK(!pt_paula_play(&a,&doc.project,0,0,0));
    {unsigned seen=0;
        for(i=0;i<100 && seen!=15;++i) {
            Delay(1);pt_paula_poll(&a,&state);
            if(state.row==0 && state.speed==3)seen|=1;
            if(state.row==1 && state.bpm==150)seen|=2;
            if(state.row==2 && state.volume[0]==16)seen|=4;
            if(state.row==3 && state.volume[0]==0)seen|=8;
        }
        CHECK(seen==15);printf("REPLAY effects speed/tempo/volume/cut observed mask=%u ticks=%lu\n",seen,(unsigned long)state.ticks);
    }
    pt_paula_stop(&a);
    doc.project.events[16].effect=15; /* F00 ends and releases the engine. */
    CHECK(!pt_paula_play(&a,&doc.project,0,0,0));
    for(i=0;i<100 && a.started;++i) {Delay(1);pt_paula_poll(&a,&state);}
    CHECK(!a.started && (*(volatile UWORD *)0xdff002 & 15)==0);
    CHECK(pt_document_load(&doc,input,length,SIZE_MAX)==PT_PROJECT_OK);
    /* Force CIA fallback and total failure. Never replace an existing vector. */
    ciab=OpenResource("ciab.resource");ciaa=OpenResource("ciaa.resource");CHECK(ciab && ciaa);
    own_b=claim(ciab,0,&holdb);CHECK(own_b);
    own_a=claim(ciaa,1,&holda);
    CHECK(pt_paula_play(&a,&doc.project,0,0,0)!=NULL && !a.started);
    CHECK(!claim(ciab,0,&holda) && !claim(ciaa,1,&holdb));
    if(own_a) {
        unclaim(ciaa,1,&holda);own_a=0;
        CHECK(!pt_paula_play(&a,&doc.project,0,0,0));Delay(10);pt_paula_poll(&a,&state);CHECK(state.ticks>=6 && state.period[0]==428);pt_paula_stop(&a);
        own_a=claim(ciaa,1,&holda);CHECK(own_a); /* Correct timer B vector removed. */
        puts("CIA fallback/vector-release PASS");
    } else puts("CIA fallback execution NOT RUN: Workbench already owns CIAA timer B; existing vector preserved");
    CHECK(!claim(ciab,0,&holda)); /* Unrelated held timer A was preserved. */
    puts("PAULA PASS: playback periods, busy audio refusal, DMA stop, immutable project, repeated restart, live edit, audition, speed/tempo/volume/cut, unsupported refusal, CIA exhaustion and unrelated-vector preservation");rc=0;
done:
    pt_paula_stop(&b);pt_paula_stop(&a);
    if(own_a)unclaim(ciaa,1,&holda);
    if(own_b)unclaim(ciab,0,&holdb);
    if(f)fclose(f);
    free(input);free(roundtrip);pt_document_release(&doc);return rc;
}
