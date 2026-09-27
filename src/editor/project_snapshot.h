#ifndef PT_EDITOR_PROJECT_SNAPSHOT_H
#define PT_EDITOR_PROJECT_SNAPSHOT_H
#include "../core/project.h"
#include <string.h>

/* Exact object-representation equality, including padding and every metadata
 * byte. Callers still normalize the permitted channel-selection cursor first.
 * memcpy avoids type-punning/strict-aliasing violations. Native project objects
 * have at least two-byte ABI alignment, sufficient for 68000 longword accesses;
 * advancing by four preserves it. Other targets need no alignment assumption. */
static inline int pt_project_snapshot_equal(const struct pt_project *left,const struct pt_project *right)
{
    const unsigned char *a=(const unsigned char *)left,*b=(const unsigned char *)right;
    size_t i;
    for(i=0;i<sizeof(*left)/sizeof(uint32_t);++i) {
        uint32_t x,y;
#if defined(__GNUC__) && defined(__m68k__)
        memcpy(&x,__builtin_assume_aligned(a,2),sizeof(x));
        memcpy(&y,__builtin_assume_aligned(b,2),sizeof(y));
#else
        memcpy(&x,a,sizeof(x));memcpy(&y,b,sizeof(y));
#endif
        if(x!=y)return 0;
        a+=sizeof(x);b+=sizeof(y);
    }
    for(i=0;i<sizeof(*left)%sizeof(uint32_t);++i)if(a[i]!=b[i])return 0;
    return 1;
}
#endif
