#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
static size_t live,peak,calls,fail;
union header {size_t n;long double alignment;void *pointer;};
static void *limited_allocate(size_t n)
{
    union header *p;if(++calls==fail || n>700000-live)return NULL;
    p=malloc(sizeof(*p)+n);if(!p)return NULL;p->n=n;live+=n;if(live>peak)peak=live;return p+1;
}
static void limited_release(void *value)
{union header *p;if(!value)return;p=(union header *)value-1;assert(live>=p->n);live-=p->n;free(p);}
#define malloc limited_allocate
#define free limited_release
#define main render_main
#include "../tools/pt24g_render.c"
#undef main
#undef malloc
#undef free
int main(int argc,char **argv)
{
    const char *value=getenv("PT_TEST_FAIL");int rc;if(value)fail=(size_t)strtoul(value,NULL,10);
    rc=render_main(argc,argv);assert(!live);
    printf("RENDER MEMORY peak=%lu calls=%lu final=0\n",(unsigned long)peak,(unsigned long)calls);return rc;
}
