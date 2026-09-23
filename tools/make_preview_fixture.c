/* Disposable high-rate stereo master for interactive cancellation evidence. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "project.h"
int main(int argc,char **argv)
{
    struct pt_project p={0};struct pt_sample sample={0};struct pt_event events[256]={{0}};
    uint16_t order=0;size_t n,w,i;uint8_t *bytes;FILE *f;int ok;
    if(argc!=2)return 2;
    pt_channels_init(&p.channels);p.speed=6;p.bpm=125;p.order_count=p.pattern_count=p.sample_count=1;
    p.orders=&order;p.events=events;p.samples=&sample;strcpy(p.title,"PREVIEW CANCEL FIXTURE");
    sample.volume=64;strcpy(sample.name,"TRUE24 STEREO 192K");
    sample.pcm.frames=96000;sample.pcm.rate=192000;sample.pcm.channels=2;sample.pcm.bits=24;
    sample.pcm.capacity=(size_t)sample.pcm.frames*2;sample.pcm.data=malloc(sample.pcm.capacity*sizeof(int32_t));
    if(!sample.pcm.data)return 3;
    for(i=0;i<sample.pcm.capacity;++i)sample.pcm.data[i]=(int32_t)((i*76543UL)%16777216)-8388608;
    if(pt_project_size(&p,&n)!=PT_PROJECT_OK) {free(sample.pcm.data);return 4;}
    bytes=malloc(n);if(!bytes) {free(sample.pcm.data);return 3;}
    ok=pt_project_encode(&p,bytes,n,&w)==PT_PROJECT_OK && w==n;
    f=ok?fopen(argv[1],"wb"):NULL;ok=f && fwrite(bytes,1,n,f)==n;if(f && fclose(f))ok=0;
    free(bytes);free(sample.pcm.data);return ok?0:5;
}
