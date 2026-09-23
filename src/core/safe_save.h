#ifndef PT_SAFE_SAVE_H
#define PT_SAFE_SAVE_H
#include <stddef.h>
enum pt_save_result { PT_SAVE_OK, PT_SAVE_INVALID, PT_SAVE_BEGIN, PT_SAVE_WRITE,
                      PT_SAVE_FINISH, PT_SAVE_VERIFY, PT_SAVE_PUBLISH, PT_SAVE_MEMORY };
/* Adapter owns one uniquely created staging file on the destination filesystem.
 * Only publish may change the destination. It must atomically publish or fail
 * with the prior destination intact. abort removes only this operation's temp.
 * finish flushes/closes, verify reads back the complete expected content.
 * A platform without safe publication must fail, not delete the old file.
 */
struct pt_save_ops {
    void *context;
    int (*begin)(void *);
    size_t (*write)(void *,const void *,size_t);
    int (*finish)(void *);
    int (*verify)(void *,const void *,size_t);
    int (*publish)(void *);
    void (*abort)(void *);
};
enum pt_save_result pt_safe_save(const struct pt_save_ops *,const void *,size_t);
#endif
