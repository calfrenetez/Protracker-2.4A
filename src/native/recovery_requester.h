#ifndef PT_NATIVE_RECOVERY_REQUESTER_H
#define PT_NATIVE_RECOVERY_REQUESTER_H
#include "recovery_preferences.h"

/* Seven controls in a separate fixed-size window; no main-editor geometry is
 * changed. The portable model owns only a preferences draft and text cursors. */
enum pt_recovery_requester_focus {
    PT_RECOVERY_FOCUS_ENABLED,PT_RECOVERY_FOCUS_DIRECTORY,
    PT_RECOVERY_FOCUS_INTERVAL,PT_RECOVERY_FOCUS_MEDIA,
    PT_RECOVERY_FOCUS_REMOVABLE,PT_RECOVERY_FOCUS_APPLY,
    PT_RECOVERY_FOCUS_CANCEL,PT_RECOVERY_FOCUS_COUNT
};
enum pt_recovery_requester_key {
    PT_RECOVERY_KEY_CHARACTER,PT_RECOVERY_KEY_TAB,PT_RECOVERY_KEY_BACKTAB,
    PT_RECOVERY_KEY_ACTIVATE,PT_RECOVERY_KEY_ESCAPE,PT_RECOVERY_KEY_BACKSPACE,
    PT_RECOVERY_KEY_DELETE,PT_RECOVERY_KEY_LEFT,PT_RECOVERY_KEY_RIGHT,
    PT_RECOVERY_KEY_HOME,PT_RECOVERY_KEY_END,PT_RECOVERY_KEY_CLEAR
};
enum pt_recovery_requester_event {
    PT_RECOVERY_EVENT_NONE,PT_RECOVERY_EVENT_CHANGED,
    PT_RECOVERY_EVENT_APPLY,PT_RECOVERY_EVENT_CANCEL
};
struct pt_recovery_requester_rect {int left,top,right,bottom;};
extern const struct pt_recovery_requester_rect
    pt_recovery_requester_controls[PT_RECOVERY_FOCUS_COUNT];
#define PT_RECOVERY_REQUESTER_WIDTH 552
#define PT_RECOVERY_REQUESTER_HEIGHT 264
struct pt_recovery_requester_model {
    struct pt_native_recovery_preferences preferences;
    char interval[6];
    uint16_t directory_cursor,interval_cursor;
    uint8_t focus;
};
/* Zero-initialize; normal Task only, serialized with all controller/document
 * transitions. Controller/model/source storage must be disjoint. No window,
 * path lookup or controller mutation occurs during open, edit or cancel. */
int pt_recovery_requester_model_open(struct pt_recovery_requester_model *,
    const struct pt_native_recovery *,pt_native_recovery_preferences_idle,void *);
unsigned pt_recovery_requester_hit(int,int);
int pt_recovery_requester_model_focus(struct pt_recovery_requester_model *,unsigned);
enum pt_recovery_requester_event pt_recovery_requester_model_key(
    struct pt_recovery_requester_model *,enum pt_recovery_requester_key,unsigned);
int pt_recovery_requester_model_cancel(struct pt_recovery_requester_model *);
enum pt_native_recovery_preferences_result pt_recovery_requester_model_apply(
    struct pt_recovery_requester_model *,struct pt_native_recovery *,
    const struct pt_native_recovery_source *,pt_native_recovery_preferences_idle,void *);

enum pt_recovery_requester_result {
    PT_RECOVERY_REQUESTER_REFUSED=-2,PT_RECOVERY_REQUESTER_UNAVAILABLE=-1,
    PT_RECOVERY_REQUESTER_CANCELLED,PT_RECOVERY_REQUESTER_NOOP,
    PT_RECOVERY_REQUESTER_APPLIED
};
struct Window;
/* Private own UserPort/window; no editor dispatch, timer IO or recovery polling
 * inside the modal interval. Close/Escape/Cancel discard only this local draft.
 * The caller owns parent input isolation/redraw and serialized idle admission. */
enum pt_recovery_requester_result pt_native_recovery_requester(struct Window *,
    struct pt_native_recovery *,const struct pt_native_recovery_source *,
    pt_native_recovery_preferences_idle,void *);
#endif
