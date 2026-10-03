/* Automatically generated header (sfdc 1.12)! Do not edit! */

#ifndef PROTO_EXEC_H
#define PROTO_EXEC_H

#include <clib/exec_protos.h>

#if defined(_CONST_BASES)
# ifndef __CONSTLIBBASEDECL__
# define __CONSTLIBBASEDECL__ const
# endif /* __CONSTLIBBASEDECL__ */
# ifndef __SEGMENTLIBBASEDECL__
# define __SEGMENTLIBBASEDECL__  __attribute__((__section__(".data")))
# endif /* __SEGMENTLIBBASEDECL__ */
#endif /* _CONST_BASES */
#ifdef __amigaos4__
# include <interfaces/exec.h>
# ifndef __NOGLOBALIFACE__
   extern struct SysIFace *ISys;
# endif /* __NOGLOBALIFACE__*/
#endif /* !__amigaos4__ */
#ifndef __NOLIBBASE__
  extern struct ExecBase *
# ifdef __CONSTLIBBASEDECL__
   __CONSTLIBBASEDECL__
# endif /* __CONSTLIBBASEDECL__ */
  SysBase
# ifdef __SEGMENTLIBBASEDECL__
 __SEGMENTLIBBASEDECL__
# endif /* __SEGMENTLIBBASEDECL__ */
;
#endif /* !__NOLIBBASE__ */

#ifndef _NO_INLINE
# if defined(__GNUC__)
#  ifdef __AROS__
#   include <defines/exec.h>
#  else
#   include <inline/exec.h>
#  endif
# else
#  include <pragmas/exec_pragmas.h>
# endif
#endif /* _NO_INLINE */

#endif /* !PROTO_EXEC_H */
