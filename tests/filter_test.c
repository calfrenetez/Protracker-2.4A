#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "pcm.h"
static const int32_t wave[32]={0,6393,12539,18204,23170,27245,30273,32137,32767,32137,30273,27245,23170,18204,12539,6393,0,-6393,-12539,-18204,-23170,-27245,-30273,-32137,-32767,-32137,-30273,-27245,-23170,-18204,-12539,-6393};
struct progress_state {uint32_t limit,last;unsigned calls;};
static int progress(void *context,uint32_t done,uint32_t total)
{
 struct progress_state *state=context;assert(done>=state->last && done<=total);state->last=done;++state->calls;
 return done<state->limit;
}
static const int32_t fractional_tone[89]={0,17679,29771,32451,24874,9435,-8987,-24568,-32383,-29963,-18071,-467,17285,29572,32513,25176,9881,-8537,-24257,-32309,-30148,-18458,-934,16886,29368,32567,25472,10325,-8085,-23940,-32228,-30328,-18842,-1400,16484,29158,32616,25763,10767,-7632,-23619,-32140,-30502,-19222,-1866,16079,28942,32657,26049,11207,-7178,-23293,-32046,-30669,-19598,-2332,15671,28721,32692,26330,11645,-6721,-22963,-31945,-30831,-19970,-2798,15259,28493,32720,26605,12080,-6264,-22627,-31838,-30986,-20339,-3263,14845,28260,32742,26875,12512,-5805,-22287,-31725,-31134,-20702,-3727};
int main(void) {
 int32_t *input=malloc(2048*2*sizeof(int32_t)),*output=malloc(2048*2*sizeof(int32_t));
 struct pt_pcm source={input,4096,2048,32000,1,16},dest={output,4096,512,8000,1,16};
 unsigned i,f;int max_error;struct progress_state state={0,0,0};
 assert(input && output);
 for(f=1;f<=6;f+=f==1?2:3) {
  for(i=0;i<2048;++i)input[i]=wave[(i*f)%32];
  assert(pt_pcm_resample_filtered(&source,&dest)==PT_PCM_OK);max_error=0;
  for(i=32;i<480;++i) {
   int expected=f==6?0:wave[(i*4*f)%32];int error=output[i]-expected;
   if(error<0)error=-error;
   if(error>max_error)max_error=error;
  }
  printf("tone %u kHz max deviation %d\n",f,max_error);fflush(stdout);assert(max_error<32);
 }
 source.frames=256;dest.frames=89;dest.rate=11025;
 for(i=0;i<256;++i)input[i]=wave[i%32];
 assert(pt_pcm_resample_filtered(&source,&dest)==PT_PCM_OK);
 max_error=0;
 for(i=24;i<65;++i) {int error=output[i]-fractional_tone[i];if(error<0)error=-error;
  if(error>max_error)max_error=error;
 }
 printf("fractional-rate passband max deviation %d\n",max_error);fflush(stdout);assert(max_error<8);
 source.frames=2048;dest.frames=512;dest.rate=8000;
 for(i=0;i<2048;++i)input[i]=wave[(i*6)%32];
 output[0]=42;assert(pt_pcm_resample_filtered_progress(&source,&dest,progress,&state)==PT_PCM_CANCELLED && output[0]==42 && state.calls==1);
 state.limit=64;state.last=0;state.calls=0;
 assert(pt_pcm_resample_filtered_progress(&source,&dest,progress,&state)==PT_PCM_CANCELLED && state.last>=64 && state.last<96 && state.calls>1);
 state.limit=UINT32_MAX;state.last=0;state.calls=0;
 assert(pt_pcm_resample_filtered_progress(&source,&dest,progress,&state)==PT_PCM_OK && state.last==dest.frames);
 source.bits=24;source.channels=2;dest.bits=24;dest.channels=2;
 for(i=0;i<2048;++i) {input[2*i]=1234567;input[2*i+1]=-7654321;}
 assert(pt_pcm_resample_filtered(&source,&dest)==PT_PCM_OK);
 for(i=0;i<512;++i)assert(output[2*i]==1234567 && output[2*i+1]==-7654321);
 dest.frames=706;dest.rate=11025;
 assert(pt_pcm_resample_filtered(&source,&dest)==PT_PCM_OK);
 for(i=0;i<706;++i)assert(output[2*i]==1234567 && output[2*i+1]==-7654321);
 dest.frames=source.frames;dest.rate=source.rate;
 assert(pt_pcm_resample_filtered(&source,&dest)==PT_PCM_OK);
 for(i=0;i<4096;++i)assert(input[i]==output[i]);
 assert(pt_pcm_resample_filtered(&source,&source)==PT_PCM_ALIAS);
 output[0]=42;dest.capacity=1;
 assert(pt_pcm_resample_filtered(&source,&dest)==PT_PCM_CAPACITY && output[0]==42);
 dest.capacity=4096;dest.frames=1;dest.rate=1;
 assert(pt_pcm_resample_filtered(&source,&dest)==PT_PCM_CAPACITY && output[0]==42);
 dest.rate=8000;dest.frames=511;
 assert(pt_pcm_resample_filtered(&source,&dest)==PT_PCM_INVALID && output[0]==42);
 /* Upsampling preserves independent stereo DC and safely extends both edges. */
 source.frames=3;dest.frames=6;dest.rate=64000;
 assert(pt_pcm_resample_filtered(&source,&dest)==PT_PCM_OK);
 for(i=0;i<6;++i)assert(output[2*i]==1234567 && output[2*i+1]==-7654321);
 free(input);free(output);puts("FILTER PASS");return 0;
}
