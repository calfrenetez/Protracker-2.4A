/* Discovery only. No card reservation, registers, interrupts or playback. */
#include <stdio.h>
#include "../src/native/amigus_reservation.h"
int main(void)
{
    struct pt_native_amigus_library library={0};
    struct pt_amigus_reservation_api api=pt_native_amigus_reservation_api(&library);
    struct pt_amigus_discovery d;
    unsigned pass;
    api.reserve=0;api.release=0; /* Probe cannot invoke reservation operations. */
    for(pass=0;pass<2;++pass) {
        int result=pt_amigus_discover(&api,&d);
        printf("AMIGUS DISCOVERY pass=%u status=%d library=%u cards=%u pcm_cards=%u closed=%u\n",
               pass,result,d.available,d.cards,d.pcm_cards,library.base==0);
        if(result!=1 || library.base) return 20;
    }
    puts("AMIGUS DISCOVERY PASS: bounded probe completed; playback=NOT_TESTED reservation=NO");
    return 0;
}
