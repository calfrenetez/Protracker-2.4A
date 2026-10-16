/* Opt-in diagnostic fixture only. Each update closes its separate owned file.
 * Library stdio allocations are outside explicit Fast/Chip pool accounting. */
#include <stdio.h>
#include <string.h>
static const char *readers_memory_trace_path;
static unsigned readers_memory_trace_count;
static void readers_memory_trace_begin(int argc,char **argv)
{
    unsigned i;const char *path;
    assert(argc==2);path=argv[1];
    assert(strlen(path)==39&&!strncmp(path,"RAM:PT24G-",10)&&
        !strcmp(path+26,"/progress.log"));
    for(i=10;i<26;++i)assert((path[i]>='0'&&path[i]<='9')||(path[i]>='a'&&path[i]<='f'));
    readers_memory_trace_path=path;
}
static void readers_memory_trace(const char *stage,unsigned x,unsigned y,unsigned n)
{
    FILE *progress;assert(readers_memory_trace_path&&++readers_memory_trace_count<=128);
    progress=fopen(readers_memory_trace_path,"w");assert(progress);
    assert(fprintf(progress,"READERS MEMORY TRACE:%s:%u:%u:%u\n",stage,x,y,n)>0);
    assert(!fclose(progress));
    assert(printf("READERS MEMORY TRACE:%s:%u:%u:%u\n",stage,x,y,n)>0);
    assert(!fflush(stdout));
}
