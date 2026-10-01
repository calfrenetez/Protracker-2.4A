#include <devices/audio.h>
struct MsgPort *CreateMsgPort(void);
struct IORequest *CreateIORequest(struct MsgPort *,ULONG);
LONG OpenDevice(const char *,ULONG,struct IORequest *,ULONG);
void BeginIO(struct IORequest *);
struct IORequest *CheckIO(struct IORequest *);
LONG WaitIO(struct IORequest *);
void CloseDevice(struct IORequest *);
void DeleteIORequest(struct IORequest *);
void DeleteMsgPort(struct MsgPort *);
struct Message *GetMsg(struct MsgPort *);
LONG AbortIO(struct IORequest *);
ULONG TypeOfMem(const void *);
