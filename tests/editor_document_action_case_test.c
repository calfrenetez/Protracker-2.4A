/* Fixed first-case diagnostic for launchers which take no fixture arguments.
 * Keep a distinct executable and acceptance claim from the complete suite. */
#define PT_DOCUMENT_ACTION_TEST_ENTRY pt_document_action_complete_fixture
#include "editor_document_action_focus_test.c"
int main(void)
{
    char program[]="PTDocumentActionCase";
    char command[]="case";
    char selected[]="admit-candidate-overlap";
    char *argv[]={program,command,selected,0};
    return pt_document_action_complete_fixture(3,argv);
}
