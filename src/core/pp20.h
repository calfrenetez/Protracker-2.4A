#ifndef PT_PP20_H
#define PT_PP20_H
#include <stddef.h>
#include <stdint.h>
enum pt_pp20_result {PT_PP20_OK,PT_PP20_UNSUPPORTED,PT_PP20_INVALID,PT_PP20_CAPACITY,PT_PP20_ALIAS};
/* Full bitstream preflight, then bounded decoding to disjoint output. Failure
   leaves output and written untouched. PX20 encryption is unsupported. */
enum pt_pp20_result pt_pp20_probe(const uint8_t *,size_t,size_t *);
enum pt_pp20_result pt_pp20_decode(const uint8_t *,size_t,uint8_t *,size_t,size_t *);
#endif
