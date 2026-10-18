#ifndef PT_PRIVATE_AUDIO_HOST_EXEC_MODEL_H
#define PT_PRIVATE_AUDIO_HOST_EXEC_MODEL_H
/* HOST API-boundary shapes only. These are NOT SDK layouts, LVOs or OS objects.
 * The production C preamble/geometry and native SP accessor are not tested here. */
#if defined(__m68k__) || defined(__amigaos__)
#error "The fake Exec/audio boundary must never be compiled for a native target"
#endif
#include <stdint.h>
#include <stddef.h>
typedef int8_t BYTE;
typedef uint8_t UBYTE;
typedef int16_t WORD;
typedef uint16_t UWORD;
typedef uint32_t ULONG;
typedef const unsigned char *CONST_STRPTR;
struct Node { UBYTE ln_Type; BYTE ln_Pri; };
struct Task { struct Node tc_Node; void *tc_SPLower,*tc_SPUpper; };
struct Library { UWORD lib_Version; };
struct ExecBase { struct Library LibNode;struct Task *ThisTask;BYTE IDNestCnt,TDNestCnt; };
struct Device { unsigned host_only; };
struct Unit { unsigned host_only; };
struct MsgPort { struct Task *mp_SigTask;UBYTE mp_SigBit; };
struct Message { struct Node mn_Node;struct MsgPort *mn_ReplyPort;UWORD mn_Length; };
struct IORequest {
    struct Message io_Message;
    struct Device *io_Device;struct Unit *io_Unit;
    UWORD io_Command;UBYTE io_Flags;BYTE io_Error;
};
struct IOAudio { struct IORequest ioa_Request;WORD ioa_AllocKey;UBYTE *ioa_Data;ULONG ioa_Length; };
enum { NT_PROCESS=13, IOF_QUICK=1, ADCMD_FREE=9, ADCMD_LOCK=13,
       ADCMD_SETPREC=12, ADIOERR_NOALLOCATION=-10,
       ADIOERR_ALLOCFAILED=-11, ADIOERR_CHANNELSTOLEN=-12 };
extern struct ExecBase *SysBase;
struct Task *FindTask(CONST_STRPTR);
struct MsgPort *CreateMsgPort(void);
struct IORequest *CreateIORequest(struct MsgPort *,ULONG);
BYTE OpenDevice(CONST_STRPTR,ULONG,struct IORequest *,ULONG);
BYTE DoIO(struct IORequest *);
void SendIO(struct IORequest *);
struct IORequest *CheckIO(const struct IORequest *);
BYTE WaitIO(struct IORequest *);
void CloseDevice(struct IORequest *);
void DeleteIORequest(struct IORequest *);
void DeleteMsgPort(struct MsgPort *);
ULONG Wait(ULONG);
#endif
