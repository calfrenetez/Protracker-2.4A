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
int main(void)
{
    const char *name=PT_DOCUMENT_PUBLICATION_LOAD?"load":"new";
    assert(printf("DOCUMENT PUBLICATION BEGIN:%s\n",name)>0);
    assert(!fflush(stdout));
    action_publish(PT_DOCUMENT_PUBLICATION_LOAD,0,0);
    assert(printf("DOCUMENT PUBLICATION PASS:%s; final-address owner and checked cleanup; injected voices\n",name)>0);
    assert(!fflush(stdout));
    return 0;
}
