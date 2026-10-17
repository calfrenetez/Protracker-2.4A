/* One separate genuine typed action admission/cancel case; no new producer. */
#include "../src/native/midi_requester.h"
#define PT_MIDI_SETUP_DOCUMENT_TEST_ENTRY original_midi_document_main
#include "midi_setup_document_test.c"
#undef PT_MIDI_SETUP_DOCUMENT_TEST_ENTRY

int main(void)
{
    struct midi_rig *r=midi_make(0);struct action_fixture *a=r->action;
    struct editor_checked_fixture *g=a->f->g;
    struct pt_midi_requester_modal *modal=calloc(1,sizeof(*modal));
    struct pt_editor *editor_before=malloc(sizeof(*editor_before));
    struct pt_document document_before=g->doc;
    struct editor_checked_fixture *checked_before=malloc(sizeof(*checked_before));
    struct pt_editor_document_action *action_before=malloc(sizeof(*action_before));
    unsigned calls=r->memory->calls,releases=r->memory->releases,candidate_calls=a->calls;
    size_t written;uint8_t *encoded;
    assert(modal&&editor_before&&checked_before&&action_before);
    /* Model/binding is separately allocated outside the complete original
     * sampler/record allocator context and all source/control containers. */
#define APART(p,n,q,m) ((uintptr_t)(p)>=(uintptr_t)(q)+(m)||(uintptr_t)(q)>=(uintptr_t)(p)+(n))
    assert(APART(modal,sizeof(*modal),r->memory,sizeof(*r->memory))&&
        APART(modal,sizeof(*modal),r,sizeof(*r))&&APART(modal,sizeof(*modal),a,sizeof(*a))&&
        APART(modal,sizeof(*modal),a->f,sizeof(*a->f))&&APART(modal,sizeof(*modal),g,sizeof(*g))&&
        APART(modal,sizeof(*modal),g->editor,sizeof(*g->editor)));
#undef APART
    assert(pt_midi_requester_modal_begin(modal,r->owner,r->capacity,g->editor,4)==PT_MIDI_SETUP_OK);
    assert(pt_midi_requester_model_select(&modal->model,PT_MIDI_SETUP_INPUT)==PT_MIDI_SETUP_OK);
    assert(pt_midi_requester_model_key(&modal->model,PT_MIDI_KEY_CLEAR,0)==PT_MIDI_EVENT_CHANGED);
    assert(pt_midi_requester_model_key(&modal->model,PT_MIDI_KEY_CHARACTER,'X')==PT_MIDI_EVENT_CHANGED);
    memcpy(editor_before,g->editor,sizeof(*editor_before));memcpy(checked_before,g,sizeof(*g));
    memcpy(action_before,&a->action,sizeof(*action_before));
    /* Real action entry, not just its sidecar admission query. Refusal must
     * precede candidate allocation, source writes, old Stop or voice callbacks. */
    assert(pt_editor_document_action_new(&a->action,&a->inputs,8,SIZE_MAX)==PT_DOCUMENT_ACTION_INVALID);
    assert(!memcmp(action_before,&a->action,sizeof(*action_before))&&
        !memcmp(checked_before,g,sizeof(*g))&&a->calls==candidate_calls&&!a->live&&!a->releases&&
        !a->f->old_releases&&!output_count);
    preserved(r,editor_before,&document_before,calls,releases);
    assert(pt_midi_requester_modal_return(modal)==PT_MIDI_SETUP_OK&&!modal->self);
    assert(pt_editor_document_action_new(&a->action,&a->inputs,8,SIZE_MAX)==PT_DOCUMENT_ACTION_PREPARED);
    assert(pt_editor_document_action_cancel(&a->action)==PT_DOCUMENT_ACTION_CANCELED);
    assert(!a->action.self&&!a->live&&a->calls>candidate_calls&&a->releases==a->calls&&
        !memcmp(checked_before,g,sizeof(*g))&&!a->f->old_releases&&!output_count);
    preserved(r,editor_before,&document_before,calls,releases);
    /* Stop the genuine original checked owner before exercising history. No
     * editor/owner snapshot is reset or silently reused after these edits. */
    assert(pt_editor_mixed_stop(g->binding));
    assert(pt_pattern_undo(&g->doc.project,&g->editor->history,1)==PT_EDIT_OK&&
        !strcmp(g->doc.project.midi_input,"source-two")&&r->memory->records==2);
    assert(pt_pattern_undo(&g->doc.project,&g->editor->history,-1)==PT_EDIT_OK&&
        !strcmp(g->doc.project.midi_input,"source-one")&&g->editor->history.cursor==1);
    encoded=malloc(a->bytes);assert(encoded);
    assert(pt_project_encode(&g->doc.project,encoded,a->bytes,&written)==PT_PROJECT_OK&&written==a->bytes&&
        !memcmp(encoded,a->encoded,a->bytes));
    free(encoded);free(editor_before);free(checked_before);free(action_before);free(modal);
    midi_finish(r);assert(!output_count);
    puts("MIDI REQUESTER DOCUMENT HOST PASS:1 case;real modal blocks action;return prepares/cancels;redo/save/positive disposal;NO NATIVE AUTHORITY");return 0;
}
