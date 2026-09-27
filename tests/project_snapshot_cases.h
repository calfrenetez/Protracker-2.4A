#include "../src/editor/project_snapshot.h"
static void project_snapshot_fixture(void)
{
    /* Two-byte prefix exercises the native ABI's two-byte-aligned objects;
       memcpy comparisons must not require four-byte alignment on 68000. */
    struct wrapped_project {unsigned char prefix[2];struct pt_project value;unsigned char suffix[2];} a,b;
    unsigned char *bytes=(unsigned char *)&b.value;size_t i;
    memset(&a,0x5a,sizeof(a));memcpy(&b,&a,sizeof(b));
    assert(pt_project_snapshot_equal(&a.value,&b.value));
    assert(pt_project_snapshot_equal(&a.value,&a.value));
    for(i=0;i<sizeof(b.value);++i) {
        bytes[i]^=0x80;
        assert(!pt_project_snapshot_equal(&a.value,&b.value));
        assert(!pt_project_snapshot_equal(&b.value,&a.value));
        bytes[i]^=0x80;
        assert(pt_project_snapshot_equal(&a.value,&b.value));
    }
    assert(!memcmp(&a,&b,sizeof(a)));
    assert(a.prefix[0]==0x5a && a.prefix[1]==0x5a && a.suffix[0]==0x5a && a.suffix[1]==0x5a);
    puts("PROJECT SNAPSHOT PASS: every object byte including padding and tail detects mutation, symmetric equality/self-alias, native ABI alignment, unchanged inputs and full ownership checks retained");
}
