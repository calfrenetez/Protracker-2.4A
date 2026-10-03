/* Automatically generated header (sfdc 1.12)! Do not edit! */

#ifndef _INLINE_EXEC_H
#define _INLINE_EXEC_H

#ifndef _PROTO_EXEC_H
#include <proto/exec.h>
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

#ifndef EXEC_BASE_NAME
#define EXEC_BASE_NAME SysBase
#endif /* !EXEC_BASE_NAME */

#define __Supervisor_base(__in_base, ___userFunction) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  void *__p____userFunction = (void *)(___userFunction);\
  register ULONG __v_ret __asm("d0");\
  register ULONG (*__v0)() __asm("d7") = __p____userFunction;\
  __asm volatile (\
                   "exg %%d7,%%a5\n"\
                   "jsr %%a6@(-30:W)\n"\
                   "exg %%d7,%%a5\n"\
                   : "=d"(__v_ret)\
                   : "a"(__p__in_base), "d"(__v0)\
                   : "fp0", "fp1", "cc", "memory", "d1", "a0", "a1" );\
  __v_ret;})

#define Supervisor(___userFunction) ({\
  __Supervisor_base((EXEC_BASE_NAME), ___userFunction);\
})

#define __InitCode_base(__in_base, ___startClass, ___version) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  ULONG __p____startClass = (ULONG)(___startClass);\
  ULONG __p____version = (ULONG)(___version);\
  register ULONG __v0 __asm("d0") = __p____startClass;\
  register ULONG __v1 __asm("d1") = __p____version;\
  __asm volatile (\
                   "jsr %%a6@(-72:W)\n"\
                   : "+d"(__v0), "+d"(__v1)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
})

#define InitCode(___startClass, ___version) ({\
  __InitCode_base((EXEC_BASE_NAME), ___startClass, ___version);\
})

#define __InitStruct_base(__in_base, ___initTable, ___memory, ___size) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  APTR __p____initTable = (APTR)(___initTable);\
  APTR __p____memory = (APTR)(___memory);\
  ULONG __p____size = (ULONG)(___size);\
  register APTR __v0 __asm("a1") = __p____initTable;\
  register APTR __v1 __asm("a2") = __p____memory;\
  register ULONG __v2 __asm("d0") = __p____size;\
  __asm volatile (\
                   "jsr %%a6@(-78:W)\n"\
                   : "+a"(__v0), "+d"(__v2)\
                   : "a"(__p__in_base), "a"(__v1)\
                   : "fp0", "fp1", "cc", "memory", "d1", "a0" );\
})

#define InitStruct(___initTable, ___memory, ___size) ({\
  __InitStruct_base((EXEC_BASE_NAME), ___initTable, ___memory, ___size);\
})

#define __MakeLibrary_base(__in_base, ___funcInit, ___structInit, ___libInit, ___dataSize, ___segList) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  APTR __p____funcInit = (APTR)(___funcInit);\
  APTR __p____structInit = (APTR)(___structInit);\
  void *__p____libInit = (void *)(___libInit);\
  ULONG __p____dataSize = (ULONG)(___dataSize);\
  ULONG __p____segList = (ULONG)(___segList);\
  register struct Library * __v_ret __asm("d0");\
  register APTR __v0 __asm("a0") = __p____funcInit;\
  register APTR __v1 __asm("a1") = __p____structInit;\
  register ULONG (*__v2)() __asm("a2") = __p____libInit;\
  register ULONG __v3 __asm("d0") = __p____dataSize;\
  register ULONG __v4 __asm("d1") = __p____segList;\
  __asm volatile (\
                   "jsr %%a6@(-84:W)\n"\
                   : "=d"(__v_ret), "+a"(__v0), "+a"(__v1), "+d"(__v4)\
                   : "a"(__p__in_base), "a"(__v2), "d"(__v3)\
                   : "fp0", "fp1", "cc", "memory" );\
  __v_ret;})

#define MakeLibrary(___funcInit, ___structInit, ___libInit, ___dataSize, ___segList) ({\
  __MakeLibrary_base((EXEC_BASE_NAME), ___funcInit, ___structInit, ___libInit, ___dataSize, ___segList);\
})

#define __MakeFunctions_base(__in_base, ___target, ___functionArray, ___funcDispBase) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  APTR __p____target = (APTR)(___target);\
  APTR __p____functionArray = (APTR)(___functionArray);\
  ULONG __p____funcDispBase = (ULONG)(___funcDispBase);\
  register APTR __v0 __asm("a0") = __p____target;\
  register APTR __v1 __asm("a1") = __p____functionArray;\
  register ULONG __v2 __asm("a2") = __p____funcDispBase;\
  __asm volatile (\
                   "jsr %%a6@(-90:W)\n"\
                   : "+a"(__v0), "+a"(__v1)\
                   : "a"(__p__in_base), "a"(__v2)\
                   : "fp0", "fp1", "cc", "memory", "d0", "d1" );\
})

#define MakeFunctions(___target, ___functionArray, ___funcDispBase) ({\
  __MakeFunctions_base((EXEC_BASE_NAME), ___target, ___functionArray, ___funcDispBase);\
})

#define __FindResident_base(__in_base, ___name) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  STRPTR __p____name = (STRPTR)(___name);\
  register struct Resident * __v_ret __asm("d0");\
  register STRPTR __v0 __asm("a1") = __p____name;\
  __asm volatile (\
                   "jsr %%a6@(-96:W)\n"\
                   : "=d"(__v_ret), "+a"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d1", "a0" );\
  __v_ret;})

#define FindResident(___name) ({\
  __FindResident_base((EXEC_BASE_NAME), ___name);\
})

#define __InitResident_base(__in_base, ___resident, ___segList) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct Resident * __p____resident = (struct Resident *)(___resident);\
  ULONG __p____segList = (ULONG)(___segList);\
  register APTR __v_ret __asm("d0");\
  register struct Resident * __v0 __asm("a1") = __p____resident;\
  register ULONG __v1 __asm("d1") = __p____segList;\
  __asm volatile (\
                   "jsr %%a6@(-102:W)\n"\
                   : "=d"(__v_ret), "+a"(__v0), "+d"(__v1)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "a0" );\
  __v_ret;})

#define InitResident(___resident, ___segList) ({\
  __InitResident_base((EXEC_BASE_NAME), ___resident, ___segList);\
})

#define __Alert_base(__in_base, ___alertNum) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  ULONG __p____alertNum = (ULONG)(___alertNum);\
  register ULONG __v0 __asm("d7") = __p____alertNum;\
  __asm volatile (\
                   "jsr %%a6@(-108:W)\n"\
                   :\
                   : "a"(__p__in_base), "d"(__v0)\
                   : "fp0", "fp1", "cc", "memory", "d0", "d1", "a0", "a1" );\
})

#define Alert(___alertNum) ({\
  __Alert_base((EXEC_BASE_NAME), ___alertNum);\
})

#define __Debug_base(__in_base, ___flags) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  ULONG __p____flags = (ULONG)(___flags);\
  register ULONG __v0 __asm("d0") = __p____flags;\
  __asm volatile (\
                   "jsr %%a6@(-114:W)\n"\
                   : "+d"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d1", "a0", "a1" );\
})

#define Debug(___flags) ({\
  __Debug_base((EXEC_BASE_NAME), ___flags);\
})

#define __Disable_base(__in_base) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  __asm volatile (\
                   "jsr %%a6@(-120:W)\n"\
                   :\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d0", "d1", "a0", "a1" );\
})

#define Disable() ({\
  __Disable_base((EXEC_BASE_NAME));\
})

#define __Enable_base(__in_base) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  __asm volatile (\
                   "jsr %%a6@(-126:W)\n"\
                   :\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d0", "d1", "a0", "a1" );\
})

#define Enable() ({\
  __Enable_base((EXEC_BASE_NAME));\
})

#define __Forbid_base(__in_base) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  __asm volatile (\
                   "jsr %%a6@(-132:W)\n"\
                   :\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d0", "d1", "a0", "a1" );\
})

#define Forbid() ({\
  __Forbid_base((EXEC_BASE_NAME));\
})

#define __Permit_base(__in_base) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  __asm volatile (\
                   "jsr %%a6@(-138:W)\n"\
                   :\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d0", "d1", "a0", "a1" );\
})

#define Permit() ({\
  __Permit_base((EXEC_BASE_NAME));\
})

#define __SetSR_base(__in_base, ___newSR, ___mask) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  ULONG __p____newSR = (ULONG)(___newSR);\
  ULONG __p____mask = (ULONG)(___mask);\
  register ULONG __v_ret __asm("d0");\
  register ULONG __v0 __asm("d0") = __p____newSR;\
  register ULONG __v1 __asm("d1") = __p____mask;\
  __asm volatile (\
                   "jsr %%a6@(-144:W)\n"\
                   : "=d"(__v_ret), "+d"(__v1)\
                   : "a"(__p__in_base), "d"(__v0)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define SetSR(___newSR, ___mask) ({\
  __SetSR_base((EXEC_BASE_NAME), ___newSR, ___mask);\
})

#define __SuperState_base(__in_base) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  register APTR __v_ret __asm("d0");\
  __asm volatile (\
                   "jsr %%a6@(-150:W)\n"\
                   : "=d"(__v_ret)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d1", "a0", "a1" );\
  __v_ret;})

#define SuperState() ({\
  __SuperState_base((EXEC_BASE_NAME));\
})

#define __UserState_base(__in_base, ___sysStack) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  APTR __p____sysStack = (APTR)(___sysStack);\
  register APTR __v0 __asm("d0") = __p____sysStack;\
  __asm volatile (\
                   "jsr %%a6@(-156:W)\n"\
                   : "+d"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d1", "a0", "a1" );\
})

#define UserState(___sysStack) ({\
  __UserState_base((EXEC_BASE_NAME), ___sysStack);\
})

#define __SetIntVector_base(__in_base, ___intNumber, ___interrupt) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  LONG __p____intNumber = (LONG)(___intNumber);\
  struct Interrupt * __p____interrupt = (struct Interrupt *)(___interrupt);\
  register struct Interrupt * __v_ret __asm("d0");\
  register LONG __v0 __asm("d0") = __p____intNumber;\
  register struct Interrupt * __v1 __asm("a1") = __p____interrupt;\
  __asm volatile (\
                   "jsr %%a6@(-162:W)\n"\
                   : "=d"(__v_ret), "+a"(__v1)\
                   : "a"(__p__in_base), "d"(__v0)\
                   : "fp0", "fp1", "cc", "memory", "d1", "a0" );\
  __v_ret;})

#define SetIntVector(___intNumber, ___interrupt) ({\
  __SetIntVector_base((EXEC_BASE_NAME), ___intNumber, ___interrupt);\
})

#define __AddIntServer_base(__in_base, ___intNumber, ___interrupt) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  LONG __p____intNumber = (LONG)(___intNumber);\
  struct Interrupt * __p____interrupt = (struct Interrupt *)(___interrupt);\
  register LONG __v0 __asm("d0") = __p____intNumber;\
  register struct Interrupt * __v1 __asm("a1") = __p____interrupt;\
  __asm volatile (\
                   "jsr %%a6@(-168:W)\n"\
                   : "+d"(__v0), "+a"(__v1)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d1", "a0" );\
})

#define AddIntServer(___intNumber, ___interrupt) ({\
  __AddIntServer_base((EXEC_BASE_NAME), ___intNumber, ___interrupt);\
})

#define __RemIntServer_base(__in_base, ___intNumber, ___interrupt) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  LONG __p____intNumber = (LONG)(___intNumber);\
  struct Interrupt * __p____interrupt = (struct Interrupt *)(___interrupt);\
  register LONG __v0 __asm("d0") = __p____intNumber;\
  register struct Interrupt * __v1 __asm("a1") = __p____interrupt;\
  __asm volatile (\
                   "jsr %%a6@(-174:W)\n"\
                   : "+d"(__v0), "+a"(__v1)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d1", "a0" );\
})

#define RemIntServer(___intNumber, ___interrupt) ({\
  __RemIntServer_base((EXEC_BASE_NAME), ___intNumber, ___interrupt);\
})

#define __Cause_base(__in_base, ___interrupt) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct Interrupt * __p____interrupt = (struct Interrupt *)(___interrupt);\
  register struct Interrupt * __v0 __asm("a1") = __p____interrupt;\
  __asm volatile (\
                   "jsr %%a6@(-180:W)\n"\
                   : "+a"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d0", "d1", "a0" );\
})

#define Cause(___interrupt) ({\
  __Cause_base((EXEC_BASE_NAME), ___interrupt);\
})

#define __Allocate_base(__in_base, ___freeList, ___byteSize) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct MemHeader * __p____freeList = (struct MemHeader *)(___freeList);\
  ULONG __p____byteSize = (ULONG)(___byteSize);\
  register APTR __v_ret __asm("d0");\
  register struct MemHeader * __v0 __asm("a0") = __p____freeList;\
  register ULONG __v1 __asm("d0") = __p____byteSize;\
  __asm volatile (\
                   "jsr %%a6@(-186:W)\n"\
                   : "=d"(__v_ret), "+a"(__v0)\
                   : "a"(__p__in_base), "d"(__v1)\
                   : "fp0", "fp1", "cc", "memory", "d1", "a1" );\
  __v_ret;})

#define Allocate(___freeList, ___byteSize) ({\
  __Allocate_base((EXEC_BASE_NAME), ___freeList, ___byteSize);\
})

#define __Deallocate_base(__in_base, ___freeList, ___memoryBlock, ___byteSize) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct MemHeader * __p____freeList = (struct MemHeader *)(___freeList);\
  APTR __p____memoryBlock = (APTR)(___memoryBlock);\
  ULONG __p____byteSize = (ULONG)(___byteSize);\
  register struct MemHeader * __v0 __asm("a0") = __p____freeList;\
  register APTR __v1 __asm("a1") = __p____memoryBlock;\
  register ULONG __v2 __asm("d0") = __p____byteSize;\
  __asm volatile (\
                   "jsr %%a6@(-192:W)\n"\
                   : "+a"(__v0), "+a"(__v1), "+d"(__v2)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d1" );\
})

#define Deallocate(___freeList, ___memoryBlock, ___byteSize) ({\
  __Deallocate_base((EXEC_BASE_NAME), ___freeList, ___memoryBlock, ___byteSize);\
})

#define __AllocMem_base(__in_base, ___byteSize, ___requirements) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  ULONG __p____byteSize = (ULONG)(___byteSize);\
  ULONG __p____requirements = (ULONG)(___requirements);\
  register APTR __v_ret __asm("d0");\
  register ULONG __v0 __asm("d0") = __p____byteSize;\
  register ULONG __v1 __asm("d1") = __p____requirements;\
  __asm volatile (\
                   "jsr %%a6@(-198:W)\n"\
                   : "=d"(__v_ret), "+d"(__v1)\
                   : "a"(__p__in_base), "d"(__v0)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define AllocMem(___byteSize, ___requirements) ({\
  __AllocMem_base((EXEC_BASE_NAME), ___byteSize, ___requirements);\
})

#define __AllocAbs_base(__in_base, ___byteSize, ___location) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  ULONG __p____byteSize = (ULONG)(___byteSize);\
  APTR __p____location = (APTR)(___location);\
  register APTR __v_ret __asm("d0");\
  register ULONG __v0 __asm("d0") = __p____byteSize;\
  register APTR __v1 __asm("a1") = __p____location;\
  __asm volatile (\
                   "jsr %%a6@(-204:W)\n"\
                   : "=d"(__v_ret), "+a"(__v1)\
                   : "a"(__p__in_base), "d"(__v0)\
                   : "fp0", "fp1", "cc", "memory", "d1", "a0" );\
  __v_ret;})

#define AllocAbs(___byteSize, ___location) ({\
  __AllocAbs_base((EXEC_BASE_NAME), ___byteSize, ___location);\
})

#define __FreeMem_base(__in_base, ___memoryBlock, ___byteSize) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  APTR __p____memoryBlock = (APTR)(___memoryBlock);\
  ULONG __p____byteSize = (ULONG)(___byteSize);\
  register APTR __v0 __asm("a1") = __p____memoryBlock;\
  register ULONG __v1 __asm("d0") = __p____byteSize;\
  __asm volatile (\
                   "jsr %%a6@(-210:W)\n"\
                   : "+a"(__v0), "+d"(__v1)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d1", "a0" );\
})

#define FreeMem(___memoryBlock, ___byteSize) ({\
  __FreeMem_base((EXEC_BASE_NAME), ___memoryBlock, ___byteSize);\
})

#define __AvailMem_base(__in_base, ___requirements) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  ULONG __p____requirements = (ULONG)(___requirements);\
  register ULONG __v_ret __asm("d0");\
  register ULONG __v0 __asm("d1") = __p____requirements;\
  __asm volatile (\
                   "jsr %%a6@(-216:W)\n"\
                   : "=d"(__v_ret), "+d"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define AvailMem(___requirements) ({\
  __AvailMem_base((EXEC_BASE_NAME), ___requirements);\
})

#define __AllocEntry_base(__in_base, ___entry) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct MemList * __p____entry = (struct MemList *)(___entry);\
  register struct MemList * __v_ret __asm("d0");\
  register struct MemList * __v0 __asm("a0") = __p____entry;\
  __asm volatile (\
                   "jsr %%a6@(-222:W)\n"\
                   : "=d"(__v_ret), "+a"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d1", "a1" );\
  __v_ret;})

#define AllocEntry(___entry) ({\
  __AllocEntry_base((EXEC_BASE_NAME), ___entry);\
})

#define __FreeEntry_base(__in_base, ___entry) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct MemList * __p____entry = (struct MemList *)(___entry);\
  register struct MemList * __v0 __asm("a0") = __p____entry;\
  __asm volatile (\
                   "jsr %%a6@(-228:W)\n"\
                   : "+a"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d0", "d1", "a1" );\
})

#define FreeEntry(___entry) ({\
  __FreeEntry_base((EXEC_BASE_NAME), ___entry);\
})

#define __Insert_base(__in_base, ___list, ___node, ___pred) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct List * __p____list = (struct List *)(___list);\
  struct Node * __p____node = (struct Node *)(___node);\
  struct Node * __p____pred = (struct Node *)(___pred);\
  register struct List * __v0 __asm("a0") = __p____list;\
  register struct Node * __v1 __asm("a1") = __p____node;\
  register struct Node * __v2 __asm("a2") = __p____pred;\
  __asm volatile (\
                   "jsr %%a6@(-234:W)\n"\
                   : "+a"(__v0), "+a"(__v1)\
                   : "a"(__p__in_base), "a"(__v2)\
                   : "fp0", "fp1", "cc", "memory", "d0", "d1" );\
})

#define Insert(___list, ___node, ___pred) ({\
  __Insert_base((EXEC_BASE_NAME), ___list, ___node, ___pred);\
})

#define __InsertMinNode_base(__in_base, ___minlist, ___minnode, ___minpred) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct MinList * __p____minlist = (struct MinList *)(___minlist);\
  struct MinNode * __p____minnode = (struct MinNode *)(___minnode);\
  struct MinNode * __p____minpred = (struct MinNode *)(___minpred);\
  register struct MinList * __v0 __asm("a0") = __p____minlist;\
  register struct MinNode * __v1 __asm("a1") = __p____minnode;\
  register struct MinNode * __v2 __asm("a2") = __p____minpred;\
  __asm volatile (\
                   "jsr %%a6@(-234:W)\n"\
                   : "+a"(__v0), "+a"(__v1)\
                   : "a"(__p__in_base), "a"(__v2)\
                   : "fp0", "fp1", "cc", "memory", "d0", "d1" );\
})

#define InsertMinNode(___minlist, ___minnode, ___minpred) ({\
  __InsertMinNode_base((EXEC_BASE_NAME), ___minlist, ___minnode, ___minpred);\
})

#define __AddHead_base(__in_base, ___list, ___node) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct List * __p____list = (struct List *)(___list);\
  struct Node * __p____node = (struct Node *)(___node);\
  register struct List * __v0 __asm("a0") = __p____list;\
  register struct Node * __v1 __asm("a1") = __p____node;\
  __asm volatile (\
                   "jsr %%a6@(-240:W)\n"\
                   : "+a"(__v0), "+a"(__v1)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d0", "d1" );\
})

#define AddHead(___list, ___node) ({\
  __AddHead_base((EXEC_BASE_NAME), ___list, ___node);\
})

#define __AddHeadMinList_base(__in_base, ___minlist, ___minnode) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct MinList * __p____minlist = (struct MinList *)(___minlist);\
  struct MinNode * __p____minnode = (struct MinNode *)(___minnode);\
  register struct MinList * __v0 __asm("a0") = __p____minlist;\
  register struct MinNode * __v1 __asm("a1") = __p____minnode;\
  __asm volatile (\
                   "jsr %%a6@(-240:W)\n"\
                   : "+a"(__v0), "+a"(__v1)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d0", "d1" );\
})

#define AddHeadMinList(___minlist, ___minnode) ({\
  __AddHeadMinList_base((EXEC_BASE_NAME), ___minlist, ___minnode);\
})

#define __AddTail_base(__in_base, ___list, ___node) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct List * __p____list = (struct List *)(___list);\
  struct Node * __p____node = (struct Node *)(___node);\
  register struct List * __v0 __asm("a0") = __p____list;\
  register struct Node * __v1 __asm("a1") = __p____node;\
  __asm volatile (\
                   "jsr %%a6@(-246:W)\n"\
                   : "+a"(__v0), "+a"(__v1)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d0", "d1" );\
})

#define AddTail(___list, ___node) ({\
  __AddTail_base((EXEC_BASE_NAME), ___list, ___node);\
})

#define __AddTailMinList_base(__in_base, ___minlist, ___minnode) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct MinList * __p____minlist = (struct MinList *)(___minlist);\
  struct MinNode * __p____minnode = (struct MinNode *)(___minnode);\
  register struct MinList * __v0 __asm("a0") = __p____minlist;\
  register struct MinNode * __v1 __asm("a1") = __p____minnode;\
  __asm volatile (\
                   "jsr %%a6@(-246:W)\n"\
                   : "+a"(__v0), "+a"(__v1)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d0", "d1" );\
})

#define AddTailMinList(___minlist, ___minnode) ({\
  __AddTailMinList_base((EXEC_BASE_NAME), ___minlist, ___minnode);\
})

#define __Remove_base(__in_base, ___node) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct Node * __p____node = (struct Node *)(___node);\
  register struct Node * __v0 __asm("a1") = __p____node;\
  __asm volatile (\
                   "jsr %%a6@(-252:W)\n"\
                   : "+a"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d0", "d1", "a0" );\
})

#define Remove(___node) ({\
  __Remove_base((EXEC_BASE_NAME), ___node);\
})

#define __RemoveMinNode_base(__in_base, ___minnode) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct MinNode * __p____minnode = (struct MinNode *)(___minnode);\
  register struct MinNode * __v0 __asm("a1") = __p____minnode;\
  __asm volatile (\
                   "jsr %%a6@(-252:W)\n"\
                   : "+a"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d0", "d1", "a0" );\
})

#define RemoveMinNode(___minnode) ({\
  __RemoveMinNode_base((EXEC_BASE_NAME), ___minnode);\
})

#define __RemHead_base(__in_base, ___list) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct List * __p____list = (struct List *)(___list);\
  register struct Node * __v_ret __asm("d0");\
  register struct List * __v0 __asm("a0") = __p____list;\
  __asm volatile (\
                   "jsr %%a6@(-258:W)\n"\
                   : "=d"(__v_ret), "+a"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d1", "a1" );\
  __v_ret;})

#define RemHead(___list) ({\
  __RemHead_base((EXEC_BASE_NAME), ___list);\
})

#define __RemHeadMinList_base(__in_base, ___minlist) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct MinList * __p____minlist = (struct MinList *)(___minlist);\
  register struct MinNode * __v_ret __asm("d0");\
  register struct MinList * __v0 __asm("a0") = __p____minlist;\
  __asm volatile (\
                   "jsr %%a6@(-258:W)\n"\
                   : "=d"(__v_ret), "+a"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d1", "a1" );\
  __v_ret;})

#define RemHeadMinList(___minlist) ({\
  __RemHeadMinList_base((EXEC_BASE_NAME), ___minlist);\
})

#define __RemTail_base(__in_base, ___list) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct List * __p____list = (struct List *)(___list);\
  register struct Node * __v_ret __asm("d0");\
  register struct List * __v0 __asm("a0") = __p____list;\
  __asm volatile (\
                   "jsr %%a6@(-264:W)\n"\
                   : "=d"(__v_ret), "+a"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d1", "a1" );\
  __v_ret;})

#define RemTail(___list) ({\
  __RemTail_base((EXEC_BASE_NAME), ___list);\
})

#define __RemTailMinList_base(__in_base, ___minlist) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct MinList * __p____minlist = (struct MinList *)(___minlist);\
  register struct MinNode * __v_ret __asm("d0");\
  register struct MinList * __v0 __asm("a0") = __p____minlist;\
  __asm volatile (\
                   "jsr %%a6@(-264:W)\n"\
                   : "=d"(__v_ret), "+a"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d1", "a1" );\
  __v_ret;})

#define RemTailMinList(___minlist) ({\
  __RemTailMinList_base((EXEC_BASE_NAME), ___minlist);\
})

#define __Enqueue_base(__in_base, ___list, ___node) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct List * __p____list = (struct List *)(___list);\
  struct Node * __p____node = (struct Node *)(___node);\
  register struct List * __v0 __asm("a0") = __p____list;\
  register struct Node * __v1 __asm("a1") = __p____node;\
  __asm volatile (\
                   "jsr %%a6@(-270:W)\n"\
                   : "+a"(__v0), "+a"(__v1)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d0", "d1" );\
})

#define Enqueue(___list, ___node) ({\
  __Enqueue_base((EXEC_BASE_NAME), ___list, ___node);\
})

#define __FindName_base(__in_base, ___list, ___name) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct List * __p____list = (struct List *)(___list);\
  STRPTR __p____name = (STRPTR)(___name);\
  register struct Node * __v_ret __asm("d0");\
  register struct List * __v0 __asm("a0") = __p____list;\
  register STRPTR __v1 __asm("a1") = __p____name;\
  __asm volatile (\
                   "jsr %%a6@(-276:W)\n"\
                   : "=d"(__v_ret), "+a"(__v0), "+a"(__v1)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d1" );\
  __v_ret;})

#define FindName(___list, ___name) ({\
  __FindName_base((EXEC_BASE_NAME), ___list, ___name);\
})

#define __AddTask_base(__in_base, ___task, ___initPC, ___finalPC) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct Task * __p____task = (struct Task *)(___task);\
  APTR __p____initPC = (APTR)(___initPC);\
  APTR __p____finalPC = (APTR)(___finalPC);\
  register APTR __v_ret __asm("d0");\
  register struct Task * __v0 __asm("a1") = __p____task;\
  register APTR __v1 __asm("a2") = __p____initPC;\
  register APTR __v2 __asm("a3") = __p____finalPC;\
  __asm volatile (\
                   "jsr %%a6@(-282:W)\n"\
                   : "=d"(__v_ret), "+a"(__v0)\
                   : "a"(__p__in_base), "a"(__v1), "a"(__v2)\
                   : "fp0", "fp1", "cc", "memory", "d1", "a0" );\
  __v_ret;})

#define AddTask(___task, ___initPC, ___finalPC) ({\
  __AddTask_base((EXEC_BASE_NAME), ___task, ___initPC, ___finalPC);\
})

#define __RemTask_base(__in_base, ___task) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct Task * __p____task = (struct Task *)(___task);\
  register struct Task * __v0 __asm("a1") = __p____task;\
  __asm volatile (\
                   "jsr %%a6@(-288:W)\n"\
                   : "+a"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d0", "d1", "a0" );\
})

#define RemTask(___task) ({\
  __RemTask_base((EXEC_BASE_NAME), ___task);\
})

#define __FindTask_base(__in_base, ___name) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  STRPTR __p____name = (STRPTR)(___name);\
  register struct Task * __v_ret __asm("d0");\
  register STRPTR __v0 __asm("a1") = __p____name;\
  __asm volatile (\
                   "jsr %%a6@(-294:W)\n"\
                   : "=d"(__v_ret), "+a"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d1", "a0" );\
  __v_ret;})

#define FindTask(___name) ({\
  __FindTask_base((EXEC_BASE_NAME), ___name);\
})

#define __SetTaskPri_base(__in_base, ___task, ___priority) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct Task * __p____task = (struct Task *)(___task);\
  LONG __p____priority = (LONG)(___priority);\
  register BYTE __v_ret __asm("d0");\
  register struct Task * __v0 __asm("a1") = __p____task;\
  register LONG __v1 __asm("d0") = __p____priority;\
  __asm volatile (\
                   "jsr %%a6@(-300:W)\n"\
                   : "=d"(__v_ret), "+a"(__v0)\
                   : "a"(__p__in_base), "d"(__v1)\
                   : "fp0", "fp1", "cc", "memory", "d1", "a0" );\
  __v_ret;})

#define SetTaskPri(___task, ___priority) ({\
  __SetTaskPri_base((EXEC_BASE_NAME), ___task, ___priority);\
})

#define __SetSignal_base(__in_base, ___newSignals, ___signalSet) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  ULONG __p____newSignals = (ULONG)(___newSignals);\
  ULONG __p____signalSet = (ULONG)(___signalSet);\
  register ULONG __v_ret __asm("d0");\
  register ULONG __v0 __asm("d0") = __p____newSignals;\
  register ULONG __v1 __asm("d1") = __p____signalSet;\
  __asm volatile (\
                   "jsr %%a6@(-306:W)\n"\
                   : "=d"(__v_ret), "+d"(__v1)\
                   : "a"(__p__in_base), "d"(__v0)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define SetSignal(___newSignals, ___signalSet) ({\
  __SetSignal_base((EXEC_BASE_NAME), ___newSignals, ___signalSet);\
})

#define __SetExcept_base(__in_base, ___newSignals, ___signalSet) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  ULONG __p____newSignals = (ULONG)(___newSignals);\
  ULONG __p____signalSet = (ULONG)(___signalSet);\
  register ULONG __v_ret __asm("d0");\
  register ULONG __v0 __asm("d0") = __p____newSignals;\
  register ULONG __v1 __asm("d1") = __p____signalSet;\
  __asm volatile (\
                   "jsr %%a6@(-312:W)\n"\
                   : "=d"(__v_ret), "+d"(__v1)\
                   : "a"(__p__in_base), "d"(__v0)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define SetExcept(___newSignals, ___signalSet) ({\
  __SetExcept_base((EXEC_BASE_NAME), ___newSignals, ___signalSet);\
})

#define __Wait_base(__in_base, ___signalSet) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  ULONG __p____signalSet = (ULONG)(___signalSet);\
  register ULONG __v_ret __asm("d0");\
  register ULONG __v0 __asm("d0") = __p____signalSet;\
  __asm volatile (\
                   "jsr %%a6@(-318:W)\n"\
                   : "=d"(__v_ret)\
                   : "a"(__p__in_base), "d"(__v0)\
                   : "fp0", "fp1", "cc", "memory", "d1", "a0", "a1" );\
  __v_ret;})

#define Wait(___signalSet) ({\
  __Wait_base((EXEC_BASE_NAME), ___signalSet);\
})

#define __Signal_base(__in_base, ___task, ___signalSet) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct Task * __p____task = (struct Task *)(___task);\
  ULONG __p____signalSet = (ULONG)(___signalSet);\
  register struct Task * __v0 __asm("a1") = __p____task;\
  register ULONG __v1 __asm("d0") = __p____signalSet;\
  __asm volatile (\
                   "jsr %%a6@(-324:W)\n"\
                   : "+a"(__v0), "+d"(__v1)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d1", "a0" );\
})

#define Signal(___task, ___signalSet) ({\
  __Signal_base((EXEC_BASE_NAME), ___task, ___signalSet);\
})

#define __AllocSignal_base(__in_base, ___signalNum) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  BYTE __p____signalNum = (BYTE)(___signalNum);\
  register BYTE __v_ret __asm("d0");\
  register BYTE __v0 __asm("d0") = __p____signalNum;\
  __asm volatile (\
                   "jsr %%a6@(-330:W)\n"\
                   : "=d"(__v_ret)\
                   : "a"(__p__in_base), "d"(__v0)\
                   : "fp0", "fp1", "cc", "memory", "d1", "a0", "a1" );\
  __v_ret;})

#define AllocSignal(___signalNum) ({\
  __AllocSignal_base((EXEC_BASE_NAME), ___signalNum);\
})

#define __FreeSignal_base(__in_base, ___signalNum) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  BYTE __p____signalNum = (BYTE)(___signalNum);\
  register BYTE __v0 __asm("d0") = __p____signalNum;\
  __asm volatile (\
                   "jsr %%a6@(-336:W)\n"\
                   : "+d"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d1", "a0", "a1" );\
})

#define FreeSignal(___signalNum) ({\
  __FreeSignal_base((EXEC_BASE_NAME), ___signalNum);\
})

#define __AllocTrap_base(__in_base, ___trapNum) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  LONG __p____trapNum = (LONG)(___trapNum);\
  register LONG __v_ret __asm("d0");\
  register LONG __v0 __asm("d0") = __p____trapNum;\
  __asm volatile (\
                   "jsr %%a6@(-342:W)\n"\
                   : "=d"(__v_ret)\
                   : "a"(__p__in_base), "d"(__v0)\
                   : "fp0", "fp1", "cc", "memory", "d1", "a0", "a1" );\
  __v_ret;})

#define AllocTrap(___trapNum) ({\
  __AllocTrap_base((EXEC_BASE_NAME), ___trapNum);\
})

#define __FreeTrap_base(__in_base, ___trapNum) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  LONG __p____trapNum = (LONG)(___trapNum);\
  register LONG __v0 __asm("d0") = __p____trapNum;\
  __asm volatile (\
                   "jsr %%a6@(-348:W)\n"\
                   : "+d"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d1", "a0", "a1" );\
})

#define FreeTrap(___trapNum) ({\
  __FreeTrap_base((EXEC_BASE_NAME), ___trapNum);\
})

#define __AddPort_base(__in_base, ___port) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct MsgPort * __p____port = (struct MsgPort *)(___port);\
  register struct MsgPort * __v0 __asm("a1") = __p____port;\
  __asm volatile (\
                   "jsr %%a6@(-354:W)\n"\
                   : "+a"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d0", "d1", "a0" );\
})

#define AddPort(___port) ({\
  __AddPort_base((EXEC_BASE_NAME), ___port);\
})

#define __RemPort_base(__in_base, ___port) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct MsgPort * __p____port = (struct MsgPort *)(___port);\
  register struct MsgPort * __v0 __asm("a1") = __p____port;\
  __asm volatile (\
                   "jsr %%a6@(-360:W)\n"\
                   : "+a"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d0", "d1", "a0" );\
})

#define RemPort(___port) ({\
  __RemPort_base((EXEC_BASE_NAME), ___port);\
})

#define __PutMsg_base(__in_base, ___port, ___message) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct MsgPort * __p____port = (struct MsgPort *)(___port);\
  struct Message * __p____message = (struct Message *)(___message);\
  register struct MsgPort * __v0 __asm("a0") = __p____port;\
  register struct Message * __v1 __asm("a1") = __p____message;\
  __asm volatile (\
                   "jsr %%a6@(-366:W)\n"\
                   : "+a"(__v0), "+a"(__v1)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d0", "d1" );\
})

#define PutMsg(___port, ___message) ({\
  __PutMsg_base((EXEC_BASE_NAME), ___port, ___message);\
})

#define __GetMsg_base(__in_base, ___port) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct MsgPort * __p____port = (struct MsgPort *)(___port);\
  register struct Message * __v_ret __asm("d0");\
  register struct MsgPort * __v0 __asm("a0") = __p____port;\
  __asm volatile (\
                   "jsr %%a6@(-372:W)\n"\
                   : "=d"(__v_ret), "+a"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d1", "a1" );\
  __v_ret;})

#define GetMsg(___port) ({\
  __GetMsg_base((EXEC_BASE_NAME), ___port);\
})

#define __ReplyMsg_base(__in_base, ___message) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct Message * __p____message = (struct Message *)(___message);\
  register struct Message * __v0 __asm("a1") = __p____message;\
  __asm volatile (\
                   "jsr %%a6@(-378:W)\n"\
                   : "+a"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d0", "d1", "a0" );\
})

#define ReplyMsg(___message) ({\
  __ReplyMsg_base((EXEC_BASE_NAME), ___message);\
})

#define __WaitPort_base(__in_base, ___port) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct MsgPort * __p____port = (struct MsgPort *)(___port);\
  register struct Message * __v_ret __asm("d0");\
  register struct MsgPort * __v0 __asm("a0") = __p____port;\
  __asm volatile (\
                   "jsr %%a6@(-384:W)\n"\
                   : "=d"(__v_ret), "+a"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d1", "a1" );\
  __v_ret;})

#define WaitPort(___port) ({\
  __WaitPort_base((EXEC_BASE_NAME), ___port);\
})

#define __FindPort_base(__in_base, ___name) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  STRPTR __p____name = (STRPTR)(___name);\
  register struct MsgPort * __v_ret __asm("d0");\
  register STRPTR __v0 __asm("a1") = __p____name;\
  __asm volatile (\
                   "jsr %%a6@(-390:W)\n"\
                   : "=d"(__v_ret), "+a"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d1", "a0" );\
  __v_ret;})

#define FindPort(___name) ({\
  __FindPort_base((EXEC_BASE_NAME), ___name);\
})

#define __AddLibrary_base(__in_base, ___library) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct Library * __p____library = (struct Library *)(___library);\
  register struct Library * __v0 __asm("a1") = __p____library;\
  __asm volatile (\
                   "jsr %%a6@(-396:W)\n"\
                   : "+a"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d0", "d1", "a0" );\
})

#define AddLibrary(___library) ({\
  __AddLibrary_base((EXEC_BASE_NAME), ___library);\
})

#define __RemLibrary_base(__in_base, ___library) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct Library * __p____library = (struct Library *)(___library);\
  register struct Library * __v0 __asm("a1") = __p____library;\
  __asm volatile (\
                   "jsr %%a6@(-402:W)\n"\
                   : "+a"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d0", "d1", "a0" );\
})

#define RemLibrary(___library) ({\
  __RemLibrary_base((EXEC_BASE_NAME), ___library);\
})

#define __OldOpenLibrary_base(__in_base, ___libName) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  STRPTR __p____libName = (STRPTR)(___libName);\
  register struct Library * __v_ret __asm("d0");\
  register STRPTR __v0 __asm("a1") = __p____libName;\
  __asm volatile (\
                   "jsr %%a6@(-408:W)\n"\
                   : "=d"(__v_ret), "+a"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d1", "a0" );\
  __v_ret;})

#define OldOpenLibrary(___libName) ({\
  __OldOpenLibrary_base((EXEC_BASE_NAME), ___libName);\
})

#define __CloseLibrary_base(__in_base, ___library) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct Library * __p____library = (struct Library *)(___library);\
  register struct Library * __v0 __asm("a1") = __p____library;\
  __asm volatile (\
                   "jsr %%a6@(-414:W)\n"\
                   : "+a"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d0", "d1", "a0" );\
})

#define CloseLibrary(___library) ({\
  __CloseLibrary_base((EXEC_BASE_NAME), ___library);\
})

#define __SetFunction_base(__in_base, ___library, ___funcOffset, ___newFunction) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct Library * __p____library = (struct Library *)(___library);\
  LONG __p____funcOffset = (LONG)(___funcOffset);\
  void *__p____newFunction = (void *)(___newFunction);\
  register APTR __v_ret __asm("d0");\
  register struct Library * __v0 __asm("a1") = __p____library;\
  register LONG __v1 __asm("a0") = __p____funcOffset;\
  register ULONG (*__v2)() __asm("d0") = __p____newFunction;\
  __asm volatile (\
                   "jsr %%a6@(-420:W)\n"\
                   : "=d"(__v_ret), "+a"(__v0), "+a"(__v1)\
                   : "a"(__p__in_base), "d"(__v2)\
                   : "fp0", "fp1", "cc", "memory", "d1" );\
  __v_ret;})

#define SetFunction(___library, ___funcOffset, ___newFunction) ({\
  __SetFunction_base((EXEC_BASE_NAME), ___library, ___funcOffset, ___newFunction);\
})

#define __SumLibrary_base(__in_base, ___library) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct Library * __p____library = (struct Library *)(___library);\
  register struct Library * __v0 __asm("a1") = __p____library;\
  __asm volatile (\
                   "jsr %%a6@(-426:W)\n"\
                   : "+a"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d0", "d1", "a0" );\
})

#define SumLibrary(___library) ({\
  __SumLibrary_base((EXEC_BASE_NAME), ___library);\
})

#define __AddDevice_base(__in_base, ___device) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct Device * __p____device = (struct Device *)(___device);\
  register struct Device * __v0 __asm("a1") = __p____device;\
  __asm volatile (\
                   "jsr %%a6@(-432:W)\n"\
                   : "+a"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d0", "d1", "a0" );\
})

#define AddDevice(___device) ({\
  __AddDevice_base((EXEC_BASE_NAME), ___device);\
})

#define __RemDevice_base(__in_base, ___device) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct Device * __p____device = (struct Device *)(___device);\
  register struct Device * __v0 __asm("a1") = __p____device;\
  __asm volatile (\
                   "jsr %%a6@(-438:W)\n"\
                   : "+a"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d0", "d1", "a0" );\
})

#define RemDevice(___device) ({\
  __RemDevice_base((EXEC_BASE_NAME), ___device);\
})

#define __OpenDevice_base(__in_base, ___devName, ___unit, ___ioRequest, ___flags) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  STRPTR __p____devName = (STRPTR)(___devName);\
  ULONG __p____unit = (ULONG)(___unit);\
  struct IORequest * __p____ioRequest = (struct IORequest *)(___ioRequest);\
  ULONG __p____flags = (ULONG)(___flags);\
  register BYTE __v_ret __asm("d0");\
  register STRPTR __v0 __asm("a0") = __p____devName;\
  register ULONG __v1 __asm("d0") = __p____unit;\
  register struct IORequest * __v2 __asm("a1") = __p____ioRequest;\
  register ULONG __v3 __asm("d1") = __p____flags;\
  __asm volatile (\
                   "jsr %%a6@(-444:W)\n"\
                   : "=d"(__v_ret), "+a"(__v0), "+a"(__v2), "+d"(__v3)\
                   : "a"(__p__in_base), "d"(__v1)\
                   : "fp0", "fp1", "cc", "memory" );\
  __v_ret;})

#define OpenDevice(___devName, ___unit, ___ioRequest, ___flags) ({\
  __OpenDevice_base((EXEC_BASE_NAME), ___devName, ___unit, ___ioRequest, ___flags);\
})

#define __CloseDevice_base(__in_base, ___ioRequest) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct IORequest * __p____ioRequest = (struct IORequest *)(___ioRequest);\
  register struct IORequest * __v0 __asm("a1") = __p____ioRequest;\
  __asm volatile (\
                   "jsr %%a6@(-450:W)\n"\
                   : "+a"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d0", "d1", "a0" );\
})

#define CloseDevice(___ioRequest) ({\
  __CloseDevice_base((EXEC_BASE_NAME), ___ioRequest);\
})

#define __DoIO_base(__in_base, ___ioRequest) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct IORequest * __p____ioRequest = (struct IORequest *)(___ioRequest);\
  register BYTE __v_ret __asm("d0");\
  register struct IORequest * __v0 __asm("a1") = __p____ioRequest;\
  __asm volatile (\
                   "jsr %%a6@(-456:W)\n"\
                   : "=d"(__v_ret), "+a"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d1", "a0" );\
  __v_ret;})

#define DoIO(___ioRequest) ({\
  __DoIO_base((EXEC_BASE_NAME), ___ioRequest);\
})

#define __SendIO_base(__in_base, ___ioRequest) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct IORequest * __p____ioRequest = (struct IORequest *)(___ioRequest);\
  register struct IORequest * __v0 __asm("a1") = __p____ioRequest;\
  __asm volatile (\
                   "jsr %%a6@(-462:W)\n"\
                   : "+a"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d0", "d1", "a0" );\
})

#define SendIO(___ioRequest) ({\
  __SendIO_base((EXEC_BASE_NAME), ___ioRequest);\
})

#define __CheckIO_base(__in_base, ___ioRequest) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct IORequest * __p____ioRequest = (struct IORequest *)(___ioRequest);\
  register struct IORequest * __v_ret __asm("d0");\
  register struct IORequest * __v0 __asm("a1") = __p____ioRequest;\
  __asm volatile (\
                   "jsr %%a6@(-468:W)\n"\
                   : "=d"(__v_ret), "+a"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d1", "a0" );\
  __v_ret;})

#define CheckIO(___ioRequest) ({\
  __CheckIO_base((EXEC_BASE_NAME), ___ioRequest);\
})

#define __WaitIO_base(__in_base, ___ioRequest) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct IORequest * __p____ioRequest = (struct IORequest *)(___ioRequest);\
  register BYTE __v_ret __asm("d0");\
  register struct IORequest * __v0 __asm("a1") = __p____ioRequest;\
  __asm volatile (\
                   "jsr %%a6@(-474:W)\n"\
                   : "=d"(__v_ret), "+a"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d1", "a0" );\
  __v_ret;})

#define WaitIO(___ioRequest) ({\
  __WaitIO_base((EXEC_BASE_NAME), ___ioRequest);\
})

#define __AbortIO_base(__in_base, ___ioRequest) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct IORequest * __p____ioRequest = (struct IORequest *)(___ioRequest);\
  register struct IORequest * __v0 __asm("a1") = __p____ioRequest;\
  __asm volatile (\
                   "jsr %%a6@(-480:W)\n"\
                   : "+a"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d0", "d1", "a0" );\
})

#define AbortIO(___ioRequest) ({\
  __AbortIO_base((EXEC_BASE_NAME), ___ioRequest);\
})

#define __AddResource_base(__in_base, ___resource) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  APTR __p____resource = (APTR)(___resource);\
  register APTR __v0 __asm("a1") = __p____resource;\
  __asm volatile (\
                   "jsr %%a6@(-486:W)\n"\
                   : "+a"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d0", "d1", "a0" );\
})

#define AddResource(___resource) ({\
  __AddResource_base((EXEC_BASE_NAME), ___resource);\
})

#define __RemResource_base(__in_base, ___resource) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  APTR __p____resource = (APTR)(___resource);\
  register APTR __v0 __asm("a1") = __p____resource;\
  __asm volatile (\
                   "jsr %%a6@(-492:W)\n"\
                   : "+a"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d0", "d1", "a0" );\
})

#define RemResource(___resource) ({\
  __RemResource_base((EXEC_BASE_NAME), ___resource);\
})

#define __OpenResource_base(__in_base, ___resName) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  STRPTR __p____resName = (STRPTR)(___resName);\
  register APTR __v_ret __asm("d0");\
  register STRPTR __v0 __asm("a1") = __p____resName;\
  __asm volatile (\
                   "jsr %%a6@(-498:W)\n"\
                   : "=d"(__v_ret), "+a"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d1", "a0" );\
  __v_ret;})

#define OpenResource(___resName) ({\
  __OpenResource_base((EXEC_BASE_NAME), ___resName);\
})

#define __RawDoFmt_base(__in_base, ___formatString, ___dataStream, ___putChProc, ___putChData) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  STRPTR __p____formatString = (STRPTR)(___formatString);\
  APTR __p____dataStream = (APTR)(___dataStream);\
  void *__p____putChProc = (void *)(___putChProc);\
  APTR __p____putChData = (APTR)(___putChData);\
  register APTR __v_ret __asm("d0");\
  register STRPTR __v0 __asm("a0") = __p____formatString;\
  register APTR __v1 __asm("a1") = __p____dataStream;\
  register VOID (*__v2)() __asm("a2") = __p____putChProc;\
  register APTR __v3 __asm("a3") = __p____putChData;\
  __asm volatile (\
                   "jsr %%a6@(-522:W)\n"\
                   : "=d"(__v_ret), "+a"(__v0), "+a"(__v1)\
                   : "a"(__p__in_base), "a"(__v2), "a"(__v3)\
                   : "fp0", "fp1", "cc", "memory", "d1" );\
  __v_ret;})

#define RawDoFmt(___formatString, ___dataStream, ___putChProc, ___putChData) ({\
  __RawDoFmt_base((EXEC_BASE_NAME), ___formatString, ___dataStream, ___putChProc, ___putChData);\
})

#define __GetCC_base(__in_base) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  register ULONG __v_ret __asm("d0");\
  __asm volatile (\
                   "jsr %%a6@(-528:W)\n"\
                   : "=d"(__v_ret)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d1", "a0", "a1" );\
  __v_ret;})

#define GetCC() ({\
  __GetCC_base((EXEC_BASE_NAME));\
})

#define __TypeOfMem_base(__in_base, ___address) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  APTR __p____address = (APTR)(___address);\
  register ULONG __v_ret __asm("d0");\
  register APTR __v0 __asm("a1") = __p____address;\
  __asm volatile (\
                   "jsr %%a6@(-534:W)\n"\
                   : "=d"(__v_ret), "+a"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d1", "a0" );\
  __v_ret;})

#define TypeOfMem(___address) ({\
  __TypeOfMem_base((EXEC_BASE_NAME), ___address);\
})

#define __Procure_base(__in_base, ___sigSem, ___bidMsg) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct SignalSemaphore * __p____sigSem = (struct SignalSemaphore *)(___sigSem);\
  struct SemaphoreMessage * __p____bidMsg = (struct SemaphoreMessage *)(___bidMsg);\
  register ULONG __v_ret __asm("d0");\
  register struct SignalSemaphore * __v0 __asm("a0") = __p____sigSem;\
  register struct SemaphoreMessage * __v1 __asm("a1") = __p____bidMsg;\
  __asm volatile (\
                   "jsr %%a6@(-540:W)\n"\
                   : "=d"(__v_ret), "+a"(__v0), "+a"(__v1)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d1" );\
  __v_ret;})

#define Procure(___sigSem, ___bidMsg) ({\
  __Procure_base((EXEC_BASE_NAME), ___sigSem, ___bidMsg);\
})

#define __Vacate_base(__in_base, ___sigSem, ___bidMsg) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct SignalSemaphore * __p____sigSem = (struct SignalSemaphore *)(___sigSem);\
  struct SemaphoreMessage * __p____bidMsg = (struct SemaphoreMessage *)(___bidMsg);\
  register struct SignalSemaphore * __v0 __asm("a0") = __p____sigSem;\
  register struct SemaphoreMessage * __v1 __asm("a1") = __p____bidMsg;\
  __asm volatile (\
                   "jsr %%a6@(-546:W)\n"\
                   : "+a"(__v0), "+a"(__v1)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d0", "d1" );\
})

#define Vacate(___sigSem, ___bidMsg) ({\
  __Vacate_base((EXEC_BASE_NAME), ___sigSem, ___bidMsg);\
})

#define __OpenLibrary_base(__in_base, ___libName, ___version) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  STRPTR __p____libName = (STRPTR)(___libName);\
  ULONG __p____version = (ULONG)(___version);\
  register struct Library * __v_ret __asm("d0");\
  register STRPTR __v0 __asm("a1") = __p____libName;\
  register ULONG __v1 __asm("d0") = __p____version;\
  __asm volatile (\
                   "jsr %%a6@(-552:W)\n"\
                   : "=d"(__v_ret), "+a"(__v0)\
                   : "a"(__p__in_base), "d"(__v1)\
                   : "fp0", "fp1", "cc", "memory", "d1", "a0" );\
  __v_ret;})

#define OpenLibrary(___libName, ___version) ({\
  __OpenLibrary_base((EXEC_BASE_NAME), ___libName, ___version);\
})

#define __InitSemaphore_base(__in_base, ___sigSem) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct SignalSemaphore * __p____sigSem = (struct SignalSemaphore *)(___sigSem);\
  register struct SignalSemaphore * __v0 __asm("a0") = __p____sigSem;\
  __asm volatile (\
                   "jsr %%a6@(-558:W)\n"\
                   : "+a"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d0", "d1", "a1" );\
})

#define InitSemaphore(___sigSem) ({\
  __InitSemaphore_base((EXEC_BASE_NAME), ___sigSem);\
})

#define __ObtainSemaphore_base(__in_base, ___sigSem) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct SignalSemaphore * __p____sigSem = (struct SignalSemaphore *)(___sigSem);\
  register struct SignalSemaphore * __v0 __asm("a0") = __p____sigSem;\
  __asm volatile (\
                   "jsr %%a6@(-564:W)\n"\
                   : "+a"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d0", "d1", "a1" );\
})

#define ObtainSemaphore(___sigSem) ({\
  __ObtainSemaphore_base((EXEC_BASE_NAME), ___sigSem);\
})

#define __ReleaseSemaphore_base(__in_base, ___sigSem) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct SignalSemaphore * __p____sigSem = (struct SignalSemaphore *)(___sigSem);\
  register struct SignalSemaphore * __v0 __asm("a0") = __p____sigSem;\
  __asm volatile (\
                   "jsr %%a6@(-570:W)\n"\
                   : "+a"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d0", "d1", "a1" );\
})

#define ReleaseSemaphore(___sigSem) ({\
  __ReleaseSemaphore_base((EXEC_BASE_NAME), ___sigSem);\
})

#define __AttemptSemaphore_base(__in_base, ___sigSem) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct SignalSemaphore * __p____sigSem = (struct SignalSemaphore *)(___sigSem);\
  register ULONG __v_ret __asm("d0");\
  register struct SignalSemaphore * __v0 __asm("a0") = __p____sigSem;\
  __asm volatile (\
                   "jsr %%a6@(-576:W)\n"\
                   : "=d"(__v_ret), "+a"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d1", "a1" );\
  __v_ret;})

#define AttemptSemaphore(___sigSem) ({\
  __AttemptSemaphore_base((EXEC_BASE_NAME), ___sigSem);\
})

#define __ObtainSemaphoreList_base(__in_base, ___sigSem) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct List * __p____sigSem = (struct List *)(___sigSem);\
  register struct List * __v0 __asm("a0") = __p____sigSem;\
  __asm volatile (\
                   "jsr %%a6@(-582:W)\n"\
                   : "+a"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d0", "d1", "a1" );\
})

#define ObtainSemaphoreList(___sigSem) ({\
  __ObtainSemaphoreList_base((EXEC_BASE_NAME), ___sigSem);\
})

#define __ReleaseSemaphoreList_base(__in_base, ___sigSem) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct List * __p____sigSem = (struct List *)(___sigSem);\
  register struct List * __v0 __asm("a0") = __p____sigSem;\
  __asm volatile (\
                   "jsr %%a6@(-588:W)\n"\
                   : "+a"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d0", "d1", "a1" );\
})

#define ReleaseSemaphoreList(___sigSem) ({\
  __ReleaseSemaphoreList_base((EXEC_BASE_NAME), ___sigSem);\
})

#define __FindSemaphore_base(__in_base, ___name) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  STRPTR __p____name = (STRPTR)(___name);\
  register struct SignalSemaphore * __v_ret __asm("d0");\
  register STRPTR __v0 __asm("a1") = __p____name;\
  __asm volatile (\
                   "jsr %%a6@(-594:W)\n"\
                   : "=d"(__v_ret), "+a"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d1", "a0" );\
  __v_ret;})

#define FindSemaphore(___name) ({\
  __FindSemaphore_base((EXEC_BASE_NAME), ___name);\
})

#define __AddSemaphore_base(__in_base, ___sigSem) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct SignalSemaphore * __p____sigSem = (struct SignalSemaphore *)(___sigSem);\
  register struct SignalSemaphore * __v0 __asm("a1") = __p____sigSem;\
  __asm volatile (\
                   "jsr %%a6@(-600:W)\n"\
                   : "+a"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d0", "d1", "a0" );\
})

#define AddSemaphore(___sigSem) ({\
  __AddSemaphore_base((EXEC_BASE_NAME), ___sigSem);\
})

#define __RemSemaphore_base(__in_base, ___sigSem) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct SignalSemaphore * __p____sigSem = (struct SignalSemaphore *)(___sigSem);\
  register struct SignalSemaphore * __v0 __asm("a1") = __p____sigSem;\
  __asm volatile (\
                   "jsr %%a6@(-606:W)\n"\
                   : "+a"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d0", "d1", "a0" );\
})

#define RemSemaphore(___sigSem) ({\
  __RemSemaphore_base((EXEC_BASE_NAME), ___sigSem);\
})

#define __SumKickData_base(__in_base) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  register ULONG __v_ret __asm("d0");\
  __asm volatile (\
                   "jsr %%a6@(-612:W)\n"\
                   : "=d"(__v_ret)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d1", "a0", "a1" );\
  __v_ret;})

#define SumKickData() ({\
  __SumKickData_base((EXEC_BASE_NAME));\
})

#define __AddMemList_base(__in_base, ___size, ___attributes, ___pri, ___base, ___name) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  ULONG __p____size = (ULONG)(___size);\
  ULONG __p____attributes = (ULONG)(___attributes);\
  LONG __p____pri = (LONG)(___pri);\
  APTR __p____base = (APTR)(___base);\
  STRPTR __p____name = (STRPTR)(___name);\
  register ULONG __v0 __asm("d0") = __p____size;\
  register ULONG __v1 __asm("d1") = __p____attributes;\
  register LONG __v2 __asm("d2") = __p____pri;\
  register APTR __v3 __asm("a0") = __p____base;\
  register STRPTR __v4 __asm("a1") = __p____name;\
  __asm volatile (\
                   "jsr %%a6@(-618:W)\n"\
                   : "+d"(__v0), "+d"(__v1), "+a"(__v3), "+a"(__v4)\
                   : "a"(__p__in_base), "d"(__v2)\
                   : "fp0", "fp1", "cc", "memory" );\
})

#define AddMemList(___size, ___attributes, ___pri, ___base, ___name) ({\
  __AddMemList_base((EXEC_BASE_NAME), ___size, ___attributes, ___pri, ___base, ___name);\
})

#define __CopyMem_base(__in_base, ___source, ___dest, ___size) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  APTR __p____source = (APTR)(___source);\
  APTR __p____dest = (APTR)(___dest);\
  ULONG __p____size = (ULONG)(___size);\
  register APTR __v0 __asm("a0") = __p____source;\
  register APTR __v1 __asm("a1") = __p____dest;\
  register ULONG __v2 __asm("d0") = __p____size;\
  __asm volatile (\
                   "jsr %%a6@(-624:W)\n"\
                   : "+a"(__v0), "+a"(__v1), "+d"(__v2)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d1" );\
})

#define CopyMem(___source, ___dest, ___size) ({\
  __CopyMem_base((EXEC_BASE_NAME), ___source, ___dest, ___size);\
})

#define __CopyMemQuick_base(__in_base, ___source, ___dest, ___size) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  APTR __p____source = (APTR)(___source);\
  APTR __p____dest = (APTR)(___dest);\
  ULONG __p____size = (ULONG)(___size);\
  register APTR __v0 __asm("a0") = __p____source;\
  register APTR __v1 __asm("a1") = __p____dest;\
  register ULONG __v2 __asm("d0") = __p____size;\
  __asm volatile (\
                   "jsr %%a6@(-630:W)\n"\
                   : "+a"(__v0), "+a"(__v1), "+d"(__v2)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d1" );\
})

#define CopyMemQuick(___source, ___dest, ___size) ({\
  __CopyMemQuick_base((EXEC_BASE_NAME), ___source, ___dest, ___size);\
})

#define __CacheClearU_base(__in_base) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  __asm volatile (\
                   "jsr %%a6@(-636:W)\n"\
                   :\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d0", "d1", "a0", "a1" );\
})

#define CacheClearU() ({\
  __CacheClearU_base((EXEC_BASE_NAME));\
})

#define __CacheClearE_base(__in_base, ___address, ___length, ___caches) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  APTR __p____address = (APTR)(___address);\
  ULONG __p____length = (ULONG)(___length);\
  ULONG __p____caches = (ULONG)(___caches);\
  register APTR __v0 __asm("a0") = __p____address;\
  register ULONG __v1 __asm("d0") = __p____length;\
  register ULONG __v2 __asm("d1") = __p____caches;\
  __asm volatile (\
                   "jsr %%a6@(-642:W)\n"\
                   : "+a"(__v0), "+d"(__v1), "+d"(__v2)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "a1" );\
})

#define CacheClearE(___address, ___length, ___caches) ({\
  __CacheClearE_base((EXEC_BASE_NAME), ___address, ___length, ___caches);\
})

#define __CacheControl_base(__in_base, ___cacheBits, ___cacheMask) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  ULONG __p____cacheBits = (ULONG)(___cacheBits);\
  ULONG __p____cacheMask = (ULONG)(___cacheMask);\
  register ULONG __v_ret __asm("d0");\
  register ULONG __v0 __asm("d0") = __p____cacheBits;\
  register ULONG __v1 __asm("d1") = __p____cacheMask;\
  __asm volatile (\
                   "jsr %%a6@(-648:W)\n"\
                   : "=d"(__v_ret), "+d"(__v1)\
                   : "a"(__p__in_base), "d"(__v0)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define CacheControl(___cacheBits, ___cacheMask) ({\
  __CacheControl_base((EXEC_BASE_NAME), ___cacheBits, ___cacheMask);\
})

#define __CreateIORequest_base(__in_base, ___port, ___size) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct MsgPort * __p____port = (struct MsgPort *)(___port);\
  ULONG __p____size = (ULONG)(___size);\
  register APTR __v_ret __asm("d0");\
  register struct MsgPort * __v0 __asm("a0") = __p____port;\
  register ULONG __v1 __asm("d0") = __p____size;\
  __asm volatile (\
                   "jsr %%a6@(-654:W)\n"\
                   : "=d"(__v_ret), "+a"(__v0)\
                   : "a"(__p__in_base), "d"(__v1)\
                   : "fp0", "fp1", "cc", "memory", "d1", "a1" );\
  __v_ret;})

#define CreateIORequest(___port, ___size) ({\
  __CreateIORequest_base((EXEC_BASE_NAME), ___port, ___size);\
})

#define __DeleteIORequest_base(__in_base, ___iorequest) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  APTR __p____iorequest = (APTR)(___iorequest);\
  register APTR __v0 __asm("a0") = __p____iorequest;\
  __asm volatile (\
                   "jsr %%a6@(-660:W)\n"\
                   : "+a"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d0", "d1", "a1" );\
})

#define DeleteIORequest(___iorequest) ({\
  __DeleteIORequest_base((EXEC_BASE_NAME), ___iorequest);\
})

#define __CreateMsgPort_base(__in_base) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  register struct MsgPort * __v_ret __asm("d0");\
  __asm volatile (\
                   "jsr %%a6@(-666:W)\n"\
                   : "=d"(__v_ret)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d1", "a0", "a1" );\
  __v_ret;})

#define CreateMsgPort() ({\
  __CreateMsgPort_base((EXEC_BASE_NAME));\
})

#define __DeleteMsgPort_base(__in_base, ___port) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct MsgPort * __p____port = (struct MsgPort *)(___port);\
  register struct MsgPort * __v0 __asm("a0") = __p____port;\
  __asm volatile (\
                   "jsr %%a6@(-672:W)\n"\
                   : "+a"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d0", "d1", "a1" );\
})

#define DeleteMsgPort(___port) ({\
  __DeleteMsgPort_base((EXEC_BASE_NAME), ___port);\
})

#define __ObtainSemaphoreShared_base(__in_base, ___sigSem) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct SignalSemaphore * __p____sigSem = (struct SignalSemaphore *)(___sigSem);\
  register struct SignalSemaphore * __v0 __asm("a0") = __p____sigSem;\
  __asm volatile (\
                   "jsr %%a6@(-678:W)\n"\
                   : "+a"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d0", "d1", "a1" );\
})

#define ObtainSemaphoreShared(___sigSem) ({\
  __ObtainSemaphoreShared_base((EXEC_BASE_NAME), ___sigSem);\
})

#define __AllocVec_base(__in_base, ___byteSize, ___requirements) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  ULONG __p____byteSize = (ULONG)(___byteSize);\
  ULONG __p____requirements = (ULONG)(___requirements);\
  register APTR __v_ret __asm("d0");\
  register ULONG __v0 __asm("d0") = __p____byteSize;\
  register ULONG __v1 __asm("d1") = __p____requirements;\
  __asm volatile (\
                   "jsr %%a6@(-684:W)\n"\
                   : "=d"(__v_ret), "+d"(__v1)\
                   : "a"(__p__in_base), "d"(__v0)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define AllocVec(___byteSize, ___requirements) ({\
  __AllocVec_base((EXEC_BASE_NAME), ___byteSize, ___requirements);\
})

#define __FreeVec_base(__in_base, ___memoryBlock) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  APTR __p____memoryBlock = (APTR)(___memoryBlock);\
  register APTR __v0 __asm("a1") = __p____memoryBlock;\
  __asm volatile (\
                   "jsr %%a6@(-690:W)\n"\
                   : "+a"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d0", "d1", "a0" );\
})

#define FreeVec(___memoryBlock) ({\
  __FreeVec_base((EXEC_BASE_NAME), ___memoryBlock);\
})

#define __CreatePool_base(__in_base, ___requirements, ___puddleSize, ___threshSize) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  ULONG __p____requirements = (ULONG)(___requirements);\
  ULONG __p____puddleSize = (ULONG)(___puddleSize);\
  ULONG __p____threshSize = (ULONG)(___threshSize);\
  register APTR __v_ret __asm("d0");\
  register ULONG __v0 __asm("d0") = __p____requirements;\
  register ULONG __v1 __asm("d1") = __p____puddleSize;\
  register ULONG __v2 __asm("d2") = __p____threshSize;\
  __asm volatile (\
                   "jsr %%a6@(-696:W)\n"\
                   : "=d"(__v_ret), "+d"(__v1)\
                   : "a"(__p__in_base), "d"(__v0), "d"(__v2)\
                   : "fp0", "fp1", "cc", "memory", "a0", "a1" );\
  __v_ret;})

#define CreatePool(___requirements, ___puddleSize, ___threshSize) ({\
  __CreatePool_base((EXEC_BASE_NAME), ___requirements, ___puddleSize, ___threshSize);\
})

#define __DeletePool_base(__in_base, ___poolHeader) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  APTR __p____poolHeader = (APTR)(___poolHeader);\
  register APTR __v0 __asm("a0") = __p____poolHeader;\
  __asm volatile (\
                   "jsr %%a6@(-702:W)\n"\
                   : "+a"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d0", "d1", "a1" );\
})

#define DeletePool(___poolHeader) ({\
  __DeletePool_base((EXEC_BASE_NAME), ___poolHeader);\
})

#define __AllocPooled_base(__in_base, ___poolHeader, ___memSize) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  APTR __p____poolHeader = (APTR)(___poolHeader);\
  ULONG __p____memSize = (ULONG)(___memSize);\
  register APTR __v_ret __asm("d0");\
  register APTR __v0 __asm("a0") = __p____poolHeader;\
  register ULONG __v1 __asm("d0") = __p____memSize;\
  __asm volatile (\
                   "jsr %%a6@(-708:W)\n"\
                   : "=d"(__v_ret), "+a"(__v0)\
                   : "a"(__p__in_base), "d"(__v1)\
                   : "fp0", "fp1", "cc", "memory", "d1", "a1" );\
  __v_ret;})

#define AllocPooled(___poolHeader, ___memSize) ({\
  __AllocPooled_base((EXEC_BASE_NAME), ___poolHeader, ___memSize);\
})

#define __FreePooled_base(__in_base, ___poolHeader, ___memory, ___memSize) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  APTR __p____poolHeader = (APTR)(___poolHeader);\
  APTR __p____memory = (APTR)(___memory);\
  ULONG __p____memSize = (ULONG)(___memSize);\
  register APTR __v0 __asm("a0") = __p____poolHeader;\
  register APTR __v1 __asm("a1") = __p____memory;\
  register ULONG __v2 __asm("d0") = __p____memSize;\
  __asm volatile (\
                   "jsr %%a6@(-714:W)\n"\
                   : "+a"(__v0), "+a"(__v1), "+d"(__v2)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d1" );\
})

#define FreePooled(___poolHeader, ___memory, ___memSize) ({\
  __FreePooled_base((EXEC_BASE_NAME), ___poolHeader, ___memory, ___memSize);\
})

#define __AttemptSemaphoreShared_base(__in_base, ___sigSem) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct SignalSemaphore * __p____sigSem = (struct SignalSemaphore *)(___sigSem);\
  register ULONG __v_ret __asm("d0");\
  register struct SignalSemaphore * __v0 __asm("a0") = __p____sigSem;\
  __asm volatile (\
                   "jsr %%a6@(-720:W)\n"\
                   : "=d"(__v_ret), "+a"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d1", "a1" );\
  __v_ret;})

#define AttemptSemaphoreShared(___sigSem) ({\
  __AttemptSemaphoreShared_base((EXEC_BASE_NAME), ___sigSem);\
})

#define __ColdReboot_base(__in_base) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  __asm volatile (\
                   "jsr %%a6@(-726:W)\n"\
                   :\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d0", "d1", "a0", "a1" );\
})

#define ColdReboot() ({\
  __ColdReboot_base((EXEC_BASE_NAME));\
})

#define __StackSwap_base(__in_base, ___newStack) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct StackSwapStruct * __p____newStack = (struct StackSwapStruct *)(___newStack);\
  register struct StackSwapStruct * __v0 __asm("a0") = __p____newStack;\
  __asm volatile (\
                   "jsr %%a6@(-732:W)\n"\
                   : "+a"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d0", "d1", "a1" );\
})

#define StackSwap(___newStack) ({\
  __StackSwap_base((EXEC_BASE_NAME), ___newStack);\
})

#define __CachePreDMA_base(__in_base, ___address, ___length, ___flags) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  APTR __p____address = (APTR)(___address);\
  ULONG * __p____length = (ULONG *)(___length);\
  ULONG __p____flags = (ULONG)(___flags);\
  register APTR __v_ret __asm("d0");\
  register APTR __v0 __asm("a0") = __p____address;\
  register ULONG * __v1 __asm("a1") = __p____length;\
  register ULONG __v2 __asm("d0") = __p____flags;\
  __asm volatile (\
                   "jsr %%a6@(-762:W)\n"\
                   : "=d"(__v_ret), "+a"(__v0), "+a"(__v1)\
                   : "a"(__p__in_base), "d"(__v2)\
                   : "fp0", "fp1", "cc", "memory", "d1" );\
  __v_ret;})

#define CachePreDMA(___address, ___length, ___flags) ({\
  __CachePreDMA_base((EXEC_BASE_NAME), ___address, ___length, ___flags);\
})

#define __CachePostDMA_base(__in_base, ___address, ___length, ___flags) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  APTR __p____address = (APTR)(___address);\
  ULONG * __p____length = (ULONG *)(___length);\
  ULONG __p____flags = (ULONG)(___flags);\
  register APTR __v0 __asm("a0") = __p____address;\
  register ULONG * __v1 __asm("a1") = __p____length;\
  register ULONG __v2 __asm("d0") = __p____flags;\
  __asm volatile (\
                   "jsr %%a6@(-768:W)\n"\
                   : "+a"(__v0), "+a"(__v1), "+d"(__v2)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d1" );\
})

#define CachePostDMA(___address, ___length, ___flags) ({\
  __CachePostDMA_base((EXEC_BASE_NAME), ___address, ___length, ___flags);\
})

#define __AddMemHandler_base(__in_base, ___memhand) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct Interrupt * __p____memhand = (struct Interrupt *)(___memhand);\
  register struct Interrupt * __v0 __asm("a1") = __p____memhand;\
  __asm volatile (\
                   "jsr %%a6@(-774:W)\n"\
                   : "+a"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d0", "d1", "a0" );\
})

#define AddMemHandler(___memhand) ({\
  __AddMemHandler_base((EXEC_BASE_NAME), ___memhand);\
})

#define __RemMemHandler_base(__in_base, ___memhand) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct Interrupt * __p____memhand = (struct Interrupt *)(___memhand);\
  register struct Interrupt * __v0 __asm("a1") = __p____memhand;\
  __asm volatile (\
                   "jsr %%a6@(-780:W)\n"\
                   : "+a"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d0", "d1", "a0" );\
})

#define RemMemHandler(___memhand) ({\
  __RemMemHandler_base((EXEC_BASE_NAME), ___memhand);\
})

#define __ObtainQuickVector_base(__in_base, ___interruptCode) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  APTR __p____interruptCode = (APTR)(___interruptCode);\
  register ULONG __v_ret __asm("d0");\
  register APTR __v0 __asm("a0") = __p____interruptCode;\
  __asm volatile (\
                   "jsr %%a6@(-786:W)\n"\
                   : "=d"(__v_ret), "+a"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d1", "a1" );\
  __v_ret;})

#define ObtainQuickVector(___interruptCode) ({\
  __ObtainQuickVector_base((EXEC_BASE_NAME), ___interruptCode);\
})

#define __NewMinList_base(__in_base, ___minlist) ({\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  struct MinList * __p____minlist = (struct MinList *)(___minlist);\
  register struct MinList * __v0 __asm("a0") = __p____minlist;\
  __asm volatile (\
                   "jsr %%a6@(-828:W)\n"\
                   : "+a"(__v0)\
                   : "a"(__p__in_base)\
                   : "fp0", "fp1", "cc", "memory", "d0", "d1", "a1" );\
})

#define NewMinList(___minlist) ({\
  __NewMinList_base((EXEC_BASE_NAME), ___minlist);\
})

#endif /* !_INLINE_EXEC_H */
