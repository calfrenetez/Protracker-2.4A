/* Automatically generated header (sfdc 1.12)! Do not edit! */

#ifndef _INLINE_DOS_H
#define _INLINE_DOS_H

#ifndef _PROTO_DOS_H
#include <proto/dos.h>
#endif

#if defined(__GNUC__)
# if (__GNUC__ >= 8)
#  define AMIGA_VA_WRAPPER_ATTR \
    __attribute__((noipa, noinline, optimize("omit-frame-pointer"), optimize("O1")))
# else
#  define AMIGA_VA_WRAPPER_ATTR \
    __attribute__((noinline, optimize("omit-frame-pointer"), optimize("O1")))
# endif
#else
# define AMIGA_VA_WRAPPER_ATTR
#endif

#ifndef DOS_BASE_NAME
#define DOS_BASE_NAME DOSBase
#endif /* !DOS_BASE_NAME */

#define __Open_base(__in_base, ___name, ___accessMode) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  STRPTR __p____name = (STRPTR)(___name);\
  LONG __p____accessMode = (LONG)(___accessMode);\
  register BPTR __v_ret __asm("d0");\
  register STRPTR __v0 __asm("d1") = __p____name;\
  register LONG __v1 __asm("d2") = __p____accessMode;\
  __asm volatile (\
                   "jsr %%a6@(-30:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define Open(___name, ___accessMode) ({\
  __Open_base((DOS_BASE_NAME), ___name, ___accessMode);\
})

#define __Close_base(__in_base, ___file) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  BPTR __p____file = (BPTR)(___file);\
  register LONG __v_ret __asm("d0");\
  register BPTR __v0 __asm("d1") = __p____file;\
  __asm volatile (\
                   "jsr %%a6@(-36:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define Close(___file) ({\
  __Close_base((DOS_BASE_NAME), ___file);\
})

#define __Read_base(__in_base, ___file, ___buffer, ___length) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  BPTR __p____file = (BPTR)(___file);\
  APTR __p____buffer = (APTR)(___buffer);\
  LONG __p____length = (LONG)(___length);\
  register LONG __v_ret __asm("d0");\
  register BPTR __v0 __asm("d1") = __p____file;\
  register APTR __v1 __asm("d2") = __p____buffer;\
  register LONG __v2 __asm("d3") = __p____length;\
  __asm volatile (\
                   "jsr %%a6@(-42:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1), "d"(__v2)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define Read(___file, ___buffer, ___length) ({\
  __Read_base((DOS_BASE_NAME), ___file, ___buffer, ___length);\
})

#define __Write_base(__in_base, ___file, ___buffer, ___length) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  BPTR __p____file = (BPTR)(___file);\
  APTR __p____buffer = (APTR)(___buffer);\
  LONG __p____length = (LONG)(___length);\
  register LONG __v_ret __asm("d0");\
  register BPTR __v0 __asm("d1") = __p____file;\
  register APTR __v1 __asm("d2") = __p____buffer;\
  register LONG __v2 __asm("d3") = __p____length;\
  __asm volatile (\
                   "jsr %%a6@(-48:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1), "d"(__v2)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define Write(___file, ___buffer, ___length) ({\
  __Write_base((DOS_BASE_NAME), ___file, ___buffer, ___length);\
})

#define __Input_base(__in_base) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  register BPTR __v_ret __asm("d0");\
  __asm volatile (\
                   "jsr %%a6@(-54:W)\n"\
                   : "=d"(__v_ret)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d1", "a0", "a1" );\
  __v_ret;})

#define Input() ({\
  __Input_base((DOS_BASE_NAME));\
})

#define __Output_base(__in_base) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  register BPTR __v_ret __asm("d0");\
  __asm volatile (\
                   "jsr %%a6@(-60:W)\n"\
                   : "=d"(__v_ret)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d1", "a0", "a1" );\
  __v_ret;})

#define Output() ({\
  __Output_base((DOS_BASE_NAME));\
})

#define __Seek_base(__in_base, ___file, ___position, ___offset) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  BPTR __p____file = (BPTR)(___file);\
  LONG __p____position = (LONG)(___position);\
  LONG __p____offset = (LONG)(___offset);\
  register LONG __v_ret __asm("d0");\
  register BPTR __v0 __asm("d1") = __p____file;\
  register LONG __v1 __asm("d2") = __p____position;\
  register LONG __v2 __asm("d3") = __p____offset;\
  __asm volatile (\
                   "jsr %%a6@(-66:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1), "d"(__v2)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define Seek(___file, ___position, ___offset) ({\
  __Seek_base((DOS_BASE_NAME), ___file, ___position, ___offset);\
})

#define __DeleteFile_base(__in_base, ___name) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  STRPTR __p____name = (STRPTR)(___name);\
  register LONG __v_ret __asm("d0");\
  register STRPTR __v0 __asm("d1") = __p____name;\
  __asm volatile (\
                   "jsr %%a6@(-72:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define DeleteFile(___name) ({\
  __DeleteFile_base((DOS_BASE_NAME), ___name);\
})

#define __Rename_base(__in_base, ___oldName, ___newName) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  STRPTR __p____oldName = (STRPTR)(___oldName);\
  STRPTR __p____newName = (STRPTR)(___newName);\
  register LONG __v_ret __asm("d0");\
  register STRPTR __v0 __asm("d1") = __p____oldName;\
  register STRPTR __v1 __asm("d2") = __p____newName;\
  __asm volatile (\
                   "jsr %%a6@(-78:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define Rename(___oldName, ___newName) ({\
  __Rename_base((DOS_BASE_NAME), ___oldName, ___newName);\
})

#define __Lock_base(__in_base, ___name, ___type) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  STRPTR __p____name = (STRPTR)(___name);\
  LONG __p____type = (LONG)(___type);\
  register BPTR __v_ret __asm("d0");\
  register STRPTR __v0 __asm("d1") = __p____name;\
  register LONG __v1 __asm("d2") = __p____type;\
  __asm volatile (\
                   "jsr %%a6@(-84:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define Lock(___name, ___type) ({\
  __Lock_base((DOS_BASE_NAME), ___name, ___type);\
})

#define __UnLock_base(__in_base, ___lock) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  BPTR __p____lock = (BPTR)(___lock);\
  register BPTR __v0 __asm("d1") = __p____lock;\
  __asm volatile (\
                   "jsr %%a6@(-90:W)\n"\
                   : "+d"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d0", "a0", "a1" );\
})

#define UnLock(___lock) ({\
  __UnLock_base((DOS_BASE_NAME), ___lock);\
})

#define __DupLock_base(__in_base, ___lock) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  BPTR __p____lock = (BPTR)(___lock);\
  register BPTR __v_ret __asm("d0");\
  register BPTR __v0 __asm("d1") = __p____lock;\
  __asm volatile (\
                   "jsr %%a6@(-96:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define DupLock(___lock) ({\
  __DupLock_base((DOS_BASE_NAME), ___lock);\
})

#define __Examine_base(__in_base, ___lock, ___fileInfoBlock) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  BPTR __p____lock = (BPTR)(___lock);\
  struct FileInfoBlock * __p____fileInfoBlock = (struct FileInfoBlock *)(___fileInfoBlock);\
  register LONG __v_ret __asm("d0");\
  register BPTR __v0 __asm("d1") = __p____lock;\
  register struct FileInfoBlock * __v1 __asm("d2") = __p____fileInfoBlock;\
  __asm volatile (\
                   "jsr %%a6@(-102:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define Examine(___lock, ___fileInfoBlock) ({\
  __Examine_base((DOS_BASE_NAME), ___lock, ___fileInfoBlock);\
})

#define __ExNext_base(__in_base, ___lock, ___fileInfoBlock) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  BPTR __p____lock = (BPTR)(___lock);\
  struct FileInfoBlock * __p____fileInfoBlock = (struct FileInfoBlock *)(___fileInfoBlock);\
  register LONG __v_ret __asm("d0");\
  register BPTR __v0 __asm("d1") = __p____lock;\
  register struct FileInfoBlock * __v1 __asm("d2") = __p____fileInfoBlock;\
  __asm volatile (\
                   "jsr %%a6@(-108:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define ExNext(___lock, ___fileInfoBlock) ({\
  __ExNext_base((DOS_BASE_NAME), ___lock, ___fileInfoBlock);\
})

#define __Info_base(__in_base, ___lock, ___parameterBlock) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  BPTR __p____lock = (BPTR)(___lock);\
  struct InfoData * __p____parameterBlock = (struct InfoData *)(___parameterBlock);\
  register LONG __v_ret __asm("d0");\
  register BPTR __v0 __asm("d1") = __p____lock;\
  register struct InfoData * __v1 __asm("d2") = __p____parameterBlock;\
  __asm volatile (\
                   "jsr %%a6@(-114:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define Info(___lock, ___parameterBlock) ({\
  __Info_base((DOS_BASE_NAME), ___lock, ___parameterBlock);\
})

#define __CreateDir_base(__in_base, ___name) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  STRPTR __p____name = (STRPTR)(___name);\
  register BPTR __v_ret __asm("d0");\
  register STRPTR __v0 __asm("d1") = __p____name;\
  __asm volatile (\
                   "jsr %%a6@(-120:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define CreateDir(___name) ({\
  __CreateDir_base((DOS_BASE_NAME), ___name);\
})

#define __CurrentDir_base(__in_base, ___lock) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  BPTR __p____lock = (BPTR)(___lock);\
  register BPTR __v_ret __asm("d0");\
  register BPTR __v0 __asm("d1") = __p____lock;\
  __asm volatile (\
                   "jsr %%a6@(-126:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define CurrentDir(___lock) ({\
  __CurrentDir_base((DOS_BASE_NAME), ___lock);\
})

#define __IoErr_base(__in_base) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  register LONG __v_ret __asm("d0");\
  __asm volatile (\
                   "jsr %%a6@(-132:W)\n"\
                   : "=d"(__v_ret)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d1", "a0", "a1" );\
  __v_ret;})

#define IoErr() ({\
  __IoErr_base((DOS_BASE_NAME));\
})

#define __CreateProc_base(__in_base, ___name, ___pri, ___segList, ___stackSize) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  STRPTR __p____name = (STRPTR)(___name);\
  LONG __p____pri = (LONG)(___pri);\
  BPTR __p____segList = (BPTR)(___segList);\
  LONG __p____stackSize = (LONG)(___stackSize);\
  register struct MsgPort * __v_ret __asm("d0");\
  register STRPTR __v0 __asm("d1") = __p____name;\
  register LONG __v1 __asm("d2") = __p____pri;\
  register BPTR __v2 __asm("d3") = __p____segList;\
  register LONG __v3 __asm("d4") = __p____stackSize;\
  __asm volatile (\
                   "jsr %%a6@(-138:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1), "d"(__v2), "d"(__v3)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define CreateProc(___name, ___pri, ___segList, ___stackSize) ({\
  __CreateProc_base((DOS_BASE_NAME), ___name, ___pri, ___segList, ___stackSize);\
})

#define __Exit_base(__in_base, ___returnCode) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  LONG __p____returnCode = (LONG)(___returnCode);\
  register LONG __v0 __asm("d1") = __p____returnCode;\
  __asm volatile (\
                   "jsr %%a6@(-144:W)\n"\
                   : "+d"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d0", "a0", "a1" );\
})

#define Exit(___returnCode) ({\
  __Exit_base((DOS_BASE_NAME), ___returnCode);\
})

#define __LoadSeg_base(__in_base, ___name) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  STRPTR __p____name = (STRPTR)(___name);\
  register BPTR __v_ret __asm("d0");\
  register STRPTR __v0 __asm("d1") = __p____name;\
  __asm volatile (\
                   "jsr %%a6@(-150:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define LoadSeg(___name) ({\
  __LoadSeg_base((DOS_BASE_NAME), ___name);\
})

#define __UnLoadSeg_base(__in_base, ___seglist) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  BPTR __p____seglist = (BPTR)(___seglist);\
  register BPTR __v0 __asm("d1") = __p____seglist;\
  __asm volatile (\
                   "jsr %%a6@(-156:W)\n"\
                   : "+d"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d0", "a0", "a1" );\
})

#define UnLoadSeg(___seglist) ({\
  __UnLoadSeg_base((DOS_BASE_NAME), ___seglist);\
})

#define __DeviceProc_base(__in_base, ___name) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  STRPTR __p____name = (STRPTR)(___name);\
  register struct MsgPort * __v_ret __asm("d0");\
  register STRPTR __v0 __asm("d1") = __p____name;\
  __asm volatile (\
                   "jsr %%a6@(-174:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define DeviceProc(___name) ({\
  __DeviceProc_base((DOS_BASE_NAME), ___name);\
})

#define __SetComment_base(__in_base, ___name, ___comment) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  STRPTR __p____name = (STRPTR)(___name);\
  STRPTR __p____comment = (STRPTR)(___comment);\
  register LONG __v_ret __asm("d0");\
  register STRPTR __v0 __asm("d1") = __p____name;\
  register STRPTR __v1 __asm("d2") = __p____comment;\
  __asm volatile (\
                   "jsr %%a6@(-180:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define SetComment(___name, ___comment) ({\
  __SetComment_base((DOS_BASE_NAME), ___name, ___comment);\
})

#define __SetProtection_base(__in_base, ___name, ___protect) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  STRPTR __p____name = (STRPTR)(___name);\
  LONG __p____protect = (LONG)(___protect);\
  register LONG __v_ret __asm("d0");\
  register STRPTR __v0 __asm("d1") = __p____name;\
  register LONG __v1 __asm("d2") = __p____protect;\
  __asm volatile (\
                   "jsr %%a6@(-186:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define SetProtection(___name, ___protect) ({\
  __SetProtection_base((DOS_BASE_NAME), ___name, ___protect);\
})

#define __DateStamp_base(__in_base, ___date) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct DateStamp * __p____date = (struct DateStamp *)(___date);\
  register struct DateStamp * __v_ret __asm("d0");\
  register struct DateStamp * __v0 __asm("d1") = __p____date;\
  __asm volatile (\
                   "jsr %%a6@(-192:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define DateStamp(___date) ({\
  __DateStamp_base((DOS_BASE_NAME), ___date);\
})

#define __Delay_base(__in_base, ___timeout) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  LONG __p____timeout = (LONG)(___timeout);\
  register LONG __v0 __asm("d1") = __p____timeout;\
  __asm volatile (\
                   "jsr %%a6@(-198:W)\n"\
                   : "+d"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d0", "a0", "a1" );\
})

#define Delay(___timeout) ({\
  __Delay_base((DOS_BASE_NAME), ___timeout);\
})

#define __WaitForChar_base(__in_base, ___file, ___timeout) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  BPTR __p____file = (BPTR)(___file);\
  LONG __p____timeout = (LONG)(___timeout);\
  register LONG __v_ret __asm("d0");\
  register BPTR __v0 __asm("d1") = __p____file;\
  register LONG __v1 __asm("d2") = __p____timeout;\
  __asm volatile (\
                   "jsr %%a6@(-204:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define WaitForChar(___file, ___timeout) ({\
  __WaitForChar_base((DOS_BASE_NAME), ___file, ___timeout);\
})

#define __ParentDir_base(__in_base, ___lock) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  BPTR __p____lock = (BPTR)(___lock);\
  register BPTR __v_ret __asm("d0");\
  register BPTR __v0 __asm("d1") = __p____lock;\
  __asm volatile (\
                   "jsr %%a6@(-210:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define ParentDir(___lock) ({\
  __ParentDir_base((DOS_BASE_NAME), ___lock);\
})

#define __IsInteractive_base(__in_base, ___file) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  BPTR __p____file = (BPTR)(___file);\
  register LONG __v_ret __asm("d0");\
  register BPTR __v0 __asm("d1") = __p____file;\
  __asm volatile (\
                   "jsr %%a6@(-216:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define IsInteractive(___file) ({\
  __IsInteractive_base((DOS_BASE_NAME), ___file);\
})

#define __Execute_base(__in_base, ___string, ___file, ___file2) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  STRPTR __p____string = (STRPTR)(___string);\
  BPTR __p____file = (BPTR)(___file);\
  BPTR __p____file2 = (BPTR)(___file2);\
  register LONG __v_ret __asm("d0");\
  register STRPTR __v0 __asm("d1") = __p____string;\
  register BPTR __v1 __asm("d2") = __p____file;\
  register BPTR __v2 __asm("d3") = __p____file2;\
  __asm volatile (\
                   "jsr %%a6@(-222:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1), "d"(__v2)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define Execute(___string, ___file, ___file2) ({\
  __Execute_base((DOS_BASE_NAME), ___string, ___file, ___file2);\
})

#define __AllocDosObject_base(__in_base, ___type, ___tags) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  ULONG __p____type = (ULONG)(___type);\
  struct TagItem * __p____tags = (struct TagItem *)(___tags);\
  register APTR __v_ret __asm("d0");\
  register ULONG __v0 __asm("d1") = __p____type;\
  register struct TagItem * __v1 __asm("d2") = __p____tags;\
  __asm volatile (\
                   "jsr %%a6@(-228:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define AllocDosObject(___type, ___tags) ({\
  __AllocDosObject_base((DOS_BASE_NAME), ___type, ___tags);\
})

#define __AllocDosObjectTagList_base(__in_base, ___type, ___tags) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  ULONG __p____type = (ULONG)(___type);\
  struct TagItem * __p____tags = (struct TagItem *)(___tags);\
  register APTR __v_ret __asm("d0");\
  register ULONG __v0 __asm("d1") = __p____type;\
  register struct TagItem * __v1 __asm("d2") = __p____tags;\
  __asm volatile (\
                   "jsr %%a6@(-228:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define AllocDosObjectTagList(___type, ___tags) ({\
  __AllocDosObjectTagList_base((DOS_BASE_NAME), ___type, ___tags);\
})

AMIGA_VA_WRAPPER_ATTR
static __stdargs APTR __AllocDosObjectTags_va(void *const __base __asm("a6"), ULONG ___type, ULONG ___tag1type, ...)
{
    const ULONG *tags = (const ULONG *)&___tag1type;
    return __AllocDosObjectTagList_base(__base, ___type, (CONST struct TagItem *)tags);
}

#define AllocDosObjectTags(...) __AllocDosObjectTags_va(DOS_BASE_NAME, __VA_ARGS__)

#define __FreeDosObject_base(__in_base, ___type, ___ptr) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  ULONG __p____type = (ULONG)(___type);\
  APTR __p____ptr = (APTR)(___ptr);\
  register ULONG __v0 __asm("d1") = __p____type;\
  register APTR __v1 __asm("d2") = __p____ptr;\
  __asm volatile (\
                   "jsr %%a6@(-234:W)\n"\
                   : "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1)\
                   : "fp0", "fp1", "cc", "memory", "d0", "a0", "a1" );\
})

#define FreeDosObject(___type, ___ptr) ({\
  __FreeDosObject_base((DOS_BASE_NAME), ___type, ___ptr);\
})

#define __DoPkt_base(__in_base, ___port, ___action, ___arg1, ___arg2, ___arg3, ___arg4, ___arg5) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct MsgPort * __p____port = (struct MsgPort *)(___port);\
  LONG __p____action = (LONG)(___action);\
  LONG __p____arg1 = (LONG)(___arg1);\
  LONG __p____arg2 = (LONG)(___arg2);\
  LONG __p____arg3 = (LONG)(___arg3);\
  LONG __p____arg4 = (LONG)(___arg4);\
  LONG __p____arg5 = (LONG)(___arg5);\
  register LONG __v_ret __asm("d0");\
  register struct MsgPort * __v0 __asm("d1") = __p____port;\
  register LONG __v1 __asm("d2") = __p____action;\
  register LONG __v2 __asm("d3") = __p____arg1;\
  register LONG __v3 __asm("d4") = __p____arg2;\
  register LONG __v4 __asm("d5") = __p____arg3;\
  register LONG __v5 __asm("d6") = __p____arg4;\
  register LONG __v6 __asm("d7") = __p____arg5;\
  __asm volatile (\
                   "jsr %%a6@(-240:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1), "d"(__v2), "d"(__v3), "d"(__v4), "d"(__v5), "d"(__v6)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define DoPkt(___port, ___action, ___arg1, ___arg2, ___arg3, ___arg4, ___arg5) ({\
  __DoPkt_base((DOS_BASE_NAME), ___port, ___action, ___arg1, ___arg2, ___arg3, ___arg4, ___arg5);\
})

#define __DoPkt0_base(__in_base, ___port, ___action) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct MsgPort * __p____port = (struct MsgPort *)(___port);\
  LONG __p____action = (LONG)(___action);\
  register LONG __v_ret __asm("d0");\
  register struct MsgPort * __v0 __asm("d1") = __p____port;\
  register LONG __v1 __asm("d2") = __p____action;\
  __asm volatile (\
                   "jsr %%a6@(-240:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define DoPkt0(___port, ___action) ({\
  __DoPkt0_base((DOS_BASE_NAME), ___port, ___action);\
})

#define __DoPkt1_base(__in_base, ___port, ___action, ___arg1) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct MsgPort * __p____port = (struct MsgPort *)(___port);\
  LONG __p____action = (LONG)(___action);\
  LONG __p____arg1 = (LONG)(___arg1);\
  register LONG __v_ret __asm("d0");\
  register struct MsgPort * __v0 __asm("d1") = __p____port;\
  register LONG __v1 __asm("d2") = __p____action;\
  register LONG __v2 __asm("d3") = __p____arg1;\
  __asm volatile (\
                   "jsr %%a6@(-240:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1), "d"(__v2)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define DoPkt1(___port, ___action, ___arg1) ({\
  __DoPkt1_base((DOS_BASE_NAME), ___port, ___action, ___arg1);\
})

#define __DoPkt2_base(__in_base, ___port, ___action, ___arg1, ___arg2) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct MsgPort * __p____port = (struct MsgPort *)(___port);\
  LONG __p____action = (LONG)(___action);\
  LONG __p____arg1 = (LONG)(___arg1);\
  LONG __p____arg2 = (LONG)(___arg2);\
  register LONG __v_ret __asm("d0");\
  register struct MsgPort * __v0 __asm("d1") = __p____port;\
  register LONG __v1 __asm("d2") = __p____action;\
  register LONG __v2 __asm("d3") = __p____arg1;\
  register LONG __v3 __asm("d4") = __p____arg2;\
  __asm volatile (\
                   "jsr %%a6@(-240:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1), "d"(__v2), "d"(__v3)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define DoPkt2(___port, ___action, ___arg1, ___arg2) ({\
  __DoPkt2_base((DOS_BASE_NAME), ___port, ___action, ___arg1, ___arg2);\
})

#define __DoPkt3_base(__in_base, ___port, ___action, ___arg1, ___arg2, ___arg3) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct MsgPort * __p____port = (struct MsgPort *)(___port);\
  LONG __p____action = (LONG)(___action);\
  LONG __p____arg1 = (LONG)(___arg1);\
  LONG __p____arg2 = (LONG)(___arg2);\
  LONG __p____arg3 = (LONG)(___arg3);\
  register LONG __v_ret __asm("d0");\
  register struct MsgPort * __v0 __asm("d1") = __p____port;\
  register LONG __v1 __asm("d2") = __p____action;\
  register LONG __v2 __asm("d3") = __p____arg1;\
  register LONG __v3 __asm("d4") = __p____arg2;\
  register LONG __v4 __asm("d5") = __p____arg3;\
  __asm volatile (\
                   "jsr %%a6@(-240:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1), "d"(__v2), "d"(__v3), "d"(__v4)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define DoPkt3(___port, ___action, ___arg1, ___arg2, ___arg3) ({\
  __DoPkt3_base((DOS_BASE_NAME), ___port, ___action, ___arg1, ___arg2, ___arg3);\
})

#define __DoPkt4_base(__in_base, ___port, ___action, ___arg1, ___arg2, ___arg3, ___arg4) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct MsgPort * __p____port = (struct MsgPort *)(___port);\
  LONG __p____action = (LONG)(___action);\
  LONG __p____arg1 = (LONG)(___arg1);\
  LONG __p____arg2 = (LONG)(___arg2);\
  LONG __p____arg3 = (LONG)(___arg3);\
  LONG __p____arg4 = (LONG)(___arg4);\
  register LONG __v_ret __asm("d0");\
  register struct MsgPort * __v0 __asm("d1") = __p____port;\
  register LONG __v1 __asm("d2") = __p____action;\
  register LONG __v2 __asm("d3") = __p____arg1;\
  register LONG __v3 __asm("d4") = __p____arg2;\
  register LONG __v4 __asm("d5") = __p____arg3;\
  register LONG __v5 __asm("d6") = __p____arg4;\
  __asm volatile (\
                   "jsr %%a6@(-240:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1), "d"(__v2), "d"(__v3), "d"(__v4), "d"(__v5)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define DoPkt4(___port, ___action, ___arg1, ___arg2, ___arg3, ___arg4) ({\
  __DoPkt4_base((DOS_BASE_NAME), ___port, ___action, ___arg1, ___arg2, ___arg3, ___arg4);\
})

#define __SendPkt_base(__in_base, ___dp, ___port, ___replyport) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct DosPacket * __p____dp = (struct DosPacket *)(___dp);\
  struct MsgPort * __p____port = (struct MsgPort *)(___port);\
  struct MsgPort * __p____replyport = (struct MsgPort *)(___replyport);\
  register struct DosPacket * __v0 __asm("d1") = __p____dp;\
  register struct MsgPort * __v1 __asm("d2") = __p____port;\
  register struct MsgPort * __v2 __asm("d3") = __p____replyport;\
  __asm volatile (\
                   "jsr %%a6@(-246:W)\n"\
                   : "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1), "d"(__v2)\
                   : "fp0", "fp1", "cc", "memory", "d0", "a0", "a1" );\
})

#define SendPkt(___dp, ___port, ___replyport) ({\
  __SendPkt_base((DOS_BASE_NAME), ___dp, ___port, ___replyport);\
})

#define __WaitPkt_base(__in_base) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  register struct DosPacket * __v_ret __asm("d0");\
  __asm volatile (\
                   "jsr %%a6@(-252:W)\n"\
                   : "=d"(__v_ret)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d1", "a0", "a1" );\
  __v_ret;})

#define WaitPkt() ({\
  __WaitPkt_base((DOS_BASE_NAME));\
})

#define __ReplyPkt_base(__in_base, ___dp, ___res1, ___res2) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct DosPacket * __p____dp = (struct DosPacket *)(___dp);\
  LONG __p____res1 = (LONG)(___res1);\
  LONG __p____res2 = (LONG)(___res2);\
  register struct DosPacket * __v0 __asm("d1") = __p____dp;\
  register LONG __v1 __asm("d2") = __p____res1;\
  register LONG __v2 __asm("d3") = __p____res2;\
  __asm volatile (\
                   "jsr %%a6@(-258:W)\n"\
                   : "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1), "d"(__v2)\
                   : "fp0", "fp1", "cc", "memory", "d0", "a0", "a1" );\
})

#define ReplyPkt(___dp, ___res1, ___res2) ({\
  __ReplyPkt_base((DOS_BASE_NAME), ___dp, ___res1, ___res2);\
})

#define __AbortPkt_base(__in_base, ___port, ___pkt) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct MsgPort * __p____port = (struct MsgPort *)(___port);\
  struct DosPacket * __p____pkt = (struct DosPacket *)(___pkt);\
  register struct MsgPort * __v0 __asm("d1") = __p____port;\
  register struct DosPacket * __v1 __asm("d2") = __p____pkt;\
  __asm volatile (\
                   "jsr %%a6@(-264:W)\n"\
                   : "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1)\
                   : "fp0", "fp1", "cc", "memory", "d0", "a0", "a1" );\
})

#define AbortPkt(___port, ___pkt) ({\
  __AbortPkt_base((DOS_BASE_NAME), ___port, ___pkt);\
})

#define __LockRecord_base(__in_base, ___fh, ___offset, ___length, ___mode, ___timeout) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  BPTR __p____fh = (BPTR)(___fh);\
  ULONG __p____offset = (ULONG)(___offset);\
  ULONG __p____length = (ULONG)(___length);\
  ULONG __p____mode = (ULONG)(___mode);\
  ULONG __p____timeout = (ULONG)(___timeout);\
  register BOOL __v_ret __asm("d0");\
  register BPTR __v0 __asm("d1") = __p____fh;\
  register ULONG __v1 __asm("d2") = __p____offset;\
  register ULONG __v2 __asm("d3") = __p____length;\
  register ULONG __v3 __asm("d4") = __p____mode;\
  register ULONG __v4 __asm("d5") = __p____timeout;\
  __asm volatile (\
                   "jsr %%a6@(-270:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1), "d"(__v2), "d"(__v3), "d"(__v4)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define LockRecord(___fh, ___offset, ___length, ___mode, ___timeout) ({\
  __LockRecord_base((DOS_BASE_NAME), ___fh, ___offset, ___length, ___mode, ___timeout);\
})

#define __LockRecords_base(__in_base, ___recArray, ___timeout) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct RecordLock * __p____recArray = (struct RecordLock *)(___recArray);\
  ULONG __p____timeout = (ULONG)(___timeout);\
  register BOOL __v_ret __asm("d0");\
  register struct RecordLock * __v0 __asm("d1") = __p____recArray;\
  register ULONG __v1 __asm("d2") = __p____timeout;\
  __asm volatile (\
                   "jsr %%a6@(-276:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define LockRecords(___recArray, ___timeout) ({\
  __LockRecords_base((DOS_BASE_NAME), ___recArray, ___timeout);\
})

#define __UnLockRecord_base(__in_base, ___fh, ___offset, ___length) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  BPTR __p____fh = (BPTR)(___fh);\
  ULONG __p____offset = (ULONG)(___offset);\
  ULONG __p____length = (ULONG)(___length);\
  register BOOL __v_ret __asm("d0");\
  register BPTR __v0 __asm("d1") = __p____fh;\
  register ULONG __v1 __asm("d2") = __p____offset;\
  register ULONG __v2 __asm("d3") = __p____length;\
  __asm volatile (\
                   "jsr %%a6@(-282:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1), "d"(__v2)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define UnLockRecord(___fh, ___offset, ___length) ({\
  __UnLockRecord_base((DOS_BASE_NAME), ___fh, ___offset, ___length);\
})

#define __UnLockRecords_base(__in_base, ___recArray) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct RecordLock * __p____recArray = (struct RecordLock *)(___recArray);\
  register BOOL __v_ret __asm("d0");\
  register struct RecordLock * __v0 __asm("d1") = __p____recArray;\
  __asm volatile (\
                   "jsr %%a6@(-288:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define UnLockRecords(___recArray) ({\
  __UnLockRecords_base((DOS_BASE_NAME), ___recArray);\
})

#define __SelectInput_base(__in_base, ___fh) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  BPTR __p____fh = (BPTR)(___fh);\
  register BPTR __v_ret __asm("d0");\
  register BPTR __v0 __asm("d1") = __p____fh;\
  __asm volatile (\
                   "jsr %%a6@(-294:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define SelectInput(___fh) ({\
  __SelectInput_base((DOS_BASE_NAME), ___fh);\
})

#define __SelectOutput_base(__in_base, ___fh) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  BPTR __p____fh = (BPTR)(___fh);\
  register BPTR __v_ret __asm("d0");\
  register BPTR __v0 __asm("d1") = __p____fh;\
  __asm volatile (\
                   "jsr %%a6@(-300:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define SelectOutput(___fh) ({\
  __SelectOutput_base((DOS_BASE_NAME), ___fh);\
})

#define __FGetC_base(__in_base, ___fh) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  BPTR __p____fh = (BPTR)(___fh);\
  register LONG __v_ret __asm("d0");\
  register BPTR __v0 __asm("d1") = __p____fh;\
  __asm volatile (\
                   "jsr %%a6@(-306:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define FGetC(___fh) ({\
  __FGetC_base((DOS_BASE_NAME), ___fh);\
})

#define __FPutC_base(__in_base, ___fh, ___ch) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  BPTR __p____fh = (BPTR)(___fh);\
  LONG __p____ch = (LONG)(___ch);\
  register LONG __v_ret __asm("d0");\
  register BPTR __v0 __asm("d1") = __p____fh;\
  register LONG __v1 __asm("d2") = __p____ch;\
  __asm volatile (\
                   "jsr %%a6@(-312:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define FPutC(___fh, ___ch) ({\
  __FPutC_base((DOS_BASE_NAME), ___fh, ___ch);\
})

#define __UnGetC_base(__in_base, ___fh, ___character) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  BPTR __p____fh = (BPTR)(___fh);\
  LONG __p____character = (LONG)(___character);\
  register LONG __v_ret __asm("d0");\
  register BPTR __v0 __asm("d1") = __p____fh;\
  register LONG __v1 __asm("d2") = __p____character;\
  __asm volatile (\
                   "jsr %%a6@(-318:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define UnGetC(___fh, ___character) ({\
  __UnGetC_base((DOS_BASE_NAME), ___fh, ___character);\
})

#define __FRead_base(__in_base, ___fh, ___block, ___blocklen, ___number) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  BPTR __p____fh = (BPTR)(___fh);\
  APTR __p____block = (APTR)(___block);\
  ULONG __p____blocklen = (ULONG)(___blocklen);\
  ULONG __p____number = (ULONG)(___number);\
  register LONG __v_ret __asm("d0");\
  register BPTR __v0 __asm("d1") = __p____fh;\
  register APTR __v1 __asm("d2") = __p____block;\
  register ULONG __v2 __asm("d3") = __p____blocklen;\
  register ULONG __v3 __asm("d4") = __p____number;\
  __asm volatile (\
                   "jsr %%a6@(-324:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1), "d"(__v2), "d"(__v3)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define FRead(___fh, ___block, ___blocklen, ___number) ({\
  __FRead_base((DOS_BASE_NAME), ___fh, ___block, ___blocklen, ___number);\
})

#define __FWrite_base(__in_base, ___fh, ___block, ___blocklen, ___number) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  BPTR __p____fh = (BPTR)(___fh);\
  APTR __p____block = (APTR)(___block);\
  ULONG __p____blocklen = (ULONG)(___blocklen);\
  ULONG __p____number = (ULONG)(___number);\
  register LONG __v_ret __asm("d0");\
  register BPTR __v0 __asm("d1") = __p____fh;\
  register APTR __v1 __asm("d2") = __p____block;\
  register ULONG __v2 __asm("d3") = __p____blocklen;\
  register ULONG __v3 __asm("d4") = __p____number;\
  __asm volatile (\
                   "jsr %%a6@(-330:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1), "d"(__v2), "d"(__v3)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define FWrite(___fh, ___block, ___blocklen, ___number) ({\
  __FWrite_base((DOS_BASE_NAME), ___fh, ___block, ___blocklen, ___number);\
})

#define __FGets_base(__in_base, ___fh, ___buf, ___buflen) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  BPTR __p____fh = (BPTR)(___fh);\
  STRPTR __p____buf = (STRPTR)(___buf);\
  ULONG __p____buflen = (ULONG)(___buflen);\
  register STRPTR __v_ret __asm("d0");\
  register BPTR __v0 __asm("d1") = __p____fh;\
  register STRPTR __v1 __asm("d2") = __p____buf;\
  register ULONG __v2 __asm("d3") = __p____buflen;\
  __asm volatile (\
                   "jsr %%a6@(-336:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1), "d"(__v2)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define FGets(___fh, ___buf, ___buflen) ({\
  __FGets_base((DOS_BASE_NAME), ___fh, ___buf, ___buflen);\
})

#define __FPuts_base(__in_base, ___fh, ___str) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  BPTR __p____fh = (BPTR)(___fh);\
  STRPTR __p____str = (STRPTR)(___str);\
  register LONG __v_ret __asm("d0");\
  register BPTR __v0 __asm("d1") = __p____fh;\
  register STRPTR __v1 __asm("d2") = __p____str;\
  __asm volatile (\
                   "jsr %%a6@(-342:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define FPuts(___fh, ___str) ({\
  __FPuts_base((DOS_BASE_NAME), ___fh, ___str);\
})

#define __VFWritef_base(__in_base, ___fh, ___format, ___argarray) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  BPTR __p____fh = (BPTR)(___fh);\
  STRPTR __p____format = (STRPTR)(___format);\
  LONG * __p____argarray = (LONG *)(___argarray);\
  register BPTR __v0 __asm("d1") = __p____fh;\
  register STRPTR __v1 __asm("d2") = __p____format;\
  register LONG * __v2 __asm("d3") = __p____argarray;\
  __asm volatile (\
                   "jsr %%a6@(-348:W)\n"\
                   : "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1), "d"(__v2)\
                   : "fp0", "fp1", "cc", "memory", "d0", "a0", "a1" );\
})

#define VFWritef(___fh, ___format, ___argarray) ({\
  __VFWritef_base((DOS_BASE_NAME), ___fh, ___format, ___argarray);\
})

AMIGA_VA_WRAPPER_ATTR
static __stdargs VOID __FWritef_va(void *const __base __asm("a6"), BPTR ___fh, CONST_STRPTR ___format, ...)
{
    const void *args = (const void *)(&___format + 1);
    __VFWritef_base(__base, ___fh, ___format, args);
}

#define FWritef(___fh, ___format, ...) __FWritef_va(DOS_BASE_NAME, ___fh, ___format, ## __VA_ARGS__)

#define __VFPrintf_base(__in_base, ___fh, ___format, ___argarray) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  BPTR __p____fh = (BPTR)(___fh);\
  STRPTR __p____format = (STRPTR)(___format);\
  APTR __p____argarray = (APTR)(___argarray);\
  register LONG __v_ret __asm("d0");\
  register BPTR __v0 __asm("d1") = __p____fh;\
  register STRPTR __v1 __asm("d2") = __p____format;\
  register APTR __v2 __asm("d3") = __p____argarray;\
  __asm volatile (\
                   "jsr %%a6@(-354:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1), "d"(__v2)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define VFPrintf(___fh, ___format, ___argarray) ({\
  __VFPrintf_base((DOS_BASE_NAME), ___fh, ___format, ___argarray);\
})

AMIGA_VA_WRAPPER_ATTR
static __stdargs LONG __FPrintf_va(void *const __base __asm("a6"), BPTR ___fh, CONST_STRPTR ___format, ...)
{
    const void *args = (const void *)(&___format + 1);
    return __VFPrintf_base(__base, ___fh, ___format, args);
}

#define FPrintf(___fh, ___format, ...) __FPrintf_va(DOS_BASE_NAME, ___fh, ___format, ## __VA_ARGS__)

#define __Flush_base(__in_base, ___fh) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  BPTR __p____fh = (BPTR)(___fh);\
  register LONG __v_ret __asm("d0");\
  register BPTR __v0 __asm("d1") = __p____fh;\
  __asm volatile (\
                   "jsr %%a6@(-360:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define Flush(___fh) ({\
  __Flush_base((DOS_BASE_NAME), ___fh);\
})

#define __SetVBuf_base(__in_base, ___fh, ___buff, ___type, ___size) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  BPTR __p____fh = (BPTR)(___fh);\
  STRPTR __p____buff = (STRPTR)(___buff);\
  LONG __p____type = (LONG)(___type);\
  LONG __p____size = (LONG)(___size);\
  register LONG __v_ret __asm("d0");\
  register BPTR __v0 __asm("d1") = __p____fh;\
  register STRPTR __v1 __asm("d2") = __p____buff;\
  register LONG __v2 __asm("d3") = __p____type;\
  register LONG __v3 __asm("d4") = __p____size;\
  __asm volatile (\
                   "jsr %%a6@(-366:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1), "d"(__v2), "d"(__v3)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define SetVBuf(___fh, ___buff, ___type, ___size) ({\
  __SetVBuf_base((DOS_BASE_NAME), ___fh, ___buff, ___type, ___size);\
})

#define __DupLockFromFH_base(__in_base, ___fh) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  BPTR __p____fh = (BPTR)(___fh);\
  register BPTR __v_ret __asm("d0");\
  register BPTR __v0 __asm("d1") = __p____fh;\
  __asm volatile (\
                   "jsr %%a6@(-372:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define DupLockFromFH(___fh) ({\
  __DupLockFromFH_base((DOS_BASE_NAME), ___fh);\
})

#define __OpenFromLock_base(__in_base, ___lock) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  BPTR __p____lock = (BPTR)(___lock);\
  register BPTR __v_ret __asm("d0");\
  register BPTR __v0 __asm("d1") = __p____lock;\
  __asm volatile (\
                   "jsr %%a6@(-378:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define OpenFromLock(___lock) ({\
  __OpenFromLock_base((DOS_BASE_NAME), ___lock);\
})

#define __ParentOfFH_base(__in_base, ___fh) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  BPTR __p____fh = (BPTR)(___fh);\
  register BPTR __v_ret __asm("d0");\
  register BPTR __v0 __asm("d1") = __p____fh;\
  __asm volatile (\
                   "jsr %%a6@(-384:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define ParentOfFH(___fh) ({\
  __ParentOfFH_base((DOS_BASE_NAME), ___fh);\
})

#define __ExamineFH_base(__in_base, ___fh, ___fib) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  BPTR __p____fh = (BPTR)(___fh);\
  struct FileInfoBlock * __p____fib = (struct FileInfoBlock *)(___fib);\
  register BOOL __v_ret __asm("d0");\
  register BPTR __v0 __asm("d1") = __p____fh;\
  register struct FileInfoBlock * __v1 __asm("d2") = __p____fib;\
  __asm volatile (\
                   "jsr %%a6@(-390:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define ExamineFH(___fh, ___fib) ({\
  __ExamineFH_base((DOS_BASE_NAME), ___fh, ___fib);\
})

#define __SetFileDate_base(__in_base, ___name, ___date) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  STRPTR __p____name = (STRPTR)(___name);\
  struct DateStamp * __p____date = (struct DateStamp *)(___date);\
  register LONG __v_ret __asm("d0");\
  register STRPTR __v0 __asm("d1") = __p____name;\
  register struct DateStamp * __v1 __asm("d2") = __p____date;\
  __asm volatile (\
                   "jsr %%a6@(-396:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define SetFileDate(___name, ___date) ({\
  __SetFileDate_base((DOS_BASE_NAME), ___name, ___date);\
})

#define __NameFromLock_base(__in_base, ___lock, ___buffer, ___len) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  BPTR __p____lock = (BPTR)(___lock);\
  STRPTR __p____buffer = (STRPTR)(___buffer);\
  LONG __p____len = (LONG)(___len);\
  register LONG __v_ret __asm("d0");\
  register BPTR __v0 __asm("d1") = __p____lock;\
  register STRPTR __v1 __asm("d2") = __p____buffer;\
  register LONG __v2 __asm("d3") = __p____len;\
  __asm volatile (\
                   "jsr %%a6@(-402:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1), "d"(__v2)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define NameFromLock(___lock, ___buffer, ___len) ({\
  __NameFromLock_base((DOS_BASE_NAME), ___lock, ___buffer, ___len);\
})

#define __NameFromFH_base(__in_base, ___fh, ___buffer, ___len) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  BPTR __p____fh = (BPTR)(___fh);\
  STRPTR __p____buffer = (STRPTR)(___buffer);\
  LONG __p____len = (LONG)(___len);\
  register LONG __v_ret __asm("d0");\
  register BPTR __v0 __asm("d1") = __p____fh;\
  register STRPTR __v1 __asm("d2") = __p____buffer;\
  register LONG __v2 __asm("d3") = __p____len;\
  __asm volatile (\
                   "jsr %%a6@(-408:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1), "d"(__v2)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define NameFromFH(___fh, ___buffer, ___len) ({\
  __NameFromFH_base((DOS_BASE_NAME), ___fh, ___buffer, ___len);\
})

#define __SplitName_base(__in_base, ___name, ___separator, ___buf, ___oldpos, ___size) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  STRPTR __p____name = (STRPTR)(___name);\
  UBYTE __p____separator = (UBYTE)(___separator);\
  STRPTR __p____buf = (STRPTR)(___buf);\
  WORD __p____oldpos = (WORD)(___oldpos);\
  LONG __p____size = (LONG)(___size);\
  register WORD __v_ret __asm("d0");\
  register STRPTR __v0 __asm("d1") = __p____name;\
  register UBYTE __v1 __asm("d2") = __p____separator;\
  register STRPTR __v2 __asm("d3") = __p____buf;\
  register WORD __v3 __asm("d4") = __p____oldpos;\
  register LONG __v4 __asm("d5") = __p____size;\
  __asm volatile (\
                   "jsr %%a6@(-414:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1), "d"(__v2), "d"(__v3), "d"(__v4)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define SplitName(___name, ___separator, ___buf, ___oldpos, ___size) ({\
  __SplitName_base((DOS_BASE_NAME), ___name, ___separator, ___buf, ___oldpos, ___size);\
})

#define __SameLock_base(__in_base, ___lock1, ___lock2) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  BPTR __p____lock1 = (BPTR)(___lock1);\
  BPTR __p____lock2 = (BPTR)(___lock2);\
  register LONG __v_ret __asm("d0");\
  register BPTR __v0 __asm("d1") = __p____lock1;\
  register BPTR __v1 __asm("d2") = __p____lock2;\
  __asm volatile (\
                   "jsr %%a6@(-420:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define SameLock(___lock1, ___lock2) ({\
  __SameLock_base((DOS_BASE_NAME), ___lock1, ___lock2);\
})

#define __SetMode_base(__in_base, ___fh, ___mode) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  BPTR __p____fh = (BPTR)(___fh);\
  LONG __p____mode = (LONG)(___mode);\
  register LONG __v_ret __asm("d0");\
  register BPTR __v0 __asm("d1") = __p____fh;\
  register LONG __v1 __asm("d2") = __p____mode;\
  __asm volatile (\
                   "jsr %%a6@(-426:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define SetMode(___fh, ___mode) ({\
  __SetMode_base((DOS_BASE_NAME), ___fh, ___mode);\
})

#define __ExAll_base(__in_base, ___lock, ___buffer, ___size, ___data, ___control) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  BPTR __p____lock = (BPTR)(___lock);\
  struct ExAllData * __p____buffer = (struct ExAllData *)(___buffer);\
  LONG __p____size = (LONG)(___size);\
  LONG __p____data = (LONG)(___data);\
  struct ExAllControl * __p____control = (struct ExAllControl *)(___control);\
  register LONG __v_ret __asm("d0");\
  register BPTR __v0 __asm("d1") = __p____lock;\
  register struct ExAllData * __v1 __asm("d2") = __p____buffer;\
  register LONG __v2 __asm("d3") = __p____size;\
  register LONG __v3 __asm("d4") = __p____data;\
  register struct ExAllControl * __v4 __asm("d5") = __p____control;\
  __asm volatile (\
                   "jsr %%a6@(-432:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1), "d"(__v2), "d"(__v3), "d"(__v4)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define ExAll(___lock, ___buffer, ___size, ___data, ___control) ({\
  __ExAll_base((DOS_BASE_NAME), ___lock, ___buffer, ___size, ___data, ___control);\
})

#define __ReadLink_base(__in_base, ___port, ___lock, ___path, ___buffer, ___size) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct MsgPort * __p____port = (struct MsgPort *)(___port);\
  BPTR __p____lock = (BPTR)(___lock);\
  STRPTR __p____path = (STRPTR)(___path);\
  STRPTR __p____buffer = (STRPTR)(___buffer);\
  ULONG __p____size = (ULONG)(___size);\
  register LONG __v_ret __asm("d0");\
  register struct MsgPort * __v0 __asm("d1") = __p____port;\
  register BPTR __v1 __asm("d2") = __p____lock;\
  register STRPTR __v2 __asm("d3") = __p____path;\
  register STRPTR __v3 __asm("d4") = __p____buffer;\
  register ULONG __v4 __asm("d5") = __p____size;\
  __asm volatile (\
                   "jsr %%a6@(-438:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1), "d"(__v2), "d"(__v3), "d"(__v4)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define ReadLink(___port, ___lock, ___path, ___buffer, ___size) ({\
  __ReadLink_base((DOS_BASE_NAME), ___port, ___lock, ___path, ___buffer, ___size);\
})

#define __MakeLink_base(__in_base, ___name, ___dest, ___soft) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  STRPTR __p____name = (STRPTR)(___name);\
  LONG __p____dest = (LONG)(___dest);\
  LONG __p____soft = (LONG)(___soft);\
  register LONG __v_ret __asm("d0");\
  register STRPTR __v0 __asm("d1") = __p____name;\
  register LONG __v1 __asm("d2") = __p____dest;\
  register LONG __v2 __asm("d3") = __p____soft;\
  __asm volatile (\
                   "jsr %%a6@(-444:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1), "d"(__v2)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define MakeLink(___name, ___dest, ___soft) ({\
  __MakeLink_base((DOS_BASE_NAME), ___name, ___dest, ___soft);\
})

#define __ChangeMode_base(__in_base, ___type, ___fh, ___newmode) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  LONG __p____type = (LONG)(___type);\
  BPTR __p____fh = (BPTR)(___fh);\
  LONG __p____newmode = (LONG)(___newmode);\
  register LONG __v_ret __asm("d0");\
  register LONG __v0 __asm("d1") = __p____type;\
  register BPTR __v1 __asm("d2") = __p____fh;\
  register LONG __v2 __asm("d3") = __p____newmode;\
  __asm volatile (\
                   "jsr %%a6@(-450:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1), "d"(__v2)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define ChangeMode(___type, ___fh, ___newmode) ({\
  __ChangeMode_base((DOS_BASE_NAME), ___type, ___fh, ___newmode);\
})

#define __SetFileSize_base(__in_base, ___fh, ___pos, ___mode) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  BPTR __p____fh = (BPTR)(___fh);\
  LONG __p____pos = (LONG)(___pos);\
  LONG __p____mode = (LONG)(___mode);\
  register LONG __v_ret __asm("d0");\
  register BPTR __v0 __asm("d1") = __p____fh;\
  register LONG __v1 __asm("d2") = __p____pos;\
  register LONG __v2 __asm("d3") = __p____mode;\
  __asm volatile (\
                   "jsr %%a6@(-456:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1), "d"(__v2)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define SetFileSize(___fh, ___pos, ___mode) ({\
  __SetFileSize_base((DOS_BASE_NAME), ___fh, ___pos, ___mode);\
})

#define __SetIoErr_base(__in_base, ___result) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  LONG __p____result = (LONG)(___result);\
  register LONG __v_ret __asm("d0");\
  register LONG __v0 __asm("d1") = __p____result;\
  __asm volatile (\
                   "jsr %%a6@(-462:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define SetIoErr(___result) ({\
  __SetIoErr_base((DOS_BASE_NAME), ___result);\
})

#define __Fault_base(__in_base, ___code, ___header, ___buffer, ___len) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  LONG __p____code = (LONG)(___code);\
  STRPTR __p____header = (STRPTR)(___header);\
  STRPTR __p____buffer = (STRPTR)(___buffer);\
  LONG __p____len = (LONG)(___len);\
  register BOOL __v_ret __asm("d0");\
  register LONG __v0 __asm("d1") = __p____code;\
  register STRPTR __v1 __asm("d2") = __p____header;\
  register STRPTR __v2 __asm("d3") = __p____buffer;\
  register LONG __v3 __asm("d4") = __p____len;\
  __asm volatile (\
                   "jsr %%a6@(-468:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1), "d"(__v2), "d"(__v3)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define Fault(___code, ___header, ___buffer, ___len) ({\
  __Fault_base((DOS_BASE_NAME), ___code, ___header, ___buffer, ___len);\
})

#define __PrintFault_base(__in_base, ___code, ___header) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  LONG __p____code = (LONG)(___code);\
  STRPTR __p____header = (STRPTR)(___header);\
  register BOOL __v_ret __asm("d0");\
  register LONG __v0 __asm("d1") = __p____code;\
  register STRPTR __v1 __asm("d2") = __p____header;\
  __asm volatile (\
                   "jsr %%a6@(-474:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define PrintFault(___code, ___header) ({\
  __PrintFault_base((DOS_BASE_NAME), ___code, ___header);\
})

#define __ErrorReport_base(__in_base, ___code, ___type, ___arg1, ___device) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  LONG __p____code = (LONG)(___code);\
  LONG __p____type = (LONG)(___type);\
  ULONG __p____arg1 = (ULONG)(___arg1);\
  struct MsgPort * __p____device = (struct MsgPort *)(___device);\
  register LONG __v_ret __asm("d0");\
  register LONG __v0 __asm("d1") = __p____code;\
  register LONG __v1 __asm("d2") = __p____type;\
  register ULONG __v2 __asm("d3") = __p____arg1;\
  register struct MsgPort * __v3 __asm("d4") = __p____device;\
  __asm volatile (\
                   "jsr %%a6@(-480:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1), "d"(__v2), "d"(__v3)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define ErrorReport(___code, ___type, ___arg1, ___device) ({\
  __ErrorReport_base((DOS_BASE_NAME), ___code, ___type, ___arg1, ___device);\
})

#define __Cli_base(__in_base) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  register struct CommandLineInterface * __v_ret __asm("d0");\
  __asm volatile (\
                   "jsr %%a6@(-492:W)\n"\
                   : "=d"(__v_ret)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d1", "a0", "a1" );\
  __v_ret;})

#define Cli() ({\
  __Cli_base((DOS_BASE_NAME));\
})

#define __CreateNewProc_base(__in_base, ___tags) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct TagItem * __p____tags = (struct TagItem *)(___tags);\
  register struct Process * __v_ret __asm("d0");\
  register struct TagItem * __v0 __asm("d1") = __p____tags;\
  __asm volatile (\
                   "jsr %%a6@(-498:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define CreateNewProc(___tags) ({\
  __CreateNewProc_base((DOS_BASE_NAME), ___tags);\
})

#define __CreateNewProcTagList_base(__in_base, ___tags) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct TagItem * __p____tags = (struct TagItem *)(___tags);\
  register struct Process * __v_ret __asm("d0");\
  register struct TagItem * __v0 __asm("d1") = __p____tags;\
  __asm volatile (\
                   "jsr %%a6@(-498:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define CreateNewProcTagList(___tags) ({\
  __CreateNewProcTagList_base((DOS_BASE_NAME), ___tags);\
})

AMIGA_VA_WRAPPER_ATTR
static __stdargs struct Process * __CreateNewProcTags_va(void *const __base __asm("a6"), ULONG ___tag1type, ...)
{
    const ULONG *tags = (const ULONG *)&___tag1type;
    return __CreateNewProcTagList_base(__base, (CONST struct TagItem *)tags);
}

#define CreateNewProcTags(...) __CreateNewProcTags_va(DOS_BASE_NAME, __VA_ARGS__)

#define __RunCommand_base(__in_base, ___seg, ___stack, ___paramptr, ___paramlen) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  BPTR __p____seg = (BPTR)(___seg);\
  LONG __p____stack = (LONG)(___stack);\
  STRPTR __p____paramptr = (STRPTR)(___paramptr);\
  LONG __p____paramlen = (LONG)(___paramlen);\
  register LONG __v_ret __asm("d0");\
  register BPTR __v0 __asm("d1") = __p____seg;\
  register LONG __v1 __asm("d2") = __p____stack;\
  register STRPTR __v2 __asm("d3") = __p____paramptr;\
  register LONG __v3 __asm("d4") = __p____paramlen;\
  __asm volatile (\
                   "jsr %%a6@(-504:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1), "d"(__v2), "d"(__v3)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define RunCommand(___seg, ___stack, ___paramptr, ___paramlen) ({\
  __RunCommand_base((DOS_BASE_NAME), ___seg, ___stack, ___paramptr, ___paramlen);\
})

#define __GetConsoleTask_base(__in_base) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  register struct MsgPort * __v_ret __asm("d0");\
  __asm volatile (\
                   "jsr %%a6@(-510:W)\n"\
                   : "=d"(__v_ret)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d1", "a0", "a1" );\
  __v_ret;})

#define GetConsoleTask() ({\
  __GetConsoleTask_base((DOS_BASE_NAME));\
})

#define __SetConsoleTask_base(__in_base, ___task) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct MsgPort * __p____task = (struct MsgPort *)(___task);\
  register struct MsgPort * __v_ret __asm("d0");\
  register struct MsgPort * __v0 __asm("d1") = __p____task;\
  __asm volatile (\
                   "jsr %%a6@(-516:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define SetConsoleTask(___task) ({\
  __SetConsoleTask_base((DOS_BASE_NAME), ___task);\
})

#define __GetFileSysTask_base(__in_base) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  register struct MsgPort * __v_ret __asm("d0");\
  __asm volatile (\
                   "jsr %%a6@(-522:W)\n"\
                   : "=d"(__v_ret)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d1", "a0", "a1" );\
  __v_ret;})

#define GetFileSysTask() ({\
  __GetFileSysTask_base((DOS_BASE_NAME));\
})

#define __SetFileSysTask_base(__in_base, ___task) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct MsgPort * __p____task = (struct MsgPort *)(___task);\
  register struct MsgPort * __v_ret __asm("d0");\
  register struct MsgPort * __v0 __asm("d1") = __p____task;\
  __asm volatile (\
                   "jsr %%a6@(-528:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define SetFileSysTask(___task) ({\
  __SetFileSysTask_base((DOS_BASE_NAME), ___task);\
})

#define __GetArgStr_base(__in_base) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  register STRPTR __v_ret __asm("d0");\
  __asm volatile (\
                   "jsr %%a6@(-534:W)\n"\
                   : "=d"(__v_ret)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d1", "a0", "a1" );\
  __v_ret;})

#define GetArgStr() ({\
  __GetArgStr_base((DOS_BASE_NAME));\
})

#define __SetArgStr_base(__in_base, ___string) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  STRPTR __p____string = (STRPTR)(___string);\
  register STRPTR __v_ret __asm("d0");\
  register STRPTR __v0 __asm("d1") = __p____string;\
  __asm volatile (\
                   "jsr %%a6@(-540:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define SetArgStr(___string) ({\
  __SetArgStr_base((DOS_BASE_NAME), ___string);\
})

#define __FindCliProc_base(__in_base, ___num) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  ULONG __p____num = (ULONG)(___num);\
  register struct Process * __v_ret __asm("d0");\
  register ULONG __v0 __asm("d1") = __p____num;\
  __asm volatile (\
                   "jsr %%a6@(-546:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define FindCliProc(___num) ({\
  __FindCliProc_base((DOS_BASE_NAME), ___num);\
})

#define __MaxCli_base(__in_base) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  register ULONG __v_ret __asm("d0");\
  __asm volatile (\
                   "jsr %%a6@(-552:W)\n"\
                   : "=d"(__v_ret)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d1", "a0", "a1" );\
  __v_ret;})

#define MaxCli() ({\
  __MaxCli_base((DOS_BASE_NAME));\
})

#define __SetCurrentDirName_base(__in_base, ___name) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  STRPTR __p____name = (STRPTR)(___name);\
  register BOOL __v_ret __asm("d0");\
  register STRPTR __v0 __asm("d1") = __p____name;\
  __asm volatile (\
                   "jsr %%a6@(-558:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define SetCurrentDirName(___name) ({\
  __SetCurrentDirName_base((DOS_BASE_NAME), ___name);\
})

#define __GetCurrentDirName_base(__in_base, ___buf, ___len) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  STRPTR __p____buf = (STRPTR)(___buf);\
  LONG __p____len = (LONG)(___len);\
  register BOOL __v_ret __asm("d0");\
  register STRPTR __v0 __asm("d1") = __p____buf;\
  register LONG __v1 __asm("d2") = __p____len;\
  __asm volatile (\
                   "jsr %%a6@(-564:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define GetCurrentDirName(___buf, ___len) ({\
  __GetCurrentDirName_base((DOS_BASE_NAME), ___buf, ___len);\
})

#define __SetProgramName_base(__in_base, ___name) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  STRPTR __p____name = (STRPTR)(___name);\
  register BOOL __v_ret __asm("d0");\
  register STRPTR __v0 __asm("d1") = __p____name;\
  __asm volatile (\
                   "jsr %%a6@(-570:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define SetProgramName(___name) ({\
  __SetProgramName_base((DOS_BASE_NAME), ___name);\
})

#define __GetProgramName_base(__in_base, ___buf, ___len) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  STRPTR __p____buf = (STRPTR)(___buf);\
  LONG __p____len = (LONG)(___len);\
  register BOOL __v_ret __asm("d0");\
  register STRPTR __v0 __asm("d1") = __p____buf;\
  register LONG __v1 __asm("d2") = __p____len;\
  __asm volatile (\
                   "jsr %%a6@(-576:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define GetProgramName(___buf, ___len) ({\
  __GetProgramName_base((DOS_BASE_NAME), ___buf, ___len);\
})

#define __SetPrompt_base(__in_base, ___name) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  STRPTR __p____name = (STRPTR)(___name);\
  register BOOL __v_ret __asm("d0");\
  register STRPTR __v0 __asm("d1") = __p____name;\
  __asm volatile (\
                   "jsr %%a6@(-582:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define SetPrompt(___name) ({\
  __SetPrompt_base((DOS_BASE_NAME), ___name);\
})

#define __GetPrompt_base(__in_base, ___buf, ___len) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  STRPTR __p____buf = (STRPTR)(___buf);\
  LONG __p____len = (LONG)(___len);\
  register BOOL __v_ret __asm("d0");\
  register STRPTR __v0 __asm("d1") = __p____buf;\
  register LONG __v1 __asm("d2") = __p____len;\
  __asm volatile (\
                   "jsr %%a6@(-588:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define GetPrompt(___buf, ___len) ({\
  __GetPrompt_base((DOS_BASE_NAME), ___buf, ___len);\
})

#define __SetProgramDir_base(__in_base, ___lock) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  BPTR __p____lock = (BPTR)(___lock);\
  register BPTR __v_ret __asm("d0");\
  register BPTR __v0 __asm("d1") = __p____lock;\
  __asm volatile (\
                   "jsr %%a6@(-594:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define SetProgramDir(___lock) ({\
  __SetProgramDir_base((DOS_BASE_NAME), ___lock);\
})

#define __GetProgramDir_base(__in_base) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  register BPTR __v_ret __asm("d0");\
  __asm volatile (\
                   "jsr %%a6@(-600:W)\n"\
                   : "=d"(__v_ret)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d1", "a0", "a1" );\
  __v_ret;})

#define GetProgramDir() ({\
  __GetProgramDir_base((DOS_BASE_NAME));\
})

#define __SystemTagList_base(__in_base, ___command, ___tags) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  STRPTR __p____command = (STRPTR)(___command);\
  struct TagItem * __p____tags = (struct TagItem *)(___tags);\
  register LONG __v_ret __asm("d0");\
  register STRPTR __v0 __asm("d1") = __p____command;\
  register struct TagItem * __v1 __asm("d2") = __p____tags;\
  __asm volatile (\
                   "jsr %%a6@(-606:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define SystemTagList(___command, ___tags) ({\
  __SystemTagList_base((DOS_BASE_NAME), ___command, ___tags);\
})

#define __System_base(__in_base, ___command, ___tags) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  STRPTR __p____command = (STRPTR)(___command);\
  struct TagItem * __p____tags = (struct TagItem *)(___tags);\
  register LONG __v_ret __asm("d0");\
  register STRPTR __v0 __asm("d1") = __p____command;\
  register struct TagItem * __v1 __asm("d2") = __p____tags;\
  __asm volatile (\
                   "jsr %%a6@(-606:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define System(___command, ___tags) ({\
  __System_base((DOS_BASE_NAME), ___command, ___tags);\
})

AMIGA_VA_WRAPPER_ATTR
static __stdargs LONG __SystemTags_va(void *const __base __asm("a6"), CONST_STRPTR ___command, ULONG ___tag1type, ...)
{
    const ULONG *tags = (const ULONG *)&___tag1type;
    return __System_base(__base, ___command, (CONST struct TagItem *)tags);
}

#define SystemTags(...) __SystemTags_va(DOS_BASE_NAME, __VA_ARGS__)

#define __AssignLock_base(__in_base, ___name, ___lock) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  STRPTR __p____name = (STRPTR)(___name);\
  BPTR __p____lock = (BPTR)(___lock);\
  register LONG __v_ret __asm("d0");\
  register STRPTR __v0 __asm("d1") = __p____name;\
  register BPTR __v1 __asm("d2") = __p____lock;\
  __asm volatile (\
                   "jsr %%a6@(-612:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define AssignLock(___name, ___lock) ({\
  __AssignLock_base((DOS_BASE_NAME), ___name, ___lock);\
})

#define __AssignLate_base(__in_base, ___name, ___path) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  STRPTR __p____name = (STRPTR)(___name);\
  STRPTR __p____path = (STRPTR)(___path);\
  register BOOL __v_ret __asm("d0");\
  register STRPTR __v0 __asm("d1") = __p____name;\
  register STRPTR __v1 __asm("d2") = __p____path;\
  __asm volatile (\
                   "jsr %%a6@(-618:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define AssignLate(___name, ___path) ({\
  __AssignLate_base((DOS_BASE_NAME), ___name, ___path);\
})

#define __AssignPath_base(__in_base, ___name, ___path) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  STRPTR __p____name = (STRPTR)(___name);\
  STRPTR __p____path = (STRPTR)(___path);\
  register BOOL __v_ret __asm("d0");\
  register STRPTR __v0 __asm("d1") = __p____name;\
  register STRPTR __v1 __asm("d2") = __p____path;\
  __asm volatile (\
                   "jsr %%a6@(-624:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define AssignPath(___name, ___path) ({\
  __AssignPath_base((DOS_BASE_NAME), ___name, ___path);\
})

#define __AssignAdd_base(__in_base, ___name, ___lock) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  STRPTR __p____name = (STRPTR)(___name);\
  BPTR __p____lock = (BPTR)(___lock);\
  register BOOL __v_ret __asm("d0");\
  register STRPTR __v0 __asm("d1") = __p____name;\
  register BPTR __v1 __asm("d2") = __p____lock;\
  __asm volatile (\
                   "jsr %%a6@(-630:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define AssignAdd(___name, ___lock) ({\
  __AssignAdd_base((DOS_BASE_NAME), ___name, ___lock);\
})

#define __RemAssignList_base(__in_base, ___name, ___lock) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  STRPTR __p____name = (STRPTR)(___name);\
  BPTR __p____lock = (BPTR)(___lock);\
  register LONG __v_ret __asm("d0");\
  register STRPTR __v0 __asm("d1") = __p____name;\
  register BPTR __v1 __asm("d2") = __p____lock;\
  __asm volatile (\
                   "jsr %%a6@(-636:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define RemAssignList(___name, ___lock) ({\
  __RemAssignList_base((DOS_BASE_NAME), ___name, ___lock);\
})

#define __GetDeviceProc_base(__in_base, ___name, ___dp) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  STRPTR __p____name = (STRPTR)(___name);\
  struct DevProc * __p____dp = (struct DevProc *)(___dp);\
  register struct DevProc * __v_ret __asm("d0");\
  register STRPTR __v0 __asm("d1") = __p____name;\
  register struct DevProc * __v1 __asm("d2") = __p____dp;\
  __asm volatile (\
                   "jsr %%a6@(-642:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define GetDeviceProc(___name, ___dp) ({\
  __GetDeviceProc_base((DOS_BASE_NAME), ___name, ___dp);\
})

#define __FreeDeviceProc_base(__in_base, ___dp) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct DevProc * __p____dp = (struct DevProc *)(___dp);\
  register struct DevProc * __v0 __asm("d1") = __p____dp;\
  __asm volatile (\
                   "jsr %%a6@(-648:W)\n"\
                   : "+d"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d0", "a0", "a1" );\
})

#define FreeDeviceProc(___dp) ({\
  __FreeDeviceProc_base((DOS_BASE_NAME), ___dp);\
})

#define __LockDosList_base(__in_base, ___flags) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  ULONG __p____flags = (ULONG)(___flags);\
  register struct DosList * __v_ret __asm("d0");\
  register ULONG __v0 __asm("d1") = __p____flags;\
  __asm volatile (\
                   "jsr %%a6@(-654:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define LockDosList(___flags) ({\
  __LockDosList_base((DOS_BASE_NAME), ___flags);\
})

#define __UnLockDosList_base(__in_base, ___flags) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  ULONG __p____flags = (ULONG)(___flags);\
  register ULONG __v0 __asm("d1") = __p____flags;\
  __asm volatile (\
                   "jsr %%a6@(-660:W)\n"\
                   : "+d"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d0", "a0", "a1" );\
})

#define UnLockDosList(___flags) ({\
  __UnLockDosList_base((DOS_BASE_NAME), ___flags);\
})

#define __AttemptLockDosList_base(__in_base, ___flags) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  ULONG __p____flags = (ULONG)(___flags);\
  register struct DosList * __v_ret __asm("d0");\
  register ULONG __v0 __asm("d1") = __p____flags;\
  __asm volatile (\
                   "jsr %%a6@(-666:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define AttemptLockDosList(___flags) ({\
  __AttemptLockDosList_base((DOS_BASE_NAME), ___flags);\
})

#define __RemDosEntry_base(__in_base, ___dlist) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct DosList * __p____dlist = (struct DosList *)(___dlist);\
  register BOOL __v_ret __asm("d0");\
  register struct DosList * __v0 __asm("d1") = __p____dlist;\
  __asm volatile (\
                   "jsr %%a6@(-672:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define RemDosEntry(___dlist) ({\
  __RemDosEntry_base((DOS_BASE_NAME), ___dlist);\
})

#define __AddDosEntry_base(__in_base, ___dlist) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct DosList * __p____dlist = (struct DosList *)(___dlist);\
  register LONG __v_ret __asm("d0");\
  register struct DosList * __v0 __asm("d1") = __p____dlist;\
  __asm volatile (\
                   "jsr %%a6@(-678:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define AddDosEntry(___dlist) ({\
  __AddDosEntry_base((DOS_BASE_NAME), ___dlist);\
})

#define __FindDosEntry_base(__in_base, ___dlist, ___name, ___flags) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct DosList * __p____dlist = (struct DosList *)(___dlist);\
  STRPTR __p____name = (STRPTR)(___name);\
  ULONG __p____flags = (ULONG)(___flags);\
  register struct DosList * __v_ret __asm("d0");\
  register struct DosList * __v0 __asm("d1") = __p____dlist;\
  register STRPTR __v1 __asm("d2") = __p____name;\
  register ULONG __v2 __asm("d3") = __p____flags;\
  __asm volatile (\
                   "jsr %%a6@(-684:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1), "d"(__v2)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define FindDosEntry(___dlist, ___name, ___flags) ({\
  __FindDosEntry_base((DOS_BASE_NAME), ___dlist, ___name, ___flags);\
})

#define __NextDosEntry_base(__in_base, ___dlist, ___flags) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct DosList * __p____dlist = (struct DosList *)(___dlist);\
  ULONG __p____flags = (ULONG)(___flags);\
  register struct DosList * __v_ret __asm("d0");\
  register struct DosList * __v0 __asm("d1") = __p____dlist;\
  register ULONG __v1 __asm("d2") = __p____flags;\
  __asm volatile (\
                   "jsr %%a6@(-690:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define NextDosEntry(___dlist, ___flags) ({\
  __NextDosEntry_base((DOS_BASE_NAME), ___dlist, ___flags);\
})

#define __MakeDosEntry_base(__in_base, ___name, ___type) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  STRPTR __p____name = (STRPTR)(___name);\
  LONG __p____type = (LONG)(___type);\
  register struct DosList * __v_ret __asm("d0");\
  register STRPTR __v0 __asm("d1") = __p____name;\
  register LONG __v1 __asm("d2") = __p____type;\
  __asm volatile (\
                   "jsr %%a6@(-696:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define MakeDosEntry(___name, ___type) ({\
  __MakeDosEntry_base((DOS_BASE_NAME), ___name, ___type);\
})

#define __FreeDosEntry_base(__in_base, ___dlist) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct DosList * __p____dlist = (struct DosList *)(___dlist);\
  register struct DosList * __v0 __asm("d1") = __p____dlist;\
  __asm volatile (\
                   "jsr %%a6@(-702:W)\n"\
                   : "+d"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d0", "a0", "a1" );\
})

#define FreeDosEntry(___dlist) ({\
  __FreeDosEntry_base((DOS_BASE_NAME), ___dlist);\
})

#define __IsFileSystem_base(__in_base, ___name) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  STRPTR __p____name = (STRPTR)(___name);\
  register BOOL __v_ret __asm("d0");\
  register STRPTR __v0 __asm("d1") = __p____name;\
  __asm volatile (\
                   "jsr %%a6@(-708:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define IsFileSystem(___name) ({\
  __IsFileSystem_base((DOS_BASE_NAME), ___name);\
})

#define __Format_base(__in_base, ___filesystem, ___volumename, ___dostype) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  STRPTR __p____filesystem = (STRPTR)(___filesystem);\
  STRPTR __p____volumename = (STRPTR)(___volumename);\
  ULONG __p____dostype = (ULONG)(___dostype);\
  register BOOL __v_ret __asm("d0");\
  register STRPTR __v0 __asm("d1") = __p____filesystem;\
  register STRPTR __v1 __asm("d2") = __p____volumename;\
  register ULONG __v2 __asm("d3") = __p____dostype;\
  __asm volatile (\
                   "jsr %%a6@(-714:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1), "d"(__v2)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define Format(___filesystem, ___volumename, ___dostype) ({\
  __Format_base((DOS_BASE_NAME), ___filesystem, ___volumename, ___dostype);\
})

#define __Relabel_base(__in_base, ___drive, ___newname) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  STRPTR __p____drive = (STRPTR)(___drive);\
  STRPTR __p____newname = (STRPTR)(___newname);\
  register LONG __v_ret __asm("d0");\
  register STRPTR __v0 __asm("d1") = __p____drive;\
  register STRPTR __v1 __asm("d2") = __p____newname;\
  __asm volatile (\
                   "jsr %%a6@(-720:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define Relabel(___drive, ___newname) ({\
  __Relabel_base((DOS_BASE_NAME), ___drive, ___newname);\
})

#define __Inhibit_base(__in_base, ___name, ___onoff) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  STRPTR __p____name = (STRPTR)(___name);\
  LONG __p____onoff = (LONG)(___onoff);\
  register LONG __v_ret __asm("d0");\
  register STRPTR __v0 __asm("d1") = __p____name;\
  register LONG __v1 __asm("d2") = __p____onoff;\
  __asm volatile (\
                   "jsr %%a6@(-726:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define Inhibit(___name, ___onoff) ({\
  __Inhibit_base((DOS_BASE_NAME), ___name, ___onoff);\
})

#define __AddBuffers_base(__in_base, ___name, ___number) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  STRPTR __p____name = (STRPTR)(___name);\
  LONG __p____number = (LONG)(___number);\
  register LONG __v_ret __asm("d0");\
  register STRPTR __v0 __asm("d1") = __p____name;\
  register LONG __v1 __asm("d2") = __p____number;\
  __asm volatile (\
                   "jsr %%a6@(-732:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define AddBuffers(___name, ___number) ({\
  __AddBuffers_base((DOS_BASE_NAME), ___name, ___number);\
})

#define __CompareDates_base(__in_base, ___date1, ___date2) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct DateStamp * __p____date1 = (struct DateStamp *)(___date1);\
  struct DateStamp * __p____date2 = (struct DateStamp *)(___date2);\
  register LONG __v_ret __asm("d0");\
  register struct DateStamp * __v0 __asm("d1") = __p____date1;\
  register struct DateStamp * __v1 __asm("d2") = __p____date2;\
  __asm volatile (\
                   "jsr %%a6@(-738:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define CompareDates(___date1, ___date2) ({\
  __CompareDates_base((DOS_BASE_NAME), ___date1, ___date2);\
})

#define __DateToStr_base(__in_base, ___datetime) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct DateTime * __p____datetime = (struct DateTime *)(___datetime);\
  register LONG __v_ret __asm("d0");\
  register struct DateTime * __v0 __asm("d1") = __p____datetime;\
  __asm volatile (\
                   "jsr %%a6@(-744:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define DateToStr(___datetime) ({\
  __DateToStr_base((DOS_BASE_NAME), ___datetime);\
})

#define __StrToDate_base(__in_base, ___datetime) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct DateTime * __p____datetime = (struct DateTime *)(___datetime);\
  register LONG __v_ret __asm("d0");\
  register struct DateTime * __v0 __asm("d1") = __p____datetime;\
  __asm volatile (\
                   "jsr %%a6@(-750:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define StrToDate(___datetime) ({\
  __StrToDate_base((DOS_BASE_NAME), ___datetime);\
})

#define __InternalLoadSeg_base(__in_base, ___fh, ___table, ___funcarray, ___stack) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  BPTR __p____fh = (BPTR)(___fh);\
  BPTR __p____table = (BPTR)(___table);\
  LONG * __p____funcarray = (LONG *)(___funcarray);\
  LONG * __p____stack = (LONG *)(___stack);\
  register BPTR __v_ret __asm("d0");\
  register BPTR __v0 __asm("d0") = __p____fh;\
  register BPTR __v1 __asm("a0") = __p____table;\
  register LONG * __v2 __asm("a1") = __p____funcarray;\
  register LONG * __v3 __asm("a2") = __p____stack;\
  __asm volatile (\
                   "jsr %%a6@(-756:W)\n"\
                   : "=d"(__v_ret), "+a"(__v1), "+a"(__v2)\
                   : "a"(__p__in_base), "d"(__v0), "a"(__v3)\
                   : "fp0", "fp1", "cc", "memory", "d1" );\
  __v_ret;})

#define InternalLoadSeg(___fh, ___table, ___funcarray, ___stack) ({\
  __InternalLoadSeg_base((DOS_BASE_NAME), ___fh, ___table, ___funcarray, ___stack);\
})

#define __InternalUnLoadSeg_base(__in_base, ___seglist, ___freefunc) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  BPTR __p____seglist = (BPTR)(___seglist);\
  void *__p____freefunc = (void *)(___freefunc);\
  register BOOL __v_ret __asm("d0");\
  register BPTR __v0 __asm("d1") = __p____seglist;\
  register VOID (*__v1)() __asm("a1") = __p____freefunc;\
  __asm volatile (\
                   "jsr %%a6@(-762:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0), "+a"(__v1)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "a0" );\
  __v_ret;})

#define InternalUnLoadSeg(___seglist, ___freefunc) ({\
  __InternalUnLoadSeg_base((DOS_BASE_NAME), ___seglist, ___freefunc);\
})

#define __NewLoadSeg_base(__in_base, ___file, ___tags) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  STRPTR __p____file = (STRPTR)(___file);\
  struct TagItem * __p____tags = (struct TagItem *)(___tags);\
  register BPTR __v_ret __asm("d0");\
  register STRPTR __v0 __asm("d1") = __p____file;\
  register struct TagItem * __v1 __asm("d2") = __p____tags;\
  __asm volatile (\
                   "jsr %%a6@(-768:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define NewLoadSeg(___file, ___tags) ({\
  __NewLoadSeg_base((DOS_BASE_NAME), ___file, ___tags);\
})

#define __NewLoadSegTagList_base(__in_base, ___file, ___tags) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  STRPTR __p____file = (STRPTR)(___file);\
  struct TagItem * __p____tags = (struct TagItem *)(___tags);\
  register BPTR __v_ret __asm("d0");\
  register STRPTR __v0 __asm("d1") = __p____file;\
  register struct TagItem * __v1 __asm("d2") = __p____tags;\
  __asm volatile (\
                   "jsr %%a6@(-768:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define NewLoadSegTagList(___file, ___tags) ({\
  __NewLoadSegTagList_base((DOS_BASE_NAME), ___file, ___tags);\
})

AMIGA_VA_WRAPPER_ATTR
static __stdargs BPTR __NewLoadSegTags_va(void *const __base __asm("a6"), CONST_STRPTR ___file, ULONG ___tag1type, ...)
{
    const ULONG *tags = (const ULONG *)&___tag1type;
    return __NewLoadSegTagList_base(__base, ___file, (CONST struct TagItem *)tags);
}

#define NewLoadSegTags(...) __NewLoadSegTags_va(DOS_BASE_NAME, __VA_ARGS__)

#define __AddSegment_base(__in_base, ___name, ___seg, ___system) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  STRPTR __p____name = (STRPTR)(___name);\
  BPTR __p____seg = (BPTR)(___seg);\
  LONG __p____system = (LONG)(___system);\
  register LONG __v_ret __asm("d0");\
  register STRPTR __v0 __asm("d1") = __p____name;\
  register BPTR __v1 __asm("d2") = __p____seg;\
  register LONG __v2 __asm("d3") = __p____system;\
  __asm volatile (\
                   "jsr %%a6@(-774:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1), "d"(__v2)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define AddSegment(___name, ___seg, ___system) ({\
  __AddSegment_base((DOS_BASE_NAME), ___name, ___seg, ___system);\
})

#define __FindSegment_base(__in_base, ___name, ___seg, ___system) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  STRPTR __p____name = (STRPTR)(___name);\
  struct Segment * __p____seg = (struct Segment *)(___seg);\
  LONG __p____system = (LONG)(___system);\
  register struct Segment * __v_ret __asm("d0");\
  register STRPTR __v0 __asm("d1") = __p____name;\
  register struct Segment * __v1 __asm("d2") = __p____seg;\
  register LONG __v2 __asm("d3") = __p____system;\
  __asm volatile (\
                   "jsr %%a6@(-780:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1), "d"(__v2)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define FindSegment(___name, ___seg, ___system) ({\
  __FindSegment_base((DOS_BASE_NAME), ___name, ___seg, ___system);\
})

#define __RemSegment_base(__in_base, ___seg) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct Segment * __p____seg = (struct Segment *)(___seg);\
  register LONG __v_ret __asm("d0");\
  register struct Segment * __v0 __asm("d1") = __p____seg;\
  __asm volatile (\
                   "jsr %%a6@(-786:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define RemSegment(___seg) ({\
  __RemSegment_base((DOS_BASE_NAME), ___seg);\
})

#define __CheckSignal_base(__in_base, ___mask) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  LONG __p____mask = (LONG)(___mask);\
  register LONG __v_ret __asm("d0");\
  register LONG __v0 __asm("d1") = __p____mask;\
  __asm volatile (\
                   "jsr %%a6@(-792:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define CheckSignal(___mask) ({\
  __CheckSignal_base((DOS_BASE_NAME), ___mask);\
})

#define __ReadArgs_base(__in_base, ___arg_template, ___array, ___args) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  STRPTR __p____arg_template = (STRPTR)(___arg_template);\
  LONG * __p____array = (LONG *)(___array);\
  struct RDArgs * __p____args = (struct RDArgs *)(___args);\
  register struct RDArgs * __v_ret __asm("d0");\
  register STRPTR __v0 __asm("d1") = __p____arg_template;\
  register LONG * __v1 __asm("d2") = __p____array;\
  register struct RDArgs * __v2 __asm("d3") = __p____args;\
  __asm volatile (\
                   "jsr %%a6@(-798:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1), "d"(__v2)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define ReadArgs(___arg_template, ___array, ___args) ({\
  __ReadArgs_base((DOS_BASE_NAME), ___arg_template, ___array, ___args);\
})

#define __FindArg_base(__in_base, ___keyword, ___arg_template) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  STRPTR __p____keyword = (STRPTR)(___keyword);\
  STRPTR __p____arg_template = (STRPTR)(___arg_template);\
  register LONG __v_ret __asm("d0");\
  register STRPTR __v0 __asm("d1") = __p____keyword;\
  register STRPTR __v1 __asm("d2") = __p____arg_template;\
  __asm volatile (\
                   "jsr %%a6@(-804:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define FindArg(___keyword, ___arg_template) ({\
  __FindArg_base((DOS_BASE_NAME), ___keyword, ___arg_template);\
})

#define __ReadItem_base(__in_base, ___name, ___maxchars, ___cSource) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  STRPTR __p____name = (STRPTR)(___name);\
  LONG __p____maxchars = (LONG)(___maxchars);\
  struct CSource * __p____cSource = (struct CSource *)(___cSource);\
  register LONG __v_ret __asm("d0");\
  register STRPTR __v0 __asm("d1") = __p____name;\
  register LONG __v1 __asm("d2") = __p____maxchars;\
  register struct CSource * __v2 __asm("d3") = __p____cSource;\
  __asm volatile (\
                   "jsr %%a6@(-810:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1), "d"(__v2)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define ReadItem(___name, ___maxchars, ___cSource) ({\
  __ReadItem_base((DOS_BASE_NAME), ___name, ___maxchars, ___cSource);\
})

#define __StrToLong_base(__in_base, ___string, ___value) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  STRPTR __p____string = (STRPTR)(___string);\
  LONG * __p____value = (LONG *)(___value);\
  register LONG __v_ret __asm("d0");\
  register STRPTR __v0 __asm("d1") = __p____string;\
  register LONG * __v1 __asm("d2") = __p____value;\
  __asm volatile (\
                   "jsr %%a6@(-816:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define StrToLong(___string, ___value) ({\
  __StrToLong_base((DOS_BASE_NAME), ___string, ___value);\
})

#define __MatchFirst_base(__in_base, ___pat, ___anchor) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  STRPTR __p____pat = (STRPTR)(___pat);\
  struct AnchorPath * __p____anchor = (struct AnchorPath *)(___anchor);\
  register LONG __v_ret __asm("d0");\
  register STRPTR __v0 __asm("d1") = __p____pat;\
  register struct AnchorPath * __v1 __asm("d2") = __p____anchor;\
  __asm volatile (\
                   "jsr %%a6@(-822:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define MatchFirst(___pat, ___anchor) ({\
  __MatchFirst_base((DOS_BASE_NAME), ___pat, ___anchor);\
})

#define __MatchNext_base(__in_base, ___anchor) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct AnchorPath * __p____anchor = (struct AnchorPath *)(___anchor);\
  register LONG __v_ret __asm("d0");\
  register struct AnchorPath * __v0 __asm("d1") = __p____anchor;\
  __asm volatile (\
                   "jsr %%a6@(-828:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define MatchNext(___anchor) ({\
  __MatchNext_base((DOS_BASE_NAME), ___anchor);\
})

#define __MatchEnd_base(__in_base, ___anchor) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct AnchorPath * __p____anchor = (struct AnchorPath *)(___anchor);\
  register struct AnchorPath * __v0 __asm("d1") = __p____anchor;\
  __asm volatile (\
                   "jsr %%a6@(-834:W)\n"\
                   : "+d"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d0", "a0", "a1" );\
})

#define MatchEnd(___anchor) ({\
  __MatchEnd_base((DOS_BASE_NAME), ___anchor);\
})

#define __ParsePattern_base(__in_base, ___pat, ___patbuf, ___patbuflen) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  STRPTR __p____pat = (STRPTR)(___pat);\
  UBYTE * __p____patbuf = (UBYTE *)(___patbuf);\
  LONG __p____patbuflen = (LONG)(___patbuflen);\
  register LONG __v_ret __asm("d0");\
  register STRPTR __v0 __asm("d1") = __p____pat;\
  register UBYTE * __v1 __asm("d2") = __p____patbuf;\
  register LONG __v2 __asm("d3") = __p____patbuflen;\
  __asm volatile (\
                   "jsr %%a6@(-840:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1), "d"(__v2)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define ParsePattern(___pat, ___patbuf, ___patbuflen) ({\
  __ParsePattern_base((DOS_BASE_NAME), ___pat, ___patbuf, ___patbuflen);\
})

#define __MatchPattern_base(__in_base, ___patbuf, ___str) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  UBYTE * __p____patbuf = (UBYTE *)(___patbuf);\
  STRPTR __p____str = (STRPTR)(___str);\
  register BOOL __v_ret __asm("d0");\
  register UBYTE * __v0 __asm("d1") = __p____patbuf;\
  register STRPTR __v1 __asm("d2") = __p____str;\
  __asm volatile (\
                   "jsr %%a6@(-846:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define MatchPattern(___patbuf, ___str) ({\
  __MatchPattern_base((DOS_BASE_NAME), ___patbuf, ___str);\
})

#define __FreeArgs_base(__in_base, ___args) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct RDArgs * __p____args = (struct RDArgs *)(___args);\
  register struct RDArgs * __v0 __asm("d1") = __p____args;\
  __asm volatile (\
                   "jsr %%a6@(-858:W)\n"\
                   : "+d"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d0", "a0", "a1" );\
})

#define FreeArgs(___args) ({\
  __FreeArgs_base((DOS_BASE_NAME), ___args);\
})

#define __FilePart_base(__in_base, ___path) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  STRPTR __p____path = (STRPTR)(___path);\
  register STRPTR __v_ret __asm("d0");\
  register STRPTR __v0 __asm("d1") = __p____path;\
  __asm volatile (\
                   "jsr %%a6@(-870:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define FilePart(___path) ({\
  __FilePart_base((DOS_BASE_NAME), ___path);\
})

#define __PathPart_base(__in_base, ___path) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  STRPTR __p____path = (STRPTR)(___path);\
  register STRPTR __v_ret __asm("d0");\
  register STRPTR __v0 __asm("d1") = __p____path;\
  __asm volatile (\
                   "jsr %%a6@(-876:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define PathPart(___path) ({\
  __PathPart_base((DOS_BASE_NAME), ___path);\
})

#define __AddPart_base(__in_base, ___dirname, ___filename, ___size) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  STRPTR __p____dirname = (STRPTR)(___dirname);\
  STRPTR __p____filename = (STRPTR)(___filename);\
  ULONG __p____size = (ULONG)(___size);\
  register BOOL __v_ret __asm("d0");\
  register STRPTR __v0 __asm("d1") = __p____dirname;\
  register STRPTR __v1 __asm("d2") = __p____filename;\
  register ULONG __v2 __asm("d3") = __p____size;\
  __asm volatile (\
                   "jsr %%a6@(-882:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1), "d"(__v2)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define AddPart(___dirname, ___filename, ___size) ({\
  __AddPart_base((DOS_BASE_NAME), ___dirname, ___filename, ___size);\
})

#define __StartNotify_base(__in_base, ___notify) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct NotifyRequest * __p____notify = (struct NotifyRequest *)(___notify);\
  register BOOL __v_ret __asm("d0");\
  register struct NotifyRequest * __v0 __asm("d1") = __p____notify;\
  __asm volatile (\
                   "jsr %%a6@(-888:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define StartNotify(___notify) ({\
  __StartNotify_base((DOS_BASE_NAME), ___notify);\
})

#define __EndNotify_base(__in_base, ___notify) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct NotifyRequest * __p____notify = (struct NotifyRequest *)(___notify);\
  register struct NotifyRequest * __v0 __asm("d1") = __p____notify;\
  __asm volatile (\
                   "jsr %%a6@(-894:W)\n"\
                   : "+d"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d0", "a0", "a1" );\
})

#define EndNotify(___notify) ({\
  __EndNotify_base((DOS_BASE_NAME), ___notify);\
})

#define __SetVar_base(__in_base, ___name, ___buffer, ___size, ___flags) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  STRPTR __p____name = (STRPTR)(___name);\
  STRPTR __p____buffer = (STRPTR)(___buffer);\
  LONG __p____size = (LONG)(___size);\
  LONG __p____flags = (LONG)(___flags);\
  register BOOL __v_ret __asm("d0");\
  register STRPTR __v0 __asm("d1") = __p____name;\
  register STRPTR __v1 __asm("d2") = __p____buffer;\
  register LONG __v2 __asm("d3") = __p____size;\
  register LONG __v3 __asm("d4") = __p____flags;\
  __asm volatile (\
                   "jsr %%a6@(-900:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1), "d"(__v2), "d"(__v3)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define SetVar(___name, ___buffer, ___size, ___flags) ({\
  __SetVar_base((DOS_BASE_NAME), ___name, ___buffer, ___size, ___flags);\
})

#define __GetVar_base(__in_base, ___name, ___buffer, ___size, ___flags) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  STRPTR __p____name = (STRPTR)(___name);\
  STRPTR __p____buffer = (STRPTR)(___buffer);\
  LONG __p____size = (LONG)(___size);\
  LONG __p____flags = (LONG)(___flags);\
  register LONG __v_ret __asm("d0");\
  register STRPTR __v0 __asm("d1") = __p____name;\
  register STRPTR __v1 __asm("d2") = __p____buffer;\
  register LONG __v2 __asm("d3") = __p____size;\
  register LONG __v3 __asm("d4") = __p____flags;\
  __asm volatile (\
                   "jsr %%a6@(-906:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1), "d"(__v2), "d"(__v3)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define GetVar(___name, ___buffer, ___size, ___flags) ({\
  __GetVar_base((DOS_BASE_NAME), ___name, ___buffer, ___size, ___flags);\
})

#define __DeleteVar_base(__in_base, ___name, ___flags) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  STRPTR __p____name = (STRPTR)(___name);\
  ULONG __p____flags = (ULONG)(___flags);\
  register LONG __v_ret __asm("d0");\
  register STRPTR __v0 __asm("d1") = __p____name;\
  register ULONG __v1 __asm("d2") = __p____flags;\
  __asm volatile (\
                   "jsr %%a6@(-912:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define DeleteVar(___name, ___flags) ({\
  __DeleteVar_base((DOS_BASE_NAME), ___name, ___flags);\
})

#define __FindVar_base(__in_base, ___name, ___type) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  STRPTR __p____name = (STRPTR)(___name);\
  ULONG __p____type = (ULONG)(___type);\
  register struct LocalVar * __v_ret __asm("d0");\
  register STRPTR __v0 __asm("d1") = __p____name;\
  register ULONG __v1 __asm("d2") = __p____type;\
  __asm volatile (\
                   "jsr %%a6@(-918:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define FindVar(___name, ___type) ({\
  __FindVar_base((DOS_BASE_NAME), ___name, ___type);\
})

#define __CliInitNewcli_base(__in_base, ___dp) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct DosPacket * __p____dp = (struct DosPacket *)(___dp);\
  register LONG __v_ret __asm("d0");\
  register struct DosPacket * __v0 __asm("a0") = __p____dp;\
  __asm volatile (\
                   "jsr %%a6@(-930:W)\n"\
                   : "=d"(__v_ret), "+a"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d1", "a1" );\
  __v_ret;})

#define CliInitNewcli(___dp) ({\
  __CliInitNewcli_base((DOS_BASE_NAME), ___dp);\
})

#define __CliInitRun_base(__in_base, ___dp) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct DosPacket * __p____dp = (struct DosPacket *)(___dp);\
  register LONG __v_ret __asm("d0");\
  register struct DosPacket * __v0 __asm("a0") = __p____dp;\
  __asm volatile (\
                   "jsr %%a6@(-936:W)\n"\
                   : "=d"(__v_ret), "+a"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d1", "a1" );\
  __v_ret;})

#define CliInitRun(___dp) ({\
  __CliInitRun_base((DOS_BASE_NAME), ___dp);\
})

#define __WriteChars_base(__in_base, ___buf, ___buflen) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  STRPTR __p____buf = (STRPTR)(___buf);\
  ULONG __p____buflen = (ULONG)(___buflen);\
  register LONG __v_ret __asm("d0");\
  register STRPTR __v0 __asm("d1") = __p____buf;\
  register ULONG __v1 __asm("d2") = __p____buflen;\
  __asm volatile (\
                   "jsr %%a6@(-942:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define WriteChars(___buf, ___buflen) ({\
  __WriteChars_base((DOS_BASE_NAME), ___buf, ___buflen);\
})

#define __PutStr_base(__in_base, ___str) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  STRPTR __p____str = (STRPTR)(___str);\
  register LONG __v_ret __asm("d0");\
  register STRPTR __v0 __asm("d1") = __p____str;\
  __asm volatile (\
                   "jsr %%a6@(-948:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define PutStr(___str) ({\
  __PutStr_base((DOS_BASE_NAME), ___str);\
})

#define __VPrintf_base(__in_base, ___format, ___argarray) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  STRPTR __p____format = (STRPTR)(___format);\
  APTR __p____argarray = (APTR)(___argarray);\
  register LONG __v_ret __asm("d0");\
  register STRPTR __v0 __asm("d1") = __p____format;\
  register APTR __v1 __asm("d2") = __p____argarray;\
  __asm volatile (\
                   "jsr %%a6@(-954:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define VPrintf(___format, ___argarray) ({\
  __VPrintf_base((DOS_BASE_NAME), ___format, ___argarray);\
})

AMIGA_VA_WRAPPER_ATTR
static __stdargs LONG __Printf_va(void *const __base __asm("a6"), CONST_STRPTR ___format, ...)
{
    const void *args = (const void *)(&___format + 1);
    return __VPrintf_base(__base, ___format, args);
}

#define Printf(___format, ...) __Printf_va(DOS_BASE_NAME, ___format, ## __VA_ARGS__)

#define __ParsePatternNoCase_base(__in_base, ___pat, ___patbuf, ___patbuflen) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  STRPTR __p____pat = (STRPTR)(___pat);\
  UBYTE * __p____patbuf = (UBYTE *)(___patbuf);\
  LONG __p____patbuflen = (LONG)(___patbuflen);\
  register LONG __v_ret __asm("d0");\
  register STRPTR __v0 __asm("d1") = __p____pat;\
  register UBYTE * __v1 __asm("d2") = __p____patbuf;\
  register LONG __v2 __asm("d3") = __p____patbuflen;\
  __asm volatile (\
                   "jsr %%a6@(-966:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1), "d"(__v2)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define ParsePatternNoCase(___pat, ___patbuf, ___patbuflen) ({\
  __ParsePatternNoCase_base((DOS_BASE_NAME), ___pat, ___patbuf, ___patbuflen);\
})

#define __MatchPatternNoCase_base(__in_base, ___patbuf, ___str) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  UBYTE * __p____patbuf = (UBYTE *)(___patbuf);\
  STRPTR __p____str = (STRPTR)(___str);\
  register BOOL __v_ret __asm("d0");\
  register UBYTE * __v0 __asm("d1") = __p____patbuf;\
  register STRPTR __v1 __asm("d2") = __p____str;\
  __asm volatile (\
                   "jsr %%a6@(-972:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define MatchPatternNoCase(___patbuf, ___str) ({\
  __MatchPatternNoCase_base((DOS_BASE_NAME), ___patbuf, ___str);\
})

#define __SameDevice_base(__in_base, ___lock1, ___lock2) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  BPTR __p____lock1 = (BPTR)(___lock1);\
  BPTR __p____lock2 = (BPTR)(___lock2);\
  register BOOL __v_ret __asm("d0");\
  register BPTR __v0 __asm("d1") = __p____lock1;\
  register BPTR __v1 __asm("d2") = __p____lock2;\
  __asm volatile (\
                   "jsr %%a6@(-984:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define SameDevice(___lock1, ___lock2) ({\
  __SameDevice_base((DOS_BASE_NAME), ___lock1, ___lock2);\
})

#define __ExAllEnd_base(__in_base, ___lock, ___buffer, ___size, ___data, ___control) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  BPTR __p____lock = (BPTR)(___lock);\
  struct ExAllData * __p____buffer = (struct ExAllData *)(___buffer);\
  LONG __p____size = (LONG)(___size);\
  LONG __p____data = (LONG)(___data);\
  struct ExAllControl * __p____control = (struct ExAllControl *)(___control);\
  register BPTR __v0 __asm("d1") = __p____lock;\
  register struct ExAllData * __v1 __asm("d2") = __p____buffer;\
  register LONG __v2 __asm("d3") = __p____size;\
  register LONG __v3 __asm("d4") = __p____data;\
  register struct ExAllControl * __v4 __asm("d5") = __p____control;\
  __asm volatile (\
                   "jsr %%a6@(-990:W)\n"\
                   : "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1), "d"(__v2), "d"(__v3), "d"(__v4)\
                   : "fp0", "fp1", "cc", "memory", "d0", "a0", "a1" );\
})

#define ExAllEnd(___lock, ___buffer, ___size, ___data, ___control) ({\
  __ExAllEnd_base((DOS_BASE_NAME), ___lock, ___buffer, ___size, ___data, ___control);\
})

#define __SetOwner_base(__in_base, ___name, ___owner_info) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  STRPTR __p____name = (STRPTR)(___name);\
  LONG __p____owner_info = (LONG)(___owner_info);\
  register BOOL __v_ret __asm("d0");\
  register STRPTR __v0 __asm("d1") = __p____name;\
  register LONG __v1 __asm("d2") = __p____owner_info;\
  __asm volatile (\
                   "jsr %%a6@(-996:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define SetOwner(___name, ___owner_info) ({\
  __SetOwner_base((DOS_BASE_NAME), ___name, ___owner_info);\
})

#define __VolumeRequestHook_base(__in_base, ___vol) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  STRPTR __p____vol = (STRPTR)(___vol);\
  register LONG __v_ret __asm("d0");\
  register STRPTR __v0 __asm("d1") = __p____vol;\
  __asm volatile (\
                   "jsr %%a6@(-1014:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define VolumeRequestHook(___vol) ({\
  __VolumeRequestHook_base((DOS_BASE_NAME), ___vol);\
})

#define __GetCurrentDir_base(__in_base) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  register BPTR __v_ret __asm("d0");\
  __asm volatile (\
                   "jsr %%a6@(-1026:W)\n"\
                   : "=d"(__v_ret)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d1", "a0", "a1" );\
  __v_ret;})

#define GetCurrentDir() ({\
  __GetCurrentDir_base((DOS_BASE_NAME));\
})

#define __PutErrStr_base(__in_base, ___str) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  STRPTR __p____str = (STRPTR)(___str);\
  register LONG __v_ret __asm("d0");\
  register STRPTR __v0 __asm("d1") = __p____str;\
  __asm volatile (\
                   "jsr %%a6@(-1128:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define PutErrStr(___str) ({\
  __PutErrStr_base((DOS_BASE_NAME), ___str);\
})

#define __ErrorOutput_base(__in_base) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  register LONG __v_ret __asm("d0");\
  __asm volatile (\
                   "jsr %%a6@(-1134:W)\n"\
                   : "=d"(__v_ret)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d1", "a0", "a1" );\
  __v_ret;})

#define ErrorOutput() ({\
  __ErrorOutput_base((DOS_BASE_NAME));\
})

#define __SelectError_base(__in_base, ___fh) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  BPTR __p____fh = (BPTR)(___fh);\
  register LONG __v_ret __asm("d0");\
  register BPTR __v0 __asm("d1") = __p____fh;\
  __asm volatile (\
                   "jsr %%a6@(-1140:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define SelectError(___fh) ({\
  __SelectError_base((DOS_BASE_NAME), ___fh);\
})

#define __DoShellMethodTagList_base(__in_base, ___method, ___tags) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  ULONG __p____method = (ULONG)(___method);\
  struct TagItem * __p____tags = (struct TagItem *)(___tags);\
  register APTR __v_ret __asm("d0");\
  register ULONG __v0 __asm("d0") = __p____method;\
  register struct TagItem * __v1 __asm("a0") = __p____tags;\
  __asm volatile (\
                   "jsr %%a6@(-1152:W)\n"\
                   : "=d"(__v_ret), "+a"(__v1)\
                   : "a"(__p__in_base), "d"(__v0)\
                   : "fp0", "fp1", "cc", "memory", "d1", "a1" );\
  __v_ret;})

#define DoShellMethodTagList(___method, ___tags) ({\
  __DoShellMethodTagList_base((DOS_BASE_NAME), ___method, ___tags);\
})

AMIGA_VA_WRAPPER_ATTR
static __stdargs APTR __DoShellMethod_va(void *const __base __asm("a6"), ULONG ___method, ULONG ___tag1type, ...)
{
    const ULONG *tags = (const ULONG *)&___tag1type;
    return __DoShellMethodTagList_base(__base, ___method, (CONST struct TagItem *)tags);
}

#define DoShellMethod(...) __DoShellMethod_va(DOS_BASE_NAME, __VA_ARGS__)

#define __ScanStackToken_base(__in_base, ___seg, ___defaultstack) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  BPTR __p____seg = (BPTR)(___seg);\
  LONG __p____defaultstack = (LONG)(___defaultstack);\
  register LONG __v_ret __asm("d0");\
  register BPTR __v0 __asm("d1") = __p____seg;\
  register LONG __v1 __asm("d2") = __p____defaultstack;\
  __asm volatile (\
                   "jsr %%a6@(-1158:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base), "d"(__v1)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define ScanStackToken(___seg, ___defaultstack) ({\
  __ScanStackToken_base((DOS_BASE_NAME), ___seg, ___defaultstack);\
})

#endif /* !_INLINE_DOS_H */
