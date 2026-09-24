#ifndef PT_NATIVE_AMIGUS_RESERVATION_H
#define PT_NATIVE_AMIGUS_RESERVATION_H
#include "../core/amigus_reservation.h"
struct Library;
/* Zero-initialize; keep address stable until core owner closes. No global base.
 * Library-returned card descriptors remain private to the reservation lifecycle;
 * no register addresses, capacity guesses or playback-enable API are exposed. */
struct pt_native_amigus_library { struct Library *base; };
struct pt_amigus_reservation_api pt_native_amigus_reservation_api(
    struct pt_native_amigus_library *);
#endif
