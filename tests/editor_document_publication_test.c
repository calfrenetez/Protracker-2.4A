/* Dedicated normally-cleaned publication probe, not the full regression suite.
 * Reuse the genuine action, checked-owner and bridge fixture without its CLI.
 * Native builds may localize this fixture TU; production units stay unchanged. */
#define PT_DOCUMENT_ACTION_NO_ENTRY 1
#include "editor_document_action_focus_test.c"

#ifndef PT_DOCUMENT_PUBLICATION_LOAD
#define PT_DOCUMENT_PUBLICATION_LOAD 1
#endif
#if PT_DOCUMENT_PUBLICATION_LOAD != 0 && PT_DOCUMENT_PUBLICATION_LOAD != 1
#error "Publication probe must select New (0) or Load (1)"
#endif
int main(int argc,char **argv)
{
    const char *name=PT_DOCUMENT_PUBLICATION_LOAD?"load":"new";
#if defined(PT_DOCUMENT_ACTION_TRACE) && PT_DOCUMENT_ACTION_TRACE
    if(argc==2){
        unsigned i;const char *path=argv[1];
        /* Only the exact fresh run namespace is a permitted fixture write. */
        assert(strlen(path)==39&&!strncmp(path,"RAM:PT24G-",10)&&
            !strcmp(path+26,"/progress.log"));
        for(i=10;i<26;++i)assert((path[i]>='0'&&path[i]<='9')||(path[i]>='a'&&path[i]<='f'));
        action_trace_path=path;
    }else assert(argc==1);
#else
    (void)argc;(void)argv;
#endif
    assert(printf("DOCUMENT PUBLICATION BEGIN:%s\n",name)>0);
    assert(!fflush(stdout));
    action_trace("entered");
    action_publish(PT_DOCUMENT_PUBLICATION_LOAD,0,0);
    assert(printf("DOCUMENT PUBLICATION PASS:%s; final-address owner and checked cleanup; injected voices\n",name)>0);
    assert(!fflush(stdout));
    return 0;
}
