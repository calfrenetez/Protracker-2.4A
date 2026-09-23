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
extern volatile uint8_t pt_replay_voices[];
static uintptr_t voice_start(void)
{return ((uintptr_t)pt_replay_voices[4]<<24)|((uintptr_t)pt_replay_voices[5]<<16)|((uintptr_t)pt_replay_voices[6]<<8)|pt_replay_voices[7];}
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
    CHECK((*(volatile UWORD *)0xdff002 & 0x20f)==0x20f);
    CHECK((*(volatile UWORD *)0xdff010 & 0xff)==0);
    {struct pt_playback previous=state;unsigned changed=0;
        for(i=0;i<10;++i) {Delay(1);pt_paula_poll(&a,&state);if(memcmp(state.wave,previous.wave,sizeof(state.wave)))changed=1;previous=state;}
        CHECK(changed);puts("SCOPE/DMA PASS: enabled hardware channels and time-varying sample preview");
    }
    printf("REPLAY initial ticks=%lu row=%u periods=%u,%u,%u,%u\n",(unsigned long)state.ticks,state.row,state.period[0],state.period[1],state.period[2],state.period[3]);
    CHECK(pt_paula_play(&b,&doc.project,0,0,0)!=NULL && !b.started);
    pt_paula_poll(&a,&state);CHECK(state.active);pt_paula_stop(&a);
    CHECK((*(volatile UWORD *)0xdff002 & 15)==0);
    roundtrip=malloc(report.bytes);CHECK(roundtrip);
    CHECK(pt_mod_export_direct(&doc.project,roundtrip,report.bytes,&written)==PT_PROJECT_OK);
    CHECK(written==(size_t)length && !memcmp(input,roundtrip,written));
    /* Saved mute/solo gates actual output without erasing effect state.
       Strict MOD export still refuses to discard these settings. */
    doc.project.channels.track[0].muted=1;
    CHECK(pt_mod_export_analyse(&doc.project,&report)==PT_PROJECT_OK && report.issues);
    CHECK(!pt_paula_play(&a,&doc.project,0,0,0));Delay(15);pt_paula_poll(&a,&state);
    CHECK(state.active && !state.volume[0] && state.volume[1] && state.period[0]==428);
    CHECK(pt_paula_play(&b,&doc.project,0,0,0)!=NULL && !b.started);
    doc.project.channels.track[0].muted=0;CHECK(!pt_paula_sync(&a,&doc.project));pt_paula_poll(&a,&state);
    CHECK(state.volume[0]==24);
    doc.project.channels.track[1].solo=1;CHECK(!pt_paula_sync(&a,&doc.project));Delay(10);pt_paula_poll(&a,&state);
    CHECK(!state.volume[0] && state.volume[1] && !state.volume[2] && !state.volume[3]);
    doc.project.channels.track[0].solo=1;CHECK(!pt_paula_sync(&a,&doc.project));pt_paula_poll(&a,&state);
    CHECK(state.volume[0]==24 && state.volume[1]);
    doc.project.channels.track[0].muted=1;CHECK(!pt_paula_sync(&a,&doc.project));pt_paula_poll(&a,&state);
    CHECK(!state.volume[0] && state.volume[1]);
    doc.project.channels.track[0].muted=0;doc.project.channels.track[0].solo=0;doc.project.channels.track[1].solo=0;
    CHECK(!pt_paula_sync(&a,&doc.project));pt_paula_poll(&a,&state);
    CHECK(state.volume[0]==24 && state.volume[1] && state.volume[2] && state.volume[3]);pt_paula_stop(&a);
    CHECK(pt_mod_export_direct(&doc.project,roundtrip,(size_t)length,&written)==PT_PROJECT_OK);
    CHECK(written==(size_t)length && !memcmp(input,roundtrip,written));
    puts("CHANNEL AUDIO PASS: muted start, live unmute, solo, shared solo, mute wins, immutable notes/samples, strict export refusal");
    /* Organisation metadata never stops classic audio or leaks into its song. */
    strcpy(doc.project.channels.track[0].name,"BASS");doc.project.channels.track[0].group=15;doc.project.channels.track[0].midi_channel=16;
    CHECK(pt_mod_export_analyse(&doc.project,&report)==PT_PROJECT_OK && report.issues==PT_EXPORT_METADATA);
    CHECK(!pt_paula_play(&a,&doc.project,0,0,0));Delay(10);pt_paula_poll(&a,&state);
    CHECK(state.active && state.period[0]==428 && state.volume[0]==24);
    strcpy(doc.project.channels.track[0].name,"BASS TWO");CHECK(!pt_paula_sync(&a,&doc.project));
    CHECK(doc.project.channels.track[0].group==15 && doc.project.channels.track[0].midi_channel==16 && !strcmp(doc.project.channels.track[0].name,"BASS TWO"));
    doc.project.channels.track[0].pan=128;CHECK(pt_paula_sync(&a,&doc.project)!=NULL);pt_paula_stop(&a);
    doc.project.channels.track[0].pan=0;doc.project.channels.track[0].group=0;doc.project.channels.track[0].midi_channel=1;
    memset(doc.project.channels.track[0].name,0,sizeof(doc.project.channels.track[0].name));
    CHECK(pt_mod_export_direct(&doc.project,roundtrip,(size_t)length,&written)==PT_PROJECT_OK && !memcmp(input,roundtrip,written));
    puts("CHANNEL DETAILS AUDIO PASS: saved names/groups/MIDI assignment preserve Paula replay, strict export refuses metadata, enhanced pan still refused");
    {char original_title[32];memcpy(original_title,doc.project.title,32);
        strcpy(doc.project.title,"LONG SONG TITLE OVER MOD LIMIT");
        CHECK(pt_mod_export_analyse(&doc.project,&report)==PT_PROJECT_OK && report.issues==PT_EXPORT_METADATA);
        CHECK(!pt_paula_play(&a,&doc.project,0,0,0));Delay(10);pt_paula_poll(&a,&state);
        CHECK(state.active && state.period[0]==428);
        strcpy(doc.project.title,"ANOTHER LONG TITLE FOR THE SONG");CHECK(!pt_paula_sync(&a,&doc.project));
        CHECK(!strcmp(doc.project.title,"ANOTHER LONG TITLE FOR THE SONG"));pt_paula_stop(&a);
        memcpy(doc.project.title,original_title,32);
        CHECK(pt_mod_export_direct(&doc.project,roundtrip,(size_t)length,&written)==PT_PROJECT_OK && !memcmp(input,roundtrip,written));
        puts("TITLE AUDIO PASS: long titles preserve playback and source metadata; strict export remains lossless");
    }
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
    /* A changed order list must never keep playing an obsolete snapshot. */
    {uint16_t orders[2]={0,0},*original=doc.project.orders;
        CHECK(!pt_paula_play(&a,&doc.project,0,0,0));Delay(5);
        doc.project.orders=orders;doc.project.order_count=2;
        CHECK(pt_paula_sync(&a,&doc.project)!=NULL && !a.started);
        CHECK((*(volatile UWORD *)0xdff002 & 15)==0);
        CHECK(!pt_paula_play(&a,&doc.project,0,1,0));Delay(10);pt_paula_poll(&a,&state);
        CHECK(state.active && state.order==1);pt_paula_stop(&a);
        doc.project.orders=original;doc.project.order_count=1;
        puts("SONG AUDIO PASS: position changes stop stale replay; restart uses the new order list");
    }
    /* Empty instruments and initial instrument-zero notes use an owned word,
       never a zero-length DMA transfer or a pointer beyond the allocation. */
    for(i=0;i<2;++i) {
        doc.project.events[0].instrument=i?0:2;
        doc.project.events[0].effect=12;doc.project.events[0].parameter=64;
        CHECK(!pt_paula_play(&a,&doc.project,0,0,0));Delay(10);
        CHECK(voice_start()==(uintptr_t)(a.data+a.bytes));
        CHECK(pt_replay_voices[20]==0 && pt_replay_voices[21]==1);
        CHECK(a.data[a.bytes]==0 && a.data[a.bytes+1]==0);pt_paula_stop(&a);
    }
    CHECK(pt_document_load(&doc,input,length,SIZE_MAX)==PT_PROJECT_OK);
    /* Optional preserved MOD headers cannot smuggle an out-of-range one-word
       repeat through the project format into the hardware snapshot. */
    {uint8_t *header=(uint8_t *)doc.project.extensions[0].data;
        header[20+30+26]=0x7f;header[20+30+27]=0xff;
        CHECK(pt_paula_play(&a,&doc.project,0,0,0)!=NULL && !a.started);
    }
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
    puts("PAULA PASS: playback periods, busy audio refusal, DMA stop, immutable project, repeated restart, live edit, audition, speed/tempo/volume/cut, unsupported refusal, owned empty-sample DMA, CIA exhaustion and unrelated-vector preservation");rc=0;
done:
    pt_paula_stop(&b);pt_paula_stop(&a);
    if(own_a)unclaim(ciaa,1,&holda);
    if(own_b)unclaim(ciab,0,&holdb);
    if(f)fclose(f);
    free(input);free(roundtrip);pt_document_release(&doc);return rc;
}
