#include <stdlib.h>
struct allocation {unsigned calls,fail,live;};
void *repro(void *ctx,size_t n) {struct allocation *a=ctx;void *p;if(++a->calls==a->fail)return 0;p=malloc(n);if(p)++a->live;return p;}
