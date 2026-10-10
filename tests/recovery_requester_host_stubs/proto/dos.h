#include <pt_host_intuition.h>
struct FileInfoBlock {LONG fib_DirEntryType;char fib_FileName[108];};
#define ACCESS_READ -2
#define ERROR_OBJECT_NOT_FOUND 205
#define ERROR_NO_MORE_ENTRIES 232
LONG GetVar(STRPTR,STRPTR,LONG,ULONG);
LONG IoErr(void);
BPTR Lock(STRPTR,LONG);
void UnLock(BPTR);
LONG Examine(BPTR,struct FileInfoBlock *);
LONG ExNext(BPTR,struct FileInfoBlock *);
LONG NameFromLock(BPTR,STRPTR,LONG);
LONG AddPart(STRPTR,STRPTR,LONG);
