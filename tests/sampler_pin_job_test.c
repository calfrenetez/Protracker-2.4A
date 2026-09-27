#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/editor/sampler.h"
#include "sampler_pin_job_cases.h"
static unsigned live;
static void *allocate(void *context,size_t bytes){void *p;(void)context;p=malloc(bytes);if(p)++live;return p;}
static void release(void *context,void *p){(void)context;if(p){assert(live);--live;free(p);}}
int main(void){struct pt_allocator a={NULL,allocate,release};pin_job_fixture(&a);assert(!live);return 0;}
