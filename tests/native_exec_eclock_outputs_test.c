/* One task's private unsubmitted timer request/port only. No output is pointed
 * into the real shared Device/Exec headers; those guards have HOST model tests.
 * A changed private owner is retained, never repaired or disposed on uncertainty. */
#include <stdint.h>
#include <assert.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/native/eclock.h"
#ifndef PT_ECLOCK_OUTPUTS_HOLD
#define PT_ECLOCK_OUTPUTS_HOLD() do {for(;;)Wait(0);} while(0)
#endif
static int unchanged(const struct pt_native_eclock *clock,
    const struct pt_native_eclock *prior,const struct timerequest *request,
    const struct MsgPort *port)
{
    return !memcmp(clock,prior,sizeof(*clock)) &&
        !memcmp(clock->request,request,sizeof(*request)) &&
        !memcmp(clock->port,port,sizeof(*port));
}
int main(void)
{
    struct pt_native_eclock clock={0},prior;
    struct timerequest request;struct MsgPort port;
    struct region {void *data;size_t bytes;} region[3];
    uint64_t ticks=99,previous=0;uint32_t frequency=77,rate=0;
    unsigned i,j,which,cases=0,controls=0;int result;
    assert(sizeof(ticks)==8 && sizeof(frequency)==4);
    assert(sizeof(ticks)==8 && sizeof(frequency)==4);
    if(!pt_native_eclock_open(&clock)) {
        puts("ECLOCK OUTPUT REFUSED: private clock unavailable");return 20;
    }
    prior=clock;memcpy(&request,clock.request,sizeof(request));memcpy(&port,clock.port,sizeof(port));
    region[0]=(struct region){clock.request,sizeof(*clock.request)};
    region[1]=(struct region){clock.port,sizeof(*clock.port)};
    region[2]=(struct region){&clock,sizeof(clock)};
    for(i=0;i<3;++i)for(which=0;which<2;++which) {
        size_t output=which?sizeof(frequency):sizeof(ticks);
        uintptr_t start=(uintptr_t)region[i].data;
        uintptr_t address[4]={start-(output-1),start,start+region[i].bytes/2,start+region[i].bytes-1};
        for(j=0;j<4;++j) {
            ticks=99;frequency=77;
            result=pt_native_eclock_read(&clock,which?&ticks:(uint64_t *)address[j],which?(uint32_t *)address[j]:&frequency);
            if(!unchanged(&clock,&prior,&request,&port))goto uncertain;
            if(result || ticks!=99 || frequency!=77)goto failed;
            ++cases;
            if(!pt_native_eclock_read(&clock,&ticks,&frequency) || !frequency ||
               (rate && (frequency!=rate || ticks<previous)) || !unchanged(&clock,&prior,&request,&port))goto failed;
            previous=ticks;rate=frequency;++controls;
        }
    }
    ticks=99;frequency=77;
    result=pt_native_eclock_read(&clock,&ticks,(uint32_t *)&clock.request->tr_time.tv_micro);
    if(!unchanged(&clock,&prior,&request,&port))goto uncertain;
    if(result || ticks!=99)goto failed;
    ++cases;
    if(!pt_native_eclock_read(&clock,&ticks,&frequency) || frequency!=rate || ticks<previous ||
       !unchanged(&clock,&prior,&request,&port))goto failed;
    ++controls;
    pt_native_eclock_close(&clock);
    if(clock.port || clock.request || clock.opened || cases!=25 || controls!=25)goto uncertain;
    puts("NATIVE ECLOCK OUTPUTS PASS:25 private owner/request/port refusals,25 actual clock controls;bytes retained,closed owner;no device-header writes or timing acceptance");
    return 0;
failed:
    /* Only a still-exact private owner may use ordinary unsubmitted close. */
    if(!unchanged(&clock,&prior,&request,&port))goto uncertain;
    pt_native_eclock_close(&clock);
    puts("ECLOCK OUTPUT FAILED: private owner closed; original clock/result refusal retained");return 20;
uncertain:
    puts("ECLOCK OUTPUT HOLD: private clock state uncertain; owner and process retained");fflush(stdout);
    PT_ECLOCK_OUTPUTS_HOLD();return 20;
}
