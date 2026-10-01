#ifndef PT_AUDIO_STUB_H
#define PT_AUDIO_STUB_H
#include <stdint.h>
typedef uint8_t UBYTE;typedef uint16_t UWORD;typedef int16_t WORD;typedef uint32_t ULONG;typedef int32_t LONG;
struct Node{int8_t ln_Pri;};struct MsgPort{unsigned mp_SigBit;};
struct Message{struct Node mn_Node;};struct Device{int unused;};struct Unit{int unused;};
struct IORequest{struct Message io_Message;struct Device *io_Device;struct Unit *io_Unit;UWORD io_Command;UBYTE io_Flags;int8_t io_Error;};
struct IOAudio{struct IORequest ioa_Request;WORD ioa_AllocKey;UBYTE *ioa_Data;ULONG ioa_Length;UWORD ioa_Period,ioa_Volume,ioa_Cycles;struct Message ioa_WriteMsg;};
#define AUDIONAME "audio.device"
#define IOF_QUICK 1
#define ADALLOC_MINPREC -128
#define ADALLOC_MAXPREC 127
#define ADCMD_ALLOCATE 32
#define ADCMD_FREE 9
#define ADCMD_SETPREC 10
#define ADCMD_LOCK 13
#define ADIOF_NOWAIT 64
#define ADIOERR_NOALLOCATION -10
#endif
