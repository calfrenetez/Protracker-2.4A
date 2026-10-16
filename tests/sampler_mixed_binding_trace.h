/* Opt-in diagnostic fixture only. Same bounded owned-RAM argument convention
 * as the qualified reader trace. Each update closes its separate file before
 * flushing a stdout observation; stdio allocations are outside test ledgers. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
static const char *mixed_binding_trace_path;
static unsigned mixed_binding_trace_count;
static void mixed_binding_trace_begin(int argc,char **argv)
{
 unsigned i;const char *path;assert(argc==2);path=argv[1];
 assert(strlen(path)==39&&!strncmp(path,"RAM:PT24G-",10)&&!strcmp(path+26,"/progress.log"));
 for(i=10;i<26;++i)assert((path[i]>='0'&&path[i]<='9')||(path[i]>='a'&&path[i]<='f'));
 mixed_binding_trace_path=path;
}
static void mixed_binding_trace(const char *stage,unsigned bits,unsigned channels,unsigned n)
{
 FILE *f;assert(mixed_binding_trace_path&&++mixed_binding_trace_count<=128);
 f=fopen(mixed_binding_trace_path,"w");assert(f);
 assert(fprintf(f,"MIXED BINDING TRACE:%s:%u:%u:%u\n",stage,bits,channels,n)>0);
 assert(!fclose(f));
 assert(printf("MIXED BINDING TRACE:%s:%u:%u:%u\n",stage,bits,channels,n)>0);
 assert(!fflush(stdout));
}
