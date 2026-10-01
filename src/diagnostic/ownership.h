/* ProTracker 2.4G diagnostic. No hardware register access in this module. */
#ifndef PT_OWNERSHIP_H
#define PT_OWNERSHIP_H

enum pt_result { PT_PASS = 0, PT_SKIP = 5, PT_FAIL = 20 };
struct pt_ownership_api {
    void *context;
    unsigned long (*reserve)(void *, unsigned long, void *);
    void (*release)(void *, unsigned long, void *);
    int (*cancelled)(void *);
};
struct pt_ownership_result {
    enum pt_result result;
    const char *stage;
    unsigned long driver_code;
    unsigned long release_code;
    int release_confirmed;
    int retained;
};

/* One functional block per call: upstream multi-block reservations can
 * partially succeed. Owners must be distinct, non-null, stable addresses.
 * The pinned driver permits a NULL-owner reserve as an unowned-state probe:
 * it returns busy for any non-NULL owner and only stores NULL when already free.
 * A failed probe requires retaining the library/card/owner addresses. */
struct pt_ownership_result pt_check_ownership(
    const struct pt_ownership_api *, unsigned long, void *, void *);
#endif
