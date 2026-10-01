#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned dma=0x200, suppress_dma, hold_dma;
#define PT_PAULA_DMA_READ() dma
#include "../src/native/paula_output.h"
static struct ExecBase executive={{36},709379};struct ExecBase *SysBase=&executive;
static struct Device device;
static struct {struct IORequest *q;unsigned pending;} requests[16];
static struct Message *messages[4];static unsigned message_count;
static unsigned live,opens,closes,commands,aborts,waits,resources,fail_resource;
static unsigned delay_start,delay_abort,delay_control,start_error,control_error,delay_free;
static union {uint16_t align;uint8_t bytes[32];} chip;
static unsigned find(struct IORequest *q){unsigned i;for(i=0;i<16;++i)if(requests[i].q==q)return i;assert(0);return 0;}
struct MsgPort *CreateMsgPort(void){struct MsgPort *p;if(++resources==fail_resource)return 0;p=calloc(1,sizeof(*p));assert(p);p->mp_SigBit=4;++live;return p;}
struct IORequest *CreateIORequest(struct MsgPort *p,ULONG n){struct IORequest *q;unsigned i;(void)p;if(++resources==fail_resource)return 0;q=calloc(1,n);assert(q);for(i=0;i<16 && requests[i].q;++i){}assert(i<16);requests[i].q=q;++live;return q;}
LONG OpenDevice(const char *name,ULONG unit,struct IORequest *q,ULONG flags){struct IOAudio *a=(struct IOAudio *)q;assert(name && !unit && !flags && !a->ioa_Length && !a->ioa_AllocKey);if(++resources==fail_resource)return -1;q->io_Device=&device;++opens;return 0;}
void BeginIO(struct IORequest *q){struct IOAudio *a=(struct IOAudio *)q;unsigned i=find(q);++commands;assert(q->io_Device==&device && !requests[i].pending);
 switch(q->io_Command){
 case ADCMD_ALLOCATE:assert(q->io_Flags==(IOF_QUICK|ADIOF_NOWAIT) && q->io_Message.mn_Node.ln_Pri==-128 && a->ioa_Length==1 && *a->ioa_Data==15);q->io_Unit=(struct Unit *)(uintptr_t)15;a->ioa_AllocKey=7;break;
 case ADCMD_SETPREC:assert(q->io_Flags==IOF_QUICK && q->io_Message.mn_Node.ln_Pri==127 && (uintptr_t)q->io_Unit==15 && a->ioa_AllocKey==7);break;
 case ADCMD_LOCK:assert(!q->io_Flags && a->ioa_AllocKey==7);requests[i].pending=1;break;
 case ADCMD_FREE:assert(a->ioa_AllocKey==7 && (uintptr_t)q->io_Unit==15);if(delay_free){q->io_Flags&=~IOF_QUICK;requests[i].pending=1;break;}for(i=0;i<16;++i)if(requests[i].q && requests[i].q->io_Command==ADCMD_LOCK)requests[i].pending=0;break;
 case CMD_WRITE:assert(a->ioa_AllocKey==7 && q->io_Flags==(IOF_QUICK|ADIOF_PERVOL|ADIOF_WRITEMESSAGE));assert(a->ioa_Data==chip.bytes && a->ioa_Length==32 && !a->ioa_Cycles && a->ioa_Period>=124 && a->ioa_Volume<=64);assert(a->ioa_WriteMsg.mn_ReplyPort && a->ioa_WriteMsg.mn_Length==sizeof(struct Message));
  if(start_error){q->io_Error=-10;break;}q->io_Flags&=~IOF_QUICK;requests[i].pending=1;if(!suppress_dma)dma|=(unsigned)(uintptr_t)q->io_Unit;if(!delay_start){assert(message_count<4);messages[message_count++]=&a->ioa_WriteMsg;}break;
 case ADCMD_PERVOL:assert(a->ioa_AllocKey==7 && q->io_Flags==IOF_QUICK);if(control_error)q->io_Error=-10;if(delay_control){q->io_Flags&=~IOF_QUICK;requests[i].pending=1;}break;
 default:assert(0);
 }
}
struct IORequest *CheckIO(struct IORequest *q){return requests[find(q)].pending?0:q;}
LONG WaitIO(struct IORequest *q){assert(CheckIO(q));++waits;return q->io_Error;}
LONG AbortIO(struct IORequest *q){assert(q->io_Command==CMD_WRITE);++aborts;if(!delay_abort){requests[find(q)].pending=0;if(!hold_dma)dma&=~(unsigned)(uintptr_t)q->io_Unit;q->io_Error=-2;}return 0;}
struct Message *GetMsg(struct MsgPort *p){struct Message *m;(void)p;if(!message_count)return 0;m=messages[0];memmove(messages,messages+1,(--message_count)*sizeof(*messages));return m;}
ULONG TypeOfMem(const void *p){return (uintptr_t)p>=(uintptr_t)chip.bytes && (uintptr_t)p<(uintptr_t)chip.bytes+sizeof(chip.bytes)?MEMF_CHIP:0;}
void CloseDevice(struct IORequest *q){assert(!q->io_Unit && !((struct IOAudio *)q)->ioa_AllocKey);++closes;}
void DeleteIORequest(struct IORequest *q){unsigned i=find(q);assert(!requests[i].pending);requests[i].q=0;--live;free(q);}
void DeleteMsgPort(struct MsgPort *p){assert(!message_count);--live;free(p);}
static void reset(void){assert(!live && opens==closes && !message_count);memset(requests,0,sizeof(requests));opens=closes=commands=aborts=waits=resources=fail_resource=delay_start=delay_abort=delay_control=start_error=control_error=delay_free=0;dma=0x200;suppress_dma=hold_dma=0;executive.LibNode.lib_Version=36;executive.ex_EClockFrequency=709379;}
static void ready(struct pt_native_paula_output *o){assert(pt_native_paula_output_open(o));assert(pt_native_paula_output_advance(o)==0);assert(pt_native_paula_output_advance(o)==0);assert(pt_native_paula_output_advance(o)==1);assert(live==12);}
static void closed(struct pt_native_paula_output *o){unsigned i;for(i=0;i<8 && !pt_native_paula_output_close(o);++i){}assert(i<8 && !live && opens==closes);assert(pt_native_paula_output_close(o));}
int main(void){struct pt_native_paula_output o;struct pt_paula_voice_api api;struct pt_paula_voice_plan p={chip.bytes,16,400,0},bad;unsigned i,cases=0;
 for(i=1;i<=13;++i){reset();o=(struct pt_native_paula_output){0};fail_resource=i;assert(!pt_native_paula_output_open(&o));assert(!live && opens==closes);++cases;}
 reset();o=(struct pt_native_paula_output){0};executive.LibNode.lib_Version=35;assert(!pt_native_paula_output_open(&o) && !resources);executive.LibNode.lib_Version=36;executive.ex_EClockFrequency=123;assert(!pt_native_paula_output_open(&o) && !resources);++cases;
 reset();o=(struct pt_native_paula_output){0};ready(&o);assert(o.caps.clock_hz==3546895);assert(pt_native_paula_output_api(&o,&api));for(i=0;i<4;++i){assert(api.start(api.context,i,&p)==1);assert(!api.start(api.context,i,&p));assert(api.control(api.context,i,500,64)==1);}assert(!pt_native_paula_output_quiesce(&o));for(i=0;i<4;++i)assert(api.stop(api.context,i)==1);assert(aborts==4 && !message_count && pt_native_paula_output_quiesce(&o));closed(&o);++cases;
 reset();o=(struct pt_native_paula_output){0};ready(&o);bad=p;bad.data=chip.bytes+1;assert(!pt_native_paula_output_start(&o,0,&bad));bad=p;bad.words=0;assert(!pt_native_paula_output_start(&o,0,&bad));bad=p;bad.period=123;assert(!pt_native_paula_output_start(&o,0,&bad));bad=p;bad.volume=65;assert(!pt_native_paula_output_start(&o,0,&bad));bad=p;bad.data=(const uint8_t *)&device;assert(!pt_native_paula_output_start(&o,0,&bad));bad=p;bad.words=17;assert(!pt_native_paula_output_start(&o,0,&bad));assert(!o.held[0]);closed(&o);++cases;
 reset();o=(struct pt_native_paula_output){0};ready(&o);delay_start=1;assert(pt_native_paula_output_start(&o,0,&p)==1 && o.held[0] && o.pending[0] && !o.notified[0]);assert(pt_native_paula_output_control(&o,0,500,0)==1);delay_abort=1;assert(!pt_native_paula_output_close(&o) && live==12 && aborts==1 && !closes && o.write[0]->ioa_Data==chip.bytes);assert(!pt_native_paula_output_close(&o) && aborts==1 && !closes);/* Late start reply must be consumed on stop. */messages[message_count++]=&o.write[0]->ioa_WriteMsg;requests[find((struct IORequest *)o.write[0])].pending=0;dma&=~1U;closed(&o);++cases;
 reset();o=(struct pt_native_paula_output){0};ready(&o);start_error=1;assert(!pt_native_paula_output_start(&o,0,&p) && o.held[0]);assert(pt_native_paula_output_stop(&o,0)==1 && !aborts);closed(&o);++cases;
 reset();o=(struct pt_native_paula_output){0};ready(&o);assert(pt_native_paula_output_start(&o,0,&p)==1);delay_control=1;assert(!pt_native_paula_output_control(&o,0,500,0));assert(!pt_native_paula_output_stop(&o,0) && o.held[0] && !o.pending[0] && o.control_pending[0]);assert(!pt_native_paula_output_close(&o) && live==12 && !closes);requests[find((struct IORequest *)o.control[0])].pending=0;closed(&o);++cases;
 reset();o=(struct pt_native_paula_output){0};ready(&o);assert(pt_native_paula_output_start(&o,0,&p)==1);control_error=1;assert(!pt_native_paula_output_control(&o,0,500,0));closed(&o);++cases;
 reset();o=(struct pt_native_paula_output){0};executive.ex_EClockFrequency=715909;ready(&o);assert(o.caps.clock_hz==3579545);executive.ex_EClockFrequency=709379;assert(!pt_native_paula_output_start(&o,0,&p));closed(&o);++cases;
 reset();o=(struct pt_native_paula_output){0};ready(&o);dma=0;assert(!pt_native_paula_output_start(&o,0,&p) && !o.held[0]);dma=0x201;assert(!pt_native_paula_output_start(&o,0,&p) && !o.held[0]);assert(!pt_native_paula_output_quiesce(&o));dma=0x200;closed(&o);++cases;
 reset();o=(struct pt_native_paula_output){0};ready(&o);suppress_dma=1;assert(!pt_native_paula_output_start(&o,0,&p) && o.held[0] && o.notified[0]);assert(!pt_native_paula_output_control(&o,0,500,0));closed(&o);++cases;
 reset();o=(struct pt_native_paula_output){0};ready(&o);assert(pt_native_paula_output_start(&o,0,&p)==1);hold_dma=1;assert(!pt_native_paula_output_stop(&o,0) && o.held[0] && !o.pending[0] && o.write[0]->ioa_Data==chip.bytes);assert(!pt_native_paula_output_close(&o) && live==12 && aborts==1);dma&=~1U;closed(&o);++cases;
 printf("PAULA OUTPUT HOST PASS: %u ownership/command cases\n",cases);return 0;}
