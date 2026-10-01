#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "../src/native/paula_reservation.h"
static unsigned live,opens,closes,waits,commands,ready,delayed,lock_done,mask,fail_command,fail_resource,resources;
static struct IORequest *active,*watch;
static struct Device device;
struct MsgPort *CreateMsgPort(void){struct MsgPort *p;if(++resources==fail_resource)return 0;p=calloc(1,sizeof(*p));assert(p);p->mp_SigBit=4;++live;return p;}
struct IORequest *CreateIORequest(struct MsgPort *p,ULONG n){struct IORequest *q;(void)p;if(++resources==fail_resource)return 0;q=calloc(1,n);assert(q);++live;return q;}
LONG OpenDevice(const char *name,ULONG unit,struct IORequest *q,ULONG flags){struct IOAudio *a=(struct IOAudio *)q;assert(name && !unit && !flags && !a->ioa_Length && !a->ioa_AllocKey);if(++resources==fail_resource)return -1;q->io_Device=&device;++opens;return 0;}
void BeginIO(struct IORequest *q){struct IOAudio *a=(struct IOAudio *)q;++commands;assert(q->io_Device==&device);assert(q->io_Command==ADCMD_ALLOCATE || q->io_Command==ADCMD_SETPREC || q->io_Command==ADCMD_LOCK || q->io_Command==ADCMD_FREE);
 if(q->io_Command==ADCMD_LOCK){assert((uintptr_t)q->io_Unit==15 && a->ioa_AllocKey==7);assert(!q->io_Flags);watch=q;lock_done=0;return;}
 assert(q->io_Flags&IOF_QUICK);active=q;ready=!delayed;if(delayed)q->io_Flags&=~IOF_QUICK;
 if(q->io_Command==ADCMD_ALLOCATE){assert(q->io_Flags&ADIOF_NOWAIT);assert(q->io_Message.mn_Node.ln_Pri==-128 && a->ioa_Length==1 && *a->ioa_Data==15);q->io_Unit=(struct Unit *)(uintptr_t)mask;a->ioa_AllocKey=mask?7:0;}
 else {assert((uintptr_t)q->io_Unit==mask && a->ioa_AllocKey==7);if(q->io_Command==ADCMD_SETPREC)assert(q->io_Message.mn_Node.ln_Pri==127);}
 q->io_Error=q->io_Command==fail_command?-1:0;
 if(q->io_Command==ADCMD_FREE && !q->io_Error && !delayed)lock_done=1;
}
struct IORequest *CheckIO(struct IORequest *q){assert(q==active || q==watch);return (q==watch?lock_done:ready)?q:0;}
LONG WaitIO(struct IORequest *q){assert(CheckIO(q));++waits;if(q==active && q->io_Command==ADCMD_FREE && !q->io_Error)lock_done=1;return q->io_Error;}
void CloseDevice(struct IORequest *q){assert(!q->io_Unit && !((struct IOAudio *)q)->ioa_AllocKey);assert(!watch || lock_done);++closes;}
void DeleteIORequest(struct IORequest *q){assert(q);assert(q!=watch || lock_done);--live;free(q);}
void DeleteMsgPort(struct MsgPort *p){assert(live==1);--live;free(p);}
static void reset(void){assert(!live);opens=closes=waits=commands=ready=delayed=lock_done=fail_command=fail_resource=resources=0;mask=15;active=watch=0;}
static void close_all(struct pt_native_paula_reservation *r){unsigned n;for(n=0;n<8 && !pt_native_paula_reservation_close(r);++n){ready=1;lock_done=1;}assert(n<8 && !live && opens==closes);assert(pt_native_paula_reservation_close(r));}
int main(void){struct pt_native_paula_reservation r;unsigned i,cases=0;
 for(i=1;i<=4;++i){reset();r=(struct pt_native_paula_reservation){0};fail_resource=i;assert(!pt_native_paula_reservation_open(&r));assert(!live && opens==closes);++cases;}
 reset();r=(struct pt_native_paula_reservation){0};assert(pt_native_paula_reservation_open(&r));assert(!commands);close_all(&r);++cases;
 reset();r=(struct pt_native_paula_reservation){0};assert(pt_native_paula_reservation_open(&r));assert(!pt_native_paula_reservation_open(&r));assert(pt_native_paula_reservation_advance(&r)==0);assert(pt_native_paula_reservation_advance(&r)==0);assert(pt_native_paula_reservation_advance(&r)==1);assert(pt_native_paula_reservation_signal(&r)==16);assert(pt_native_paula_reservation_advance(&r)==1);assert(!waits);close_all(&r);++cases;
 reset();r=(struct pt_native_paula_reservation){0};assert(pt_native_paula_reservation_open(&r));delayed=1;assert(pt_native_paula_reservation_advance(&r)==0);assert(pt_native_paula_reservation_advance(&r)==0 && commands==1 && !waits);assert(!pt_native_paula_reservation_close(&r) && live==3);assert(pt_native_paula_reservation_advance(&r)==-1);ready=1;assert(!pt_native_paula_reservation_close(&r) && commands==2);assert(!pt_native_paula_reservation_close(&r) && live==3);close_all(&r);++cases;
 for(i=0;i<2;++i){reset();r=(struct pt_native_paula_reservation){0};mask=i?3:0;assert(pt_native_paula_reservation_open(&r));assert(pt_native_paula_reservation_advance(&r)==0);assert(pt_native_paula_reservation_advance(&r)==-1 && commands==1);close_all(&r);++cases;}
 reset();r=(struct pt_native_paula_reservation){0};fail_command=ADCMD_SETPREC;assert(pt_native_paula_reservation_open(&r));assert(pt_native_paula_reservation_advance(&r)==0);assert(pt_native_paula_reservation_advance(&r)==0);assert(pt_native_paula_reservation_advance(&r)==-1 && !watch);close_all(&r);++cases;
 reset();r=(struct pt_native_paula_reservation){0};assert(pt_native_paula_reservation_open(&r));assert(pt_native_paula_reservation_advance(&r)==0);assert(pt_native_paula_reservation_advance(&r)==0);assert(pt_native_paula_reservation_advance(&r)==1);lock_done=1;assert(pt_native_paula_reservation_advance(&r)==-1);close_all(&r);++cases;
 reset();r=(struct pt_native_paula_reservation){0};assert(pt_native_paula_reservation_open(&r));assert(pt_native_paula_reservation_advance(&r)==0);assert(pt_native_paula_reservation_advance(&r)==0);assert(pt_native_paula_reservation_advance(&r)==1);fail_command=ADCMD_FREE;assert(!pt_native_paula_reservation_close(&r));assert(!pt_native_paula_reservation_close(&r) && live==3 && !closes);assert(!pt_native_paula_reservation_close(&r) && commands==4);/* Model external completion repair only; production never resubmits. */r.command->ioa_Request.io_Error=0;close_all(&r);++cases;
 printf("PAULA RESERVATION HOST PASS: %u ownership cases\n",cases);return 0;}
