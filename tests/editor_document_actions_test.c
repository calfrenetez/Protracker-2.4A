/* The thirteen normal-cleanup document actions, excluding prerequisite suites. */
#define PT_DOCUMENT_ACTION_TEST_ENTRY pt_document_action_complete_fixture
#include "editor_document_action_focus_test.c"
int main(void)
{
    char program[]="PTDocumentActions";
    char command[]="actions";
    char *argv[]={program,command,0};
    return pt_document_action_complete_fixture(2,argv);
}
