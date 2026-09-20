/* Diagnostic only: all recording storage exists before CIA acquisition. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/dos.h>
#include "document.h"
#include "../src/native/paula.h"
#ifndef RECORD_BYTES
#define RECORD_BYTES 36U
#endif
uint8_t *pt_flow_data;
volatile uint16_t pt_flow_count,pt_flow_fetches,pt_flow_limited;
uint16_t pt_flow_capacity;
extern volatile uint8_t pt_replay_enabled;
static void *allocate(void *ctx,size_t n) {(void)ctx;return malloc(n);}
static void release(void *ctx,void *p) {(void)ctx;free(p);}
#define CHECK(c) do {if(!(c)) {printf("FAIL line %u: %s\n",__LINE__,#c);goto done;}}while(0)
int main(int argc,char **argv)
{
    struct pt_allocator allocator={NULL,allocate,release};struct pt_document doc;
    struct pt_paula player={0};FILE *f=NULL;uint8_t *input=NULL;
    long length,budget;char *end;unsigned i,j,frames;int rc=20;const char *error,*reason;
    pt_document_init(&doc,&allocator);
    CHECK(argc==3);budget=strtol(argv[2],&end,10);CHECK(!*end && budget>0 && budget<=4096);
    pt_flow_capacity=(uint16_t)budget;pt_flow_data=calloc((size_t)budget,RECORD_BYTES);CHECK(pt_flow_data);
    f=fopen(argv[1],"rb");CHECK(f);CHECK(!fseek(f,0,SEEK_END));length=ftell(f);CHECK(length>0 && length<=65536);rewind(f);
    input=malloc((size_t)length);CHECK(input);CHECK(fread(input,1,(size_t)length,f)==(size_t)length);fclose(f);f=NULL;
    CHECK(pt_document_load(&doc,input,(size_t)length,SIZE_MAX)==PT_PROJECT_OK);
    error=pt_paula_play(&player,&doc.project,0,0,0);
    if(error)printf("PLAY ERROR %s\n",error);
    CHECK(!error);
    for(frames=0;frames<2000 && pt_replay_enabled;++frames)Delay(1);
    reason=pt_flow_limited?"tick-budget":(pt_replay_enabled?"deadline":"native-stop");
    pt_paula_stop(&player); /* Removes interrupt before reading/freeing records. */
    CHECK(strcmp(reason,"deadline"));CHECK(pt_flow_count>0 && pt_flow_count<=pt_flow_capacity);
    CHECK((*(volatile UWORD *)0xdff002 & 15)==0);
    printf("FLOW schema=1 bytes=%u count=%u reason=%s\n",RECORD_BYTES,pt_flow_count,reason);
    for(i=0;i<pt_flow_count;++i) {
        printf("T ");for(j=0;j<RECORD_BYTES;++j)printf("%02x",pt_flow_data[i*RECORD_BYTES+j]);putchar('\n');
    }
    puts("FLOW PASS dma=0");rc=0;
done:
    pt_paula_stop(&player);if(f)fclose(f);free(input);free(pt_flow_data);pt_flow_data=NULL;
    pt_document_release(&doc);return rc;
}
