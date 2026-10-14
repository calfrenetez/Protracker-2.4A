/* Fixed-case diagnostic for launchers which take no fixture arguments.
 * Keep a distinct executable and acceptance claim from the complete suite. */
#ifndef PT_DOCUMENT_ACTION_CASE
#define PT_DOCUMENT_ACTION_CASE "admit-candidate-overlap"
#endif
#define PT_DOCUMENT_ACTION_TEST_ENTRY pt_document_action_complete_fixture
#include "editor_document_action_focus_test.c"
int main(void)
{
    char program[]="PTDocumentActionCase";
    char command[]="case";
    char selected[]=PT_DOCUMENT_ACTION_CASE;
    char *argv[]={program,command,selected,0};
    return pt_document_action_complete_fixture(3,argv);
}
