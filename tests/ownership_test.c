#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "ownership.h"

struct mock {
    void *owner;
    unsigned int reserves, frees, checks;
    unsigned int cancel_at, fail_at;
    int overwrite, wrong_free, sticky_free;
};
static unsigned long reserve(void *ctx, unsigned long flag, void *owner)
{
    struct mock *m = ctx;
    ++m->reserves;
    if (m->reserves == m->fail_at) return 0x401;
    if (m->owner && m->owner != owner && !m->overwrite) return 0x100 | flag;
    m->owner = owner;
    return 0;
}
static void release(void *ctx, unsigned long flag, void *owner)
{
    struct mock *m = ctx;
    (void)flag;
    ++m->frees;
    if (!m->sticky_free && (m->owner == owner || m->wrong_free)) m->owner = NULL;
}
static int cancelled(void *ctx)
{
    struct mock *m = ctx;
    return ++m->checks == m->cancel_at;
}
int main(void)
{
    int a, b, foreign;
    unsigned int i;
    struct mock m = {0};
    struct pt_ownership_api api = { &m, reserve, release, cancelled };
    struct pt_ownership_result r;
    for (i = 1; i <= 2; ++i) {
        memset(&m, 0, sizeof(m));
        r = pt_check_ownership(&api, i, &a, &b);
        assert(r.result == PT_PASS && !m.owner && m.reserves == 5);
    }
    memset(&m, 0, sizeof(m)); m.owner = &foreign;
    r = pt_check_ownership(&api, 1, &a, &b);
    assert(r.result == PT_SKIP && m.owner == &foreign && !m.frees);
    for (i = 1; i <= 5; ++i) {
        memset(&m, 0, sizeof(m)); m.fail_at = i;
        r = pt_check_ownership(&api, 2, &a, &b);
        assert(r.result == PT_FAIL && !m.owner);
    }
    for (i = 1; i <= 3; ++i) {
        memset(&m, 0, sizeof(m)); m.cancel_at = i;
        r = pt_check_ownership(&api, 1, &a, &b);
        assert(r.result == PT_FAIL && !m.owner && !strcmp(r.stage, "cancelled"));
    }
    memset(&m, 0, sizeof(m)); m.overwrite = 1;
    r = pt_check_ownership(&api, 1, &a, &b);
    assert(r.result == PT_FAIL && !m.owner && !strcmp(r.stage, "exclude-b"));
    memset(&m, 0, sizeof(m)); m.wrong_free = 1;
    r = pt_check_ownership(&api, 1, &a, &b);
    assert(r.result == PT_FAIL && !m.owner && !strcmp(r.stage, "wrong-owner-release"));
    memset(&m, 0, sizeof(m)); m.sticky_free = 1;
    r = pt_check_ownership(&api, 1, &a, &b);
    assert(r.result == PT_FAIL && !strcmp(r.stage, "reacquire-b"));
    memset(&m, 0, sizeof(m));
    assert(pt_check_ownership(&api, 3, &a, &b).result == PT_FAIL);
    assert(pt_check_ownership(&api, 1, &a, &a).result == PT_FAIL);
    assert(pt_check_ownership(&api, 1, NULL, &b).result == PT_FAIL);
    assert(pt_check_ownership(NULL, 1, &a, &b).result == PT_FAIL);
    assert(m.reserves == 0 && m.frees == 0);
    puts("ownership: 18 scenarios passed (mock driver, not hardware)");
    return 0;
}
