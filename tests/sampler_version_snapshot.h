#ifndef PT_TEST_SAMPLER_VERSION_SNAPSHOT_H
#define PT_TEST_SAMPLER_VERSION_SNAPSHOT_H
#include <stddef.h>
/* Test-only complete header comparison except the actual private reference
 * counter and two intrusive list links. Never a production ownership grant. */
int pt_test_version_header_equal(const void *,const void *,size_t);
void pt_test_version_header_fixture(void);
#endif
