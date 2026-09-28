/* Private UI fixture: sample1 changes only below Paula's output precision. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mod_project.h"
int main(int argc,char **argv)
{
    struct pt_project p={0};struct pt_sample samples[2]={{0}};
    struct pt_event events[256]={{0}};int32_t pcm[2][256];uint16_t order=0;
    const unsigned periods[4]={428,339,285,214};
    size_t n,w;unsigned i,j;uint8_t *bytes;FILE *f;int ok;
    if(argc!=2)return 2;
    pt_channels_init(&p.channels);p.speed=6;p.bpm=125;
    p.order_count=p.pattern_count=1;p.sample_count=2;
    p.orders=&order;p.events=events;p.samples=samples;strcpy(p.title,"PAULA MASTER TEST");
    for(i=0;i<2;++i) {
        struct pt_sample *s=samples+i;
        s->volume=32;s->loop=PT_LOOP_FORWARD;s->loop_end=256;
        s->pcm.frames=s->pcm.capacity=256;s->pcm.rate=PT_CLASSIC_RATE;
        s->pcm.channels=1;s->pcm.bits=i?16:24;s->pcm.data=pcm[i];
        strcpy(s->name,i?"16BIT TONE":"24BIT LOW BITS");
        for(j=0;j<256;++j)pcm[i][j]=i?((int32_t)(j%64)-32)*512+11:(int32_t)j+1;
    }
    for(i=0;i<4;++i) {
        events[i].kind=PT_NOTE_PERIOD;events[i].pitch=(uint16_t)periods[i];
        events[i].instrument=(uint8_t)(1+i%2);
    }
    if(pt_project_size(&p,&n)!=PT_PROJECT_OK)return 3;
    bytes=malloc(n);if(!bytes)return 4;
    ok=pt_project_encode(&p,bytes,n,&w)==PT_PROJECT_OK && w==n;
    f=ok?fopen(argv[1],"wb"):NULL;ok=f && fwrite(bytes,1,n,f)==n;
    if(f && fclose(f))ok=0;
    free(bytes);return ok?0:5;
}
