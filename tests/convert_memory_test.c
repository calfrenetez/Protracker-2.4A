/* Ceiling permits decoded masters plus bounded file workspace, but refuses
 * an additional whole encoded MOD input or output buffer. */
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#ifndef PT_CONVERT_LIMIT
#define PT_CONVERT_LIMIT 550000
#endif
static size_t live,peak;
union allocation_header {size_t bytes;long double alignment;void *pointer;};
static void *limited_allocate(size_t n)
{
    union allocation_header *p;
    if(n>PT_CONVERT_LIMIT-live)return NULL;
    p=malloc(sizeof(*p)+n);if(!p)return NULL;p->bytes=n;live+=n;if(live>peak)peak=live;return p+1;
}
static void limited_release(void *value)
{union allocation_header *p;if(!value)return;p=(union allocation_header *)value-1;assert(live>=p->bytes);live-=p->bytes;free(p);}
#define malloc limited_allocate
#define free limited_release
#define main convert_main
#include "../tools/pt24g_convert.c"
#undef main
#undef malloc
#undef free
int main(int argc,char **argv)
{
    int rc=convert_main(argc,argv);assert(!live);
    printf("CONVERTER MEMORY: peak=%lu limit=%lu final=0\n",(unsigned long)peak,(unsigned long)PT_CONVERT_LIMIT);return rc;
}
