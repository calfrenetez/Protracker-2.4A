#include <stdio.h>
#include <proto/dos.h>
#include "../src/native/paula_reservation.h"
/* Reservation-only fixture. No WRITE/voice/data/DMA operations. One bounded
 * attempt, including expected second-client refusal; no retry/reset/recovery. */
static int close_bounded(struct pt_native_paula_reservation *r)
{unsigned i;for(i=0;i<50;++i){if(pt_native_paula_reservation_close(r))return 1;Delay(1);}return 0;}
static int advance_bounded(struct pt_native_paula_reservation *r)
{unsigned i;int result;for(i=0;i<50;++i){result=pt_native_paula_reservation_advance(r);if(result)return result;Delay(1);}return 0;}
int main(void)
{
    struct pt_native_paula_reservation a={0},b={0};int ready,refused,closed_a,closed_b;
    if(!pt_native_paula_reservation_open(&a) || !close_bounded(&a))return 10;
    if(!pt_native_paula_reservation_open(&a))return 11;
    ready=advance_bounded(&a);
    if(ready!=1){close_bounded(&a);printf("PAULA RESERVATION REFUSED: initial owner unavailable\n");return 12;}
    if(!pt_native_paula_reservation_open(&b)){close_bounded(&a);return 13;}
    refused=advance_bounded(&b);
    /* Contender must refuse all four channels, preserve original key/mask and
     * leave the first owner's theft observer pending. */
    if(refused!=-1 || b.mask || pt_native_paula_reservation_advance(&a)!=1) {
        close_bounded(&b);close_bounded(&a);return 14;
    }
    closed_b=close_bounded(&b);closed_a=close_bounded(&a);
    if(!closed_a || !closed_b || a.port || a.command || a.lock || a.mask || a.locked || a.opened || b.port || b.command || b.lock || b.mask || b.opened)return 15;
    printf("PAULA RESERVATION PASS: 3 native cases; idle close, exclusive ownership, busy refusal; requests/locks/channels/devices/ports closed\n");
    return 0;
}
